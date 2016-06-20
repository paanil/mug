#!/bin/sh

@echo off
mkdir build
echo Compiling mug test build...
g++ -std=c++11 -DTEST_BUILD -o build/mug_test src/*.cpp

echo Running unit tests...
echo ---
build/mug_test

echo ---
echo Compiling mug...
g++ -std=c++11 -o build/mug src/*.cpp

echo ---
echo Compiling code_gen_tests...
build/mug -s -o build/code_gen_tests.s tests/code_gen_tests.mug

echo Assembling code_gen_tests...
nasm -f win64 -o build/code_gen_tests.o build/code_gen_tests.s

echo Linking code_gen_tests...
gcc -o build/code_gen_tests tests/code_gen_tests.c build/code_gen_tests.o

echo Running code_gen_tests...
echo ---
build/code_gen_tests

echo ---
echo Compiling external_call_test...
build/mug -s -o build/external_call_test.s tests/external_call_test.mug
build/mug -s -o build/external_fibo.s tests/external_fibo.mug

echo Assembling external_call_test...
nasm -f win64 -o build/external_call_test.o build/external_call_test.s
nasm -f win64 -o build/external_fibo.o build/external_fibo.s

echo Linking external_call_test...
gcc -o build/external_call_test tests/external_call_test.c build/external_call_test.o build/external_fibo.o

echo Running external_call_test...
echo ---
build/external_call_test
