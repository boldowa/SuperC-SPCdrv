/**
 * バイナリデータ管理
 */

#include "gstdafx.h"
#include "binaryman.h"

/**
 * バイナリデータ初期化
 *   return : true  - 初期化成功
 *            false - 初期化失敗(メモリ不足時に返ります)
 */
bool bininit(BinMan* bin)
{

	if(bin == NULL)
	{
		/* NULLポインタが渡された場合は、動的にメモリを確保します */
		bin = (BinMan*)malloc(sizeof(BinMan));
		if(bin == NULL)
		{
			/* 割り当て失敗 */
			return false;
		}
		bin->reqfree = true;
	}
	else
	{
		/* 確保済みのメモリが指定された場合は、メモリの開放は不要です */
		bin->reqfree = false;
	}

	/* バイナリデータを初期化します */
	bin->dataInx = 0;
	bin->ismax = false;
	memset(bin->data, 0, DATA_MAX_SIZE);

	return true;
}

/**
 * バイナリデータを解放します
 */
void binfree(BinMan* bin)
{
	if(bin != NULL)
	{
		/* メモリ解放が必要な場合は、メモリを解放します */
		if(bin->reqfree == true)
		{
			free(bin);
		}
	}
}

/**
 * 指定されたデータを、バッファに追加します
 *   return : true  - 追加成功
 *            false - 追加失敗(データサイズが大きすぎる場合に返ります)
 */
bool bindataadd(BinMan* bin, const byte* data, const int writeLen)
{
	int writableLen;
	int wcnt;

	writableLen = DATA_MAX_SIZE - bin->dataInx;
	/* 書き込みサイズが書き込み可能サイズより多い場合、書き込み失敗とします */
	if(writableLen < writeLen)
	{
		return false;
	}

	/* データをコピーします */
	for(wcnt = 0; wcnt < writeLen; wcnt++)
	{
		bin->data[bin->dataInx++] = data[wcnt];
	}

	return true;
}

