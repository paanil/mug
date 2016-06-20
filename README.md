# mug

A compiler that outputs AMD64 assembly for NASM to assemble.
The source language is also called mug.
The compiler's output uses Windows compatible calling conventions.

The whole thing was made by
* Ilari Paananen ilari.k.paananen@student.jyu.fi

The work should be worth 5 credits because it has:
- all the mandatory features (3 credits)
- IR generation (1 credit)
- AMD64 code generation with naive register allocation (1 credit)

## Summary

- target language AMD64, Windows compatible calling convention
- recommended OS for testing: Windows
- needed to compile mug: g++ with C++11 support (GCC 5.3.0 works)
- needed to assemble and link mug's output: NASM and GCC

## Compiling the compiler

The mug compiler can be compiled at least with gcc version 5.3.0
using the following command:

    g++ -std=c++11 -o mug src/*.cpp

## Running the compiler

Running mug without parameters prints the following help text:

    Usage: mug [-s|-c] [-o <output-file>] <source-file>

    By default, mug runs three phases:
    (1) <source-file> ==> mug ==> out.s
    (2) out.s ==> nasm ==> out.o
    (3) out.o ==> gcc ==> out.exe

    If -s is given, mug stops after phase (1).
    If -c is given, mug stops after phase (2).
    If both -s and -c are given, only the last one is effective.
    If -o is given, the final output of mug is <output-file>.

    Example usage: mug -c -o banana.o banana.mug

To compile an executable with mug, NASM and GCC should be in path as

    nasm and gcc

NOTE: mug invokes nasm with the flag -f win64. One should be able to compile
for Linux if one gives the -s flag to mug and invokes nasm and gcc by hand.

NOTE: mug targets AMD64 and uses Windows specific calling conventions.
The latter makes it impossible(?) to call mug functions from C on Linux
and also the other way around.

### Examples

Invoking mug can create three kinds of files:
- assembly files created by mug (.s)
- object files created by nasm (.o)
- executables created by gcc (on Windows, .exe)

The examples/example.mug is a program that exits with return value 42.
Here is a list of all the possible ways it can be compiled with mug and
all the resulting output files.

1. Only assembly, output file not specified:

        mug -s examples/example.mug
        => out.s

2. Only assembly, output file 'example.s':

        mug -s -o example.s examples/example.mug
        => example.s

3. Assembly and object file, output file not specified:

        mug -c examples/example.mug
        => out.s, out.o

4. Assembly and object file, output file 'example.o':

        mug -c -o example.o examples/example.mug
        => out.s, example.o

5. Assembly, object file, and executable, output file not specified:

        mug examples/example.mug
        => out.s, out.o, out.exe

6. Assembly, object file, and executable, output file 'example':

        mug -o example examples/example.mug
        => out.s, out.o, example.exe

As can be seen, the output file can be specified only for the last file
to be created. Other files will have the default name ('out' dot something).

There are more examples under the examples/ directory. The fibo.mug and
fibo_iterative.mug can be easily tested by first compiling with the method 5
and then running the resulting out.exe:

    mug examples/<file-name>.mug
    out.exe

The hello example has .mug and .c file so gcc needs to be invoked by hand:

    mug -c examples/hello.mug
    gcc -o hello examples/hello.c out.o
    hello.exe

NOTE: The hello example works only on Windows.

## Testing the compiler

The run_tests.sh script compiles the mug compiler and the test
build of mug. It runs the test build which runs the unit tests.
It also compiles the code generation tests and runs them.
Then it compiles the external call test and runs it.
All of the files created are placed in a build/ directory.

NOTE: The external call test and code generation tests work
only on Windows because there is C involved.

All the script does can be done by hand.

Test build of mug can be compiled with the following command:

    g++ -std=c++11 -DTEST_BUILD -o mug_test src/*.cpp

The mug_test will run the unit tests and print the results of said tests.

Code generation tests need to be compiled with mug (not the test build one)
and linked to the test program (written in C) with gcc:

    mug -c tests/code_gen_tests.mug
    gcc -o code_gen_tests tests/code_gen_tests.c out.o

The code_gen_tests calls different functions written in mug (the language).
It compares the return values to the expected ones and prints the results.

External call test has two .mug files and one .c file. It can be compiled with:

    mug -c -o call_test.o tests/external_call_test.mug
    mug -c -o fibo.o tests/external_fibo.mug
    gcc -o external_call_test tests/external_call_test.c call_test.o fibo.o

The external_call_test calls fibo() defined in the external_fibo.mug and
compares the result with the expected one. The test will print "OK" or "ERROR"
by calling print_char() defined in external_call_test.c.

## Still missing or misbehaving features

- Size of all types (even boolean) is 64 bits. The suffix in uint8, int16,
  etc. doesn't mean anything.
- There are no pointer types.
- For loops are not supported.
- Top level statements have no effect (but they are allowed for parser tests
  etc. and they are intended to work some day).
- Globals don't work either.
- Callee save registers are not used.

## Initial idea

Mug is going to be a compiler for a programming language called mug. Mug language will be designed alongside the mug compiler. The language is going to be a simple procedural programming language with some of the following features:
* single and multi line comments
* data types
  * integers with different sizes
  * pointer types
  * boolean
* if, while, and for statements
* procedures and functions

Mug will either use LLVM as it's back end or generate AMD64 assembly from some intermediate language.

Host language: C/C++

Testing plan: unit tests and examples

NASM/AMD64 program for Windows that prints "mug":

            global  main
            extern  puts
            
            section .text
    main:
            push    rbp
            mov     rcx, team
            sub     rsp, 32
            call    puts
            add     rsp, 32
            mov     rax, 0
            pop     rbp
            ret
            
            align   8
    team:   db      "mug", 0
