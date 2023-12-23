#!/bin/bash

clear 
cd $(dirname "$0")

export JELOS
export ARCH=aarch64-libreelec-linux

cmake --toolchain ~/stepmania/arm64-toolchain.txt -S . -B ./build.rk3566 \
-DWITH_MP3=ON \
-DWITH_SDL=ON \
-DWITH_XINERAMA=OFF \
-DWITH_X11=OFF \
-DWITH_XRANDR=OFF \
-DWiTH_LIBXTST=OFF \
-DWITH_SYSTEM_GLEW=OFF \
-DWITH_GTK3=OFF \
-DWITH_SYSTEM_FFMPEG=OFF \
-DWITH_SYSTEM_PNG=ON \
-Wno-dev 
