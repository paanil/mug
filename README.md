# mug

## Group

* Ilari Paananen ilari.k.paananen@student.jyu.fi

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
