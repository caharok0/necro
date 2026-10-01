# Build pipeline

1. Build the PSP executable with PSPDEV.
2. Confirm `EBOOT.PBP` exists.
3. Put it under `PSP_GAME/SYSDIR/`.
4. Use UMDGenCLI `create` to produce the ISO.
5. Test the PBP in PPSSPP before copying the final ISO to PSP storage.

The official PSPDEV project documents both local installation and the `pspdev/pspdev:latest` Docker image. UMDGenCLI documents the `create <dir> <out>` command for PSP UMD images.
