#!/usr/bin/env python3
"""The committed signature manifests must regenerate identically anywhere."""
import json
import pathlib
import unittest

REPO = pathlib.Path(__file__).resolve().parents[1]
MANIFESTS = ("tests/function_coverage/builtin_signatures_manifest.json",
             "tests/function_coverage/density_signatures_manifest.json")


def manifests():
    for name in MANIFESTS:
        yield name, json.loads((REPO / name).read_text())


class SignatureManifestPortabilityTest(unittest.TestCase):
    def test_model_paths_are_posix(self):
        for name, manifest in manifests():
            for model in manifest["models"]:
                self.assertNotIn("\\", model["file"], name)

    def test_build_id_names_no_host(self):
        for name, manifest in manifests():
            self.assertNotIn("(", manifest["stanc_build_id"], name)


if __name__ == "__main__":
    unittest.main()
