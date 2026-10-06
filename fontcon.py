#!/usr/bin/env python3
"""
fontcon.py - Bitmap font -> .h converter for UpdateOS

Reads a grid image containing glyphs and generates a C header with a
gfx_font_t struct + its charmap array, compatible with updgr.c's
gfx_print().

Convention (fixed, no threshold):
  - light pixel (luminance >= 128)   -> ink  (1)
  - dark pixel  (luminance <  128)   -> bg   (0)
  - transparent (alpha < 128)        -> bg   (0)

Format expected by gfx_print():
  - MSB first (bit 7 = leftmost pixel of the byte)
  - 1 = pixel on
  - row_bytes = (width + 7) // 8
  - rows stored top to bottom, chars stored left-to-right, top-to-bottom

Requirements: Pillow (pip install Pillow)
"""

import os
import re
import sys

try:
    from PIL import Image
except ImportError:
    print("Error: Pillow is required. Install with:  pip install Pillow")
    sys.exit(1)


# ====================================================================
#  Helpers
# ====================================================================

def ask(prompt, default=None, cast=str):
    if default is not None:
        raw = input(f"{prompt} [{default}]: ").strip()
        if not raw:
            return default
    else:
        raw = input(f"{prompt}: ").strip()
        while raw == "":
            print("  (required)")
            raw = input(f"{prompt}: ").strip()
    try:
        return cast(raw)
    except (ValueError, TypeError):
        print(f"  Invalid value, using default: {default}")
        return default


def sanitize_ident(s):
    s = re.sub(r"[^0-9a-zA-Z_]", "_", s)
    if s and s[0].isdigit():
        s = "_" + s
    return s or "font"


def c_escape(s):
    return s.replace("\\", "\\\\").replace('"', '\\"')


# ====================================================================
#  Image processing
# ====================================================================

def load_binarized(path):
    """
    Return (bits, w, h). bits[y][x] = 0 or 1 (1 = ink).

    Rules:
      - Transparent (alpha < 128)       -> 0
      - Light  (luminance >= 128)       -> 1
      - Dark   (luminance <  128)       -> 0
    """
    img = Image.open(path).convert("RGBA")
    w, h = img.size
    px = img.load()
    bits = [[0] * w for _ in range(h)]

    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]

            if a < 128:
                bits[y][x] = 0
                continue

            lum = (r * 299 + g * 587 + b * 114) // 1000
            bits[y][x] = 1 if lum >= 128 else 0

    return bits, w, h


def pack_glyph(rows, w, h):
    row_bytes = (w + 7) // 8
    out = []
    for y in range(h):
        row = rows[y]
        for bx in range(row_bytes):
            b = 0
            for bit in range(8):
                x = bx * 8 + bit
                if x < w and row[x]:
                    b |= (1 << (7 - bit))
            out.append(b)
    return out


def preview_glyph(rows, w, h):
    return ["".join("#" if rows[y][x] else "." for x in range(w))
            for y in range(h)]


# ====================================================================
#  Header writer
# ====================================================================

def write_header(out_path, meta, charmap):
    name   = meta["name"]
    ident  = meta["ident"]
    author = meta["author"]
    w      = meta["width"]
    h      = meta["height"]
    first  = meta["first_char"]
    last   = meta["last_char"]
    flags  = meta["flags"]

    guard = f"FONT_{ident.upper()}_H"

    lines = []
    lines.append(f"#ifndef {guard}")
    lines.append(f"#define {guard}")
    lines.append("")
    lines.append("#include <stdint.h>")
    lines.append('#include "../updgr.h"')
    lines.append("")
    lines.append(f"/* Font:   {c_escape(name)}")
    lines.append(f" * Author: {c_escape(author)}")
    lines.append(f" * Cell:   {w}x{h}")
    lines.append(f" * Chars:  {first}..{last} ({last - first + 1} glyphs)")
    lines.append(f" * Flags:  0x{flags:02X}")
    lines.append(" */")
    lines.append("")
    lines.append(f"static const uint8_t _charmap_{ident}[] = {{")

    per_line = 12
    idx = 0
    for code in range(first, last + 1):
        if idx >= len(charmap):
            break
        glyph = charmap[idx]
        idx += 1

        if code == 0x20:
            label = "space"
        elif 32 <= code < 127:
            label = chr(code)
        else:
            label = "?"

        lines.append(f"    /* {code:3d}  0x{code:02X}  '{label}' */")
        for i in range(0, len(glyph), per_line):
            chunk = glyph[i:i + per_line]
            hexs = ", ".join(f"0x{b:02X}" for b in chunk)
            lines.append(f"    {hexs},")

    lines.append("};")
    lines.append("")
    lines.append(f"static gfx_font_t {ident} = {{")
    lines.append(f'    .name       = "{c_escape(name)}",')
    lines.append(f'    .author     = "{c_escape(author)}",')
    lines.append(f"    .width      = {w},")
    lines.append(f"    .height     = {h},")
    lines.append(f"    .code       = (uint8_t *)_charmap_{ident},")
    lines.append(f"    .charcount  = {last - first + 1},")
    lines.append(f"    .first_char = {first},")
    lines.append(f"    .last_char  = {last},")
    lines.append(f"    .flags      = 0x{flags:02X},")
    lines.append("};")
    lines.append("")
    lines.append(f"#endif /* {guard} */")
    lines.append("")

    with open(out_path, "w") as f:
        f.write("\n".join(lines))


# ====================================================================
#  Main
# ====================================================================

def main():
    print("=" * 62)
    print("  fontcon.py  -  bitmap font -> C header")
    print("=" * 62)
    print()
    print("  Convention:")
    print("    * light pixel  (luminance >= 128) -> ink  (1)")
    print("    * dark pixel   (luminance <  128) -> bg   (0)")
    print("    * transparent  (alpha < 128)      -> bg   (0)")
    print()

    # ---------- metadata ----------
    print("-- Metadata " + "-" * 49)
    name   = ask("  Font name (identifier-friendly, e.g. default_8x16)")
    ident  = sanitize_ident(name)
    author = ask("  Author", default="Unknown")

    # ---------- image ----------
    print()
    print("-- Image " + "-" * 52)
    while True:
        img_path = ask("  Path to image (PNG/JPG/BMP)")
        if os.path.isfile(img_path):
            break
        print(f"  File not found: {img_path}")

    bits, img_w, img_h = load_binarized(img_path)
    print(f"  Loaded image: {img_w} x {img_h}")

    # ---------- cell size ----------
    print()
    print("-- Cell size " + "-" * 48)
    cell_w = ask("  Cell width  (px)", default=8,  cast=int)
    cell_h = ask("  Cell height (px)", default=16, cast=int)

    if cell_w <= 0 or cell_h <= 0:
        print("Error: cell dimensions must be > 0")
        sys.exit(1)

    cols = img_w // cell_w
    rows = img_h // cell_h

    if cols == 0 or rows == 0:
        print(f"Error: image {img_w}x{img_h} too small for cell {cell_w}x{cell_h}")
        sys.exit(1)

    total = cols * rows
    print(f"  Grid detected: {cols} cols x {rows} rows = {total} glyphs")

    if img_w % cell_w or img_h % cell_h:
        print("  (note: leftover pixels ignored)")

    # ---------- char mapping ----------
    print()
    print("-- Character mapping " + "-" * 41)
    first_char = ask("  First character code (e.g. 32 = space)",
                     default=32, cast=int)
    last_char  = first_char + total - 1
    shown = chr(last_char) if 32 <= last_char < 127 else "?"
    print(f"  Last character code (auto): {last_char} ('{shown}')")

    if last_char > 255:
        print("  Warning: last_char > 255, clamping")
        last_char = 255
        total = last_char - first_char + 1

    # ---------- flags ----------
    print()
    print("-- Flags " + "-" * 52)
    print("    0x00 = normal")
    print("    0x01 = flip horizontal")
    print("    0x02 = flip vertical")
    print("    0x03 = flip both")
    flags = ask("  Flags (hex, e.g. 0x00)", default="0x00",
                cast=lambda s: int(s, 0))

    # ---------- output ----------
    print()
    print("-- Output " + "-" * 51)
    out_dir = ask("  Output directory", default="kernel/fonts")
    os.makedirs(out_dir, exist_ok=True)
    out_path = os.path.join(out_dir, f"{ident}.h")

    # ---------- build charmap ----------
    charmap = []
    glyphs_preview = []

    for r in range(rows):
        for c in range(cols):
            cx = c * cell_w
            cy = r * cell_h
            rows_bits = [
                [bits[cy + ry][cx + rx] for rx in range(cell_w)]
                for ry in range(cell_h)
            ]
            charmap.append(pack_glyph(rows_bits, cell_w, cell_h))
            if len(glyphs_preview) < 4:
                glyphs_preview.append(rows_bits)

    # ---------- write header ----------
    meta = {
        "name":       name,
        "ident":      ident,
        "author":     author,
        "width":      cell_w,
        "height":     cell_h,
        "first_char": first_char,
        "last_char":  last_char,
        "flags":      flags,
    }
    write_header(out_path, meta, charmap)

    # ---------- preview ----------
    print()
    print("-- Preview of first glyphs " + "-" * 35)
    for i, g in enumerate(glyphs_preview):
        code = first_char + i
        label = "space" if code == 0x20 else (chr(code) if 32 <= code < 127 else "?")
        print(f"  '{label}' (code {code}):")
        for ln in preview_glyph(g, cell_w, cell_h):
            print(f"    {ln}")
        print()

    # ---------- done ----------
    print("=" * 62)
    print(f"  OK: {out_path}")
    print(f"  Glyphs: {len(charmap)}   Cell: {cell_w}x{cell_h}")
    print(f"  Struct: {ident}")
    print("=" * 62)
    print()
    print("Usage:")
    print(f'  #include "fonts/{ident}.h"')
    print(f"  gfx_setfont({ident});")
    print(f'  gfx_print(10, 10, "Hello, kernel!", 0x00FFFFFF);')


if __name__ == "__main__":
    main()