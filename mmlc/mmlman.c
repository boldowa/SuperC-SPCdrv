/**
 * MML データ管理
 *
 */

#include "gstdafx.h"
#include "mmlman.h"
#include "pathfunc.h"

/**
 * ファイルサイズ取得
 *    fp : サイズ計算対象ファイルのポインタ
 */
int fsize(FILE *fp)
{
	int defp, sz;
	defp = ftell(fp);		/* 初期位置の記憶(ファイル位置を戻すのに使用) */

	fseek(fp, 0, SEEK_END);		/* ファイルサイズをファイル末端位置から求める */
	sz = ftell(fp);

	fseek(fp, defp, SEEK_SET);	/* ファイル位置を元に戻す */
	return sz;
}

/**
 * mmlファイルオープン
 *   fname  : 読み込みファイル名
 *   return : 読み込み結果
 *              true  : 読み込み成功
 *              false : 読み込み失敗
 */
bool mmlopen(MmlMan* mml, char* fname)
{
	if(mml == NULL)
	{
		/* NULLポインタが渡された場合は、動的メモリ確保を行います */
		mml = (MmlMan*)malloc(sizeof(MmlMan));
		if(mml == NULL)
		{
			return false;
		}
		mml->reqfree = true;
	}
	else
	{
		/* 確保済みメモリが渡された場合は、メモリ解放不要 */
		mml->reqfree = false;
	}

	/* mmlファイルを開きます */
	mml->fp = fopen(fname, "r");
	if(mml->fp == NULL)
	{
		/* 開くのに失敗した場合は、メモリを解放して終了します */
		if(mml->reqfree == true)
		{
			free(mml);
		}
		return false;
	}

	/* 変数を初期化します */
	mml->iseof = false;
	mml->firstcharcol = 0;
	mml->buffInx = 0;
	mml->macroRoot = NULL;
	mml->macroTail = NULL;
	mml->macroExecRoot = NULL;
	mml->macroExecCur = NULL;
	mml->maxEDL = 0;

	mml->line = 0;
	mml->column = 0;

	memset(mml->spcTitle, '\0', 32);
	memset(mml->spcGame, '\0', 32);
	memset(mml->spcComposer, '\0', 32);
	memset(mml->spcDumper, '\0', 16);
	memset(mml->spcComment, '\0', 32);

	mml->playingTime = 300;
	mml->fadeTime = 10000;

	/* ファイルパスを取得します */
	strcpy(mml->fname, fname);
	getFileDir(mml->fdir, fname);

	/* 読み込みバッファをクリアします */
	memset(mml->readbuff, '\0', MML_BUFFER_SIZE);

	/* 読み込みファイルサイズが0バイトのときは、ファイル末端フラグを立てます */
	if(fsize(mml->fp) == 0)
	{
		mml->iseof = true;
	}

	return true;
}

/**
 * mmlファイルクローズ
 *
 */
void mmlclose(MmlMan* mml)
{
	if(mml != NULL)
	{
		Macro* mac;
		Macro* next;

		/* マクロ(辞書)削除 */
		mac = mml->macroRoot;
		while(NULL != mac)
		{
			next = mac->next;
			free(mac);
			mac = next;
		}

		/* マクロ(実行オブジェクト)削除 */
		mac = mml->macroExecRoot;
		while(NULL != mac)
		{
			next = mac->child;
			free(mac);
			mac = next;
		}

		/* ファイルを閉じます */
		if(mml->fp != NULL)
		{
			fclose(mml->fp);
		}

		/* メモリ解放が必要な場合は、メモリを解放します */
		if(mml->reqfree == true)
		{
			free(mml);
		}
	}
}

/**
 * mmlファイルから1文字読み込む
 */
char mmlgetch(MmlMan* mml)
{
	if(mml == NULL)
	{
		return '\0';
	}

	if(NULL != mml->macroExecCur)
	{
		if(strlen(mml->macroExecCur->data) > mml->macroExecCur->macroinx)
		{
			return mml->macroExecCur->data[mml->macroExecCur->macroinx];
		}

		mml->macroExecCur = mml->macroExecCur->parrent;
		if(NULL != mml->macroExecCur)
		{
			free(mml->macroExecCur->child);
			mml->macroExecCur->child = NULL;
			return mmlgetch(mml);
		}

		/* 実行マクロなし */
		free(mml->macroExecRoot);
		mml->macroExecRoot = NULL;
	}

	return mml->readbuff[mml->buffInx];
}


/**
 * mmlファイルを1文字読み進めます
 *   引数を無視する場合、単純に1文字進める用途で使用できます
 */
char mmlgetforward(MmlMan* mml)
{
	char ch;
	char* readaddr;

	if(mml == NULL)
	{
		return '\0';
	}

	if(NULL != mml->macroExecCur)
	{
		if(strlen(mml->macroExecCur->data) > mml->macroExecCur->macroinx)
		{
			return mml->macroExecCur->data[mml->macroExecCur->macroinx++];
		}

		mml->macroExecCur = mml->macroExecCur->parrent;
		if(NULL != mml->macroExecCur)
		{
			free(mml->macroExecCur->child);
			mml->macroExecCur->child = NULL;
			return mmlgetforward(mml);
		}

		/* 実行マクロなし */
		free(mml->macroExecRoot);
		mml->macroExecRoot = NULL;
	}

	mml->isnewline = mml->iseof = false;
	ch = mml->readbuff[mml->buffInx++];

	if(ch == '\0' || ch == '\n')
	{
		if(ch == '\n')
		{
			mml->isnewline = true;
			mml->column = 0;
		}
		readaddr = fgets(mml->readbuff, MML_BUFFER_SIZE, mml->fp);
		if(readaddr == NULL)
		{
			mml->iseof = true;
			return '\0';
		}
		mml->line++;
		mml->buffInx = 0;
	}
	mml->column++;
	mml->firstcharcol = mml->column;

	return ch;
}

/**
 * スペース以外になるまで読み込む
 * ( mml get until space )
 */
char mmlgetusp(MmlMan* mml)
{
	char ch = ' ';

	while( (    (ch == '\t')
	         || (ch == ' ' )))
	{
		ch = mmlgetforward(mml);
	}
	if(mml->isnewline == true)
	{
		mml->firstcharcol = mml->column+1;
	}

	return ch;
}

/**
 * 文字比較
 *  比較対象文字列と一致した場合、一致した分だけmmlを読み進める
 */
bool mmlcmpstr(MmlMan* mml, const char* const str, const int length)
{
	if(strncmp(&mml->readbuff[mml->buffInx], str, length) == 0)
	{
		mml->buffInx += length;
		mml->column += length;
		return true;
	}

	return false;
}

/**
 * 行内で最初の(空白以外の)文字かどうか
 */
bool isFirstChar(MmlMan* mml)
{
	if(mml->column != mml->firstcharcol)
	{
		return false;
	}

	return true;
}

void skipchars(MmlMan* mml, int skips)
{
	mml->buffInx += skips;
}

void skipspaces(MmlMan* mml)
{
	char ch;
	ch = mmlgetch(mml);
	while( (    (ch == '\t')
	         || (ch == ' ' )))
	{
		mmlgetforward(mml);
		ch = mmlgetch(mml);
	}
}

/**
 * マクロの定義(#macro 構文の定義)を追加します
 */
bool addmacro(MmlMan* mml)
{
	Macro* mac;
	Macro* mSearch;
	int inx = 0;
	char ch = ' ';

	/* マクロデータの作成 */
	mac = (Macro*)malloc(sizeof(Macro));
	if(mac == NULL)
	{
		return false;
	}

	/* メモリ初期化 */
	memset(mac, 0, sizeof(Macro));

	/* スペース読み飛ばし */
	while( (    (ch == '\t')
	         || (ch == ' ' )))
	{
		mml->buffInx++;
		ch = mmlgetch(mml);
	}

	/* マクロ名取得 */
	inx = 0;
	while( !(   (ch == '\t')
	         || (ch == ' ' )))
	{
		mac->name[inx++] = ch;
		mml->buffInx++;
		ch = mmlgetch(mml);
	}
	mac->name[inx] = '\0';

	/* スペース読み飛ばし */
	while( (    (ch == '\t')
	         || (ch == ' ' )))
	{
		mml->buffInx++;
		ch = mmlgetch(mml);
	}

	/* 行末までのデータのコピー */
	inx = 0;
	while( !(    (ch == '\n')
	          || (ch == '\0')))
	{
		mac->data[inx++] = ch;
		mml->buffInx++;
		ch = mmlgetch(mml);
	}
	mac->data[inx] = '\0';

	/* データが取れているかチェック */
	if((0 == strlen(mac->name)) || (0 == strlen(mac->data)))
	{
		/* マクロ名・マクロデータいずれか空 */
		free(mac);
		return false;
	}

	/* 初回追加 */
	if(NULL == mml->macroTail)
	{
		mml->macroRoot = mml->macroTail = mac;
		return true;
	}

	/* 重複チェック */
	mSearch = mml->macroRoot;
	while(NULL != mSearch)
	{
		if(0 == strcmp(mSearch->name, mac->name))
		{
			/* 重複あり */
			free(mac);
			return false;
		}
		mSearch = mSearch->next;
	}
	mml->macroTail->next = mac;
	mml->macroTail = mac;

	return true;
}

