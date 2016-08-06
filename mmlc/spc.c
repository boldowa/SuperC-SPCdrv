/**
 * SPC関連処理
 */
#include "gstdafx.h"
#include "mmlman.h"
#include "binaryman.h"
#include "compile.h"
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

	/* ID666情報を生成します */
	id666 = (stID666_Text*)spc;
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
	memcpy(id666->dumper, mml->spcDumper, 32);
	memcpy(id666->comment, mml->spcComment, 32);

	/* DSPレジスタ情報をセットします */
	spc[0x1015d] = (core->dir >> 8);

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
	*(word*)&spc[0x100 + core->seqBasePoint] = aramWritePtr;
	memcpy(&spc[0x100 + aramWritePtr], seq->data, seq->dataInx);

	freeSize = 0x10000 - aramWritePtr;
	printf("Make SPC success.\n");
	printf("  Total data size: %d bytes\n", aramWritePtr);
	printf("  %d bytes(%.2f%%) free.\n", freeSize, (((double)freeSize/0x10000)*100));

	return true;
}

