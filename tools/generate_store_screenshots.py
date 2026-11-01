#!/usr/bin/env python3
"""Render Rebble App Store screenshots at exact platform resolutions.

Mirrors Wrist Cast watch UI (no watch bezel — store requirement).
"""
from __future__ import annotations

import os
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "store" / "screenshots"
RES = ROOT / "resources" / "images"

PLATFORMS = {
    "basalt": (144, 168, True),   # Pebble Time — color
    "diorite": (144, 168, False),  # Pebble 2 — B&W
    "flint": (144, 168, False),    # Pebble 2 Duo — B&W
    "emery": (200, 228, True),    # Pebble Time 2 — color
}

BLACK = (0, 0, 0)
WHITE = (255, 255, 255)
LIGHT_GRAY = (170, 170, 170)
DARK_GRAY = (85, 85, 85)
DUKE_BLUE = (0, 85, 170)
CYAN = (0, 255, 255)
BRIGHT_GREEN = (0, 255, 0)
RED = (255, 0, 0)
YELLOW = (255, 170, 0)
CHROME_YELLOW = (255, 170, 0)


def font(size: int) -> ImageFont.ImageFont:
    candidates = [
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
    ]
    for path in candidates:
        if os.path.isfile(path):
            return ImageFont.truetype(path, size)
    return ImageFont.load_default()


def font_bold(size: int) -> ImageFont.ImageFont:
    candidates = [
        "C:/Windows/Fonts/segoeuib.ttf",
        "C:/Windows/Fonts/arialbd.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
    ]
    for path in candidates:
        if os.path.isfile(path):
            return ImageFont.truetype(path, size)
    return font(size)


def text_center(draw, text, y, w, fnt, fill):
    bbox = draw.textbbox((0, 0), text, font=fnt)
    tw = bbox[2] - bbox[0]
    draw.text(((w - tw) // 2, y), text, font=fnt, fill=fill)


def text_right(draw, text, y, w, fnt, fill, pad=4):
    bbox = draw.textbbox((0, 0), text, font=fnt)
    tw = bbox[2] - bbox[0]
    draw.text((w - pad - tw, y), text, font=fnt, fill=fill)


def bw_sprite(spr: Image.Image) -> Image.Image:
    spr = spr.copy()
    px = spr.load()
    for y in range(spr.height):
        for x in range(spr.width):
            r, g, b, a = px[x, y]
            if a < 32:
                continue
            lum = (r + g + b) // 3
            if lum < 20:
                px[x, y] = (0, 0, 0, 0)
            else:
                px[x, y] = (255, 255, 255, a)
    return spr


def load_sprite(path: Path, color: bool) -> Image.Image | None:
    if not path.is_file():
        return None
    spr = Image.open(path).convert("RGBA")
    if not color:
        spr = bw_sprite(spr)
    return spr


def paste_scaled(base: Image.Image, spr: Image.Image, dest: tuple[int, int, int, int]):
    x, y, w, h = dest
    if w < 1 or h < 1:
        return
    resized = spr.resize((w, h), Image.NEAREST)
    base.paste(resized, (x, y), resized)


def paste_centered_scaled(base: Image.Image, spr: Image.Image, cx: int, cy: int,
                          max_w: int, max_h: int):
    sx = max(1, max_w // spr.width)
    sy = max(1, max_h // spr.height)
    scale = min(sx, sy, 3)
    tw = spr.width * scale
    th = spr.height * scale
    paste_scaled(base, spr, (cx - tw // 2, cy - th // 2, tw, th))


def render_title(w: int, h: int, color: bool) -> Image.Image:
    img = Image.new("RGB", (w, h), BLACK)
    draw = ImageDraw.Draw(img)
    f18 = font_bold(max(14, w // 10))
    f14 = font(max(11, w // 13))
    dim = LIGHT_GRAY if color else WHITE

    title = load_sprite(RES / "title.png", color)
    if title is not None:
        paste_scaled(img, title, (0, 0, w, h))
    else:
        text_center(draw, "WRIST CAST", 28, w, font_bold(max(18, w // 8)), WHITE)

    bar_h = 46
    draw.rectangle((0, h - bar_h, w, h), fill=BLACK)
    text_center(draw, "Flick to cast · feel the bite", h - bar_h + 2, w, f14, dim)
    text_center(draw, "SELECT to start", h - bar_h + 20, w, f18, WHITE)
    return img


def render_ready(w: int, h: int, color: bool) -> Image.Image:
    img = Image.new("RGB", (w, h), BLACK)
    draw = ImageDraw.Draw(img)
    f18 = font_bold(max(14, w // 10))
    f14 = font(max(11, w // 13))
    dim = LIGHT_GRAY if color else WHITE
    water = DUKE_BLUE if color else BLACK

    water_y = h * 2 // 5
    draw.rectangle((0, 0, w, water_y), fill=BLACK)

    water_spr = load_sprite(RES / "water.png", color)
    if water_spr is not None:
        stretch_h = max(28, water_spr.height)
        paste_scaled(img, water_spr, (0, water_y - 2, w, stretch_h))
        draw.rectangle((0, water_y - 2 + stretch_h, w, h), fill=water)
    else:
        draw.rectangle((0, water_y, w, h), fill=water)

    bobber = load_sprite(RES / "bobber.png", color)
    bx, by = w // 2 - 10, water_y + 20
    rod = load_sprite(RES / "rod.png", color)
    tip = (w // 2 + 14, water_y - 4)
    rod_xy = None
    if rod is not None:
        rod_xy = ((w - rod.width) // 2 + 10, water_y - 8)
        tip = (rod_xy[0] + rod.width // 2, rod_xy[1] + 3)

    if rod is not None and rod_xy is not None:
        img.paste(rod, rod_xy, rod)
    draw.line((tip[0], tip[1], bx, by), fill=dim, width=1)
    if bobber is not None:
        paste_centered_scaled(img, bobber, bx, by + 8, bobber.width, bobber.height)
    else:
        draw.ellipse((bx - 6, by - 6, bx + 6, by + 6), fill=RED if color else WHITE)

    text_right(draw, "12 caught", 2, w, f14, dim)
    text_center(draw, "Flick to cast", h - 40, w, f18, WHITE)
    text_center(draw, "or SELECT to cast", h - 22, w, f14, dim)
    return img


def render_bite(w: int, h: int, color: bool) -> Image.Image:
    img = render_ready(w, h, color)
    draw = ImageDraw.Draw(img)
    f28 = font_bold(max(22, w // 6))
    f18 = font_bold(max(14, w // 10))
    bite = RED if color else WHITE

    # Cover ready captions
    draw.rectangle((0, h - 44, w, h), fill=BLACK)
    text_center(draw, "BITE!", 18, w, f28, bite)
    text_center(draw, "BITE! SELECT", h - 40, w, f18, WHITE)
    return img


def render_fight(w: int, h: int, color: bool) -> Image.Image:
    img = Image.new("RGB", (w, h), BLACK)
    draw = ImageDraw.Draw(img)
    f18 = font_bold(max(14, w // 10))
    f14 = font(max(11, w // 13))
    dim = LIGHT_GRAY if color else WHITE
    green = BRIGHT_GREEN if color else WHITE
    yellow = CHROME_YELLOW if color else WHITE
    accent = CYAN if color else WHITE
    bad = RED if color else WHITE

    text_center(draw, "FIGHT!", 4, w, f18, WHITE)

    bar_x, bar_w = 8, w - 16
    bar_y, bar_h = h // 2 - 16, 28
    lo, hi, tension, reel = 32, 68, 48, 55

    draw.rectangle((bar_x, bar_y, bar_x + bar_w, bar_y + bar_h), fill=dim if color else DARK_GRAY)
    gx = bar_x + bar_w * lo // 100
    gw = bar_w * (hi - lo) // 100
    draw.rectangle((gx, bar_y, gx + gw, bar_y + bar_h), fill=green)
    needle = bar_x + bar_w * tension // 100
    draw.rectangle((needle - 2, bar_y - 4, needle + 3, bar_y + bar_h + 4), fill=accent)

    py = bar_y + bar_h + 10
    draw.rectangle((bar_x, py, bar_x + bar_w, py + 8), fill=dim if color else DARK_GRAY)
    draw.rectangle((bar_x, py, bar_x + bar_w * reel // 100, py + 8), fill=yellow)

    draw.text((bar_x, bar_y - 18), "Tension", font=f14, fill=WHITE)
    draw.text((bar_x, py + 10), "Reel", font=f14, fill=WHITE)
    text_center(draw, "Hold UP reel · DOWN ease", h - 28, w, f14, dim)
    # tiny warn cue unused — keep tension in green story
    _ = bad
    return img


def render_caught(w: int, h: int, color: bool) -> Image.Image:
    img = Image.new("RGB", (w, h), BLACK)
    draw = ImageDraw.Draw(img)
    f18 = font_bold(max(14, w // 10))
    f14 = font(max(11, w // 13))
    dim = LIGHT_GRAY if color else WHITE

    text_center(draw, "Total 12  Streak 3", 6, w, f14, dim)

    fish = load_sprite(RES / "fish_rare.png", color)
    if fish is not None:
        paste_centered_scaled(img, fish, w // 2, h // 2 - 24, w - 24, h // 2)

    text_center(draw, "Bass", h // 2 + 28, w, f18, WHITE)
    text_center(draw, "Rare", h // 2 + 50, w, f14, dim)
    text_center(draw, "SELECT: again", h - 24, w, f14, WHITE)
    return img


SCENES = [
    ("01-title.png", render_title),
    ("02-ready.png", render_ready),
    ("03-bite.png", render_bite),
    ("04-fight.png", render_fight),
    ("05-caught.png", render_caught),
]


def main() -> None:
    for plat, (w, h, color) in PLATFORMS.items():
        dest = OUT / plat
        dest.mkdir(parents=True, exist_ok=True)
        for filename, fn in SCENES:
            img = fn(w, h, color)
            path = dest / filename
            img.save(path, "PNG", optimize=True)
            print(f"wrote {path.relative_to(ROOT)} ({w}x{h})")
    print("Done:", OUT)


if __name__ == "__main__":
    main()
