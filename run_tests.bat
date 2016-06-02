@echo off
mkdir build
echo Compiling mug...
g++ -std=c++11 src\*.cpp -o build\mug.exe
rem g++ -Wall -Wextra -Wzero-as-null-pointer-constant -std=c++11 src\*.cpp -o build\mug.exe

echo Compiling code_gen_tests...
build\mug.exe tests\code_gen_tests.mug > build\code_gen_tests.s

echo Assembling...
yasm -fwin64 build\code_gen_tests.s -o build\code_gen_tests.o

echo Linking...
gcc tests\code_gen_tests.c build\code_gen_tests.o -o build\code_gen_tests.exe

echo Running code_gen_tests...
build\code_gen_tests.exe
