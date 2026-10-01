#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
OUT="$ROOT/dist"

rm -rf "$OUT"
mkdir -p "$OUT/PSP_GAME/SYSDIR"

if [ ! -f "$ROOT/EBOOT.PBP" ]; then
    echo "Missing EBOOT.PBP"
    exit 1
fi

UNPACK=${UNPACK_PBP:-unpack-pbp}
UMDGEN=${UMDGEN:-umdgen}

command -v "$UNPACK" >/dev/null 2>&1 || {
    echo "Missing unpack-pbp"
    exit 1
}

command -v "$UMDGEN" >/dev/null 2>&1 || {
    echo "Missing UMDGenCLI"
    exit 1
}

TMP="$OUT/pbp"
mkdir -p "$TMP"

"$UNPACK" "$ROOT/EBOOT.PBP" -o "$TMP" >/dev/null

[ -f "$TMP/DATA.PSP" ] || {
    echo "unpack-pbp did not produce DATA.PSP."
    exit 1
}

[ -f "$TMP/PARAM.SFO" ] || {
    echo "unpack-pbp did not produce PARAM.SFO."
    exit 1
}

cp "$TMP/PARAM.SFO" "$OUT/PSP_GAME/PARAM.SFO"
cp "$TMP/DATA.PSP" "$OUT/PSP_GAME/SYSDIR/EBOOT.BIN"

if [ -f "$TMP/ICON0.PNG" ]; then
    cp "$TMP/ICON0.PNG" "$OUT/PSP_GAME/ICON0.PNG"
fi

rm -rf "$TMP"

"$UMDGEN" create "$OUT" "$ROOT/Necro_Morselli.iso" --label NMC00001 --padding umd
"$UMDGEN" verify "$ROOT/Necro_Morselli.iso"

echo "Created $ROOT/Necro_Morselli.iso"
ls -lh "$ROOT/Necro_Morselli.iso"
