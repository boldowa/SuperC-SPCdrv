/**
 * MML データ管理
 *
 */
#ifndef _MMLMAN_H
#define _MMLMAN_H

/**
 * 定数宣言
 */

#define MML_BUFFER_SIZE 1024
#define MACRO_NAME_MAX  64
#define MACRO_DATA_MAX  256

/**
 * 変数宣言
 */

/**
 * 構造体宣言
 */
/* マクロデータ */
typedef struct stMacro
{
	char name[MACRO_NAME_MAX];
	char data[MACRO_DATA_MAX];
	int  macroinx;
	/* macro実行時参照 */
	struct stMacro* parrent;
	struct stMacro* child;
	/* macro追加時参照 */
	struct stMacro* next;
} Macro;

/* MML管理 */
typedef struct stMmlMan
{
	/**
	 * mmlファイルポインタ
	 * ※直接更新(ex. fopen(mml->fp)) は、
	 *   mmlman.c以外禁止
	 */
	FILE *fp;

	/**
	 * ファイル名
	 */
	char fname[260];

	/**
	 * mml読み込みバッファ
	 * ※直接読出し(ex. mml->readbuff[foo]) は、
	 *   mmlman.c以外禁止
	 */
	char readbuff[MML_BUFFER_SIZE];
	/**
	 * mml読み込みバッファインデックス
	 * ※直接更新(ex. mml->buffInx=0) は、
	 *   mmlman.c以外禁止
	 */
	int buffInx;
	/**
         * 開放処理要フラグ
	 * ※直接更新(ex. mml->reqfree = false) は、
	 *   mmlman.c以外禁止
	 */
	bool reqfree;

	/**
	 * ファイル末尾かどうか
	 */
	bool iseof;

	/**
	 * 改行されたかどうか
	 */
	bool isnewline;

	/**
	 * 行
	 */
	int line;

	/**
	 * 列
	 */
	int column;

	/**
	 * 最初の文字の位置
	 */
	int firstcharcol;

	/**
	 * MMLマクロ 追加用
	 */
	Macro* macroRoot;
	Macro* macroTail;
	/**
	 * MMLマクロ 実行用
	 */
	Macro* macroExecRoot;
	Macro* macroExecCur;
}MmlMan;

/**
 * 関数宣言
 */
bool mmlopen(MmlMan*, char*);
void mmlclose(MmlMan*);
char mmlgetch(MmlMan*);
char mmlgetforward(MmlMan*);
char mmlgetusp(MmlMan*);
bool mmlcmpstr(MmlMan*, const char* const, const int);
bool isFirstChar(MmlMan*);
void skipChars(MmlMan*, int);
void skipspaces(MmlMan*);
bool addmacro(MmlMan*);

#endif /* _MMLMAN_H */
