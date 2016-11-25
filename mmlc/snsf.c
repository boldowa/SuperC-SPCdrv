/**
 * SNSF関連処理
 */
#include <zlib.h>
#include "gstdafx.h"
#include "mmlman.h"
#include "binaryman.h"
#include "compile.h"
#include "timefunc.h"
#include "spc.h"
#include "snsf.h"

#define ROMSIZE 0x8000*4

/**
 * snesバイナリーデータポインタ
 */
extern char binary_snes_bin_start;
extern char binary_snes_bin_size;


/* チェックサムの計算 */
void calcChecksum(byte* data, int size)
{
	int remainStart;
	int remainSize;
	int i;
	int blocks;

	word checksum;

	*(int*)&data[0x7fdc] = 0x00000000;

	blocks = size / 0x80000;
	remainSize = size % 0x80000;
	remainStart = size - remainSize;

	checksum = 0;
	for(i=0; i<size; i++)
	{
		checksum += data[i];
	}

	if(0 != blocks)
	{
		for(i=0; i<size; i++)
		{
			checksum += data[(i%remainSize) + remainStart];
		}
	}

	checksum += 2*255;

	*(word*)&data[0x7fdc] = ~checksum;
	*(word*)&data[0x7fde] = checksum;
}


/**
 * snsfデータの生成
 */
int makeSNSF(byte* snsf, stSpcCore* core, MmlMan *mml, BinMan* seq, stBrrListData* blist)
{
	byte snes[ROMSIZE+8];
	byte* bin;
	int esa;
	int binInx = 0;
	int datasize = 0;
	int dataDest = 0;
	int seqbase = 0;
	int dataHead = 0;
	int dirtbl = 0;;
	FILE *brr;
	stBrrListData *blread;
	byte brrBuf[1024];
	uLong compressResult;
	uLong dataLength;
	int tagIndex;

	byte* snesStart;
	int snesLen;

	snesStart = (byte*)&binary_snes_bin_start;
	snesLen = (int)&binary_snes_bin_size;

	/* snesデータの初期化 */
	memset(snes, 0xff, ROMSIZE+8);

	/* snesデータコピー */
	memcpy(snes+8, snesStart, snesLen);

	/* snsf用ヘッダ */
	*(int*)&snes[0] = 0;		/* dest location */
	*(int*)&snes[4] = ROMSIZE;	/* data size */

	/* binary書き込み先 */
	bin = &snes[0x8000+8];

	/* ESAを決定します */
	esa = (core->location + core->size);
	if(0 != (esa & 0xff))
	{
		esa += 0x100;
		esa &= 0xff00;
	}

	/* DIRテーブル位置を算出します */
	dirtbl = (core->dir - core->location +4);

	/* コアデータを書き出します */
	*(word*)&bin[0] = core->size;
	*(word*)&bin[2] = core->location;
	memcpy(&bin[4], core->data, core->size);
	binInx = core->size + 4;

	/* ESA値を書き換えます */
	bin[core->esaLoc-core->location+4] = (esa >> 8);

	/* データ書き込み先を決定します */
	dataDest = esa + (mml->maxEDL * 0x800);
	if(0 == mml->maxEDL)
	{
		dataDest = esa + 4;
	}

	dataHead = binInx;
	binInx += 2;
	*(word*)&bin[binInx] = dataDest;
	binInx += 2;

	/* BRRデータを書き出します */
	blread = blist;
	while(blread != NULL)
	{
		int brrsize;
		word brrLoop;
		int readLen;

		brr = fopen(blread->fname, "rb");
		if(brr == NULL)
		{
			return false;
		}

		brrsize = fsize(brr);

		if(0 == brrsize)
		{
			fclose(brr);
			continue;
		}
		if( brrsize < 11 || (0 != ((brrsize -2) % 9)) )
		{
			puterror("makeSNSF: BRR file size error (%s).", blread->fname);
			return 0;
		}
		if(0x10000 <= ((brrsize-2)+dataDest))
		{
			puterror("makeSNSF: Insert data size is too big.");
			return 0;
		}

		/* BRRループヘッダ読み出し */
		fread(&brrLoop, sizeof(word), 1, brr);
		brrLoop += dataDest;

		/* DIRテーブルの書き出し */
		*(word*)&bin[dirtbl + (blread->brrInx*4)] = dataDest;
		*(word*)&bin[dirtbl + (blread->brrInx*4) + 2] = brrLoop;
		/* BRRデータの追加 */
		while(0 != (readLen = fread(brrBuf, sizeof(byte), 1024, brr)))
		{
			memcpy(&bin[binInx], brrBuf, readLen);
			binInx += readLen;
			dataDest += readLen;
			datasize += readLen;
		}

		fclose(brr);
		blread = blread->next;
	}

	/* シーケンスデータを書き出します */
	seqbase = dataDest;
	if(0x10000 <= (seq->dataInx+dataDest))
	{
		puterror("makeSNSF: Insert data size is too big.");
		return 0;
	}
	memcpy(&bin[binInx], seq->data, seq->dataInx);
	binInx += seq->dataInx;
	datasize += seq->dataInx;
	*(word*)&bin[core->seqBasePoint-core->location+4] = seqbase;

	/* データサイズを書き込みます */
	*(word*)&bin[dataHead] = datasize;

	*(word*)&bin[binInx] = 0;
	binInx += 2;
	*(word*)&bin[binInx] = core->location;
	binInx += 2;

	/* チェックサムを更新します */
	calcChecksum(&snes[8], ROMSIZE);

#ifdef DEBUG
	{
		FILE *of;

		of = fopen("a.smc", "wb");
		fwrite(snes+8, ROMSIZE+8, 1, of);
		fclose(of);
	}
#endif

	/* snsfデータ情報を作成します */
	memcpy(&snsf[0], "PSF", 3);	/* 0x0000 - 0x0002 : PSF Header                      */
	snsf[3] = 0x23;			/* 0x0003          : Version - SNSF(0x23)            */
	memset(&snsf[4], 0, 4);		/* 0x0004 - 0x0007 : Reserved area(SRAM DATA) size   */
					/* 0x0008 - 0x000b : Compress data size              */
					/* 0x000c - 0x000f : CRC32                           */
					/* 0x---- - 0x---- : Reserved Data Area (*Nothing)   */
					/* 0x0010 - 0x???? : snsf zlib compressed data       */
	/* snesデータをzlib圧縮します */
	dataLength = ROMSIZE+8;
	compressResult = compress2(&snsf[0x10], &dataLength, snes, ROMSIZE+8, 9);
	if(Z_OK != compressResult)
	{
		puterror("makeSNSF: zlib compress error.");
		return 0;
	}
	*(unsigned int*)&snsf[8] = dataLength;
	*(unsigned int*)&snsf[12] = crc32(crc32(0L, Z_NULL, 0), &snsf[0x10], dataLength);

	/*----- タグデータの書き込み -----*/

	/* タイトル */
	tagIndex = 0x10 + dataLength;
	sprintf((char*)&snsf[tagIndex], "[TAG]title=%s", mml->spcTitle);
	tagIndex += strlen((char*)&snsf[tagIndex]);
	snsf[tagIndex++] = '\xa';

	/* アーティスト */
	sprintf((char*)&snsf[tagIndex], "artist=%s", mml->spcComposer);
	tagIndex += strlen((char*)&snsf[tagIndex]);
	snsf[tagIndex++] = '\xa';

	/* ゲームタイトル */
	sprintf((char*)&snsf[tagIndex], "game=%s", mml->spcGame);
	tagIndex += strlen((char*)&snsf[tagIndex]);
	snsf[tagIndex++] = '\xa';

	/* 年 */
	sprintf((char*)&snsf[tagIndex], "year=");
	tagIndex += strlen((char*)&snsf[tagIndex]);
	setYear((char*)&snsf[tagIndex]);
	tagIndex += strlen((char*)&snsf[tagIndex]);
	snsf[tagIndex++] = '\xa';

	/* コメント */
	sprintf((char*)&snsf[tagIndex], "comment=%s", mml->spcComment);
	tagIndex += strlen((char*)&snsf[tagIndex]);
	snsf[tagIndex++] = '\xa';

	/* 作成者 */
	sprintf((char*)&snsf[tagIndex], "snsfby=%s", mml->spcDumper);
	tagIndex += strlen((char*)&snsf[tagIndex]);
	snsf[tagIndex++] = '\xa';

	/* 再生時間 */
	sprintf((char*)&snsf[tagIndex], "length=%d", mml->playingTime);
	tagIndex += strlen((char*)&snsf[tagIndex]);
	snsf[tagIndex++] = '\xa';

	/* フェード時間 */
	sprintf((char*)&snsf[tagIndex], "fade=%d.%d", mml->fadeTime/1000, mml->fadeTime%1000);
	tagIndex += strlen((char*)&snsf[tagIndex]);
	/* snsf[tagIndex++] = '\xa'; */

	return tagIndex;
}

