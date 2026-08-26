#!/usr/bin/env python3
"""Unit tests for the measurement report parsers and artifact schema."""

import json
import pathlib
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

REPO = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "harnesses"))
import vectorize_ab  # noqa: E402


class VectorizeAbTest(unittest.TestCase):
    def test_vector_comparison_is_nonfinite_safe(self):
        same = vectorize_ab.compare_vectors(
            ["1", "nan", "-inf"], ["1.0", "nan", "-inf"])
        self.assertTrue(same["same_shape"])
        self.assertEqual(same["changed"], 0)
        self.assertEqual(same["max_rel"], 0.0)
        self.assertTrue(same["nonfinite_match"])

        mismatch = vectorize_ab.compare_vectors(["1"], ["inf"])
        self.assertEqual(mismatch["max_rel"], "inf")
        self.assertEqual(mismatch["changed"], 1)
        self.assertEqual(mismatch["finite_changed"], 0)
        self.assertFalse(mismatch["nonfinite_match"])
        self.assertEqual(mismatch["differences"][0]["index"], 0)
        self.assertRegex(mismatch["differences"][0]["left_bits"],
                         r"^0x[0-9a-f]{16}$")

    def test_execution_status_includes_exit_and_timeout(self):
        ok = {"returncode": 0, "timeout": False}
        rejected = {"returncode": 1, "timeout": False}
        timed_out = {"returncode": 0, "timeout": True}
        self.assertTrue(vectorize_ab.execution_matches_status(ok, ["OK"]))
        self.assertTrue(vectorize_ab.execution_matches_status(
            rejected, ["EVAL_FAIL", "domain"]))
        self.assertFalse(vectorize_ab.execution_matches_status(
            rejected, ["OK", "1"]))
        self.assertFalse(vectorize_ab.execution_matches_status(
            timed_out, ["OK", "1"]))

    def test_ab_only_matching_evaluation_failure_passes(self):
        rejected = {
            "returncode": 1,
            "timeout": False,
            "elapsed_ns": 1,
            "stdout": "EVAL_FAIL identical domain error\n",
            "stderr": "WA none\n",
        }
        with mock.patch.object(vectorize_ab, "run_command",
                               side_effect=[rejected, rejected]):
            result = vectorize_ab.semantic_point(
                "check", "model", "data", {"off": "a", "on": "b"},
                0, None, 1, 1e-9)
        self.assertTrue(result["ok"])
        self.assertFalse(result["off"]["reference"]["referenced"])
        self.assertTrue(result["ab"]["error_match"])

    def test_ab_only_matching_compile_failure_fails(self):
        failed = {
            "returncode": 1,
            "timeout": False,
            "elapsed_ns": 1,
            "stdout": "COMPILE_FAIL identical lower error\n",
            "stderr": "",
        }
        with mock.patch.object(vectorize_ab, "run_command",
                               side_effect=[failed, failed]):
            result = vectorize_ab.semantic_point(
                "check", "model", "data", {"off": "a", "on": "b"},
                0, None, 1, 1e-9)
        self.assertFalse(result["ok"])
        self.assertFalse(result["off"]["reference"]["ok"])

    def test_ab_only_finite_values_must_meet_gate(self):
        off = {
            "returncode": 0, "timeout": False, "elapsed_ns": 1,
            "stdout": "OK 1 2\n", "stderr": "WA none\n",
        }
        on = {
            "returncode": 0, "timeout": False, "elapsed_ns": 1,
            "stdout": "OK 1 3\n", "stderr": "WA none\n",
        }
        with mock.patch.object(vectorize_ab, "run_command",
                               side_effect=[off, on]):
            result = vectorize_ab.semantic_point(
                "check", "model", "data", {"off": "a", "on": "b"},
                0, None, 1, 1e-9)
        self.assertFalse(result["ok"])
        self.assertTrue(result["ab"]["ab_only_finite_gate"]["applied"])
        self.assertFalse(result["ab"]["ab_only_finite_gate"]["values_ok"])

    def test_ab_only_write_array_values_must_meet_gate(self):
        off = {
            "returncode": 0, "timeout": False, "elapsed_ns": 1,
            "stdout": "WANAMES x\nWAVALS 1\nOK 0\n", "stderr": "",
        }
        on = {
            "returncode": 0, "timeout": False, "elapsed_ns": 1,
            "stdout": "WANAMES x\nWAVALS 2\nOK 0\n", "stderr": "",
        }
        with mock.patch.object(vectorize_ab, "run_command",
                               side_effect=[off, on]):
            result = vectorize_ab.semantic_point(
                "check", "model", "data", {"off": "a", "on": "b"},
                0, None, 1, 1e-9)
        self.assertFalse(result["ok"])
        self.assertFalse(
            result["ab"]["ab_only_finite_gate"]["wa_values_ok"])

    def test_write_array_failure_outcomes_do_not_collapse(self):
        absent = vectorize_ab.parse_wa_outcome("OK 1\n")
        failed_a = vectorize_ab.parse_wa_outcome(
            "WANAMES FAIL domain A\nWAVALS FAIL\nOK 1\n")
        failed_b = vectorize_ab.parse_wa_outcome(
            "WANAMES FAIL domain B\nWAVALS FAIL\nOK 1\n")
        self.assertFalse(vectorize_ab.compare_wa_outcomes(
            absent, failed_a)["category_match"])
        compared = vectorize_ab.compare_wa_outcomes(failed_a, failed_b)
        self.assertTrue(compared["category_match"])
        self.assertFalse(compared["reason_match"])

    def test_timeout_output_is_decoded(self):
        timeout = subprocess.TimeoutExpired(
            ["probe"], 1, output=b"OK 1\n", stderr=b"last line\n")
        with mock.patch.object(vectorize_ab.subprocess, "run",
                               side_effect=timeout):
            result = vectorize_ab.run_command(["probe"], 1)
        self.assertTrue(result["timeout"])
        self.assertEqual(result["stdout"], "OK 1\n")
        self.assertEqual(result["stderr"], "last line\n")

    def test_prep_dispositions_are_parsed(self):
        rows = vectorize_ab.parse_prep(
            "noise\n"
            "stanli_prep graph=log_prob stage=reroll ns=41 ops=7 "
            "regions=2 packed_rows=1 term_density=3 element_density=4 "
            "term_widen=5 element_store=6\n")
        self.assertEqual(len(rows), 1)
        self.assertEqual(rows[0]["graph"], "log_prob")
        self.assertEqual(rows[0]["packed_rows"], 1)
        self.assertEqual(rows[0]["element_store"], 6)

    def test_reroll_summary_keeps_both_final_op_counts(self):
        rows = vectorize_ab.parse_prep(
            "stanli_prep graph=log_prob stage=total ns=1 ops=7 slots=8\n"
            "stanli_prep graph=write_array stage=total ns=2 ops=5 slots=6\n"
            "stanli_prep graph=log_prob stage=reroll ns=3 regions=1 "
            "packed_rows=0 term_density=1 element_density=0 term_widen=0 "
            "element_store=0\n"
            "stanli_prep graph=write_array stage=reroll ns=4 regions=2 "
            "packed_rows=0 term_density=0 element_density=0 term_widen=0 "
            "element_store=2\n")
        evidence = vectorize_ab.summarize_reroll([{
            "source_pass": "on",
            "runtime_reroll": "on",
            "samples": [{"sample": 1, "rows": rows}],
        }])
        sample = evidence[0]["samples"][0]
        self.assertEqual(sample["log_prob"]["final_ops"], 7)
        self.assertEqual(sample["write_array"]["final_ops"], 5)
        self.assertEqual(sample["write_array"]["element_store"], 2)

    def test_dump_summary_is_parsed(self):
        parsed = vectorize_ab.parse_dump(
            "slots=19 ops=7 result=18\n"
            "SUMMARY ops=7 scalar_out=2 vector_out=5\n"
            "  NORMAL_LPDF total=1 scalar=0\n")
        self.assertEqual(parsed["ops"], 7)
        self.assertEqual(parsed["summary"]["scalar_out"], 2)
        self.assertEqual(parsed["opcodes"]["NORMAL_LPDF"]["total"], 1)
        self.assertEqual(vectorize_ab.dump_problems(parsed), [])
        self.assertIn("missing SUMMARY",
                      vectorize_ab.dump_problems({"slots": 1, "ops": 1,
                                                  "result": 0}))

    def test_gradient_output_calibration_and_abba_order(self):
        self.assertIsNone(vectorize_ab.parse_bench_output(
            "1 2 3 4\nnot the final row\n"))
        parsed = vectorize_ab.parse_bench_output(
            "model output\n200.0 3.5 80.0 7\n")
        self.assertEqual(parsed["gradient_ns"], 200.0)
        self.assertEqual(parsed["n_params"], 7)
        self.assertEqual(vectorize_ab.calibrated_iterations(
            [100.0, 200.0], 0.001, 10_000), 5000)

        calls = []

        def fake_run(_bench, mir, _data, iterations, _timeout):
            mode = str(mir)
            calls.append((mode, iterations))
            latency = 100.0 if mode == "off" else 200.0
            return {
                "ok": True,
                "returncode": 0,
                "timeout": False,
                "elapsed_ns": 1,
                "stderr_tail": "",
                "result": {
                    "gradient_ns": latency,
                    "sink": 0.0,
                    "forward_ns": 50.0,
                    "n_params": 2,
                },
            }

        with mock.patch.object(vectorize_ab, "bench_run",
                               side_effect=fake_run):
            measured = vectorize_ab.gradient_benchmark(
                "bench", {"off": "off", "on": "on"}, "data", 2, 8,
                0.001, 10_000, 5)
        self.assertEqual(calls[:2], [("off", 8), ("on", 8)])
        self.assertEqual(
            [run["source_pass"] for run in measured["runs"]],
            ["off", "on", "on", "off"] * 2)
        self.assertEqual(measured["iterations"], 5000)
        self.assertEqual(measured["on_over_off"], 2.0)

    def test_report_writes_all_artifacts(self):
        graph = {
            "model": "probe",
            "source_pass": "off",
            "runtime_reroll": "on",
            "graph": {"summary": {"ops": 1, "scalar_out": 1}},
            "samples": [{
                "sample": 1,
                "elapsed_ns": 4,
                "rows": [{
                    "graph": "log_prob", "stage": "reroll", "ns": 3,
                    "regions": 0, "packed_rows": 0, "term_density": 0,
                    "element_density": 0, "term_widen": 0,
                    "element_store": 0,
                }],
            }],
        }
        with tempfile.TemporaryDirectory() as temp:
            out = pathlib.Path(temp)
            summary = vectorize_ab.write_reports(
                out, {"schema": 1}, [{"model": "probe", "ok": True}],
                [graph], [{
                    "model": "probe", "mir_changed": False,
                    "changed_values": 0, "points": 1,
                }], [], [])
            self.assertTrue(summary["ok"])
            expected = {
                "manifest.json", "corpus.jsonl", "graphs.jsonl",
                "bench.tsv", "summary.json", "summary.md",
            }
            self.assertEqual({path.name for path in out.iterdir()}, expected)
            self.assertTrue(json.loads((out / "summary.json").read_text())["ok"])
            header = (out / "bench.tsv").read_text().splitlines()[0]
            self.assertIn("write_array_ops", header)
            self.assertIn("log_prob_reroll_packed_rows", header)
            self.assertIn("write_array_reroll_element_store", header)


if __name__ == "__main__":
    unittest.main()
