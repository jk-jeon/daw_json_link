@echo off

echo "Making build directory"
cd build

echo "Starting full build"
cmake --build . --config Debug --target full -j 2
