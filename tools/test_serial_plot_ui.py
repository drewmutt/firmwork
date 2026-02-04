import os
import sys
import unittest

os.environ.setdefault("MPLBACKEND", "Agg")
os.environ["SERIAL_PLOT_NO_SHOW"] = "1"
os.environ["SERIAL_PLOT_SKIP_OPEN"] = "1"
os.environ["SERIAL_PLOT_TEST_MODE"] = "1"

import serial_plot


class SerialPlotUiTests(unittest.TestCase):
    def test_main_headless_smoke(self):
        argv = sys.argv[:]
        try:
            sys.argv = ["serial_plot.py"]
            state = serial_plot.main()
            self.assertIsInstance(state, dict)
        finally:
            sys.argv = argv

    def test_stream_window_opens(self):
        argv = sys.argv[:]
        try:
            sys.argv = ["serial_plot.py"]
            state = serial_plot.main()
            state["open_stream_window"]()
            fig = state["get_stream_fig"]()
            self.assertIsNotNone(fig)
        finally:
            sys.argv = argv


if __name__ == "__main__":
    unittest.main()
