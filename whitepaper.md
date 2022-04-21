---
authors:
    - Peter Storeng
date: April 2022
keywords: kdb+, q, C
---

# kdb+ Under the Hood


## Introduction

 kdb+ is mostly implemented in the relatively low language C making q a relatively high level language. Abstracting the lower level stuff behind the q language gives lots of benefits: q becomes very powerful and means we can do a lot with a small amount of code, coding time is very quick, we don't need to worry about compiling and we have performance optimisations built in.


 Still an understanding of the basic building blocks of kdb+ is important for the informed developer. It allows an understanding of why some operations are fast with low memory overhead while others are less so. 

 The C API provides a gateway to what kdb+ is doing "under the hood". We will inspect the K data structure - the building block for kdb+. 

## Prerequisites
To get the most out of this paper it is helpful to have a basic understanding of the C language and what pointers are. Hexadecimal byte representation is good to understand too.

The functions used here assume kdb+ version of 4.0 or greater, a 64 bit version of kdb+ and a little endian machine.

## Section 1: The K Object
The k0 struct is defined in the k.h (C language) header file and formatted nicely here (see below): https://code.kx.com/q/interfaces/c-client-for-q/#the-k-object-structure (for V3.0 and later).

The functions used in this section can be found here: https://github.com/ps288/kdbUnderTheHood

```c
typedef struct k0{
  signed char m,a;   // m,a are for internal use.
  signed char t;     // The object's type
  C u;               // The object's attribute flags
  I r;               // The object's reference count
  union{
    // The atoms are held in the following members:
    G g;H h;I i;J j;E e;F f;S s;
    // The following members are used for more complex data.
    struct k0*k;
    struct{
      J n;            // number of elements in vector
      G G0[1];
    };
  };
}*K;

typedef char*S,C;typedef unsigned char G;typedef short H;typedef int I;typedef long long J;typedef float E;typedef double F;typedef void V;

```

A new type K is also created which is pointer to k0 struct. Every object in kdb+ is actually a K type in the C language!

The first 5 members of the k0 struct we will refer to as the header of the object as they contain meta data. Assuming an int in C to be 4 bytes (which is almost always the case) means that the header will occupy 8 bytes. Leaving the 6th member to contain the data. 

### Byte Representation

We would like to inspect our kdb+ objects as a list of bytes and compare to the basic k0 struct.

#### Attempt 1

A Naive First attempt would be to use -8!:


```q
q)-8!7j
0x0100000011000000f90700000000000000
q)
```

This isn't quite right. -8! is used for serializing for IPC. Although for atoms and simple lists the output of -8! would be similar to the actual byte representations the "header" parts are completely different to the header of k0 struct. For more complex data types -8! bears no resemblance to the k0 struct.

#### Attempt 2

We use the C API to read the literal bytes representing the k0 struct for the underlying object and put these bytes into a kdb+ byte list. We call this function rbx - short for return bytes. We use the 2: dynamic load capability to load this function into kdb+.

```q
q)rbx 7j
0x0080f900010000000700000000000000
q)
```

This is correct, however not that clear. 

#### Attempt 3

Lets write a function in q to split the result of rbx into the corresponding members of the k0 struct. We'll call this function formatBytes. We add some comments to the output below to help see the values.

```q
q)formatBytes 7j
,0x00    <---- m - for internal use (size: 1 byte)
,0x80    <---- a - for internal use (size: 1 byte)
,0xf9    <---- t - the kdb type     (size: 1 byte)
,0x01    <---- attribute flag, not really relevant for atoms (size: 1 byte) 
0x03000000    <---- r, the reference count of this object    (size: 4 bytes)
0x0700000000000000    <---- j - the 8 byte long containing   (size: 8 bytes)

*Note the helper function creates a couple of temporary extra references to the object
Which is why the value for r here differs to the result of rbx
```

Finally we have another formatting function to convert bytes to an integer:

```q
q)formatInts formatBytes 7j
0        <-- m
-128     <-- a
-7       <-- t
0        <-- C
3i       <-- u
7        <-- j
```

A couple of things to note when doing this conversion from bytes to ints.

First, generally we can use sv to convert a list of 2/4/8 bytes to shorts/ints/longs https://code.kx.com/q/ref/sv/#bytes-to-integer . This doesn't work for single byte values. It's tempting to simply cast to a short/int/long - this works for unsigned values. But in m,a & t case we actually have signed one byte numbers so this doesn't work. Thus we must manually do the conversion from two's complement:

```q
q)"j"$0xf9   //incorrect
249
q)
q)TWOSC:-128 64 32 16 8 4 2 1;
q)twosc:{sum TWOSC * 0b vs x}
q)twosc 0xf9
-7
```

Second, my machine, and indeed most machines, uses little endian format however kdb+ when encoding/decoding assumes big endianess so we have to reverse the byte list before using sv.

```q
q)0x0 sv 0x0700000000000000
504403158265495552
q)0x0 sv reverse 0x0700000000000000
7
```

However we will stick to using the byte representation as it gives us a better feel for the size of each member in terms of number of bytes.

### Recap

The first 5 members (8 bytes) of the k0 struct are comparable against all kdb+ objects so can be considered as the header. However the 6th member of the struct changes depending on the type. A union in C is a special data type that allows you to store different data types in the same memory location. Depending on the type, t, kdb+ knows which member of the union is the 6th element. The union will contain one (and only one) of the members listed inside it. In this case the 6th member is a C long long.

For the 6th member we have 3 different cases. Recall the union part of the K struct:

``` C
union{
    // The atoms are held in the following members:
    G g;H h;I i;J j;E e;F f;S s;
    // The following members are used for more complex data.
    struct k0*k;
    struct{
      J n;            // number of elements in vector
      G G0[1];
    };
```

1) The case where we have an atom, where the union member is one of: G g;H h;I i;J j;E e;F f;S s;
2) The case where we have a list where the union member is: 
   struct{
      J n;            // number of elements in vector
      G G0[1];
    };

3) The case where we have more complex data. A table for example.

### Lists

```q
q)formatBytes 7 8h
,0x01
,0x80
,0x05
,0x00
0x03000000
0x0200000000000000  -|
0x0700              -|---> the 6th member of k0 struct
0x0800              -|
```
Lists are where things start to get interesting. In this case the 6th member is:  struct {J n;G G0[1];}.
The 8 bytes (J n, a long long) are the length of the list, followed by the n entries in the list. (Note that prior to v3.0 this was I n; - a 4 byte (32 bit) integer and hence the old 2 billion list limit: 2billion ~ 2 xexp 31) (31 as one bit needed for the sign).

Here kdb+ takes advantage of an old C hack, known as the struct hack (now formalised in later versions of C and known as a flexible array member) where the last member is a one element array. In actual fact the memory is managed manually by kdb+ and is allocated so that all list elements fit in and that the size of G0 is really n*sizeof(type).

### Mixed Lists

```q
q)ml:(1 2;1;17 4)
q)formatBytes ml
,0x02
,0x80
,0x00
,0x00
0x03000000
0x0300000000000000    <-- a 3 element list
0x4021755e217f0000    <-- pointer (memory address) to 1 2
0x300e705e217f0000    <-- pointer to 1
0x0025755e217f0000    <-- pointer to 17 4
```

A mixed list with n elements actually contains a list of n memory locations, pointer to k0 structs ie K objects. So we see how kdb+ essentially uses nesting to build more complex data structures.

We can now see why for more complex data IPC serialization is quite different. For IPC serialization a pointer to somewhere in local memory is useless to the process on the other end which would not have access to it. Thus references must be fully resolved in place.

### Symbols
```q
q)formatBytes `testSymbol
,0x00
,0x80
,0xf5
,0x00
0x02000000
0x8855660100000000  <-- memory address for "testSymbol"
```
Recall that symbols are actually interned strings. So really a symbol is a pointer (ie. a memory address) to a single copy of that string. 

### Dictionaries, Tables & Keyed Tables

```q
q)d:`a`b`c!enlist each til 3
q)
q)formatBytes d
,0x01
,0x80
,0x63
,0x00
0x03000000
0x0200000000000000   <--- 2 elements in this "list"
0x004c22df8b7f0000   <--- location of k0 struct - a list of keys
0xc04c22df8b7f0000   <--- location of k0 struct - a list of values

```

A dictionary is stored as a 2 element list of lists.

```q
q)t:flip d
q)
q)formatBytes t
,0x00
,0x80
,0x62
,0x00
0x03000000
0xc00825df8b7f0000   <--- location of dictionary d
```

```bash
kt:1!t
q)
q)kt
a| b c
-| ---
0| 1 2
q)
q)formatBytes kt
,0x01
,0x80
,0x63
,0x00
0x03000000
0x0200000000000000      <--- 2 elements in this "list"
0x601180a6e67f0000      <--- location of the key table
0xc00f80a6e67f0000      <--- location of the value table

```

We will revisit these data structures in the next section.

## Section 2: Memory Management

We see that for the kdb objects which are not atoms or simple lists kdb makes use of pointers. We would like to inspect the k0 structs at those memory addresses to confirm what we describe above is really correct. We need a new function similar to rbx which instead of taking the kdb object directly takes a list of bytes representing the location to that kdb object. We again implement this function using the C API and again lets wrap it in a kdb formatting function so we can view the data more clearly - lets call that function formatBytesMem.

```q
q)formatBytes p:1 2 3
,0x02
,0x80
,0x07
,0x00
0x04000000
0x0300000000000000
0x0100000000000000
0x0200000000000000
0x0300000000000000
q)
q)formatBytes enlist p
,0x01
,0x80
,0x00
,0x00
0x02000000
0x0100000000000000
0x805fe0e8b77f0000       <-- points to p
q)
q)formatBytesMem 0x805fe0e8b77f0000
,0x02
,0x80
,0x07
,0x00
0x00000000
0x0300000000000000
0x0100000000000000
0x0200000000000000
0x0300000000000000
```
We see it works as expected.

> **WARNING**: Because we are now accessing memory directly this can be very dangerous. In particular trying to access an invalid memory location is undefined and will likely cause a segmentation fault.

```q
q)formatBytesMem 0x005c009fa57f0089   <-- random address
Sorry, this application or an associated library has encountered a fatal error and will exit.
If known, please email the steps to reproduce this error to tech@kx.com
with a copy of the kdb+ startup banner and the info printed below.
Thank you.
SIGSEGV: Fault address (nil)
```

### Symbols are Interned Strings

We can now "prove" that symbols are interned strings (only one copy stored in memory). In C strings are stored as a list of characters with a special null terminator at the end so we can again use the C API to read consecutive characters at a memory location until we encounter a null terminator. We implement this function, dynamically load it and call it getSymMemLocation.

```q
q)a:`testSymbol;
q)b:`testSymbol;
q)
q)formatBytes a
,0x00
,0x80
,0xf5
,0x00
0x03000000
0x0838870000000000  
q)
q)formatBytes b
,0x00
,0x80
,0xf5
,0x00
0x03000000
0x0838870000000000
q)
q)getSymMemLocation 0x0838870000000000
`testSymbol
```
So we see that a and b despite being 2 separate variables contain the same memory location and that at that memory location we have `testSymbol stored as expected.

### Memory usage & The meaning of "m"

We know that kdb+’s buddy-memory algorithm allocates according to powers of 2. Lets see how that memory is used.

```q
q).Q.w[]`used
367280
q)l:01b
q).Q.w[]`used
367312
q)367312-367280
32
```
We create a simple boolean list and by comparing the bytes used before and after kdb+ has assigned 32 bytes for the list l which as expected is a power of 2.

However the list itself is only using 18 of those bytes: 8 for the header, 8 for the length of the list and 2 for the 2 booleans. We can confirm that with counting the result from rbx.

```q
q)count rbx l
18
```
We can check the location of the k0 struct for l by putting it in a mixed list:

```q
q)formatBytes enlist l
,0x01
,0x80
,0x00
,0x00
0x02000000
0x0100000000000000
0x809fd542537f0000    <-- location of l
```

lets append some elements to l

```q
q).Q.w[]`used
367744
q)l,:13?0b
q).Q.w[]`used
367744
q)count rbx l
31
q)formatBytes enlist l
,0x01
,0x80
,0x00
,0x00
0x02000000
0x0100000000000000
0x809fd542537f0000     <-- l still at the same location
```
There has been no increase in the bytes used because we still had some space in what kdb+ had allocated for l. What happens we go over that limit:

```q
q).Q.w[]`used
367808
q)l,:2?0b
q).Q.w[]`used
367840
q)367840-367808
32
q)
q)formatBytes enlist l
,0x01
,0x80
,0x00
,0x00
0x02000000
0x0100000000000000
0x404cd242537f0000   <-- new memory address for l
```
We see that now we have another 32 bytes used and the location of l has changed. We have now allocated 64 bytes for l at a different location and the original location for l is now unused so a net increase in 32 bytes.

We now see that the m value in the k0 struct corresponds to the amount of bytes allocated where:
bytes allocated = 2 xexp 4+m. (Recall m is the first result of formatBytes)

```q
q)(first formatBytes ::) each (`int$-16+2 xexp 5+til 5)?\:0b
,0x01
,0x02
,0x03
,0x04
,0x05
\\ subtract 16 for header and length of list
```

### Dictionaries, Tables & Keyed Tables Revisited

Lets use formatBytesMem to prove that a dictionary is just a list of keys and a list of values

```q
q)d:`c1`c2`c3!(n?`3;n?100.0;10*n?100)
q)
q)formatBytes d
,0x01
,0x80
,0x63
,0x00
0x03000000
0x0200000000000000
0x004dd242537f0000  <-- keys location
0x8056d142537f0000  <-- values location

q)formatBytesMem 0x004dd242537f0000  <-- keys location
,0x02
,0x80
,0x0b   <-- list of symbols
,0x00
0x00000000
0x0300000000000000
0x08748d0100000000  |
0x18468d0100000000  |--> Locations of the key values
0x48468d0100000000  |

q)getSymMemLocation  each (0x08748d0100000000;0x18468d0100000000;0x48468d0100000000)
`c1`c2`c3

q)formatBytesMem 0x8056d142537f0000  <-- values location
,0x02
,0x80
,0x00    <-- mixed list
,0x00
0x00000000
0x0300000000000000
0x00dcd142537f0000   <-- list of values for c1
0x0054d042537f0000   <-- list of values for c2
0x0078d042537f0000   <-- list of values for c3

q)formatBytesMem each (0x00dcd142537f0000;0x0054d042537f0000;0x0078d042537f0000)
,0x06 ,0x80 ,0x0b ,0x00 0x00000000 0x6400000000000000 0xa8468d0100000000 0xd8..
,0x06 ,0x80 ,0x09 ,0x00 0x00000000 0x6400000000000000 0x000078aff9f94140 0x00..
,0x06 ,0x80 ,0x07 ,0x00 0x00000000 0x6400000000000000 0xbc02000000000000 0x70..
```

Lets see that a table is just a wrapper around a dictionary:

```q
q)t:flip d
q)
q)formatBytes t
,0x00
,0x80
,0x62
,0x00
0x03000000
0xa044d242537f0000  <-- location of d

q)formatBytesMem[0xa044d242537f0000]~'formatBytes d
11110111b
```

Result is as expected - note that the reference count may change dynamically but are otherwise identical.

Now a keyed table is just an association of 2 tables:

```q
q)kt:1!t
q)formatBytes kt
,0x01
,0x80
,0x63
,0x00
0x03000000
0x0200000000000000
0x4056d142537f0000   <-- key table address
0xa048d142537f0000   <-- value table address
q)formatBytesMem 0x4056d142537f0000  <-- key table address
,0x00
,0x80
,0x62
,0x00
0x00000000
0x804ed242537f0000  <-- dictionary address

q)formatBytesMem 0x804ed242537f0000  <-- dictionary address
,0x01
,0x80
,0x63
,0x00
0x00000000
0x0200000000000000
0x0048d242537f0000  <-- list of keys address
0x2048d242537f0000  <-- list of values address

q)formatBytesMem 0x0048d242537f0000 <-- list of keys address
,0x01
,0x80
,0x0b  <-- symbol list
,0x00
0x00000000
0x0100000000000000  <-- 1 element list
0x08748d0100000000  <-- address of symbol

q)getSymMemLocation 0x08748d0100000000
`c1
```

### Operators and Functions

We know that when we run value on operators we get a numeric code, that is because that is how they are represented under the hood:

```q
q)formatBytes[&]
,0x00
,0x80
,0x66
,0x00
0x11000000
0x0500000000000000
q)value[&]
5
```

Similarly running value on a lambda function is similar to how it is represented under the hood.

```q
q)f:{x+y*17}
q)
q)value f
0xa0624361410003
`x`y
`symbol$()
,`
17
7 5 6 3 4 2 2
"..f"
""
-1
"{x+y*17}"

q)formatBytes[f]
,0x03
,0x80
,0x64
,0x00
0x03000000
0x0a00000000000000
0x8048d242537f0000  <-- lets check this out
0x400cd042537f0000
0xd00bd042537f0000
0x6038d242537f0000
0xf048d142537f0000
0x002bd142537f0000
0x404ad242537f0000
0xc04bd142537f0000
0x7008d142537f0000
0xc02dd242537f0000

q)formatBytesMem 0x8048d242537f0000
,0x01
,0x80
,0x04
,0x00
0x00000000
0x0700000000000000
,0xa0  |
,0x62  |
,0x43  |
,0x61  |-- This list of bytes matched the first output
,0x41  |-- from value[f]
,0x00  |
,0x03  |

```

## Conclusion

We have provided an overview of how kdb+ data objects are layed out in memory. An understanding of what is going on "under the hood" can lead to better coding decisions are more efficient code. To do this we made use of the low level C language through the C API. 

Kdb+ has many positives but one drawback is that it is inefficient at implementing algorithms with many scalar operations. However C has no such limitation and thus for example where implementing a specific sorting algorithm in q directly would be slow. Implementing in C and dynamically loading through the C API is an efficient way to implement custom algorithms and bring your algorithms to the data rather than vice versa. An understanding of the k0 struct and the C API makes this possible.
