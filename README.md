# gskit

A lightweight C++20 tool and library to inspect, validate, and clean 3D Gaussian Splatting (`.ply`) files before importing them into game engines (Unreal Engine, Unity) or custom rasterizers. Zero external dependencies.

---

## Why?

Most 3DGS training pipelines (**`gsplat`**, **Nerfstudio**, **Inria**, **PostShot**) export standard binary PLY files. While standard viewers handle edge cases gracefully, importing these raw files into custom compute shaders, game engines (Unreal Engine / Unity), or custom rasterizers can run into practical issues:

- Gradient explosions or half-precision artifacts resulting in `NaN` / `Inf` positions or scales (causing black pixel/tile artifacts during rasterization).
- Corrupted or unnormalized quaternions ($[0,0,0,0]$) breaking rotation matrix reconstructions.
- Degenerate splats (needle-like aspect ratios $> 1000\times$) causing rasterizer overdraw.
- Residual low-opacity splats ($\alpha < 10^{-4}$) taking up memory and vertex buffer bandwidth.

`gskit` acts as a quick pre-ingest filter: it scans the raw binary stream, reports anomalies via CLI / JSON, and prunes broken splats to output a clean PLY file.

---

## What It Checks

| Anomaly | Level | Description |
| :--- | :--- | :--- |
| `MISSING_PROPERTY` | Error | Missing required fields (`x,y,z`, `rot_0..3`, `scale_0..2`, `opacity`, `f_dc_0..2`) |
| `UNEXPECTED_SH_DEGREE` | Error | SH coefficient count doesn't match standard degrees (0, 1, 2, or 3) |
| `NON_FINITE_*` | Error/Warn | `NaN` or `Inf` found in position, scale, rotation, opacity, or SH |
| `ZERO_QUATERNION` | Error | Quaternions with near-zero length |
| `NON_NORMALIZED_QUATERNION` | Warn/Error | Quaternion magnitude deviates significantly from 1.0 |
| `INVALID_SCALE` | Warn | Activated scale ($\exp(s)$) is ridiculously huge ($> 10^6$) or tiny ($< 10^{-8}$) |
| `ANISOTROPIC_SCALE` | Warn | Needle splats where $\max(s) / \min(s) > 1000$ |
| `DEAD_GAUSSIAN` | Warn | Practically invisible splats ($\text{sigmoid}(\text{opacity}) < 10^{-4}$) |
| `INVALID_COLOR` | Warn | Projected DC base color ($f_{dc} \cdot 0.28209 + 0.5$) out of normal RGB range |
| `AABB_BOUNDS` | Info | Computes scene min/max bounding box on valid positions |

---

## Building

Requires a C++20 capable compiler (MSVC 2022, GCC 11+, or Clang 14+) and CMake 3.20+.

```bash
git clone https://github.com/Kutsukey/gskit.git
cd gskit
cmake -B build
cmake --build build --config Release

```

---

## Usage

### 1. Validate a PLY File

Scans the file and prints issues to stdout with exit codes (`0` = clean, `1` = warnings, `2` = errors):

```bash
# Basic validation
./gskit validate scene.ply

# Strict mode (fails on warnings)
./gskit validate scene.ply --strict

```

### 2. Sanitize and Prune

Filters out invalid splats and writes a new, valid PLY:

```bash
# Strip corrupt/NaN splats
./gskit sanitize corrupt.ply -o clean.ply

# Also remove nearly invisible ghost splats
./gskit sanitize corrupt.ply -o clean.ply --drop-ghosts

```

### 3. Pipeline JSON Inspection (`info`)

Outputs structured metadata and bounds for integration with Python scripts, DCC tools, or CI pipelines:

```bash
./gskit info scene.ply

```

```json
{
  "file": "scene.ply",
  "gaussianCount": 1845210,
  "shDegree": 3,
  "errorCount": 0,
  "warningCount": 2,
  "ghostCount": 8,
  "needleCount": 1,
  "bounds": {
    "min": [-4.120, -2.450, -0.890],
    "max": [5.310, 3.840, 2.150]
  },
  "issues": [
    {
      "issue": "ANISOTROPIC_SCALE",
      "severity": "WARNING",
      "message": "Activated scale value has extreme aspect ratio: 1420.5",
      "gaussianIndex": 4210
    }
  ]
}

```

---

## Planned Features

* [ ] Spatial box pruning (`--bbox-min`, `--bbox-max`) to crop scene bounds
* [ ] Memory-mapped file I/O and OpenMP multithreading for 10M+ splat files
* [ ] Unreal Engine 5 Editor Commandlet integration
* [ ] Vector quantization (LVQ) for splat compression

---

## License

Apache-2.0. See [LICENSE](LICENSE).