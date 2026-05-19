
#ifndef KERNEL_TYPES_H
#define KERNEL_TYPES_H

typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned int   uint32;
typedef unsigned long  uint64;


typedef signed char    int8;
typedef signed short   int16;
typedef signed int     int32;
typedef signed long    int64;

typedef uint64  uintptr;

//Booléen
typedef int bool;
#define true 1
#define false 0

// NULL
#define NULL ((void*)0)
#endif



