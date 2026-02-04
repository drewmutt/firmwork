import os
import unittest

os.environ.setdefault("MPLBACKEND", "Agg")

import serial_plot


class DummySerial:
    def __init__(self, lines):
        self._lines = [l if isinstance(l, bytes) else l.encode() for l in lines]

    def readline(self):
        if not self._lines:
            return b""
        return self._lines.pop(0)

    def close(self):
        pass


class SerialPlotTests(unittest.TestCase):
    def test_parse_float_or_none(self):
        self.assertIsNone(serial_plot._parse_float_or_none("auto"))
        self.assertIsNone(serial_plot._parse_float_or_none(""))
        self.assertAlmostEqual(serial_plot._parse_float_or_none(" 2.5 "), 2.5)

    def test_parse_int(self):
        self.assertEqual(serial_plot._parse_int("", 9600), 9600)
        self.assertEqual(serial_plot._parse_int("115200", 9600), 115200)
        self.assertEqual(serial_plot._parse_int("115200.0", 9600), 115200)

    def test_parse_float_list(self):
        self.assertEqual(serial_plot._parse_float_list("3.14"), [3.14])
        self.assertEqual(serial_plot._parse_float_list("1,2,3"), [1.0, 2.0, 3.0])
        self.assertIsNone(serial_plot._parse_float_list("x=2.5 y=9"))
        self.assertIsNone(serial_plot._parse_float_list(""))

    def test_parse_line_values(self):
        self.assertEqual(serial_plot._parse_line_values("3.14"), [3.14])
        self.assertEqual(serial_plot._parse_line_values("1,2,3"), [1.0, 2.0, 3.0])
        self.assertEqual(serial_plot._parse_line_values("x=2.5 y=9"), [2.5])
        self.assertIsNone(serial_plot._parse_line_values(""))

    def test_serial_reader_read_line(self):
        reader = serial_plot.SerialReader("dummy", 0)
        reader._ser = DummySerial(["123\n", "abc\n", ""])
        self.assertEqual(reader._read_line_blocking(), "123")
        self.assertEqual(reader._read_line_blocking(), "abc")
        self.assertIsNone(reader._read_line_blocking())


if __name__ == "__main__":
    unittest.main()
