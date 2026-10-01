#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$ROOT"

if ! command -v psp-gcc >/dev/null 2>&1; then
    echo "Error: psp-gcc not found." >&2
    exit 1
fi

export PSPSDK="${PSPSDK:-/usr/local/pspdev/psp/sdk}"
export PATH="/usr/local/pspdev/bin:$PATH"

echo "PSPSDK=$PSPSDK"
echo "Building Necro Morselli..."

make clean || true
make

echo "Build finished."

if [ ! -f EBOOT.PBP ]; then
    echo "Error: EBOOT.PBP was not created." >&2
    exit 1
fi

echo "EBOOT.PBP created successfully:"
ls -lh EBOOT.PBP
