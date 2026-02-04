#!/usr/bin/env python3
"""
serial_plot.py

Plot numeric values from a serial port in real time.
X axis is time (seconds since start). Y auto-scales to data by default.
"""

from __future__ import annotations

import argparse
import os
import sys
import time
import threading
from collections import deque
import glob

try:
    import tkinter as tk
    from tkinter import ttk
    _HAS_TK = True
except Exception:
    _HAS_TK = False
from typing import Deque, Optional, Tuple, List

import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from matplotlib.widgets import Button, CheckButtons, RadioButtons, TextBox

try:
    import serial
except Exception as exc:  # pragma: no cover - runtime import guard
    print("ERROR: pyserial is required. Install with: pip install pyserial", file=sys.stderr)
    raise SystemExit(1) from exc


class SerialReader:
    def __init__(self, port: str, baud: int, timeout: float = 0.1) -> None:
        self._port = port
        self._baud = baud
        self._timeout = timeout
        self._ser: Optional[serial.Serial] = None
        self._buf = bytearray()
        self._last_emit = time.monotonic()
        self._stop = threading.Event()
        self._thread: Optional[threading.Thread] = None
        self._lock = threading.Lock()
        self._lines: Deque[Tuple[float, str]] = deque(maxlen=10000)
        self._error: Optional[str] = None

    def open(self) -> None:
        self.close()
        self._error = None
        self._ser = serial.Serial(self._port, self._baud, timeout=self._timeout)
        self._start_thread()

    def close(self) -> None:
        self._error = None
        self._stop.set()
        if self._thread is not None:
            self._thread.join(timeout=1.0)
            self._thread = None
        if self._ser is not None:
            try:
                self._ser.close()
            finally:
                self._ser = None
        self._stop.clear()

    def _start_thread(self) -> None:
        if self._thread is not None and self._thread.is_alive():
            return
        self._thread = threading.Thread(target=self._read_loop, daemon=True)
        self._thread.start()

    def _read_loop(self) -> None:
        while not self._stop.is_set():
            try:
                line = self._read_line_blocking()
            except Exception as exc:
                self._error = str(exc)
                self._stop.set()
                break
            if line is None:
                continue
            ts = time.time()
            with self._lock:
                self._lines.append((ts, line))

    def _read_line_blocking(self) -> Optional[str]:
        if self._ser is None:
            return None
        raw = self._ser.readline()
        if not raw:
            return None
        text = raw.decode(errors="ignore").strip()
        return text if text else None

    def get_lines(self, max_lines: int = 1000) -> list[Tuple[float, str]]:
        out: list[Tuple[float, str]] = []
        with self._lock:
            while self._lines and len(out) < max_lines:
                out.append(self._lines.popleft())
        return out

    def get_error(self) -> Optional[str]:
        return self._error


def _parse_float_or_none(text: str) -> Optional[float]:
    t = text.strip().lower()
    if t in ("", "auto", "none"):
        return None
    return float(t)


def _parse_int(text: str, default: int) -> int:
    t = text.strip().lower()
    if not t:
        return default
    return int(float(t))


def _parse_float_list(text: str) -> Optional[List[float]]:
    parts = [p.strip() for p in text.split(",")]
    vals: List[float] = []
    for part in parts:
        if not part:
            continue
        try:
            vals.append(float(part))
        except ValueError:
            return None
    return vals if vals else None


def _parse_first_float(text: str) -> Optional[float]:
    cleaned = []
    for ch in text:
        if ch.isdigit() or ch in ".-+eE":
            cleaned.append(ch)
        else:
            cleaned.append(" ")
    for token in "".join(cleaned).replace(",", " ").split():
        try:
            return float(token)
        except ValueError:
            continue
    return None


def _parse_line_values(text: str) -> Optional[List[float]]:
    if "," in text:
        vals = _parse_float_list(text)
        if vals is not None:
            return vals
    first = _parse_first_float(text)
    return [first] if first is not None else None


def main() -> int:
    ap = argparse.ArgumentParser(description="Plot serial values over time.")
    ap.add_argument("--port", type=str, default="/dev/cu.usbserial-0001", help="Serial port.")
    ap.add_argument("--baud", type=int, default=115200, help="Baud rate.")
    ap.add_argument("--x-min", type=str, default="auto", help="X min or 'auto'.")
    ap.add_argument("--x-max", type=str, default="auto", help="X max or 'auto'.")
    ap.add_argument("--x-window", type=float, default=10.0, help="Auto X window size in seconds.")
    ap.add_argument("--y-min", type=str, default="auto", help="Y min or 'auto'.")
    ap.add_argument("--y-max", type=str, default="auto", help="Y max or 'auto'.")
    ap.add_argument("--y-auto", action="store_true", default=True, help="Auto-scale Y to data.")
    ap.add_argument("--no-y-auto", dest="y_auto", action="store_false", help="Disable auto-scale Y.")
    ap.add_argument("--max-points", type=int, default=20000, help="Max points to keep in memory.")
    args = ap.parse_args()

    reader = SerialReader(args.port, args.baud)
    status_msg = ""
    if os.environ.get("SERIAL_PLOT_SKIP_OPEN") == "1":
        status_msg = "Not connected: skipped open (SERIAL_PLOT_SKIP_OPEN=1)"
    else:
        try:
            reader.open()
            status_msg = f"Connected: {args.port} @ {args.baud}"
        except Exception as exc:
            status_msg = f"Not connected: {exc}"

    start_t = time.time()
    xs: Deque[float] = deque(maxlen=args.max_points)
    series: List[Deque[float]] = []
    series_count = 0

    fig, ax = plt.subplots()
    fig.subplots_adjust(bottom=0.45)
    lines: List = []
    ax.set_xlabel("Time (s)")
    ax.set_ylabel("Value")
    ax.set_title("Serial Plot")

    hover = ax.annotate(
        "",
        xy=(0, 0),
        xytext=(10, 10),
        textcoords="offset points",
        bbox=dict(boxstyle="round,pad=0.2", fc="yellow", alpha=0.8),
        arrowprops=dict(arrowstyle="->", color="gray"),
    )
    hover.set_visible(False)

    status_ax = fig.add_axes([0.06, 0.02, 0.78, 0.18])
    status_ax.set_axis_off()
    status_text = status_ax.text(0.0, 1.0, "", va="top", ha="left")
    log_lines: Deque[str] = deque(maxlen=200)
    log_offset = 0
    log_visible = 6
    rate_text = ax.text(0.99, 0.99, "0.0 values/s", transform=ax.transAxes, va="top", ha="right")
    total_text = ax.text(0.99, 0.94, "total: 0", transform=ax.transAxes, va="top", ha="right")
    stats_text = ax.text(0.99, 0.89, "y min/max/avg: - / - / -", transform=ax.transAxes, va="top", ha="right")

    # Controls
    ax_port = fig.add_axes([0.06, 0.32, 0.26, 0.05])
    ax_pick = fig.add_axes([0.33, 0.32, 0.08, 0.05])
    ax_baud = fig.add_axes([0.43, 0.32, 0.12, 0.05])
    ax_xwin = fig.add_axes([0.57, 0.32, 0.10, 0.05])
    ax_apply = fig.add_axes([0.69, 0.32, 0.08, 0.05])
    ax_stream = fig.add_axes([0.79, 0.32, 0.08, 0.05])
    ax_pause = fig.add_axes([0.89, 0.32, 0.08, 0.05])
    ax_clear = fig.add_axes([0.89, 0.24, 0.08, 0.05])
    ax_xmin = fig.add_axes([0.06, 0.24, 0.14, 0.05])
    ax_xmax = fig.add_axes([0.22, 0.24, 0.14, 0.05])
    ax_ymin = fig.add_axes([0.06, 0.16, 0.14, 0.05])
    ax_ymax = fig.add_axes([0.22, 0.16, 0.14, 0.05])
    ax_checks = fig.add_axes([0.40, 0.225, 0.18, 0.09])
    ax_log_up = fig.add_axes([0.86, 0.12, 0.08, 0.05])
    ax_log_dn = fig.add_axes([0.86, 0.06, 0.08, 0.05])
    ax_zoom_in = fig.add_axes([0.69, 0.24, 0.08, 0.05])
    ax_zoom_out = fig.add_axes([0.79, 0.24, 0.08, 0.05])
    ax_fit = fig.add_axes([0.89, 0.16, 0.08, 0.05])

    tb_port = TextBox(ax_port, "", initial=args.port)
    tb_baud = TextBox(ax_baud, "", initial=str(args.baud))
    tb_xmin = TextBox(ax_xmin, "", initial=str(args.x_min))
    tb_xmax = TextBox(ax_xmax, "", initial=str(args.x_max))
    tb_xwin = TextBox(ax_xwin, "", initial=str(args.x_window))
    tb_ymin = TextBox(ax_ymin, "", initial=str(args.y_min))
    tb_ymax = TextBox(ax_ymax, "", initial=str(args.y_max))
    btn_apply = Button(ax_apply, "Apply")
    btn_pick = Button(ax_pick, "Ports")
    btn_stream = Button(ax_stream, "Stream")
    btn_pause = Button(ax_pause, "Pause")
    btn_clear = Button(ax_clear, "Clear")
    btn_zoom_in = Button(ax_zoom_in, "Zoom +")
    btn_zoom_out = Button(ax_zoom_out, "Zoom -")
    btn_fit = Button(ax_fit, "Fit")
    ax_zoom_in.set_visible(False)
    ax_zoom_out.set_visible(False)
    ax_fit.set_visible(False)
    btn_log_up = Button(ax_log_up, "Log ▲")
    btn_log_dn = Button(ax_log_dn, "Log ▼")
    checks = CheckButtons(ax_checks, ["Auto X", "Auto Y"], [args.x_min == "auto", args.y_auto])

    label_size = 8  # ~50% of default
    for ax_l, text in (
        (ax_port, "Port"),
        (ax_baud, "Baud"),
        (ax_xwin, "X window"),
        (ax_xmin, "X min"),
        (ax_xmax, "X max"),
        (ax_ymin, "Y min"),
        (ax_ymax, "Y max"),
    ):
        ax_l.text(0.0, 1.15, text, transform=ax_l.transAxes, va="bottom", ha="left", fontsize=label_size)

    def pick_port_dialog() -> None:
        ports = sorted(glob.glob("/dev/cu.usb*"))
        if _HAS_TK:
            root = tk.Tk()
            root.title("Select Serial Port")
            root.geometry("360x120")
            root.resizable(False, False)

            ttk.Label(root, text="Port:").pack(pady=(10, 2))
            combo = ttk.Combobox(root, values=ports, state="readonly")
            if ports:
                combo.set(ports[0])
            combo.pack(fill="x", padx=12)

            def apply_and_close() -> None:
                if combo.get():
                    tb_port.set_val(combo.get())
                root.destroy()

            ttk.Button(root, text="Select", command=apply_and_close).pack(pady=10)
            root.mainloop()
            return

        # Fallback: matplotlib radio-button picker
        import matplotlib.pyplot as plt

        fig_p, ax_p = plt.subplots(figsize=(4, 2 + 0.25 * max(1, len(ports))))
        fig_p.suptitle("Select Serial Port")
        ax_p.set_position([0.05, 0.05, 0.9, 0.85])
        ax_p.set_axis_off()
        if not ports:
            ports = ["(none)"]
        radios = RadioButtons(ax_p, ports, active=0)

        def on_pick(label: str) -> None:
            if label != "(none)":
                tb_port.set_val(label)
            plt.close(fig_p)

        radios.on_clicked(on_pick)
        plt.show()

    auto_x = args.x_min == "auto" or args.x_max == "auto"
    auto_y = args.y_auto
    x_min_val = _parse_float_or_none(args.x_min)
    x_max_val = _parse_float_or_none(args.x_max)
    x_window = float(args.x_window)
    y_min_val = _parse_float_or_none(args.y_min)
    y_max_val = _parse_float_or_none(args.y_max)

    def _render_log() -> None:
        nonlocal log_offset
        max_offset = max(0, len(log_lines) - log_visible)
        log_offset = max(0, min(log_offset, max_offset))
        start = max(0, len(log_lines) - log_visible - log_offset)
        end = start + log_visible
        chunk = list(log_lines)[start:end]
        status_text.set_text("\n".join(chunk))

    def _add_log(msg: str) -> None:
        truncated = msg.replace("\n", " ")
        if len(truncated) > 140:
            truncated = truncated[:137] + "..."
        log_lines.append(truncated)
        _render_log()

    def _set_status(msg: str) -> None:
        nonlocal status_msg
        status_msg = msg
        _add_log(status_msg)

    def apply_settings(event=None) -> None:
        nonlocal auto_x, auto_y, x_min_val, x_max_val, x_window, reader, start_t, status_msg
        nonlocal series_count, series, lines, y_min_val, y_max_val
        port = tb_port.text.strip()
        baud = _parse_int(tb_baud.text, args.baud)
        x_min_val = _parse_float_or_none(tb_xmin.text)
        x_max_val = _parse_float_or_none(tb_xmax.text)
        x_window = float(tb_xwin.text.strip() or args.x_window)
        y_min_val = _parse_float_or_none(tb_ymin.text)
        y_max_val = _parse_float_or_none(tb_ymax.text)
        auto_x = checks.get_status()[0]
        auto_y = checks.get_status()[1]

        try:
            if port != reader._port or baud != reader._baud:
                reader.close()
                reader = SerialReader(port, baud)
                reader.open()
                start_t = time.time()
                xs.clear()
                series.clear()
                lines.clear()
                series_count = 0
                stream_lines.clear()
                value_times.clear()
                _set_status(f"Connected: {port} @ {baud}")
        except Exception as exc:
            _set_status(f"Not connected: {exc}")

    stream_fig = None
    stream_ax = None
    stream_text = None
    stream_lines: Deque[str] = deque(maxlen=500)
    stream_visible = 30
    lines_read = 0
    values_parsed = 0
    value_times: Deque[float] = deque()
    paused = False

    def _scroll_log(direction: int) -> None:
        nonlocal log_offset
        log_offset += direction
        _render_log()

    def _open_stream_window() -> None:
        nonlocal stream_fig, stream_ax, stream_text
        if stream_fig is not None and plt.fignum_exists(stream_fig.number):
            _set_status("Stream window already open")
            return
        backend = plt.get_backend().lower()
        if "agg" in backend:
            _set_status(f"Stream window disabled on non-interactive backend: {backend}")
            return
        stream_fig, stream_ax = plt.subplots()
        stream_fig.canvas.manager.set_window_title("Serial Stream")
        stream_ax.set_axis_off()
        stream_text = stream_ax.text(0.0, 1.0, "", va="top", ha="left", family="monospace")
        try:
            stream_fig.show()
        except Exception:
            pass
        _set_status("Stream window opened")

    btn_apply.on_clicked(apply_settings)
    btn_pick.on_clicked(lambda _evt: pick_port_dialog())
    btn_stream.on_clicked(lambda _evt: _open_stream_window())
    btn_log_up.on_clicked(lambda _evt: _scroll_log(-1))
    btn_log_dn.on_clicked(lambda _evt: _scroll_log(1))
    btn_pause.on_clicked(lambda _evt: _toggle_pause())
    btn_clear.on_clicked(lambda _evt: _clear_data())
    btn_zoom_in.on_clicked(lambda _evt: _zoom(0.5))
    btn_zoom_out.on_clicked(lambda _evt: _zoom(2.0))
    btn_fit.on_clicked(lambda _evt: _fit())

    _add_log(status_msg)

    def on_hover(event) -> None:
        if event.inaxes != ax or not xs or not series:
            hover.set_visible(False)
            return
        xdata = list(xs)
        mx, my = event.x, event.y
        d2_min = None
        best = None
        for si, ys in enumerate(series):
            ydata = list(ys)
            pts = ax.transData.transform(list(zip(xdata, ydata)))
            for i, (px, py) in enumerate(pts):
                d2 = (px - mx) ** 2 + (py - my) ** 2
                if d2_min is None or d2 < d2_min:
                    d2_min = d2
                    best = (i, si, ydata[i])
        if d2_min is not None and d2_min < 100 and best is not None:
            i, si, yv = best
            hover.xy = (xdata[i], yv)
            hover.set_text(f"s{si}: x={xdata[i]:.3f}, y={yv:.3f}")
            hover.set_visible(True)
        else:
            hover.set_visible(False)

    fig.canvas.mpl_connect("motion_notify_event", on_hover)

    def update(_frame: int):
        # Drain buffered lines from the reader thread
        nonlocal lines_read, values_parsed, series_count, series
        if paused:
            return tuple(lines) + (status_text, hover, rate_text, total_text, stats_text)
        err = reader.get_error()
        if err:
            _set_status(f"Serial error: {err}")
            return tuple(lines) + (status_text, hover, rate_text, total_text, stats_text)
        ts_latest = None
        for ts_abs, raw_line in reader.get_lines(5000):
            ts = ts_abs - start_t
            ts_latest = ts
            m = int(ts // 60)
            s = ts - m * 60
            stream_lines.append(f"[{m:02d}:{s:06.3f}] {raw_line}")
            lines_read += 1
            vals = _parse_line_values(raw_line)
            if vals is None:
                continue
            if series_count == 0:
                series_count = len(vals)
                series = [deque(maxlen=args.max_points) for _ in range(series_count)]
                colors = plt.rcParams["axes.prop_cycle"].by_key().get("color", [])
                for i in range(series_count):
                    color = colors[i % len(colors)] if colors else None
                    line_i, = ax.plot([], [], linewidth=1.2, color=color)
                    lines.append(line_i)
                _set_status(f"Detected {series_count} series")
            if len(vals) != series_count:
                _set_status(f"Skipping line with {len(vals)} values (expected {series_count})")
                continue
            values_parsed += 1
            xs.append(ts)
            for i, v in enumerate(vals):
                series[i].append(v)
            value_times.append(ts)

        if xs and series:
            xdata = list(xs)
            for i, line_i in enumerate(lines):
                line_i.set_data(xdata, list(series[i]))
            if auto_x:
                xmax = xs[-1]
                xmin = max(0.0, xmax - x_window)
                ax.set_xlim(xmin, xmax)
            else:
                if x_min_val is not None and x_max_val is not None:
                    ax.set_xlim(x_min_val, x_max_val)

            if auto_y:
                all_vals = [v for s in series for v in s]
                if all_vals:
                    y_min = min(all_vals)
                    y_max = max(all_vals)
                else:
                    y_min = 0.0
                    y_max = 1.0
                if y_min == y_max:
                    y_min -= 1.0
                    y_max += 1.0
                pad = 0.05 * (y_max - y_min)
                ax.set_ylim(y_min - pad, y_max + pad)
            else:
                if y_min_val is not None and y_max_val is not None:
                    ax.set_ylim(y_min_val, y_max_val)
            all_vals = [v for s in series for v in s]
            y_min = min(all_vals) if all_vals else 0.0
            y_max = max(all_vals) if all_vals else 1.0
            y_avg = sum(all_vals) / max(1, len(all_vals)) if all_vals else 0.0
            stats_text.set_text(f"y min/max/avg: {y_min:.3f} / {y_max:.3f} / {y_avg:.3f}")

        if ts_latest is not None:
            while value_times and (ts_latest - value_times[0]) > 1.0:
                value_times.popleft()
        if value_times and ts_latest is not None:
            window = max(0.001, ts_latest - value_times[0])
            rate = len(value_times) / window
            rate_text.set_text(f"{rate:0.1f} values/s")
            total_text.set_text(f"total: {values_parsed}")

        if stream_fig is not None and stream_text is not None and plt.fignum_exists(stream_fig.number):
            header = f"lines: {lines_read}  values: {values_parsed}"
            tail = list(stream_lines)[-stream_visible:]
            body = "\n".join(tail) if tail else "(no data yet)"
            stream_text.set_text(header + "\n" + body)
            stream_fig.canvas.draw_idle()
        return tuple(lines) + (status_text, hover, rate_text, total_text, stats_text)

    def _toggle_pause() -> None:
        nonlocal paused
        paused = not paused
        btn_pause.label.set_text("Start" if paused else "Pause")
        _set_status("Paused" if paused else "Running")
        ax_zoom_in.set_visible(paused)
        ax_zoom_out.set_visible(paused)
        ax_fit.set_visible(paused)
        fig.canvas.draw_idle()

    def _clear_data() -> None:
        nonlocal lines_read, values_parsed, start_t, series_count, series, lines
        xs.clear()
        series.clear()
        for ln in lines:
            ln.set_data([], [])
        lines.clear()
        series_count = 0
        stream_lines.clear()
        value_times.clear()
        lines_read = 0
        values_parsed = 0
        start_t = time.time()
        rate_text.set_text("0.0 values/s")
        total_text.set_text("total: 0")
        for ln in lines:
            ln.set_data([], [])
        if stream_fig is not None and stream_text is not None:
            stream_text.set_text("(cleared)")
        _set_status("Cleared")

    def _zoom(factor: float) -> None:
        nonlocal x_window
        x_window = max(0.1, x_window * factor)
        tb_xwin.set_val(f"{x_window:.3f}")
        _set_status(f"Zoom {'in' if factor < 1.0 else 'out'}: window {x_window:.3f}s")

    def _fit() -> None:
        if not xs:
            return
        ax.set_xlim(min(xs), max(xs))
        all_vals = [v for s in series for v in s]
        if all_vals:
            ax.set_ylim(min(all_vals), max(all_vals))
        _set_status("Fit to data")

    if os.environ.get("SERIAL_PLOT_TEST_MODE") == "1":
        return {
            "open_stream_window": _open_stream_window,
            "get_stream_fig": lambda: stream_fig,
        }

    if os.environ.get("SERIAL_PLOT_NO_SHOW") == "1":
        update(0)
        plt.close(fig)
        reader.close()
        return 0

    _ = FuncAnimation(fig, update, interval=50, blit=False, cache_frame_data=False)
    plt.show()
    reader.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
