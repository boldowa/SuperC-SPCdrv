/**
 * Pre compile Header for GCC
 *
 */

#ifndef _GSTDAFX_H
#define _GSTDAFX_H

/**
 * Defines...
 *
 */

/* define bool */
#if !defined __cplusplus && !defined bool
 #ifdef _Bool
  typedef _Bool bool;
 #else
  typedef unsigned int bool;
 #endif

 #ifndef true
  #define true 1
 #endif

 #ifndef false
  #define false 0
 #endif
#endif
/* end define bool */

#ifndef MAX_PATH
  #define MAX_PATH 260
#endif


/* data typing */
#ifndef byte
 typedef unsigned char byte;
#endif

#ifndef word
 typedef unsigned short word;
#endif

#ifndef dword
 typedef unsigned long dword;
#endif


/**
 * Include files...
 *
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

/**
 * macros
 */
/* 通常出力用 */
extern bool vdebug;
#define putdebug(fmt, ...) if(vdebug) fprintf(stderr, "[DEBUG] " fmt "\n", ##__VA_ARGS__)
#define putinfo(fmt, ...)             fprintf(stderr, "[INFO ] " fmt "\n", ##__VA_ARGS__)
#define putwarn(fmt, ...)             fprintf(stderr, "[WARN ] " fmt "\n", ##__VA_ARGS__)
#define puterror(fmt, ...)            fprintf(stderr, "[ERROR] " fmt "\n", ##__VA_ARGS__)
#define putfatal(fmt, ...)            fprintf(stderr, "[FATAL] " fmt "\n", ##__VA_ARGS__)
/* mmlエラー出力用 */
#define putmmldebug(date, fname, line, row, message)   if(vdebug) fprintf(stderr, "[DEBUG] %s %s(line=%d, col=%d): %s\n", date, fname, line, row, message)
#define putmmlinfo(date, fname, line, row, message)               fprintf(stderr, "[INFO ] %s %s(line=%d, col=%d): %s\n", date, fname, line, row, message)
#define putmmlwarn(date, fname, line, row, message)               fprintf(stderr, "[WARN ] %s %s(line=%d, col=%d): %s\n", date, fname, line, row, message)
#define putmmlerror(date, fname, line, row, message)              fprintf(stderr, "[ERROR] %s %s(line=%d, col=%d): %s\n", date, fname, line, row, message)
#define putmmlfatal(date, fname, line, row, message)              fprintf(stderr, "[FATAL] %s %s(line=%d, col=%d): %s\n", date, fname, line, row, message)
#define putmmlunknown(date, fname, line, row, message)            fprintf(stderr, "[?????] %s Unknown error code(in %s, %dL) : %s(line=%d, row=%d): %s\n", date, __FILE__, __LINE__, fname, line, row, message)

/**
 * RFC1321 GLOBAL.H - RSAREF types and constants
 */

/* PROTOTYPES should be set to one if and only if the compiler supports
  function argument prototyping.
The following makes PROTOTYPES default to 0 if it has not already
  been defined with C compiler flags.
 */
#ifndef PROTOTYPES
#define PROTOTYPES 0
#endif

/* POINTER defines a generic pointer type */
typedef unsigned char *POINTER;

/* UINT2 defines a two byte word */
typedef unsigned short int UINT2;

/* UINT4 defines a four byte word */
typedef unsigned long int UINT4;

/* PROTO_LIST is defined depending on how PROTOTYPES is defined above.
If using PROTOTYPES, then PROTO_LIST returns the list, otherwise it
  returns an empty list.
 */
#if PROTOTYPES
#define PROTO_LIST(list) list
#else
#define PROTO_LIST(list) ()
#endif

#endif /* _GSTDAFX */

