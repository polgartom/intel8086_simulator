@echo off 

call .\build.bat 
call .\build\assembler.exe
call nasm.exe mock\listing_0039_more_movs.asm
call .\build\main.exe .\mock\a.out --decode
call .\build\main.exe .\mock\listing_0039_more_movs --decode
@REM call echo -----------DONE-----------