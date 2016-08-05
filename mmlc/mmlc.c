/**
 * mml compiler for SuperC main program
 *
 */

#include "gstdafx.h"
#include "mmlc.h"
#include "compile.h"
#include "mmlman.h"
#include "binaryman.h"
#include "errorman.h"
#include "timefunc.h"

#define BRR_BUF 2048

/************************************************************/
/* オプション関連定義                                       */
/************************************************************/
enum{
	OPT_Debug = 1,
	OPT_Help,
	OPT_Version,
};
static const Option Opt[] = {
	{OPT_Debug, "debug", 'd', "verbose debug info"},
	{OPT_Help, "help", '?', "show usage"},
	{OPT_Version, "version", 'v', "show version"},
	{0, NULL, 0, NULL}
};

bool helped = false;
bool vdebug = false; /* TODO: リリース時はfalseにする */
char* in_file_name = NULL;
char* out_file_name = NULL;

/************************************************************/
/* SPC出力用定義                                            */
/************************************************************/
typedef struct {
	char headerInfo[33];
	unsigned char cutHeaderAndData[2];
	unsigned char tagType[1];
	unsigned char tagVersion[1];
	unsigned char pc[2];
	unsigned char a[1];
	unsigned char x[1];
	unsigned char y[1];
	unsigned char psw[1];
	unsigned char sp[1];
	unsigned char reserved[2];
	char songTitle[32];
	char gameTitle[32];
	char dumper[16];
	char comment[32];
	unsigned char musicInfo[19];		/* Not used, and lazy :p */
	char composer[32];
	unsigned char defaultChannelDisable[1];
	unsigned char emulator[1];
	unsigned char unused[45];
} stID666_Text;
typedef struct {
	char DirTxt[4];		/* DIR */
	byte dir[2];
	char TableTxt[4];	/* TBL */
	byte locTable[2];
	char CodeTxt[5];	/* CODE */
	byte codeStart[2];
} stSuperCHead;


/******************************************************************************/


/**************************************************************/
/* バージョン情報出力                                         */
/**************************************************************/
#define printVersion() \
	printf("mmlc for SuperC SPC Driver v%s\n", MMLCONV_VERSION);\
	printf("  by boldowa\n");\
	printf("  since    : Sep 02 2010\n");\
	printf("  compiled : %s\n", __DATE__);\


/**************************************************************/
/* 使い方表示                                                 */
/*   program : プログラム名                                   */
/**************************************************************/
void printUsage(const char* program)
{
	const Option *op;
	printf("Usage: %s [options] input-file output-file\n", program);
	printf("Options:\n");
	for(op = Opt; op->id != 0; op++)
	{
		printf("  -%c, --%-14s %.45s\n", op->sname, op->name, op->description);
	}
}

/**************************************************************/
/* オプション展開                                             */
/*   argc : オプション引数個数                                */
/*   argv : オプション引数                                    */
/**************************************************************/
bool parseOption(const int argc, const char **argv)
{
	int ind;
	bool err = false;
	const Option *op;
	int op_id;
	char *cmd = NULL;

	ind=1;
	while(ind<argc)
	{
		op_id = 0;
		cmd = (char*)argv[ind++];
		if(cmd[0] == '-')
		{
			for(op = Opt; op->id != 0; op++)
			{
				if(cmd[1] == '-')
				{
					if(!strcmp(cmd+2, op->name))
						op_id = op->id;
				}
				else
				{
					if(cmd[1] == op->sname && cmd[2] == '\0')
						op_id = op->id;
				}
			} /* op loop */
			if(!op_id){
				puterror("Invalid command %s", cmd);
				err = true;
			}
			switch(op_id)
			{
			case OPT_Help:
				printUsage(argv[0]);
				helped = true;
				return true;
			case OPT_Version:
				printVersion();
				helped = true;
				return true;
			case OPT_Debug:
				vdebug = true;
			default:
				break;
			}
		}
		else
		{
			if(!in_file_name)
			{
				in_file_name = cmd;
			}
			else if(!out_file_name)
			{
				out_file_name = cmd;
			}
			else
			{
				puts("Error: too many argument.");
				err = true;
			}
		}
	} /* argc loop */
	if(!in_file_name)
	{
		puterror("Missing input-file argument.");
		err = true;
	}
	if(!out_file_name)
	{
		puterror("Missing output-file argument.");
		err = true;
	}
	if(err)
	{
		puts("Please try '-?' or '--help' option, and you can get more information.");
		return false;
	}
	putdebug("--- Option parse complete.");
	putdebug("--- Input file  : \"%s\"", in_file_name);
	putdebug("--- Output file : \"%s\"", out_file_name);
	return true;
}

/**
 * コンパイルエラーを表示します
 */
void showCompileError(ErrorNode* err, char* fname)
{
	/**
	 * 現在出力中のエラー
	 */
	ErrorNode* curErr = NULL;

	curErr = err;
	while(curErr != NULL)
	{
		/* エラーレベル毎に出力フォーマットを変更して */
		/* エラーを表示します                         */
		switch(curErr->level)
		{
			case ERR_DEBUG:
				putmmldebug(curErr->date, fname, curErr->line, curErr->column, curErr->message);
				break;
			case ERR_INFO:
				putmmlinfo(curErr->date, fname, curErr->line, curErr->column, curErr->message);
				break;
			case ERR_WARN:
				putmmlwarn(curErr->date, fname, curErr->line, curErr->column, curErr->message);
				break;
			case ERR_ERROR:
				putmmlerror(curErr->date, fname, curErr->line, curErr->column, curErr->message);
				break;
			case ERR_FATAL:
				putmmlfatal(curErr->date, fname, curErr->line, curErr->column, curErr->message);
				break;
			default:
				putmmlunknown(curErr->date, fname, curErr->line, curErr->column, curErr->message);
				break;
		}

		/* 次のエラーを取得します */
		curErr = curErr->nextError;
	}

	return;
}

/**************************************************************/
/* メイン関数                                                 */
/*   argc : オプション引数個数                                */
/*   argv : オプション引数                                    */
/**************************************************************/
int main(const int argc, const char** argv)
{
	FILE *outf = NULL;
	FILE *corefile = NULL;
	FILE *brrFile = NULL;
	ErrorNode* errList = NULL;
	ErrorLevel errLevel = ERR_DEBUG;
	MmlMan mml;
	BinMan binary;
	TIME startTime, endTime;
	stBrrListData *brrList = NULL;
	stBrrListData *blread = NULL;
	byte* spcCore;
	int coresize;
	int spccodesize;
	byte spcBuffer[0x10100];
	byte brrBuffer[BRR_BUF];
	int readLen;
	stSuperCHead* spcdrvhead;
	stID666_Text* id666;
	int spcWritePtr;

	getTime(&startTime);

	/* 初期化 */
	memset(&mml, 0, sizeof(MmlMan));
	memset(&binary, 0, sizeof(BinMan));
	memset(spcBuffer, 0, 0x10100);

	/* オプション展開 */
	if(!parseOption(argc, argv))
	{
		return -1;
	}
	if(helped) /* Show Usage && Show version */
	{
		return 0;
	}

	/* SPCドライバコアファイルを読み込みます */
	corefile = fopen("spccore.bin", "rb");
	if(NULL == corefile)
	{
		putfatal("Can't open \"spccore.bin\".");
		return -1;
	}
	coresize = fsize(corefile);
	spcCore = (byte*)malloc(coresize);
	if(NULL == spcCore)
	{
		putfatal("Can't open \"spccore.bin\". (memory allocate error)");
		fclose(corefile);
		return -1;
	}
	fread(spcCore, sizeof(byte), coresize, corefile);
	fclose(corefile);

	/* mmlファイルを読出します */
	if(mmlopen(&mml, in_file_name) == false)
	{
		puterror("Input file \"%s\" is not found.", in_file_name);
		free(spcCore);
		return -1;
	}
	putdebug("--- MML read success.");

	/* 変換バッファを作成します */
	if(bininit(&binary) == false)
	{
		mmlclose(&mml);
		free(spcCore);
		puterror("Memory capacity over.");
		return -1;
	}
	putdebug("--- Create compile buffer success.");

	/* 出力先ファイルをあらかじめロックします */
	if(!(outf = fopen(out_file_name, "wb")))
	{
		binfree(&binary);
		mmlclose(&mml);
		free(spcCore);
		puterror("Output file \"%s\" couldn't open.", out_file_name);
		return -1;
	}
	putdebug("--- Output file create success.");

	/* mmlをコンパイルします */
	putdebug("########## COMPILE START ##########");
	errList = compile(&mml, &binary, &brrList);
	if(errList != NULL)
	{
		/* エラーレベルを取得します */
		errLevel = getErrorLevel(errList);

		/* エラーを表示します */
		showCompileError(errList, mml.fname);

		/* エラー表示用に確保したメモリを解放します */
		errorclear(errList);
	}
	if(errLevel >= ERR_ERROR)
	{ /* コンパイルエラー */
		putdebug("########## COMPILE ERROR ##########");
		puts("Compile failed...");
		deleteBrrListData(brrList);
		mmlclose(&mml);
		binfree(&binary);
		fclose(outf);
		free(spcCore);
		remove(out_file_name);
		return -1;
	}

	/* SPCデータを作成する準備をします */
	id666 = (stID666_Text*)&spcBuffer[0];
	spcdrvhead = (stSuperCHead*)&spcCore[0];
	spccodesize = coresize - sizeof(stSuperCHead);

	/* データチェック */
	if(0 != strcmp(spcdrvhead->DirTxt, "DIR"))
	{
		putfatal("Dir not match");
		deleteBrrListData(brrList);
		mmlclose(&mml);
		binfree(&binary);
		fclose(outf);
		free(spcCore);
		remove(out_file_name);
		return -1;
	}
	if(0 != strcmp(spcdrvhead->TableTxt, "TBL"))
	{
		putfatal("Table not match");
		deleteBrrListData(brrList);
		mmlclose(&mml);
		binfree(&binary);
		fclose(outf);
		free(spcCore);
		remove(out_file_name);
		return -1;
	}
	if(0 != strcmp(spcdrvhead->CodeTxt, "CODE"))
	{
		putfatal("Code not match");
		deleteBrrListData(brrList);
		mmlclose(&mml);
		binfree(&binary);
		fclose(outf);
		free(spcCore);
		remove(out_file_name);
		return -1;
	}

	printf("DIR : 0x%x\n", *(word*)spcdrvhead->dir);
	printf("TBL : 0x%x\n", *(word*)spcdrvhead->locTable);
	printf("CODE START : 0x%x\n", *(word*)&spcdrvhead->codeStart);
	printf("CODESIZE : 0x%x\n", spccodesize);

	/* ID666 ヘッダを書き込みます */
	memcpy(id666->headerInfo, "SNES-SPC700 Sound File Data v0.30", 33);
	memcpy(id666->cutHeaderAndData, "\x1a\x1a", 2);
	memcpy(id666->tagType, "\x1a", 1);
	memcpy(id666->tagVersion, "\x1e", 1);
	*(word *)id666->pc = *(word *)&spcdrvhead->codeStart;
	/* DSPレジスタ情報、DIR情報がないと黒猫SPCの表示がヘンになる */
	spcBuffer[0x1005d] = spcdrvhead->dir[1];

	/* SPCコアデータを、SPCバッファにコピーします */
	memcpy(&spcBuffer[0x100 + *(word *)&spcdrvhead->codeStart], &spcCore[sizeof(stSuperCHead)], spccodesize);

	/* BRR、およびシーケンスを書き込むための準備 */
	spcWritePtr = *(word *)&spcdrvhead->codeStart + spccodesize;

	/* BRRデータの読み出し */
	blread = brrList;
	while(blread != NULL)
	{
		int brrsize;
		word brrHead;

		brrFile = fopen(brrList->fname, "rb");
		if(brrFile == NULL)
		{
			deleteBrrListData(brrList);
			mmlclose(&mml);
			binfree(&binary);
			free(spcCore);
			puterror("Make SPC: BRR file read error (%s).", brrList->fname);
			return -1;
		}

		brrsize = fsize(brrFile);

		/* ファイルサイズ0 = ダミーファイルなので、処理とばす */
		if( 0 == brrsize )
		{
			fclose(brrFile);
			continue;
		}

		if( brrsize < 11 || (0 != ((brrsize -2) % 9)) )
		{
			deleteBrrListData(brrList);
			mmlclose(&mml);
			binfree(&binary);
			free(spcCore);
			fclose(brrFile);
			puterror("Make SPC: BRR file size error (%s).", brrList->fname);
			return -1;
		}

		/* DIRテーブル情報の書き出し */
		fread(&brrHead, sizeof(word), 1, brrFile);
		brrHead += spcWritePtr;
		printf("BRR %s : 0x%x bytes @0x%x\n", blread->fname, brrsize-2, spcWritePtr);
		*(word*)&spcBuffer[0x100 +  *(word*)spcdrvhead->dir + (blread->brrInx*4)] = spcWritePtr;
		*(word*)&spcBuffer[0x100 +  *(word*)spcdrvhead->dir + (blread->brrInx*4) +2] = brrHead;

		/* SPCバッファに追加 */
		while(0 != (readLen = fread(brrBuffer, sizeof(byte), BRR_BUF, brrFile)))
		{
			memcpy(&spcBuffer[spcWritePtr+0x100], brrBuffer, readLen);
			spcWritePtr += readLen;
		}

		fclose(brrFile);

		blread = brrList->next;
	}

	/* シーケンスデータをコピーする */
	printf("SEQ : 0x%x\n", spcWritePtr);
	*(word *)&spcBuffer[0x100 + *(word*)&spcdrvhead->locTable] = spcWritePtr;
	memcpy(&spcBuffer[0x100 + spcWritePtr], binary.data, binary.dataInx);

	/* binaryデータを書き出します */
	/* fwrite(binary.data, sizeof(byte), binary.dataInx, outf); */
	fwrite(spcBuffer, sizeof(byte), 0x10100, outf);
	fclose(outf);

	/* 終了処理 */
	deleteBrrListData(brrList);
	mmlclose(&mml);
	binfree(&binary);
	free(spcCore);
	putdebug("########## COMPILE SUCCESS ##########");

	getTime(&endTime);
	printf("Complete. Compiled in %.3f seconds.\n", getTimeToPass(&startTime, &endTime));

	return 0;
}

