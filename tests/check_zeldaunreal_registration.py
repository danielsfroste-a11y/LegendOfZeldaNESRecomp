#!/usr/bin/env python3
"""Source registration and fork integration guard. ROM-free."""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
source = (root / "src" / "cyc_extras.c").read_text()
snippet = (root / "src" / "zelda_unreal_tcp.inc").read_text()
assert source.count('#include "zelda_unreal_tcp.inc"') == 1
assert source.count('cyc_tcp_register("zelda_unreal_frame"') == 1
assert source.count('cyc_tcp_register("zelda_unreal_room"') == 1
assert snippet.count("/* ZELDAUNREAL_TCP_EXTENSION_BEGIN */") == 1
assert snippet.count("/* ZELDAUNREAL_TCP_EXTENSION_END */") == 1
assert source.count("static void frame_end(") == 1
assert "static void zelda_unreal_frame(" in snippet
assert "static void zelda_unreal_room(" in snippet
print("ZeldaUnreal source integration: PASS")
