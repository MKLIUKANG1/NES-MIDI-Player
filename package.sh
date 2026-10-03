#!/bin/bash
cd "$(dirname "$0")"
for dll in libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll SDL2.dll; do
    cp -u "/mingw64/bin/$dll" . && echo "OK: $dll"
done
echo "--- Ready to distribute ---"
ls -la *.exe *.dll
