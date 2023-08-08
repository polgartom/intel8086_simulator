@echo off 

mkdir .\build
pushd .\build

cl -Zi ..\src\main.c

popd .\build