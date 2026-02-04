#!/usr/bin/env python3
"""
gen.py

Generate cursive(ish) text paths and export as X,Y + penDown commands compatible with your plotter.

Modes:
  1) Outline mode (default): converts the font outline into strokes (draws the outline).
  2) One-line mode (--one-line): rasterizes the filled glyph, skeletonizes it, and traces centerline strokes.

Simulation:
  --simulate animates the drawing with pen-down (solid) and pen-up travel (dashed).

Output:
  Writes a C header with:
    typedef PlotCmd { float x_mm, y_mm; uint8_t penDown; uint16_t dwell_ms; }
    static const PlotCmd PLOT_PROGRAM[] = { ... }
    static const size_t PLOT_PROGRAM_LEN = ...

Notes:
  - "Cursive" depends heavily on the font. For reliable script, pass --font /path/to/script.ttf.
  - One-line mode is skeleton-based; it can be jagged unless you use smoothing/simplify and finer raster.
"""

from __future__ import annotations

import argparse
import math
import os
from dataclasses import dataclass
from typing import List, Optional, Sequence, Tuple

import numpy as np
from matplotlib.font_manager import FontProperties, findfont
from matplotlib.path import Path
from matplotlib.textpath import TextPath
from matplotlib.transforms import Affine2D


# -----------------------------
# Logging / Debug
# -----------------------------

class Log:
    def __init__(self, debug: bool) -> None:
        self.debug_enabled = debug

    def info(self, msg: str) -> None:
        print(msg)

    def debug(self, msg: str) -> None:
        if self.debug_enabled:
            print(f"[DEBUG] {msg}")


# -----------------------------
# Data types
# -----------------------------

@dataclass(frozen=True)
class PlotCmd:
    x_mm: float
    y_mm: float
    pen_down: int  # 0/1
    dwell_ms: int = 0


@dataclass(frozen=True)
class Stroke:
    pts: List[Tuple[float, float]]

@dataclass(frozen=True)
class OneLineDebugOptions:
    show_mask: bool = False
    show_closed_mask: bool = False
    show_distance: bool = False
    show_skeleton: bool = False
    show_trace: bool = False


# -----------------------------
# Geometry helpers
# -----------------------------

def _dist(a: Tuple[float, float], b: Tuple[float, float]) -> float:
    return math.hypot(a[0] - b[0], a[1] - b[1])


def _lerp(a: float, b: float, t: float) -> float:
    return a + (b - a) * t


def _quad_bezier(p0, p1, p2, t: float) -> Tuple[float, float]:
    u = 1.0 - t
    x = (u * u) * p0[0] + 2 * u * t * p1[0] + (t * t) * p2[0]
    y = (u * u) * p0[1] + 2 * u * t * p1[1] + (t * t) * p2[1]
    return (x, y)


def _cubic_bezier(p0, p1, p2, p3, t: float) -> Tuple[float, float]:
    u = 1.0 - t
    x = (u * u * u) * p0[0] + 3 * (u * u) * t * p1[0] + 3 * u * (t * t) * p2[0] + (t * t * t) * p3[0]
    y = (u * u * u) * p0[1] + 3 * (u * u) * t * p1[1] + 3 * u * (t * t) * p2[1] + (t * t * t) * p3[1]
    return (x, y)


def _approx_segments_count(length_est: float, max_seg_len: float, min_n: int = 4, max_n: int = 200) -> int:
    if max_seg_len <= 0:
        return max_n
    n = int(math.ceil(length_est / max_seg_len))
    return max(min_n, min(max_n, n))


def _resample_polyline(points: Sequence[Tuple[float, float]], max_step: float) -> List[Tuple[float, float]]:
    """
    Ensure consecutive points are no more than max_step apart.
    """
    if not points:
        return []
    if max_step <= 0:
        return list(points)

    out = [points[0]]
    for i in range(1, len(points)):
        a = out[-1]
        b = points[i]
        d = _dist(a, b)
        if d <= max_step:
            out.append(b)
            continue
        steps = int(math.ceil(d / max_step))
        for s in range(1, steps + 1):
            t = s / steps
            out.append((_lerp(a[0], b[0], t), _lerp(a[1], b[1], t)))
    return out


# -----------------------------
# Font resolving
# -----------------------------

class FontResolver:
    CANDIDATE_FAMILIES = [
        "Pacifico",
        "Dancing Script",
        "Brush Script MT",
        "Apple Chancery",
        "Snell Roundhand",
        "Segoe Script",
        "Comic Sans MS",
        "DejaVu Sans",
    ]

    def __init__(self, log: Log) -> None:
        self._log = log

    def resolve(self, font_arg: Optional[str]) -> Tuple[FontProperties, str]:
        if font_arg:
            if os.path.isfile(font_arg):
                fp = FontProperties(fname=font_arg)
                self._log.debug(f"Using font file: {font_arg}")
                return fp, font_arg
            fp = FontProperties(family=font_arg)
            path = findfont(fp, fallback_to_default=True)
            self._log.debug(f"Resolved font family '{font_arg}' -> {path}")
            return fp, path

        for fam in self.CANDIDATE_FAMILIES:
            fp = FontProperties(family=fam)
            path = findfont(fp, fallback_to_default=True)
            self._log.debug(f"Candidate font '{fam}' -> {path}")
            if path and os.path.isfile(path):
                return fp, path

        fp = FontProperties(family="DejaVu Sans")
        path = findfont(fp, fallback_to_default=True)
        return fp, path


# -----------------------------
# Outline: TextPath -> strokes
# -----------------------------

class TextToStrokes:
    def __init__(self, log: Log) -> None:
        self._log = log

    def build_path(self, text: str, font: FontProperties, font_size: float) -> Path:
        tp = TextPath((0, 0), text, size=font_size, prop=font, usetex=False)
        self._log.debug(f"TextPath: vertices={len(tp.vertices)}, codes={len(tp.codes) if tp.codes is not None else 0}")
        return tp

    def path_to_strokes(self, p: Path, curve_max_seg_len: float) -> List[Stroke]:
        """
        Convert matplotlib Path into polyline strokes, flattening curves.
        """
        verts = p.vertices
        codes = p.codes
        if codes is None:
            return [Stroke(list(map(tuple, verts)))]

        strokes: List[Stroke] = []
        cur: List[Tuple[float, float]] = []

        last = (0.0, 0.0)
        start = (0.0, 0.0)
        i = 0

        def flush():
            nonlocal cur
            if len(cur) >= 2:
                strokes.append(Stroke(cur))
            cur = []

        while i < len(codes):
            code = codes[i]
            v = (float(verts[i][0]), float(verts[i][1]))

            if code == Path.MOVETO:
                flush()
                cur = [v]
                start = v
                last = v
                i += 1
                continue

            if code == Path.LINETO:
                cur.append(v)
                last = v
                i += 1
                continue

            if code == Path.CURVE3:
                if i + 1 >= len(verts):
                    break
                ctrl = v
                end = (float(verts[i + 1][0]), float(verts[i + 1][1]))
                length_est = _dist(last, ctrl) + _dist(ctrl, end)
                n = _approx_segments_count(length_est, curve_max_seg_len)
                for s in range(1, n + 1):
                    t = s / n
                    cur.append(_quad_bezier(last, ctrl, end, t))
                last = end
                i += 2
                continue

            if code == Path.CURVE4:
                if i + 2 >= len(verts):
                    break
                c1 = v
                c2 = (float(verts[i + 1][0]), float(verts[i + 1][1]))
                end = (float(verts[i + 2][0]), float(verts[i + 2][1]))
                length_est = _dist(last, c1) + _dist(c1, c2) + _dist(c2, end)
                n = _approx_segments_count(length_est, curve_max_seg_len)
                for s in range(1, n + 1):
                    t = s / n
                    cur.append(_cubic_bezier(last, c1, c2, end, t))
                last = end
                i += 3
                continue

            if code == Path.CLOSEPOLY:
                if cur and _dist(cur[-1], start) > 1e-9:
                    cur.append(start)
                flush()
                i += 1
                continue

            self._log.debug(f"Unknown path code {code} at {i}; flushing")
            flush()
            last = v
            i += 1

        flush()
        self._log.debug(f"Strokes={len(strokes)}")
        return strokes


# -----------------------------
# Scaling to mm box (outline mode)
# -----------------------------

class StrokeScaler:
    def __init__(self, log: Log) -> None:
        self._log = log

    def _bbox(self, strokes: Sequence[Stroke]) -> Tuple[float, float, float, float]:
        xs, ys = [], []
        for st in strokes:
            for (x, y) in st.pts:
                xs.append(x)
                ys.append(y)
        if not xs:
            return (0, 0, 0, 0)
        return (min(xs), min(ys), max(xs), max(ys))

    def scale_to_box(self, strokes: List[Stroke], width_mm: float, height_mm: float, margin_mm: float) -> List[Stroke]:
        if width_mm <= 0 or height_mm <= 0:
            raise ValueError("width_mm and height_mm must be > 0")

        minx, miny, maxx, maxy = self._bbox(strokes)
        w = maxx - minx
        h = maxy - miny
        self._log.debug(f"Font bbox min=({minx:.3f},{miny:.3f}) max=({maxx:.3f},{maxy:.3f}) size=({w:.3f},{h:.3f})")

        if w <= 0 or h <= 0:
            raise ValueError("Text path has zero size. Check text/font.")

        inner_w = max(1e-9, width_mm - 2 * margin_mm)
        inner_h = max(1e-9, height_mm - 2 * margin_mm)
        s = min(inner_w / w, inner_h / h)

        scaled: List[Stroke] = []
        for st in strokes:
            pts = []
            for (x, y) in st.pts:
                xx = (x - minx) * s + margin_mm
                yy = (y - miny) * s + margin_mm
                pts.append((xx, yy))
            scaled.append(Stroke(pts))

        minx2, miny2, maxx2, maxy2 = self._bbox(scaled)
        w2 = maxx2 - minx2
        h2 = maxy2 - miny2
        dx = (width_mm - w2) / 2.0 - minx2
        dy = (height_mm - h2) / 2.0 - miny2

        self._log.debug(f"Scale s={s:.6f}, scaled bbox=({w2:.3f},{h2:.3f}), translate=({dx:.3f},{dy:.3f})")

        out: List[Stroke] = []
        for st in scaled:
            out.append(Stroke([(x + dx, y + dy) for (x, y) in st.pts]))
        return out


# -----------------------------
# One-line: skeletonize filled glyph
# -----------------------------

def _make_text_path_mm(text: str, font_prop: FontProperties, font_size: float,
                       width_mm: float, height_mm: float, margin_mm: float, log: Log) -> Path:
    tp = TextPath((0, 0), text, size=font_size, prop=font_prop, usetex=False)

    verts = tp.vertices
    minx, miny = float(verts[:, 0].min()), float(verts[:, 1].min())
    maxx, maxy = float(verts[:, 0].max()), float(verts[:, 1].max())
    w, h = (maxx - minx), (maxy - miny)
    if w <= 0 or h <= 0:
        raise ValueError("Text path has zero size. Check text/font.")

    inner_w = max(1e-9, width_mm - 2 * margin_mm)
    inner_h = max(1e-9, height_mm - 2 * margin_mm)
    s = min(inner_w / w, inner_h / h)

    # Normalize -> scale -> margin
    t = Affine2D().translate(-minx, -miny).scale(s).translate(margin_mm, margin_mm)
    p1 = t.transform_path(tp)

    # Center
    v1 = p1.vertices
    minx2, miny2 = float(v1[:, 0].min()), float(v1[:, 1].min())
    maxx2, maxy2 = float(v1[:, 0].max()), float(v1[:, 1].max())
    w2, h2 = (maxx2 - minx2), (maxy2 - miny2)
    dx = (width_mm - w2) / 2.0 - minx2
    dy = (height_mm - h2) / 2.0 - miny2
    p2 = Affine2D().translate(dx, dy).transform_path(p1)

    log.debug(f"--one-line fit: scale={s:.6f}, bbox=({w2:.2f}x{h2:.2f}), center_shift=({dx:.2f},{dy:.2f})")
    return p2


def _path_to_mask_mm_contains(p_mm: Path, width_mm: float, height_mm: float, pixel_mm: float, log: Log) -> np.ndarray:
    w_px = max(8, int(math.ceil(width_mm / pixel_mm)))
    h_px = max(8, int(math.ceil(height_mm / pixel_mm)))

    xs = (np.arange(w_px) + 0.5) * pixel_mm
    ys = (np.arange(h_px) + 0.5) * pixel_mm
    X, Y = np.meshgrid(xs, ys)
    pts = np.column_stack([X.ravel(), Y.ravel()])

    inside = p_mm.contains_points(pts)
    mask = inside.reshape((h_px, w_px))

    fill_ratio = float(mask.mean()) if mask.size else 0.0
    log.debug(f"--one-line raster (contains): pixel_mm={pixel_mm}, size_px=({w_px}x{h_px}), fill_ratio={fill_ratio:.4f}")
    return mask.astype(bool)


def _path_to_mask_mm_render(p_mm: Path, width_mm: float, height_mm: float, pixel_mm: float, log: Log) -> np.ndarray:
    from matplotlib.backends.backend_agg import FigureCanvasAgg
    from matplotlib.figure import Figure
    from matplotlib.patches import PathPatch

    w_px = max(8, int(math.ceil(width_mm / pixel_mm)))
    h_px = max(8, int(math.ceil(height_mm / pixel_mm)))
    dpi = 25.4 / pixel_mm

    fig = Figure(figsize=(width_mm / 25.4, height_mm / 25.4), dpi=dpi)
    fig.patch.set_facecolor("black")
    fig.patch.set_alpha(1.0)
    canvas = FigureCanvasAgg(fig)
    ax = fig.add_axes([0, 0, 1, 1])
    ax.set_xlim(0, width_mm)
    ax.set_ylim(0, height_mm)
    ax.set_aspect("equal", adjustable="box")
    ax.set_axis_off()
    ax.set_facecolor("black")

    patch = PathPatch(p_mm, facecolor="white", edgecolor="white", lw=0)
    patch.set_transform(ax.transData)
    if hasattr(patch, "set_fillrule"):
        patch.set_fillrule("evenodd")
    ax.add_patch(patch)

    canvas.draw()
    buf = np.asarray(canvas.buffer_rgba())
    rgb = buf[..., :3].astype(np.uint16)
    mask = (rgb.mean(axis=2) > 10)
    mask = np.flipud(mask)

    fill_ratio = float(mask.mean()) if mask.size else 0.0
    log.debug(f"--one-line raster (render): pixel_mm={pixel_mm}, size_px=({w_px}x{h_px}), fill_ratio={fill_ratio:.4f}")
    return mask.astype(bool)


def _path_to_mask_mm(p_mm: Path, width_mm: float, height_mm: float, pixel_mm: float,
                     method: str, log: Log) -> np.ndarray:
    if method == "contains":
        return _path_to_mask_mm_contains(p_mm, width_mm, height_mm, pixel_mm, log)
    return _path_to_mask_mm_render(p_mm, width_mm, height_mm, pixel_mm, log)


def _binary_dilate(mask: np.ndarray, radius_px: int) -> np.ndarray:
    if radius_px <= 0:
        return mask
    pad = radius_px
    padded = np.pad(mask, ((pad, pad), (pad, pad)), mode="constant", constant_values=False)
    out = np.zeros_like(mask, dtype=bool)
    h, w = mask.shape
    for dr in range(-radius_px, radius_px + 1):
        r0 = pad + dr
        r1 = r0 + h
        for dc in range(-radius_px, radius_px + 1):
            c0 = pad + dc
            c1 = c0 + w
            out |= padded[r0:r1, c0:c1]
    return out


def _binary_erode(mask: np.ndarray, radius_px: int) -> np.ndarray:
    if radius_px <= 0:
        return mask
    return ~_binary_dilate(~mask, radius_px)


def _binary_close(mask: np.ndarray, radius_px: int) -> np.ndarray:
    if radius_px <= 0:
        return mask
    return _binary_erode(_binary_dilate(mask, radius_px), radius_px)


def _zhang_suen_thin(mask: np.ndarray, max_iter: int, log: Log) -> np.ndarray:
    img = mask.astype(np.uint8).copy()

    def neighbors8(a: np.ndarray) -> Tuple[np.ndarray, ...]:
        P2 = np.roll(a, -1, axis=0)
        P3 = np.roll(P2,  1, axis=1)
        P4 = np.roll(a,  1, axis=1)
        P5 = np.roll(P4,  1, axis=0)
        P6 = np.roll(a,  1, axis=0)
        P7 = np.roll(P6, -1, axis=1)
        P8 = np.roll(a, -1, axis=1)
        P9 = np.roll(P8, -1, axis=0)
        return P2, P3, P4, P5, P6, P7, P8, P9

    def clear_borders(a: np.ndarray) -> None:
        a[0, :] = 0
        a[-1, :] = 0
        a[:, 0] = 0
        a[:, -1] = 0

    clear_borders(img)

    changed = True
    it = 0
    while changed and it < max_iter:
        changed = False
        it += 1

        P2, P3, P4, P5, P6, P7, P8, P9 = neighbors8(img)
        B = P2 + P3 + P4 + P5 + P6 + P7 + P8 + P9

        seq = [P2, P3, P4, P5, P6, P7, P8, P9, P2]
        A = np.zeros_like(img)
        for k in range(8):
            A += ((seq[k] == 0) & (seq[k + 1] == 1)).astype(np.uint8)

        m1 = (img == 1) & (B >= 2) & (B <= 6) & (A == 1) & ((P2 * P4 * P6) == 0) & ((P4 * P6 * P8) == 0)
        if np.any(m1):
            img[m1] = 0
            changed = True

        clear_borders(img)

        P2, P3, P4, P5, P6, P7, P8, P9 = neighbors8(img)
        B = P2 + P3 + P4 + P5 + P6 + P7 + P8 + P9

        seq = [P2, P3, P4, P5, P6, P7, P8, P9, P2]
        A = np.zeros_like(img)
        for k in range(8):
            A += ((seq[k] == 0) & (seq[k + 1] == 1)).astype(np.uint8)

        m2 = (img == 1) & (B >= 2) & (B <= 6) & (A == 1) & ((P2 * P4 * P8) == 0) & ((P2 * P6 * P8) == 0)
        if np.any(m2):
            img[m2] = 0
            changed = True

        clear_borders(img)

    log.debug(f"--one-line thinning: iterations={it}, skeleton_pixels={int(img.sum())}")
    return img.astype(bool)


def _distance_transform_chamfer(mask: np.ndarray) -> np.ndarray:
    h, w = mask.shape
    inf = 10 ** 9
    dist = np.full((h, w), inf, dtype=np.int32)
    dist[~mask] = 0

    w_ortho = 3
    w_diag = 4

    for r in range(h):
        for c in range(w):
            if not mask[r, c]:
                continue
            d = dist[r, c]
            if r > 0:
                d = min(d, dist[r - 1, c] + w_ortho)
                if c > 0:
                    d = min(d, dist[r - 1, c - 1] + w_diag)
                if c + 1 < w:
                    d = min(d, dist[r - 1, c + 1] + w_diag)
            if c > 0:
                d = min(d, dist[r, c - 1] + w_ortho)
            dist[r, c] = d

    for r in range(h - 1, -1, -1):
        for c in range(w - 1, -1, -1):
            if not mask[r, c]:
                continue
            d = dist[r, c]
            if r + 1 < h:
                d = min(d, dist[r + 1, c] + w_ortho)
                if c > 0:
                    d = min(d, dist[r + 1, c - 1] + w_diag)
                if c + 1 < w:
                    d = min(d, dist[r + 1, c + 1] + w_diag)
            if c + 1 < w:
                d = min(d, dist[r, c + 1] + w_ortho)
            dist[r, c] = d

    return dist


def _ridge_skeleton_from_dist(mask: np.ndarray, dist: np.ndarray, log: Log) -> np.ndarray:
    skel = np.zeros_like(mask, dtype=bool)
    h, w = mask.shape

    for r in range(1, h - 1):
        for c in range(1, w - 1):
            if not mask[r, c]:
                continue
            d = dist[r, c]
            if d == 0:
                continue
            local = dist[r - 1:r + 2, c - 1:c + 2]
            if d >= local.max():
                skel[r, c] = True

    log.debug(f"--one-line ridge: skeleton_pixels={int(skel.sum())}")
    return skel


def _show_one_line_debug(mask: np.ndarray,
                         closed_mask: Optional[np.ndarray],
                         dist: Optional[np.ndarray],
                         skel: np.ndarray,
                         strokes_mm: Optional[List[Stroke]],
                         width_mm: float,
                         height_mm: float,
                         pixel_mm: float,
                         opts: OneLineDebugOptions) -> None:
    import matplotlib.pyplot as plt

    views: List[Tuple[str, np.ndarray, str]] = []
    if opts.show_mask:
        views.append(("Mask", mask.astype(float), "gray"))
    if opts.show_closed_mask and closed_mask is not None:
        views.append(("Closed Mask", closed_mask.astype(float), "gray"))
    if opts.show_distance and dist is not None:
        views.append(("Distance", dist.astype(float), "viridis"))
    if opts.show_skeleton:
        views.append(("Skeleton", skel.astype(float), "gray"))

    if views:
        n = len(views)
        fig, axs = plt.subplots(1, n, figsize=(4 * n, 4))
        if n == 1:
            axs = [axs]
        for ax, (title, img, cmap) in zip(axs, views):
            ax.imshow(img, origin="lower", extent=(0, width_mm, 0, height_mm), cmap=cmap)
            ax.set_title(title)
            ax.set_aspect("equal", adjustable="box")
        plt.show()

    if opts.show_trace and strokes_mm is not None:
        fig, ax = plt.subplots()
        for st in strokes_mm:
            xs = [p[0] for p in st.pts]
            ys = [p[1] for p in st.pts]
            ax.plot(xs, ys, linewidth=1.0)
        ax.set_xlim(0, width_mm)
        ax.set_ylim(0, height_mm)
        ax.set_aspect("equal", adjustable="box")
        ax.set_title("Traced Strokes")
        plt.show()


def _skeleton_degrees(skel: np.ndarray) -> np.ndarray:
    a = skel.astype(np.uint8)
    deg = np.zeros_like(a, dtype=np.uint8)
    for dr in (-1, 0, 1):
        for dc in (-1, 0, 1):
            if dr == 0 and dc == 0:
                continue
            deg += np.roll(np.roll(a, dr, axis=0), dc, axis=1)
    deg[~skel] = 0
    return deg


def _trace_skeleton_to_strokes(skel: np.ndarray, pixel_mm: float, width_mm: float, height_mm: float,
                               min_stroke_mm: float, log: Log) -> List[Stroke]:
    H, W = skel.shape
    deg = _skeleton_degrees(skel)

    def nbrs(r: int, c: int) -> List[Tuple[int, int]]:
        out = []
        for dr in (-1, 0, 1):
            for dc in (-1, 0, 1):
                if dr == 0 and dc == 0:
                    continue
                rr, cc = r + dr, c + dc
                if 0 <= rr < H and 0 <= cc < W and skel[rr, cc]:
                    out.append((rr, cc))
        return out

    visited = np.zeros_like(skel, dtype=bool)

    def to_mm(r: int, c: int) -> Tuple[float, float]:
        x = (c + 0.5) * pixel_mm
        y = (r + 0.5) * pixel_mm
        x = max(0.0, min(width_mm, x))
        y = max(0.0, min(height_mm, y))
        return (x, y)

    endpoints = list(zip(*np.where((skel) & (deg == 1))))
    junctions = int(((skel) & (deg >= 3)).sum())
    log.debug(f"--one-line trace: endpoints={len(endpoints)}, junction_pixels={junctions}")

    def walk(start: Tuple[int, int]) -> List[Tuple[int, int]]:
        path = [start]
        visited[start] = True
        prev = None
        cur = start

        while True:
            n = nbrs(cur[0], cur[1])
            if prev is not None and deg[cur] != 2:
                break

            nxt = None
            for cand in n:
                if prev is not None and cand == prev:
                    continue
                if not visited[cand]:
                    nxt = cand
                    break

            if nxt is None:
                for cand in n:
                    if prev is not None and cand == prev:
                        continue
                    nxt = cand
                    break

            if nxt is None:
                break

            if visited[nxt]:
                if nxt == start:
                    path.append(nxt)
                break

            path.append(nxt)
            visited[nxt] = True
            prev, cur = cur, nxt

        return path

    pix_strokes: List[List[Tuple[int, int]]] = []

    for ep in endpoints:
        if visited[ep]:
            continue
        p = walk(ep)
        if len(p) >= 2:
            pix_strokes.append(p)

    remaining = list(zip(*np.where(skel & (~visited))))
    for p in remaining:
        if visited[p]:
            continue
        q = walk(p)
        if len(q) >= 2:
            pix_strokes.append(q)

    out: List[Stroke] = []
    kept = 0
    dropped = 0
    for pix in pix_strokes:
        pts = [to_mm(r, c) for (r, c) in pix]
        length = 0.0
        for i in range(1, len(pts)):
            length += _dist(pts[i - 1], pts[i])
        if length < min_stroke_mm:
            dropped += 1
            continue
        kept += 1
        out.append(Stroke(pts))

    log.debug(f"--one-line strokes: raw={len(pix_strokes)} kept={kept} dropped={dropped} min_stroke_mm={min_stroke_mm}")
    return out


def _trace_skeleton_to_single_stroke(skel: np.ndarray, pixel_mm: float, width_mm: float, height_mm: float,
                                     log: Log) -> List[Stroke]:
    H, W = skel.shape

    def nbrs(r: int, c: int) -> List[Tuple[int, int]]:
        out = []
        for dr in (-1, 0, 1):
            for dc in (-1, 0, 1):
                if dr == 0 and dc == 0:
                    continue
                rr, cc = r + dr, c + dc
                if 0 <= rr < H and 0 <= cc < W and skel[rr, cc]:
                    out.append((rr, cc))
        return out

    def to_mm(r: int, c: int) -> Tuple[float, float]:
        x = (c + 0.5) * pixel_mm
        y = (r + 0.5) * pixel_mm
        x = max(0.0, min(width_mm, x))
        y = max(0.0, min(height_mm, y))
        return (x, y)

    nodes = list(zip(*np.where(skel)))
    if not nodes:
        return []

    adj = {}
    for (r, c) in nodes:
        adj[(r, c)] = nbrs(r, c)

    def edge_key(a: Tuple[int, int], b: Tuple[int, int]) -> Tuple[int, int, int, int]:
        return (a[0], a[1], b[0], b[1]) if a <= b else (b[0], b[1], a[0], a[1])

    edges = set()
    for a, ns in adj.items():
        for b in ns:
            edges.add(edge_key(a, b))

    def remaining_nodes() -> List[Tuple[int, int]]:
        out = []
        for a, ns in adj.items():
            for b in ns:
                if edge_key(a, b) in edges:
                    out.append(a)
                    break
        return out

    def has_remaining_edge(a: Tuple[int, int]) -> bool:
        for b in adj[a]:
            if edge_key(a, b) in edges:
                return True
        return False

    def pick_start() -> Optional[Tuple[int, int]]:
        candidates = remaining_nodes()
        if not candidates:
            return None
        endpoints = [a for a in candidates if sum(1 for b in adj[a] if edge_key(a, b) in edges) == 1]
        return endpoints[0] if endpoints else candidates[0]

    def nearest_node(a: Tuple[int, int], candidates: List[Tuple[int, int]]) -> Tuple[int, int]:
        best = candidates[0]
        best_d = (a[0] - best[0]) ** 2 + (a[1] - best[1]) ** 2
        for c in candidates[1:]:
            d = (a[0] - c[0]) ** 2 + (a[1] - c[1]) ** 2
            if d < best_d:
                best_d = d
                best = c
        return best

    path_nodes: List[Tuple[int, int]] = []
    cur = pick_start()
    if cur is None:
        return []
    path_nodes.append(cur)

    while edges:
        if not has_remaining_edge(cur):
            candidates = remaining_nodes()
            if not candidates:
                break
            nxt = nearest_node(cur, candidates)
            path_nodes.append(nxt)  # pen-down bridge between components
            cur = nxt
            continue

        nxt = None
        for cand in adj[cur]:
            k = edge_key(cur, cand)
            if k in edges:
                nxt = cand
                edges.remove(k)
                break

        if nxt is None:
            candidates = remaining_nodes()
            if not candidates:
                break
            nxt = nearest_node(cur, candidates)
            path_nodes.append(nxt)
            cur = nxt
            continue

        path_nodes.append(nxt)
        cur = nxt

    # Remove immediate duplicates
    dedup = [path_nodes[0]]
    for p in path_nodes[1:]:
        if p != dedup[-1]:
            dedup.append(p)

    pts = [to_mm(r, c) for (r, c) in dedup]
    log.debug(f"--one-line walk: nodes={len(nodes)} remaining_edges={len(edges)} path_pts={len(pts)}")
    return [Stroke(pts)]


def text_to_centerline_strokes_mm(text: str, font_prop: FontProperties,
                                  width_mm: float, height_mm: float,
                                  font_size: float, margin_mm: float,
                                  raster_mm: float, thin_max_iter: int,
                                  min_stroke_mm: float, close_mm: float,
                                  skeleton_method: str, mask_method: str,
                                  trace_mode: str, log: Log,
                                  debug_opts: Optional[OneLineDebugOptions] = None) -> List[Stroke]:
    p_mm = _make_text_path_mm(text, font_prop, font_size, width_mm, height_mm, margin_mm, log)
    mask = _path_to_mask_mm(p_mm, width_mm, height_mm, raster_mm, mask_method, log)
    if mask.sum() == 0:
        raise RuntimeError("Rasterized mask is empty. Increase font_size or reduce margin or raster_mm.")
    closed_mask: Optional[np.ndarray] = None
    if close_mm > 0:
        radius_px = int(math.ceil(close_mm / raster_mm))
        if radius_px > 0:
            closed_mask = _binary_close(mask, radius_px)
            mask = closed_mask
            log.debug(f"--one-line close: close_mm={close_mm:.3f} radius_px={radius_px}")
    dist: Optional[np.ndarray] = None
    if skeleton_method == "ridge":
        dist = _distance_transform_chamfer(mask)
        skel = _ridge_skeleton_from_dist(mask, dist, log=log)
        if thin_max_iter > 0:
            skel = _zhang_suen_thin(skel, max_iter=thin_max_iter, log=log)
    else:
        skel = _zhang_suen_thin(mask, max_iter=thin_max_iter, log=log)
    if skel.sum() == 0:
        raise RuntimeError("Skeleton is empty. Try smaller raster_mm or different font.")
    if trace_mode == "walk":
        strokes_mm = _trace_skeleton_to_single_stroke(skel, raster_mm, width_mm, height_mm, log)
    else:
        strokes_mm = _trace_skeleton_to_strokes(skel, raster_mm, width_mm, height_mm, min_stroke_mm, log)
    if not strokes_mm:
        raise RuntimeError("No usable centerline strokes. Try smaller raster_mm or lower min_stroke_mm.")
    if debug_opts is not None and (debug_opts.show_mask or debug_opts.show_closed_mask
                                   or debug_opts.show_distance or debug_opts.show_skeleton
                                   or debug_opts.show_trace):
        _show_one_line_debug(mask, closed_mask, dist, skel, strokes_mm, width_mm, height_mm, raster_mm, debug_opts)
    return strokes_mm


# -----------------------------
# One-line polish: smooth / simplify / ordering / stitching
# -----------------------------

def chaikin_smooth(pts: List[Tuple[float, float]], iters: int) -> List[Tuple[float, float]]:
    if iters <= 0 or len(pts) < 3:
        return pts
    out = pts
    for _ in range(iters):
        new_pts = [out[0]]
        for i in range(len(out) - 1):
            x0, y0 = out[i]
            x1, y1 = out[i + 1]
            q = (0.75 * x0 + 0.25 * x1, 0.75 * y0 + 0.25 * y1)
            r = (0.25 * x0 + 0.75 * x1, 0.25 * y0 + 0.75 * y1)
            new_pts.extend([q, r])
        new_pts.append(out[-1])
        out = new_pts
    return out


def _perp_dist(p, a, b) -> float:
    (x, y), (x1, y1), (x2, y2) = p, a, b
    dx, dy = x2 - x1, y2 - y1
    if dx == 0 and dy == 0:
        return _dist(p, a)
    t = ((x - x1) * dx + (y - y1) * dy) / (dx * dx + dy * dy)
    t = 0.0 if t < 0 else 1.0 if t > 1 else t
    proj = (x1 + t * dx, y1 + t * dy)
    return _dist(p, proj)


def rdp_simplify(pts: List[Tuple[float, float]], eps: float) -> List[Tuple[float, float]]:
    if eps <= 0 or len(pts) < 3:
        return pts

    def rec(a: int, b: int, keep: List[bool]) -> None:
        max_d = -1.0
        idx = -1
        for i in range(a + 1, b):
            d = _perp_dist(pts[i], pts[a], pts[b])
            if d > max_d:
                max_d, idx = d, i
        if max_d > eps and idx != -1:
            keep[idx] = True
            rec(a, idx, keep)
            rec(idx, b, keep)

    keep = [False] * len(pts)
    keep[0] = keep[-1] = True
    rec(0, len(pts) - 1, keep)
    return [p for p, k in zip(pts, keep) if k]


def stroke_bbox_x(st: Stroke) -> Tuple[float, float]:
    xs = [p[0] for p in st.pts]
    return (min(xs), max(xs))


def stroke_endpoints(st: Stroke) -> Tuple[Tuple[float, float], Tuple[float, float]]:
    return (st.pts[0], st.pts[-1])


def stroke_bbox_y(st: Stroke) -> Tuple[float, float]:
    ys = [p[1] for p in st.pts]
    return (min(ys), max(ys))


def reorder_strokes_tlbr(strokes: List[Stroke], up_penalty: float, left_penalty: float, log: Log) -> List[Stroke]:
    if not strokes:
        return strokes

    remaining = strokes[:]
    ordered: List[Stroke] = []

    # Start near top-left (high y, low x)
    remaining.sort(key=lambda s: (-stroke_bbox_y(s)[1], stroke_bbox_x(s)[0]))
    cur = remaining.pop(0)
    ordered.append(cur)
    cur_pt = cur.pts[-1]

    while remaining:
        best_i = -1
        best_rev = False
        best_cost = 1e18

        for i, st in enumerate(remaining):
            a, b = stroke_endpoints(st)

            def cost_to(pt: Tuple[float, float]) -> float:
                dx = cur_pt[0] - pt[0]
                dy = cur_pt[1] - pt[1]
                return _dist(cur_pt, pt) + left_penalty * max(0.0, dx) + up_penalty * max(0.0, dy)

            cost_f = cost_to(a)
            cost_r = cost_to(b)

            if cost_f < best_cost:
                best_cost = cost_f
                best_i = i
                best_rev = False
            if cost_r < best_cost:
                best_cost = cost_r
                best_i = i
                best_rev = True

        nxt = remaining.pop(best_i)
        if best_rev:
            nxt = Stroke(list(reversed(nxt.pts)))

        ordered.append(nxt)
        cur_pt = nxt.pts[-1]

    log.debug(f"--optimize-tlbr: strokes_in={len(strokes)} strokes_out={len(ordered)}")
    return ordered


def reorder_strokes_lr(strokes: List[Stroke], lr_penalty: float, log: Log) -> List[Stroke]:
    if not strokes:
        return strokes

    remaining = strokes[:]
    ordered: List[Stroke] = []

    remaining.sort(key=lambda s: stroke_bbox_x(s)[0])
    cur = remaining.pop(0)
    ordered.append(cur)
    cur_pt = cur.pts[-1]
    cur_x = cur_pt[0]

    while remaining:
        best_i = -1
        best_rev = False
        best_cost = 1e18

        for i, st in enumerate(remaining):
            a, b = stroke_endpoints(st)

            cost_f = _dist(cur_pt, a) + lr_penalty * max(0.0, cur_x - a[0])
            cost_r = _dist(cur_pt, b) + lr_penalty * max(0.0, cur_x - b[0])

            if cost_f < best_cost:
                best_cost = cost_f
                best_i = i
                best_rev = False
            if cost_r < best_cost:
                best_cost = cost_r
                best_i = i
                best_rev = True

        nxt = remaining.pop(best_i)
        if best_rev:
            nxt = Stroke(list(reversed(nxt.pts)))

        ordered.append(nxt)
        cur_pt = nxt.pts[-1]
        cur_x = cur_pt[0]

    log.debug(f"--optimize-lr: strokes_in={len(strokes)} strokes_out={len(ordered)} lr_penalty={lr_penalty}")
    return ordered


def force_left_to_right(strokes: List[Stroke], log: Log) -> List[Stroke]:
    if not strokes:
        return strokes
    out: List[Stroke] = []
    for st in strokes:
        a, b = stroke_endpoints(st)
        if a[0] <= b[0]:
            out.append(st)
        else:
            out.append(Stroke(list(reversed(st.pts))))
    log.debug(f"--force-lr: strokes_in={len(strokes)} strokes_out={len(out)}")
    return out


def stitch_strokes(strokes: List[Stroke], stitch_mm: float, log: Log) -> List[Stroke]:
    if stitch_mm <= 0 or not strokes:
        return strokes

    out: List[Stroke] = []
    cur = strokes[0].pts[:]

    for st in strokes[1:]:
        if _dist(cur[-1], st.pts[0]) <= stitch_mm:
            if _dist(cur[-1], st.pts[0]) > 1e-9:
                cur.append(st.pts[0])
            cur.extend(st.pts[1:])
        else:
            out.append(Stroke(cur))
            cur = st.pts[:]
    out.append(Stroke(cur))

    log.debug(f"--stitch-mm: in={len(strokes)} out={len(out)} stitch_mm={stitch_mm}")
    return out


def bridge_strokes(strokes: List[Stroke], bridge_mm: float, log: Log) -> List[Stroke]:
    if bridge_mm <= 0 or not strokes:
        return strokes

    out: List[Stroke] = []
    cur = strokes[0].pts[:]

    for st in strokes[1:]:
        if _dist(cur[-1], st.pts[0]) <= bridge_mm:
            if _dist(cur[-1], st.pts[0]) > 1e-9:
                cur.append(st.pts[0])
            cur.extend(st.pts[1:])
        else:
            out.append(Stroke(cur))
            cur = st.pts[:]
    out.append(Stroke(cur))

    log.debug(f"--bridge-mm: in={len(strokes)} out={len(out)} bridge_mm={bridge_mm}")
    return out


# -----------------------------
# Exporter
# -----------------------------

class PlotProgramExporter:
    def __init__(self, log: Log) -> None:
        self._log = log

    def to_plot_cmds(
        self,
        strokes_mm: Sequence[Stroke],
        poly_step_mm: float,
        pen_settle_ms: int,
        segment_dwell_ms: int,
    ) -> List[PlotCmd]:
        cmds: List[PlotCmd] = []

        for si, st in enumerate(strokes_mm):
            pts = _resample_polyline(st.pts, poly_step_mm)
            if len(pts) < 2:
                continue

            sx, sy = pts[0]
            cmds.append(PlotCmd(sx, sy, 0, 0))               # travel pen up
            cmds.append(PlotCmd(sx, sy, 1, pen_settle_ms))   # pen down + settle

            for (x, y) in pts[1:]:
                cmds.append(PlotCmd(x, y, 1, segment_dwell_ms))

            ex, ey = pts[-1]
            cmds.append(PlotCmd(ex, ey, 0, 0))               # pen up at end

            self._log.debug(f"Stroke {si}: pts_in={len(st.pts)} pts_out={len(pts)} start=({sx:.2f},{sy:.2f}) end=({ex:.2f},{ey:.2f})")

        self._log.debug(f"Total cmds={len(cmds)}")
        return cmds

    def write_c_header(self, out_path: str, cmds: Sequence[PlotCmd]) -> None:
        self._log.info(f"Writing C header: {out_path}")
        with open(out_path, "w", encoding="utf-8") as f:
            f.write("static const PlotCmd PLOT_PROGRAM[] = {\n")
            for c in cmds:
                f.write(f"    {{ {c.x_mm:.3f}f, {c.y_mm:.3f}f, {int(c.pen_down)}, {int(c.dwell_ms)} }},\n")
            f.write("};\n\n")
            f.write("static const size_t PLOT_PROGRAM_LEN = sizeof(PLOT_PROGRAM) / sizeof(PLOT_PROGRAM[0]);\n")


# -----------------------------
# Simulation
# -----------------------------

def simulate(cmds: Sequence[PlotCmd], width_mm: float, height_mm: float, speed_mm_per_s: float, debug: bool) -> None:
    import matplotlib.pyplot as plt
    from matplotlib.animation import FuncAnimation
    from matplotlib.widgets import Slider

    if not cmds:
        raise ValueError("No commands to simulate.")

    segments: List[Tuple[Tuple[float, float], Tuple[float, float], int]] = []
    for i in range(1, len(cmds)):
        a = (cmds[i - 1].x_mm, cmds[i - 1].y_mm)
        b = (cmds[i].x_mm, cmds[i].y_mm)
        pen = cmds[i].pen_down
        if _dist(a, b) < 1e-9:
            continue
        segments.append((a, b, pen))

    if debug:
        print(f"[DEBUG] simulate: cmds={len(cmds)} segments={len(segments)}")

    interval_ms = 20
    dt = interval_ms / 1000.0
    step_per_frame = max(0.05, speed_mm_per_s * dt)

    expanded: List[Tuple[float, float, float, float, int]] = []
    for (a, b, pen) in segments:
        d = _dist(a, b)
        n = max(1, int(math.ceil(d / step_per_frame)))
        for k in range(1, n + 1):
            t0 = (k - 1) / n
            t1 = k / n
            x0 = _lerp(a[0], b[0], t0)
            y0 = _lerp(a[1], b[1], t0)
            x1 = _lerp(a[0], b[0], t1)
            y1 = _lerp(a[1], b[1], t1)
            expanded.append((x0, y0, x1, y1, pen))

    if debug:
        print(f"[DEBUG] simulate: expanded frames={len(expanded)} step_per_frame={step_per_frame:.3f}mm")

    fig, ax = plt.subplots()
    fig.subplots_adjust(bottom=0.2)
    ax.set_xlim(0, width_mm)
    ax.set_ylim(0, height_mm)
    ax.set_aspect("equal", adjustable="box")
    ax.set_title("Simulation (solid=pen down, dashed=pen up)")

    down_line, = ax.plot([], [], linewidth=1.5)
    up_line, = ax.plot([], [], linewidth=1.0, linestyle="--")

    down_x: List[float] = []
    down_y: List[float] = []
    up_x: List[float] = []
    up_y: List[float] = []

    def _append_seg(xs: List[float], ys: List[float], x0, y0, x1, y1):
        if xs:
            xs.append(float("nan"))
            ys.append(float("nan"))
        xs.extend([x0, x1])
        ys.extend([y0, y1])

    step_text = ax.text(0.01, 0.99, "", transform=ax.transAxes, va="top")

    cur_idx = 0
    advance_accum = 0.0
    base_speed = max(1e-6, speed_mm_per_s)
    speed_scale = 1.0

    def init():
        down_line.set_data([], [])
        up_line.set_data([], [])
        step_text.set_text("")
        return down_line, up_line, step_text

    def update(frame_idx: int):
        nonlocal cur_idx, advance_accum
        if cur_idx >= len(expanded):
            return down_line, up_line, step_text

        advance_accum += speed_scale
        steps = int(advance_accum)
        if steps <= 0:
            step_text.set_text(f"Step {cur_idx} / {len(expanded)}")
            return down_line, up_line, step_text

        for _ in range(steps):
            if cur_idx >= len(expanded):
                break
            x0, y0, x1, y1, pen = expanded[cur_idx]
            if pen:
                _append_seg(down_x, down_y, x0, y0, x1, y1)
                down_line.set_data(down_x, down_y)
            else:
                _append_seg(up_x, up_y, x0, y0, x1, y1)
                up_line.set_data(up_x, up_y)
            cur_idx += 1
            advance_accum -= 1.0

        step_text.set_text(f"Step {cur_idx} / {len(expanded)}")
        return down_line, up_line, step_text

    slider_ax = fig.add_axes([0.15, 0.06, 0.7, 0.03])
    speed_slider = Slider(slider_ax, "Speed mm/s", 5.0, 2000.0, valinit=speed_mm_per_s, valstep=1.0)

    def _set_speed(val: float) -> None:
        nonlocal speed_scale
        speed_scale = max(0.05, float(val) / base_speed)

    speed_slider.on_changed(_set_speed)

    _set_speed(speed_mm_per_s)
    anim = FuncAnimation(fig, update, frames=len(expanded), init_func=init, blit=True, interval=interval_ms, repeat=False)
    plt.show()


# -----------------------------
# Main
# -----------------------------

def main() -> int:
    ap = argparse.ArgumentParser(description="Export cursive text as X,Y pen commands for plotter.")
    ap.add_argument("text", type=str, help="Word/phrase to render.")
    ap.add_argument("--width-mm", type=float, required=True, help="Target drawing width in mm (required).")
    ap.add_argument("--height-mm", type=float, required=True, help="Target drawing height in mm (required).")
    ap.add_argument("--out", type=str, default="plot_program.h", help="Output C header path.")
    ap.add_argument("--font", type=str, default=None, help="Font family name or path to .ttf/.otf for script/cursive.")
    ap.add_argument("--font-size", type=float, default=100.0, help="Font size used for path generation.")
    ap.add_argument("--margin-mm", type=float, default=2.0, help="Margin inside width/height box.")

    # Outline parameters
    ap.add_argument("--curve-seg-mm", type=float, default=1.0, help="Outline curve flattening resolution (smaller => more points).")

    # Output sampling + timings
    ap.add_argument("--poly-step-mm", type=float, default=0.7, help="Max distance between output points (mm).")
    ap.add_argument("--pen-settle-ms", type=int, default=50, help="Dwell after pen down at stroke start.")
    ap.add_argument("--segment-dwell-ms", type=int, default=0, help="Optional dwell per segment point.")

    # Simulation
    ap.add_argument("--simulate", action="store_true", help="Animate the drawing (pen up/down).")
    ap.add_argument("--sim-speed-mmps", type=float, default=80.0, help="Simulation speed in mm/s.")

    # One-line mode
    ap.add_argument("--one-line", action="store_true",
                    help="Generate a centerline path by skeletonizing the filled glyph (instead of drawing the outline).")
    ap.add_argument("--raster-mm", type=float, default=0.12, help="Raster resolution for --one-line in mm per pixel.")
    ap.add_argument("--min-stroke-mm", type=float, default=0.8, help="Drop tiny skeleton strokes shorter than this length (mm).")
    ap.add_argument("--thin-max-iter", type=int, default=300, help="Max iterations for skeleton thinning (safety cap).")
    ap.add_argument("--close-mm", type=float, default=0.0, help="Morphological closing radius in mm (fills small gaps before thinning).")
    ap.add_argument("--skeleton-method", choices=["thin", "ridge"], default="thin",
                    help="Skeletonization method: thin (Zhang-Suen) or ridge (distance-transform medial axis).")
    ap.add_argument("--mask-method", choices=["render", "contains"], default="render",
                    help="Rasterization method for --one-line: render (Agg) or contains (point-in-path).")
    ap.add_argument("--trace-mode", choices=["segments", "walk"], default="segments",
                    help="Tracing: segments (split strokes) or walk (single continuous stroke with retracing).")

    # One-line polish
    ap.add_argument("--smooth-chaikin", type=int, default=2, help="Chaikin smoothing iterations for --one-line strokes. 0 disables.")
    ap.add_argument("--rdp-eps-mm", type=float, default=0.12, help="RDP simplify epsilon in mm (after smoothing). 0 disables.")
    ap.add_argument("--optimize-lr", action="store_true", help="Reorder strokes left->right with minimal pen-up travel. Allows reversing strokes.")
    ap.add_argument("--lr-penalty", type=float, default=15.0, help="Penalty per mm for moving left (x regression) during pen-up optimization.")
    ap.add_argument("--force-lr", action="store_true", help="Force every stroke to run left-to-right.")
    ap.add_argument("--optimize-tlbr", action="store_true",
                    help="Reorder strokes to prefer top-left -> bottom-right flow with minimal pen-up travel. Allows reversing strokes.")
    ap.add_argument("--tlbr-penalty-up", type=float, default=15.0, help="Penalty per mm for moving up during TLBR optimization.")
    ap.add_argument("--tlbr-penalty-left", type=float, default=15.0, help="Penalty per mm for moving left during TLBR optimization.")
    ap.add_argument("--stitch-mm", type=float, default=0.5, help="Stitch adjacent strokes if endpoints are within this distance (mm). 0 disables.")
    ap.add_argument("--bridge-mm", type=float, default=0.0, help="Draw a straight pen-down bridge between strokes within this distance (mm).")

    ap.add_argument("--debug", action="store_true", help="Enable verbose debug output.")
    ap.add_argument("--debug-views", action="store_true", help="Show one-line debug views (mask/close/dist/skeleton/trace).")
    ap.add_argument("--debug-mask", action="store_true", help="Show rasterized mask view for --one-line.")
    ap.add_argument("--debug-closed-mask", action="store_true", help="Show closed mask view for --one-line.")
    ap.add_argument("--debug-distance", action="store_true", help="Show distance transform view (ridge method).")
    ap.add_argument("--debug-skeleton", action="store_true", help="Show skeleton view for --one-line.")
    ap.add_argument("--debug-trace", action="store_true", help="Show traced strokes view for --one-line.")
    args = ap.parse_args()

    log = Log(args.debug)

    if not args.text.strip():
        raise ValueError("Text must be non-empty.")
    if args.width_mm <= 0 or args.height_mm <= 0:
        raise ValueError("--width-mm and --height-mm must be > 0")

    resolver = FontResolver(log)
    font_prop, font_path = resolver.resolve(args.font)
    log.info(f"Font resolved -> {font_path}")

    # Build strokes in mm
    if args.one_line:
        debug_opts = OneLineDebugOptions(
            show_mask=args.debug_views or args.debug_mask,
            show_closed_mask=args.debug_views or args.debug_closed_mask,
            show_distance=args.debug_views or args.debug_distance,
            show_skeleton=args.debug_views or args.debug_skeleton,
            show_trace=args.debug_views or args.debug_trace,
        )
        strokes_mm = text_to_centerline_strokes_mm(
            text=args.text,
            font_prop=font_prop,
            width_mm=args.width_mm,
            height_mm=args.height_mm,
            font_size=args.font_size,
            margin_mm=args.margin_mm,
            raster_mm=args.raster_mm,
            thin_max_iter=args.thin_max_iter,
            min_stroke_mm=args.min_stroke_mm,
            close_mm=args.close_mm,
            skeleton_method=args.skeleton_method,
            mask_method=args.mask_method,
            trace_mode=args.trace_mode,
            log=log,
            debug_opts=debug_opts,
        )

        # Smooth + simplify
        polished: List[Stroke] = []
        for st in strokes_mm:
            pts = st.pts
            if args.smooth_chaikin > 0:
                pts = chaikin_smooth(pts, args.smooth_chaikin)
            if args.rdp_eps_mm > 0:
                pts = rdp_simplify(pts, args.rdp_eps_mm)
            polished.append(Stroke(pts))
        strokes_mm = polished

        # Order + stitch
        if args.optimize_lr:
            strokes_mm = reorder_strokes_lr(list(strokes_mm), args.lr_penalty, log)
        if args.force_lr:
            strokes_mm = force_left_to_right(list(strokes_mm), log)
        if args.optimize_tlbr:
            strokes_mm = reorder_strokes_tlbr(list(strokes_mm), args.tlbr_penalty_up, args.tlbr_penalty_left, log)
        if args.stitch_mm > 0:
            strokes_mm = stitch_strokes(list(strokes_mm), args.stitch_mm, log)
        if args.bridge_mm > 0:
            strokes_mm = bridge_strokes(list(strokes_mm), args.bridge_mm, log)

        log.debug(f"--one-line final: strokes={len(strokes_mm)}")

    else:
        tts = TextToStrokes(log)
        tp = tts.build_path(args.text, font_prop, args.font_size)
        strokes = tts.path_to_strokes(tp, curve_max_seg_len=args.curve_seg_mm)
        if not strokes:
            raise RuntimeError("No strokes generated. Try a different font or text.")
        scaler = StrokeScaler(log)
        strokes_mm = scaler.scale_to_box(strokes, args.width_mm, args.height_mm, args.margin_mm)

    exporter = PlotProgramExporter(log)
    cmds = exporter.to_plot_cmds(
        strokes_mm=strokes_mm,
        poly_step_mm=args.poly_step_mm,
        pen_settle_ms=args.pen_settle_ms,
        segment_dwell_ms=args.segment_dwell_ms,
    )
    if not cmds:
        raise RuntimeError("No plot commands produced.")

    exporter.write_c_header(args.out, cmds)
    log.info(f"Done. Commands: {len(cmds)}")

    if args.simulate:
        simulate(cmds, args.width_mm, args.height_mm, args.sim_speed_mmps, args.debug)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
