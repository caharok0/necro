# Necro Morselli — PSP-3000

2D pixel-art RPG/homebrew project targeting 480×272 on PSP. This revision expands the earlier prototype with a title screen, five locations, quest progression, several NPCs, multiple enemy encounters, a stronger sanctum guardian, turn-based combat, leveling, potions, gold, save/load, menu and quest log.

## Current build state

This package is **source-complete for the current feature set**, but `EBOOT.PBP`/ISO are not included because this environment does not have the PSPDEV cross-compiler installed. The official PSPDEV project provides the PSP compiler/SDK and publishes a Docker image `pspdev/pspdev:latest`; use `tools/build_psp.sh` on a machine with Docker or an installed PSPDEV SDK.

## Build

With PSPDEV installed:

```sh
make clean && make
```

With Docker:

```sh
./tools/build_psp.sh
```

The build produces `EBOOT.PBP`.

## ISO

After `EBOOT.PBP` exists, create a folder tree containing:

```text
ISO_ROOT/
└── PSP_GAME/
    ├── PARAM.SFO
    └── SYSDIR/
        └── EBOOT.PBP
```

Then use a PSP UMD image builder such as UMDGenCLI to create the ISO. See `tools/build_iso.sh` for the exact expected layout/command.

## Controls

- D-pad: move / menu selection
- X: confirm / interact
- O: back / close
- Triangle: menu
- Start: quick save

## Notes

The artwork is intentionally lightweight and procedural so the project stays small and fast on PSP hardware. `assets/necro_reference.png` is retained as the visual reference used for the main character.
