@echo off
REM use: profile 32 myfile.txt

IF "%1"=="" (
	ECHO Error: missing args
	EXIT /B 1
)
IF "%2"=="" (
	ECHO Error: missing args
	EXIT /B 1
)

set "exe=user.exe"

mingw32-make clean
mingw32-make prof DEPTH=%1
"%exe%"
gprof %exe% gmon.out > analysis/%2

del gmon.out
mingw32-make clean
