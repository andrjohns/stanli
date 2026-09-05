#!/usr/bin/env python3
"""Focused tests for the corpus benchmark TSV serializer."""

import csv
import io
import pathlib
import sys
import unittest

REPO = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO))

from harnesses.corpus_bench import (COLS, parse_grad_count, row_line,
                                     upgrade_header)


class RowLineTests(unittest.TestCase):
    def parse(self, row):
        text = "\t".join(COLS) + "\n" + row_line(row)
        return next(csv.DictReader(io.StringIO(text), delimiter="\t"))

    def test_empty_final_note_is_quoted_without_changing_its_value(self):
        line = row_line({"model": "m", "note": ""})
        self.assertTrue(line.endswith('\t""\n'))
        self.assertFalse(line.endswith("\t\n"))
        self.assertEqual(self.parse({"model": "m", "note": ""})["note"], "")

    def test_nonempty_note_is_unchanged(self):
        row = {"model": "m", "note": "stanli_sample_timeout"}
        self.assertTrue(row_line(row).endswith("\tstanli_sample_timeout\n"))
        self.assertEqual(self.parse(row)["note"], "stanli_sample_timeout")

    def test_old_row_without_stanli_grads_round_trips(self):
        old_cols = [c for c in COLS if c != "stanli_grads"]
        text = ("\t".join(old_cols) + "\n"
                + "\t".join("m" if c == "model" else "" for c in old_cols)
                + "\n")
        old_row = next(csv.DictReader(io.StringIO(text), delimiter="\t"))
        self.assertEqual(self.parse(old_row)["stanli_grads"], "")


class UpgradeHeaderTests(unittest.TestCase):
    def test_noop_when_header_already_current(self):
        self.assertIsNone(upgrade_header(COLS, {}))

    def test_rewrites_old_header_to_current_cols(self):
        old_cols = [c for c in COLS if c != "stanli_grads"]
        rows = {"m": {**{c: "" for c in old_cols}, "model": "m"}}
        text = upgrade_header(old_cols, rows)
        header = text.splitlines()[0]
        self.assertEqual(header, "\t".join(COLS))


class ParseGradCountTests(unittest.TestCase):
    def test_extracts_count_from_stderr(self):
        stderr = ("stanli_run: 3 of 1000 draws could not produce generated "
                  "quantities, written as nan: bad\n"
                  "stanli_run: 12345 gradient evaluations\n")
        self.assertEqual(parse_grad_count(stderr), "12345")

    def test_empty_when_absent(self):
        self.assertEqual(parse_grad_count("stanli_run: boom\n"), "")


if __name__ == "__main__":
    unittest.main()
