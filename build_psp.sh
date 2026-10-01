#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
if command -v psp-gcc >/dev/null 2>&1; then
  make clean
  make
  exit 0
fi
if ! command -v docker >/dev/null 2>&1; then
  echo "PSPDEV/psp-gcc and Docker are both unavailable." >&2
  exit 1
fi
docker run --rm -v "$ROOT:/src" -w /src pspdev/pspdev:latest sh -lc 'make clean && make'
