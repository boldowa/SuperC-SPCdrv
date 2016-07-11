/**
 * mmlコンパイル
 */

#ifndef _COMPILE_H
#define _COMPILE_H

#include "mmlman.h"
#include "binaryman.h"
#include "errorman.h"

/**
 * 定数
 */
#define TRACKS 10
#define TRACK_SIZE_MAX 0x1000
#define SUBROUTINE_MAX 128
#define DATA_MAX 0x4000
#define BASEPOINT 0x2000

/**
 * 関数
 */
ErrorNode* compile(MmlMan*, BinMan*);

#endif /* _COMPILE_H */

