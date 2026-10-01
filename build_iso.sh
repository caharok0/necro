#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
OUT="$ROOT/dist"
rm -rf "$OUT"
mkdir -p "$OUT/PSP_GAME/SYSDIR"
[ -f "$ROOT/EBOOT.PBP" ] || { echo "Missing EBOOT.PBP. Run tools/build_psp.sh first." >&2; exit 1; }
UNPACK=${UNPACK_PBP:-unpack-pbp}
UMDGEN=${UMDGEN:-umdgen}
command -v "$UNPACK" >/dev/null 2>&1 || { echo "Missing unpack-pbp (PSPDEV tool)." >&2; exit 1; }
command -v "$UMDGEN" >/dev/null 2>&1 || { echo "Missing UMDGenCLI. Set UMDGEN=/path/to/umdgen." >&2; exit 1; }
TMP="$OUT/pbp"
mkdir -p "$TMP"
"$UNPACK" "$ROOT/EBOOT.PBP" -o "$TMP" >/dev/null
[ -f "$TMP/DATA.PSP" ] || { echo "unpack-pbp did not produce DATA.PSP." >&2; exit 1; }
[ -f "$TMP/PARAM.SFO" ] || { echo "unpack-pbp did not produce PARAM.SFO." >&2; exit 1; }
cp "$TMP/PARAM.SFO" "$OUT/PSP_GAME/PARAM.SFO"
cp "$TMP/DATA.PSP" "$OUT/PSP_GAME/SYSDIR/EBOOT.BIN"
[ -f "$TMP/ICON0.PNG" ] && cp "$TMP/ICON0.PNG" "$OUT/PSP_GAME/ICON0.PNG" || true
rm -rf "$TMP"
"$UMDGEN" create "$OUT" "$ROOT/Necro_Morselli.iso" --label NMC00001 --padding umd
"$UMDGEN" verify "$ROOT/Necro_Morselli.iso"
echo "Created $ROOT/Necro_Morselli.iso"
