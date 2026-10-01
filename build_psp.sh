#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$ROOT"

export PSPSDK="${PSPSDK:-/usr/local/pspdev/psp/sdk}"
export PATH="/usr/local/pspdev/bin:$PATH"

echo "PSPSDK=$PSPSDK"
echo "psp-gcc:"
psp-gcc --version

echo "Cleaning..."
make clean || true

echo "Building..."
make

echo "Checking EBOOT.PBP..."

if [ ! -f EBOOT.PBP ]; then
    echo "ERROR: EBOOT.PBP was not created."
    exit 1
fi

echo "SUCCESS: EBOOT.PBP created."
ls -lh EBOOT.PBP
