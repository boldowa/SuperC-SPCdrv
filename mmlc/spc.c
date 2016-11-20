/**
 * SPC関連処理
 */
#include "gstdafx.h"
#include "mmlman.h"
#include "binaryman.h"
#include "compile.h"
#include "timefunc.h"
#include "spc.h"

/**
 * SPCコアファイルの読み出し
 */
bool coreread(stSpcCore* core, char* fname)
{
	FILE *fp;
	char fpref[4];	/* データ識別子 */

	if(NULL == core)
	{
		core = (stSpcCore*)malloc(sizeof(stSpcCore));
		if(NULL == core)
		{
			return false;
		}
		core->reqfree = true;
	}
	else
	{
		/* 確保済みメモリが渡された場合は、メモリ解放不要 */
		core->reqfree = false;
	}

	/* データを初期化する */
	core->data = NULL;
	core->size = 0;
	core-> ver = 0;
	core->verMiner = 0;
	core->seqBasePoint = 0;
	core->dir = 0;
	core->esaLoc = 0;
	core->location = 0;

	/* SPCコアファイルを開きます */
	fp = fopen(fname, "rb");
	if(NULL == fp)
	{
		if(true == core->reqfree)
		{
			free(core);
		}
		return false;
	}

	while(true)
	{
		int readLen;

		/* データ識別子読み込み */
		readLen = fread(fpref, sizeof(char), 4, fp);
		if(0 == readLen)
		{
			break;
		}

		if(0 == memcmp(fpref, "VER ", 4))
		{
			/* バージョン情報 */
			readLen = fread(&core->ver, sizeof(byte), 1, fp);
			if(1 != readLen)
			{
				break;
			}
			readLen = fread(&core->verMiner, sizeof(byte), 1, fp);
			if(1 != readLen)
			{
				break;
			}
		}
		else if(0 == memcmp(fpref, "DIR ", 4))
		{
			/* DIR情報 */
			readLen = fread(&core->dir, sizeof(word), 1, fp);
			if(1 != readLen)
			{
				break;
			}
		}
		else if(0 == memcmp(fpref, "ESA ", 4))
		{
			/* ESA情報 */
			readLen = fread(&core->esaLoc, sizeof(word), 1, fp);
			if(1 != readLen)
			{
				break;
			}
		}
		else if(0 == memcmp(fpref, "TBL ", 4))
		{
			/* シーケンスデータアドレスの場所 */
			readLen = fread(&core->seqBasePoint, sizeof(word), 1, fp);
			if(1 != readLen)
			{
				break;
			}
		}
		else if(0 == memcmp(fpref, "LOC ", 4))
		{
			/* SPCコアの読み込み先 */
			readLen = fread(&core->location, sizeof(word), 1, fp);
			if(1 != readLen)
			{
				break;
			}
		}
		else if(0 == memcmp(fpref, "DATA", 4))
		{
			/* SPCコアデータ, ファイル末尾までデータを読み出す */

			size_t pt;
			int sz;

			/* 読み込みサイズ取得 */
			pt = ftell(fp);
			fseek(fp, 0, SEEK_END);
			sz = ftell(fp) - pt;
			fseek(fp, pt, SEEK_SET);
			if(0 == sz)
			{
				break;
			}

			/* コアデータのメモリ確保 */
			core->data = (byte*)malloc(sz);
			if(NULL == core->data)
			{
				break;
			}

			/* コアデータ読み出し */
			readLen = fread(core->data, sizeof(byte), sz, fp);
			if(sz != readLen)
			{
				free(core->data);
				core->data = NULL;
				break;
			}
			core->size = sz;
		}
		else
		{
			/* 不正なデータ識別子 */
			break;
		}
	}

	/* コアファイルを閉じる */
	fclose(fp);

	if(NULL == core->data)
	{
		if(true == core->reqfree)
		{
			free(core);
		}
		return false;
	}

	return true;
}

/**
 * SPCコアのメモリ解放
 */
void freecore(stSpcCore* core)
{
	if(NULL != core)
	{
		free(core->data);

		if(true == core->reqfree)
		{
			free(core);
		}
	}
}

/**
 * SPCデータの生成
 */
bool makeSPC(byte* spc, stSpcCore* core, MmlMan *mml, BinMan* seq, stBrrListData* blist)
{
	stID666_Text* id666;
	int coreEnd;
	int aramWritePtr;
	int freeSize;
	int esa;
	stBrrListData* blread;
	FILE* brr;
	byte brrBuf[1024];
	char tmpstr[6];

	/* ID666情報を生成します */
	id666 = (stID666_Text*)spc;
	memset(id666, 0, sizeof(stID666_Text));
	memcpy(id666->headerInfo, "SNES-SPC700 Sound File Data v0.30", 33);
	memcpy(id666->cutHeaderAndData, "\x1a\x1a", 2);
	memcpy(id666->tagType, "\x1a", 1);
	memcpy(id666->tagVersion, "\x1e", 1);
	*(word *)id666->pc = core->location;
	if(0 == strcmp("", mml->spcTitle))
	{
		memcpy(id666->songTitle, mml->fname, 32);
	}
	else
	{
		memcpy(id666->songTitle, mml->spcTitle, 32);
	}
	memcpy(id666->gameTitle, mml->spcGame, 32);
	memcpy(id666->composer, mml->spcComposer, 32);
	memcpy(id666->dumper, mml->spcDumper, 16);
	memcpy(id666->comment, mml->spcComment, 32);
	setDateStrForSPC(id666->date);
	id666->defaultChannelDisable = '0';
	id666->emulator = '0';
	memset(tmpstr, 0, 6);
	sprintf(tmpstr, "%03d", mml->playingTime);
	memcpy(id666->musicLength, tmpstr, 3);
	memset(tmpstr, 0, 6);
	sprintf(tmpstr, "%05d", mml->fadeTime);
	memcpy(id666->fadeTime, tmpstr, 5);

	/* SPCコアデータをセットします */
	memcpy(&spc[0x100 + core->location], core->data, core->size);
	coreEnd = core->location + core->size;

	/* ESAを決定します */
	esa = coreEnd & 0xff00;
	if(0 != (coreEnd & 0xff))
	{
		esa += 0x100;
	}
	spc[0x100 + core->esaLoc] = (esa >> 8);

	/* データ書き込み先を決定します */
	aramWritePtr = esa + (mml->maxEDL * 0x800);
	if(0 == mml->maxEDL)
	{
		aramWritePtr = esa + 4;
	}

	/* BRRデータをセットします */
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
			puterror("makeSPC: BRR file size error (%s).", blread->fname);
			return false;
		}
		if(0x10000 <= ((brrsize-2)+aramWritePtr))
		{
			puterror("makeSPC: Insert data size is too big.");
			return false;
		}

		/* BRRループヘッダ読み出し */
		fread(&brrLoop, sizeof(word), 1, brr);
		brrLoop += aramWritePtr;

		/* DIRテーブルの書き出し */
		*(word*)&spc[0x100 + core->dir + (blread->brrInx*4)] = aramWritePtr;
		*(word*)&spc[0x100 + core->dir + (blread->brrInx*4) + 2] = brrLoop;
		/* BRRデータの追加 */
		while(0 != (readLen = fread(brrBuf, sizeof(byte), 1024, brr)))
		{
			memcpy(&spc[0x100+aramWritePtr], brrBuf, readLen);
			aramWritePtr += readLen;
		}

		fclose(brr);
		blread = blread->next;
	}

	/* シーケンスデータをセットします */
	if(0x10000 <= (seq->dataInx+aramWritePtr))
	{
		puterror("makeSPC: Insert data size is too big.");
		return false;
	}
	putdebug("seqStart: 0x%x", aramWritePtr);
	*(word*)&spc[0x100 + core->seqBasePoint] = aramWritePtr;
	memcpy(&spc[0x100 + aramWritePtr], seq->data, seq->dataInx);

	/* DSPレジスタ情報をセットします */
	spc[0x1014c] = 0;			/* KON */
	spc[0x1015c] = 0xff;			/* KOF */
	spc[0x1015d] = (core->dir >> 8);	/* DIR */
	spc[0x1016d] = (esa >> 8);		/* ESA */
	spc[0x1017d] = 0;			/* EDL */

	freeSize = 0x10000 - aramWritePtr;
	printf("Make SPC success.\n");
	printf("  Total data size: %d bytes\n", aramWritePtr);
	printf("  %d bytes(%.2f%%) free.\n", freeSize, (((double)freeSize/0x10000)*100));

	return true;
}

/**
 * SNES link向けの、バイナリデータの生成
 */
int makeBin(byte* bin, stSpcCore* core, MmlMan *mml, BinMan* seq, stBrrListData* blist)
{
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
			puterror("makeBin: BRR file size error (%s).", blread->fname);
			return 0;
		}
		if(0x10000 <= ((brrsize-2)+dataDest))
		{
			puterror("makeBin: Insert data size is too big.");
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
		puterror("makeBin: Insert data size is too big.");
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

	return binInx;
}

