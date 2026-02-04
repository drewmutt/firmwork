# RemoteDrawGenerator

Generate plotter-ready stroke commands from text using font outlines or a
centerline skeleton, and optionally simulate the pen path.

## Methodology

This tool provides two modes:

1) Outline mode (default)
   - Uses matplotlib's `TextPath` to extract font outlines.
   - Curves are flattened into polylines at a configurable resolution.
   - The outline polylines are scaled into the target mm box.
   - Output is a sequence of pen-up travel moves and pen-down strokes.

2) One-line mode (`--one-line`)
   - Rasterizes the filled glyphs into a binary mask at `--raster-mm`.
   - Optional morphological closing (`--close-mm`) fills small gaps.
   - Skeletonizes the mask to a centerline:
     - `thin`: Zhang-Suen thinning on the binary mask.
     - `ridge`: distance-transform ridges (medial axis), optionally thinned.
   - Traces the skeleton pixels into polylines.
   - Optional smoothing (Chaikin), simplification (RDP), ordering, stitching,
     and bridging.
   - Exports the result as pen commands.

The one-line mode is best for a "single-stroke cursive" look, but quality
depends heavily on the font. Some script fonts do not form closed counters
for small loops; this can prevent loops from appearing in the skeleton.

## Usage

Basic outline mode:

```
python gen.py "Hello" --width-mm 80 --height-mm 40 --out plot_program.h
```

One-line mode:

```
python gen.py "Hey" --width-mm 50 --height-mm 50 --one-line \
  --font "Pacifico" \
  --raster-mm 0.04 --smooth-chaikin 4 --rdp-eps-mm 0.06 \
  --optimize-lr --stitch-mm 2.0 --bridge-mm 1.0 \
  --poly-step-mm 0.3 --simulate
```

Simulation:

```
python gen.py "Hey" --width-mm 50 --height-mm 50 --one-line --simulate
```

## Arguments

### Core

- `text`: required positional string to render.
- `--width-mm`: target drawing width in mm (required).
- `--height-mm`: target drawing height in mm (required).
- `--out`: output C header path (default `plot_program.h`).
- `--font`: font family name or path to a `.ttf`/`.otf` file.
- `--font-size`: font size used for path generation (default `100.0`).
- `--margin-mm`: inner margin for the drawing box (default `2.0`).

### Outline mode (default)

- `--curve-seg-mm`: maximum length of flattened curve segments (smaller = more points).

### One-line mode

- `--one-line`: enable centerline skeletonization mode.
- `--raster-mm`: raster resolution in mm per pixel.
- `--min-stroke-mm`: drop traced skeleton strokes shorter than this length.
- `--thin-max-iter`: max iterations for thinning (safety cap).
- `--close-mm`: morphological closing radius in mm (fills small gaps before thinning).
- `--skeleton-method`: `thin` (Zhang-Suen) or `ridge` (distance-transform ridges).
- `--mask-method`: `render` (Agg rasterization) or `contains` (point-in-path).
- `--trace-mode`: `segments` (split strokes) or `walk` (single continuous stroke with retracing).

### One-line polish

- `--smooth-chaikin`: Chaikin smoothing iterations (0 disables).
- `--rdp-eps-mm`: Ramer-Douglas-Peucker simplify epsilon in mm (0 disables).
- `--optimize-lr`: reorder strokes left-to-right to reduce pen-up travel.
- `--lr-penalty`: penalty per mm for leftward movement during ordering.
- `--force-lr`: force every stroke to run left-to-right.
- `--optimize-tlbr`: reorder strokes to prefer top-left to bottom-right flow.
- `--tlbr-penalty-up`: penalty per mm for moving up during TLBR ordering.
- `--tlbr-penalty-left`: penalty per mm for moving left during TLBR ordering.
- `--stitch-mm`: merge strokes when endpoints are within this distance.
- `--bridge-mm`: draw a straight pen-down bridge between strokes within this distance.

### Output sampling + timings

- `--poly-step-mm`: maximum distance between output points in mm.
- `--pen-settle-ms`: dwell after pen down at stroke start.
- `--segment-dwell-ms`: optional dwell at each segment point.

### Simulation

- `--simulate`: animate the drawing (pen down/up).
- `--sim-speed-mmps`: initial simulation speed in mm/s.

### Debug

- `--debug`: verbose debug output to stdout.
- `--debug-views`: show all one-line debug views.
- `--debug-mask`: show raster mask view.
- `--debug-closed-mask`: show post-closing mask view.
- `--debug-distance`: show distance-transform view (ridge method).
- `--debug-skeleton`: show skeleton view.
- `--debug-trace`: show traced strokes view.
