@echo off 

mkdir .\build
pushd .\build

cl -Zi ..\src\main.c
cl -Zi ..\assembler\assembler.c

popd .\build