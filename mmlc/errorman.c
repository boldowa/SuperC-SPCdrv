/**
 * エラー管理
 */

#include "gstdafx.h"
#include "errorman.h"

/**
 * メモリエラー
 */
static ErrorNode memerr = {0, 0, MemoryError, ERR_FATAL, "", "Memory capacity over.", NULL};

#ifdef _WIN32
/**
 * 現在日時の取得
 * (Windows向け)
 */
void setDate(char* sDate)
{
	SYSTEMTIME tm;
	GetLocalTime(&tm);

	sprintf(sDate, "%hu/%02hu/%02hu %02hu:%02hu:%02hu.%03hu",
		tm.wYear,
		tm.wMonth,
		tm.wDay,
		tm.wHour,
		tm.wMinute,
		tm.wSecond,
		tm.wMilliseconds
	       );
}
#else
/**
 * 現在日時の取得
 * (Linux向け)
 */
void setDate(char* sDate)
{
	struct timeval time;
	struct tm* time_st;

	gettimeofday(&time, NULL);
	time_st = localtime(&time.tv_sec);

	sprintf(sDate, "%d/%02d/%02d %02d:%02d:%02d.%06d",
	        time_st->tm_year+1900,
		time_st->tm_mon+1,
		time_st->tm_mday,
		time_st->tm_hour,
		time_st->tm_min,
		time_st->tm_sec,
		(int)time.tv_usec
		);
}
#endif

ErrorNode* getmemerror()
{
	setDate(memerr.date);
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
	setDate(e->date);

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

