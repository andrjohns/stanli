#!/usr/bin/env python3
"""Mutation tests for the function-model oracle gate."""
import copy
import pathlib
import sys
import tempfile
import types
import unittest
from unittest import mock

REPO = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(REPO / "tools"))
import check_function_models as gate
import gen_function_models as generator


class FunctionModelGateTest(unittest.TestCase):
    def test_write_array_failure_cannot_hide_behind_successful_gradient(self):
        with self.assertRaises(ValueError):
            gate.parse("WANAMES FAIL unsupported function\nWAVALS FAIL\nOK 1 2\n")

    def test_nonfinite_probe_is_not_a_passing_comparison(self):
        for value in ("nan", "inf", "-inf"):
            with self.assertRaises(ValueError):
                gate.parse(f"OK 1 2\nWANAMES a\nWAVALS {value}\n")

    def test_permuted_outputs_fail_even_when_sum_and_gradient_match(self):
        expected = {"lp_grad": [3.0, 1.0], "names": ["a", "b"],
                    "values": [1.0, 2.0]}
        reference = {"source_sha256": "source", "points": [expected] * 3}
        args = types.SimpleNamespace(build=REPO, data=REPO, stanc=REPO)
        with mock.patch.object(gate, "source_digest", return_value="source"), \
             mock.patch.object(gate, "run", return_value="OK 3 1\nWANAMES a,b\nWAVALS 2 1\n"):
            with self.assertRaisesRegex(ValueError, "values\\[a\\]"):
                gate.compare("mutation", reference, args)

    def test_reference_requires_three_points_and_current_source(self):
        args = types.SimpleNamespace()
        with mock.patch.object(gate, "source_digest", return_value="source"):
            with self.assertRaisesRegex(ValueError, "source changed"):
                gate.compare("mutation", {"source_sha256": "stale"}, args)
            with self.assertRaisesRegex(ValueError, "three reference points"):
                gate.compare("mutation", {"source_sha256": "source", "points": []}, args)

    def test_crlf_checkout_replays_but_source_edits_still_fail(self):
        with tempfile.TemporaryDirectory() as directory:
            fixtures = pathlib.Path(directory)
            source = fixtures / "mutation.stan"
            original = b"parameters { real x; }\nmodel { target += x; }\n"
            source.write_bytes(original)
            expected = {"lp_grad": [3.0, 1.0], "names": ["x"], "values": [3.0]}
            reference = {"source_sha256": gate.source_digest(source),
                         "points": [expected] * 3}
            args = types.SimpleNamespace(build=REPO, data=REPO, stanc=REPO)
            source.write_bytes(original.replace(b"\n", b"\r\n"))
            with mock.patch.object(gate, "FIXTURES", fixtures), \
                 mock.patch.object(gate, "run", return_value="OK 3 1\nWANAMES x\nWAVALS 3\n") as run:
                gate.compare("mutation", reference, args)
                self.assertEqual(run.call_count, 3)
                run.reset_mock()
                source.write_bytes(source.read_bytes().replace(b"+= x", b"+= 2*x"))
                with self.assertRaisesRegex(ValueError, "source changed"):
                    gate.compare("mutation", reference, args)
                run.assert_not_called()

    def test_binary_digest_does_not_normalize_line_endings(self):
        with tempfile.TemporaryDirectory() as directory:
            binary = pathlib.Path(directory) / "compiler"
            binary.write_bytes(b"\x00\xff\r\n")
            original = gate.digest(binary)
            binary.write_bytes(b"\x00\xff\n")
            self.assertNotEqual(gate.digest(binary), original)

    def test_removing_a_runtime_function_probe_fails_the_inventory_gate(self):
        groups = copy.deepcopy(generator.GROUPS)
        del groups["containers"]["reverse"]
        with mock.patch.object(generator, "GROUPS", groups):
            with self.assertRaisesRegex(ValueError, "recipes: reverse"):
                generator.render()


if __name__ == "__main__":
    unittest.main()
