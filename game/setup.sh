#!/bin/bash

cd SDL2-2.0.12
make clean
mkdir build
cd build
../configure
make
sudo make install
cd ../../

cd SDL2_gfx-1.0.4
make clean
mkdir build
cd build
../configure
make
sudo make install
cd ../../

cd SDL2_image-2.0.5
make clean
mkdir build
cd build
../configure
make
sudo make install
cd ../../

cd SDL2_mixer-2.0.4
make clean
mkdir build
cd build
../configure
make
sudo make install
cd ../../

cd SDL2_ttf-2.0.15
make clean
mkdir build
cd build
../configure
make
sudo make install
cd ../../

# brew install sdl2 sdl2_image sdl2_ttf sdl2_mixer sdl2_gfx sdl2_net
