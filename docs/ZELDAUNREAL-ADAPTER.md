# ZeldaUnreal debug adapter (experimental, opt-in)

This branch adds two developer TCP commands to the cycle-based host:
`zelda_unreal_frame` and `zelda_unreal_room`. The handlers are provided by
`src/zelda_unreal_tcp.inc` and registered in `src/cyc_extras.c`.

The feature exports existing original game state **read-only** for an external
Unreal spectator; it does not modify NES input, physics, combat, or saves.
Use the corresponding `danielsfroste-a11y/ZeldaUnreal` v0.5 Python adapter,
with `--require-terrain --prefetch-neighbors`.

Validation here:
- ROM-free C mock compiles the **exact included handler source** and exercises
  valid/invalid frames, hex tile + attribute serialization, neighbors, and
  avoiding overwriting live simulated room tiles.
- Python registration guard checks inclusion/registration exactly once.
- These tests are source/contract tests, **not proof that the full game builds
  or runs**, because the cycle backend requires a user-supplied NES ROM.
- On Windows, build locally using the upstream documented setup/CMake process,
  and point the project's `nesrecomp` submodule at a verified engine revision.
- Keep ROM files, saves and extracted assets OUT of repositories and Actions.
- The normal original TCP debug server is only available when explicitly enabled.

Source snapshot: original fork master at
`59959a7ddfb3e86e424a75224af0cf828fb0ea04`.
Upstream `mstan/LegendOfZeldaNESRecomp` license and external assets remain
governed by their respective terms. Do not merge until real Windows runtime
regression evidence is available.
