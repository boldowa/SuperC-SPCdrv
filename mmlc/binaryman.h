/**
 * バイナリデータ管理
 */

#ifndef _BINARYMAN_H
#define _BINARYMAN_H

/**
 * 定数
 */
#define DATA_MAX_SIZE 0x4000

/**
 * 変数
 */

/**
 * 構造体
 */
typedef struct stBinMan
{
	/**
	 * バイナリデータポインタ
	 * ※直接更新(ex. fopen(bin->fp)) は、
	 *   binaryman.c以外禁止
	 */
	/* FILE *fp; */  /* 使用しない */

	/**
	 * バイナリデータバッファ
	 * ※直接書き出し(ex. bin->data[foo]=X) は、
	 *   binaryman.c以外禁止
	 */
	byte data[DATA_MAX_SIZE];

	/**
	 * バイナリデータインデックス
	 * ※直接更新(ex. mml->buffInx=0) は、
	 *   binaryman.c以外禁止
	 */
	int dataInx;

	/**
         * 開放処理要フラグ
	 * ※直接更新(ex.u bin->reqfree = false) は、
	 *   binaryman.c以外禁止
	 */
	bool reqfree;

	/**
	 * データサイズ上限フラグ
	 */
	bool ismax;
} BinMan;

/**
 * 関数
 */
bool bininit(BinMan*);
void binfree(BinMan*);
bool bindataadd(BinMan*, const byte*, const int);

#endif /* _BINARYMAN_H */
