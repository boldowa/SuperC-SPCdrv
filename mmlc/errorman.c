/**
 * エラー管理
 */

#include "gstdafx.h"
#include "errorman.h"
#include "timefunc.h"

/**
 * メモリエラー
 */
static ErrorNode memerr = {0, 0, MemoryError, ERR_FATAL, "", "Memory capacity over.", NULL};
ErrorNode* getmemerror()
{
	setDateStr(memerr.date);
	return &memerr;
}

/**
 * エラー追加
 */
void erroradd(ErrorNode* const err, ErrorNode* dest)
{
	ErrorNode* ptr;

	/* エラーを追加します */
	ptr = dest;
	while(ptr->nextError != NULL)
	{
		ptr = ptr->nextError;
	}
	ptr->nextError = err;

	return;
}

/**
 * エラークリア
 */
void errorclear(ErrorNode* err)
{
	/**
	 * 現在のエラー
	 */
	ErrorNode* curErr = NULL;

	/**
	 * 次のエラー
	 */
	ErrorNode* nextErr = NULL;

	curErr = err;
	while(curErr != NULL)
	{
		nextErr = curErr->nextError;
		if(nextErr != &memerr)
		{
			free(curErr);
		}

		curErr = nextErr;
	}

	return;
}

/**
 * エラー作成
 * return : 作成結果 - NULL     : 作成失敗(メモリ不足)
 *                     NULL以外 : 作成成功
 */
ErrorNode* createError(const MmlMan* const mml)
{
	ErrorNode* e = NULL;

	e = (ErrorNode*)malloc(sizeof(ErrorNode));
	if(e == NULL)
	{
		return NULL;
	}

	e->line = mml->line;
	e->column = mml->column;
	e->nextError = NULL;
	setDateStr(e->date);

	return e;
}

/**
 * エラーレベルを取得します
 */
ErrorLevel getErrorLevel(ErrorNode* err)
{
	ErrorNode* curErr;
	ErrorLevel curLevel = ERR_DEBUG;

	curErr = err;
	while(curErr != NULL)
	{
		if(curErr->level > curLevel)
		{
			curLevel = curErr->level;
		}

		curErr = curErr->nextError;
	}

	return curLevel;
}

