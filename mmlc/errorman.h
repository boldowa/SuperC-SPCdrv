/**
 * エラー管理
 */

#ifndef _ERRORMAN_H
#define _ERRORMAN_H

#include "mmlman.h"

typedef enum eErrorType
{
	/**
	 * エラーなし
	 */
	ErrorNone = 0,
	/**
	 * 文法エラー
	 */
	SyntaxError,

	/**
	 * プログラムエラー
	 */
	ProgramError,

	/**
	 * メモリエラー
	 */
	MemoryError,

} ErrorType;

/**
 * エラーレベル
 */
typedef enum
{
	/**
	 * デバッグ情報
	 */
	ERR_DEBUG,

	/**
	 * 情報
	 */
	ERR_INFO,

	/**
	 * ワーニング(コンパイル上問題なし、文法難あり)
	 */
	ERR_WARN,

	/**
	 * エラー(問題あり)
	 */
	ERR_ERROR,

	/**
	 * コンパイルを中断する程の致命的エラー
	 */
	ERR_FATAL,

} ErrorLevel;

/**
 * エラーリスト
 */
typedef struct stErrorNode
{
	/**
	 * エラー発生行
	 */
	int line;

	/**
	 * エラー発生列
	 */
	int column;

	/**
	 * エラー種別
	 */
	ErrorType type;

	/**
	 * エラーレベル
	 */
	ErrorLevel level;

	/**
	 * エラー発生時間
	 */
	char date[64];

	/**
	 * エラーメッセージ
	 * 255文字まで
	 */
	char message[256];

	/**
	 * 次エラー
	 */
	struct stErrorNode* nextError;

} ErrorNode;

/**
 * メモリ確保エラー
 */
//extern ErrorNode memerr;

/**
 * 関数
 */
ErrorNode* getmemerror();
void erroradd(ErrorNode* const, ErrorNode*);
void errorclear(ErrorNode*);
ErrorNode* createError(const MmlMan* const);
ErrorLevel getErrorLevel(ErrorNode*);

#endif /* ERRORMAN_H */

