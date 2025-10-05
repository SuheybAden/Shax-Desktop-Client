#!/bin/bash

# Make the build directory if it doesn't already exist
mkdir -p build

cmake -B ./build
cmake --build ./build/

chmod +x ./build/shax-desktop-client