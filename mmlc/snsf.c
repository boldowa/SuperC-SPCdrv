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
#include "address.h"
#include "snsf.h"


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
 * SNESデータの生成
 */
int buildSnes(byte* snes, stSpcCore* core, MmlMan* mml, BinMan* seq, stBrrListData* blist)
{
	int esa;

	byte* snesStart;
	int snesLen;

	byte* coreData;
	byte* brrSetNumberTable;
	byte* brrSetAddressTable;
	byte* brrDataAddressTable;
	byte* seqDataAddressTable;
	byte* brrSetData;
	int mdPtr = 0;
	int coreLocOnSnes;

	int brrCounter=1;
	stBrrListData *blread;
	byte brrBuf[1024];

	int snesSeqLoc;

	snesStart = (byte*)&binary_snes_bin_start;
	snesLen = (int)&binary_snes_bin_size;

	/* データ書き込み用テーブル初期化 */
	brrSetNumberTable = &snes[0x8000];
	brrSetAddressTable = &snes[0x8100];
	brrDataAddressTable = &snes[0x8300];
	seqDataAddressTable = &snes[0x8600];
	brrSetData = &snes[0x8900];
	mdPtr = 0x10000;

	/* snesデータの初期化 */
	memset(snes, 0xff, ROMSIZE);

	/* snesデータをコピー */
	memcpy(snes, snesStart, snesLen);

	/* bank0末端に書かれた情報から、コア挿入先を取得します */
	coreLocOnSnes = snes2pc(*(int*)&snes[0x7fbd] & 0x00ffffff);
	putdebug("core on snes: 0x%06x", pc2snes(coreLocOnSnes));
	coreData = &snes[coreLocOnSnes];

	/* ESAを決定します */
	esa = (core->location + core->size);
	if(0 != (esa & 0xff))
	{
		esa += 0x100;
		esa &= 0xff00;
	}

	/* SPCドライバプログラムを書き出します */
	*(word*)&coreData[0] = core->size;
	*(word*)&coreData[2] = core->location;
	memcpy(&coreData[4], core->data, core->size);
	/* ESA値を更新します                        */
	/* (※SPCドライバのmake段階で設定できてりゃ */
	/*  こんなこと必要ないんですけど…          */
	coreData[core->esaLoc-core->location+4] = (esa >> 8);
	/* データ末端を書き出します */
	*(word*)&coreData[core->size+4] = 0;			/* size = 0      */
	*(word*)&coreData[core->size+6] = core->location;	/* boot = 0x???? */

	/*----------------------------------------------*/
	/* 以下、SNESデータの構築を行う処理             */
	/* SNSFで鳴るよう適当に構築する                 */
	/*----------------------------------------------*/
	brrSetNumberTable[1] = 1;			/* BRRセット とりあえず1番を使う */
	*(word*)&brrSetAddressTable[2] = 0x8900;	/* BRRセット1 = 0x8900 に格納    */
	brrSetData[0] = mml->maxEDL;			/* 曲の初期EDL値(EDLのMAX値) */
	brrSetData++;

	/* BRRデータを書き出します */
	blread = blist;
	while(blread != NULL)
	{
		int brrsize;
		word brrLoop;
		int readLen;
		int snesBrrLoc;
		FILE *brr;

		brr = fopen(blread->fname, "rb");
		if(brr == NULL)
		{
			/* BRRファイル読み込みエラー */
			/* mmlコンパイルに成功している場合は、 */
			/* このルートに来ることはまずありえない */
			return 0;
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
		/*
		if(0x10000 <= ((brrsize-2)+dataDest))
		{
			puterror("makeSNSF: Insert data size is too big.");
			return 0;
		}
		*/

		/* データ位置 */
		brrSetData[0] = brrCounter++;
		brrSetData++;
		snesBrrLoc = pc2snes(mdPtr);
		*(word*)&brrDataAddressTable[3] = (snesBrrLoc&0xffff);
		brrDataAddressTable[5] = (snesBrrLoc>>16);
		brrDataAddressTable += 3;
		/* BRRループヘッダ読み出し */
		fread(&brrLoop, sizeof(word), 1, brr);
		*(word*)&snes[mdPtr] = brrLoop;
		*(word*)&snes[mdPtr+2] = brrsize-2;
		mdPtr += 4;

		/* BRRデータの追加 */
		while(0 != (readLen = fread(brrBuf, sizeof(byte), 1024, brr)))
		{
			memcpy(&snes[mdPtr], brrBuf, readLen);
			mdPtr += readLen;
		}

		fclose(brr);
		blread = blread->next;
	}
	brrSetData[0] = 0;	/* 末端 */

	putdebug("[seq_loc_pc] 0x%06x", mdPtr);
	snesSeqLoc = pc2snes(mdPtr);
	putdebug("[seq_loc_snes] 0x%06x", snesSeqLoc);
	*(word*)&seqDataAddressTable[3] = (snesSeqLoc&0xffff);
	seqDataAddressTable[5] = (snesSeqLoc>>16);

	*(word*)&snes[mdPtr] = seq->dataInx;
	mdPtr += 2;
	memcpy(&snes[mdPtr], seq->data, seq->dataInx);

	/* チェックサムを更新します */
	calcChecksum(snes, ROMSIZE);

	return 1;
}


/**
 * snsfデータの生成
 */
int makeSNSF(byte* snsf, stSpcCore* core, MmlMan *mml, BinMan* seq, stBrrListData* blist)
{
	/* snsf圧縮用データ作成領域 */
	byte snes[ROMSIZE+8];

	/* SNSF情報 */
	uLong compressResult;
	uLong dataLength;
	int tagIndex = 0;

	/* snsf用ヘッダ書き込み */
	*(int*)&snes[0] = 0;		/* dest location */
	*(int*)&snes[4] = ROMSIZE;	/* data size */

	/* ROMデータの構築 */
	if(0 == buildSnes(&snes[8], core, mml, seq, blist))
	{
		/* データ構築失敗 */
		return 0;
	}

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
	tagIndex = 0x10 + dataLength;

	/* タイトル */
	if(0 == strcmp("", mml->spcTitle))
	{
		sprintf((char*)&snsf[tagIndex], "[TAG]title=%s", mml->fname);
		tagIndex += strlen((char*)&snsf[tagIndex]);
		snsf[tagIndex++] = '\xa';
	}
	else
	{
		sprintf((char*)&snsf[tagIndex], "[TAG]title=%s", mml->spcTitle);
		tagIndex += strlen((char*)&snsf[tagIndex]);
		snsf[tagIndex++] = '\xa';
	}

	/* アーティスト */
	if(0 != strcmp("", mml->spcComposer))
	{
		sprintf((char*)&snsf[tagIndex], "artist=%s", mml->spcComposer);
		tagIndex += strlen((char*)&snsf[tagIndex]);
		snsf[tagIndex++] = '\xa';
	}

	/* ゲームタイトル */
	if(0 != strcmp("", mml->spcGame))
	{
		sprintf((char*)&snsf[tagIndex], "game=%s", mml->spcGame);
		tagIndex += strlen((char*)&snsf[tagIndex]);
		snsf[tagIndex++] = '\xa';
	}

	/* 年 */
	sprintf((char*)&snsf[tagIndex], "year=");
	tagIndex += strlen((char*)&snsf[tagIndex]);
	setYear((char*)&snsf[tagIndex]);
	tagIndex += strlen((char*)&snsf[tagIndex]);
	snsf[tagIndex++] = '\xa';

	/* コメント */
	if(0 != strcmp("", mml->spcComment))
	{
		sprintf((char*)&snsf[tagIndex], "comment=%s", mml->spcComment);
		tagIndex += strlen((char*)&snsf[tagIndex]);
		snsf[tagIndex++] = '\xa';
	}

	/* 作成者 */
	if(0 != strcmp("", mml->spcDumper))
	{
		sprintf((char*)&snsf[tagIndex], "snsfby=%s", mml->spcDumper);
		tagIndex += strlen((char*)&snsf[tagIndex]);
		snsf[tagIndex++] = '\xa';
	}

	/* 再生時間 */
	sprintf((char*)&snsf[tagIndex], "length=%d", mml->playingTime);
	tagIndex += strlen((char*)&snsf[tagIndex]);
	snsf[tagIndex++] = '\xa';

	/* フェード時間 */
	sprintf((char*)&snsf[tagIndex], "fade=%d.%d", mml->fadeTime/1000, mml->fadeTime%1000);
	tagIndex += strlen((char*)&snsf[tagIndex]);
	/* snsf[tagIndex++] = '\xa'; */

	/* snsfデータサイズ(タグ書き込み先末端)を返す */
	return tagIndex;
}

