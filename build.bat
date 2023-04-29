@echo off 

mkdir .\build
pushd .\build

cl -Zi ..\sim86.c ..\simulator.c ..\decoder.c ..\printer.c ..\main.c

popd .\build