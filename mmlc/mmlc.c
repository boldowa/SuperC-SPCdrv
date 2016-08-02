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
	ErrorNode* errList = NULL;
	ErrorLevel errLevel = ERR_DEBUG;
	MmlMan mml;
	BinMan binary;
	TIME startTime, endTime;

	getTime(&startTime);

	memset(&mml, 0, sizeof(MmlMan));
	memset(&binary, 0, sizeof(BinMan));

	if(!parseOption(argc, argv))
	{
		return -1;
	}
	if(helped) /* Show Usage && Show version */
	{
		return 0;
	}

	/* mmlファイルを読出します */
	if(mmlopen(&mml, in_file_name) == false)
	{
		puterror("Input file \"%s\" is not found.", in_file_name);
		return -1;
	}
	putdebug("--- MML read success.");

	/* 変換バッファを作成します */
	if(bininit(&binary) == false)
	{
		mmlclose(&mml);
		puterror("Memory capacity over.");
		return -1;
	}
	putdebug("--- Create compile buffer success.");

	/* 出力先ファイルをあらかじめロックします */
	if(!(outf = fopen(out_file_name, "wb")))
	{
		binfree(&binary);
		mmlclose(&mml);
		puterror("Output file \"%s\" couldn't open.", out_file_name);
		return -1;
	}
	putdebug("--- Output file create success.");

	/* mmlをコンパイルします */
	putdebug("########## COMPILE START ##########");
	errList = compile(&mml, &binary);
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
		mmlclose(&mml);
		binfree(&binary);
		fclose(outf);
		remove(out_file_name);
		return -1;
	}

	/* binaryデータを書き出します */
	fwrite(binary.data, sizeof(byte), binary.dataInx, outf);
	fclose(outf);

	/* 終了処理 */
	mmlclose(&mml);
	binfree(&binary);
	putdebug("########## COMPILE SUCCESS ##########");

	getTime(&endTime);
	printf("Complete. Compiled in %.3f seconds.\n", getTimeToPass(&startTime, &endTime));

	return 0;
}

