# Simple Noter for Windows 3.11 (16-bit)

This repository contains my first big project using clean, 16-bit WinAPI. It's client for my (too) simple PHP backend for notetaking.
It utilizes lot of experiments with WinAPI, networking and resource files.

## What is included in this repo?

Quite big WinAPI project providing whole client API for first (1.0) version of Noter backend adding quite classic GUI.
The whole program is in Polish with hardcoded labels (yes, I know it's a bit non professional), because I couldn't find how to make string table on 16-bit Windows without need to allocate additional memory for it while loading (I wanted to move it to resource section to make it translatable). Will try to find a solution for that problem someday.
Portions of this software contain work done by other people (UTF-8 encode and decode procedures, JSON parser and BZip2 decompression algorithm). Check out **Codes created by others** section of this README file.

## What to use to compile this code?

The code provided here was made to compile using Open Watcom IDE, which is quite good for modern MS-DOS/WfW3.11 development.
It's possible that it would compile using old Microsoft compilers, but I haven't tested it. It's best to use Watcom for it.
In fact, this project is heavily tied to Watcom coding style, so I think there might be serious problems with compiling it outside such environment.

## Components needed to run the program

As I'm doing my best to make this program more customizable, more and more components are moved to DLLs that are now necessary to run this program.
For now, the codepage definition is needed, for which code can be found here: [CP1250 DLL](https://github.com/Magnetic-Fox/CP1250-DLL) repository.

## Codes created by others

This software uses codes created by other people.

I've used:
* `utf8_decode.c` and `utf8_decode.h` &copy; 2005 JSON.org
* `utf8_encode.cpp` &copy; Ondřej Hruška
* `json.c` and `json.h` &copy; 2012-2021 json-parser authors
* `bzip2.c` (patched by me to files `unbzip2.c` and `unbzip2.h`) &copy; 1996, 1997 Julian Seward

Thank You for creating really useful procedures! Without 'em my program would never work the way it does!

## Known problems

As JSON and Noter backend itself utilizes UTF-8, there is need to convert text to and from it.

* I've implemented 1250 codepage translation table only (which is used mostly in Polish Windows distributions). If You like to use this program with Your own codepage, You'll have to create conversion table on Your own according to the `CP1250-DLL` repository.
* Converting from ANSI to UTF-8 and then to URL encoded takes huge amounts of memory and can quickly exceed 32K size of internal string variable even if written text is not so long. This will cause string class to reallocate internal buffer and make it twice as big, which unfortunately causes program to hang (at least under Windows XP). I'm not sure why this happens (OS should not allow allocating too big memory section when using non-huge pointers or at least Watcom compiler should be aware of this problem). Probably the best solution for this would be to program own memory handler and simple "string-like" class. As this would take huge amount of time to do, I haven't done it for now - maybe someday.

## Tests information

The program was tested under Windows XP (32-bit version running at 64-bit Intel Pentium), under Windows for Workgroups 3.11 (running on Intel 386) and under Linux Mint (using Wine).

## Disclaimer

I've made much effort to provide here working and checked codes with hope it will be useful.
**However, these codes are provided here "AS IS", with absolutely no warranty! I take no responsibility for using them - DO IT ON YOUR OWN RISK!**

## License

Codes provided here are free for personal use.
If you like to use any part of these codes in your software, just please give me some simple credits and it will be okay. ;)
In case you would like to make paid software and use parts of these codes - please, contact me before.

*Bartłomiej "Magnetic-Fox" Węgrzyn,
4th November, 2023 - 17th August, 2024*