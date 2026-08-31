#!/usr/bin/env python3
"""Mutation tests for the function-model oracle gate."""
import copy
import pathlib
import sys
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
        with mock.patch.object(gate, "digest", return_value="source"), \
             mock.patch.object(gate, "run", return_value="OK 3 1\nWANAMES a,b\nWAVALS 2 1\n"):
            with self.assertRaisesRegex(ValueError, "values\\[a\\]"):
                gate.compare("mutation", reference, args)

    def test_reference_requires_three_points_and_current_source(self):
        args = types.SimpleNamespace()
        with mock.patch.object(gate, "digest", return_value="source"):
            with self.assertRaisesRegex(ValueError, "source changed"):
                gate.compare("mutation", {"source_sha256": "stale"}, args)
            with self.assertRaisesRegex(ValueError, "three reference points"):
                gate.compare("mutation", {"source_sha256": "source", "points": []}, args)

    def test_removing_a_runtime_function_probe_fails_the_inventory_gate(self):
        groups = copy.deepcopy(generator.GROUPS)
        del groups["containers"]["reverse"]
        with mock.patch.object(generator, "GROUPS", groups):
            with self.assertRaisesRegex(ValueError, "recipes: reverse"):
                generator.render()


if __name__ == "__main__":
    unittest.main()
