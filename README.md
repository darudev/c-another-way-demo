# Demo

## Intro

Only 64-bit Linux at this point. Tested on Arch.

A subset of things I've learned from game programmers regarding architecture and coding practices for C projects. Still learning so there of course things here that I don't fully understand the how and why for yet.

This is also of course not by any means my ideas! I just tried to implement them my own way. 

The true sources are works by Casey Muratori and Ryan Fleury. And whoever they got their ideas from. 

## Single header file library

Simple dependency management and usage of small(er) C libraries. The whole library is just on .h file!

Made famous by [Sean Barret's stb libs](https://github.com/nothings/stb).

## Base layer

The base layer is single header library thing. Just drop the .h file into project.
Usage is pretty simple. Define the implementation **once**. Then include the .h file! 

### Arena

This is the **BIG** shift regarding manual memory management!

Object groups thinking. Opposed to single object thinking.

We pass an memory allocator around. The Arena. Which is basically a byte array and a counter of where in the byte array we are.    
And dynamic memory is allocated from it.    
At some point we are done with the data allocated.   
At that point we reset the Arena. The whole byte array. We "free" memory. Meaning ALL the objects allocated gets freed at one single point.

This shift in thinking kind of makes garbage collection, RIIA and the borrow checker irrelevant. So much complexity.. POFF!

Way simpler and surprisingly useful in a large number of scenarios when programming. 

No system call. Just set a variable to zero. So yeah, also very performant!

### String

Now by using the base layer we can use Arenas. For example to handle Strings.

No god damn null termination! We have the length of the String stored along the actual bytes (chars). 

### Your own "standard" lib

And with Arena and String in the Single Header Library we can finally attempt a modern sane C standard lib.

Like having a string compare that actually return true or false wheter the strings match or not! Shocking stuff for sure. 


## Unity builds and build script

Another thing I've learned is that simple build scripts are now possible. Everything is just one single compilation unit. Compiles very fast. And therefore I think that things like make, cmake, ninja and all that stuff ain't needed. 

## Example CSV

The demo is just a simple CSV parser. Uhm, maybe not "Just", because turned out to be a tad more work than I expected! 

Anyway, it reads a CSV with the following columns: U64, S32, String, B32, F64

## Usage

Clone repo. 

    chmod +x build

    ./build debug run demo.csv
