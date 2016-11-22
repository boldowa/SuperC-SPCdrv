/**
 * SuperC mml compile main code
 */

#include "gstdafx.h"
#include "mmlman.h"
#include "compile.h"
#include "md5.h"

/****************************************
 * 非公開定数
 ****************************************/

/* タイマ設定値 (SuperC アセンブリ定義に合わせる) */
#define TIMER 16

/* サブルーチントラック数 */
#define SUB_TRACKS 2

/* 曲用BRRインデクス開始位置 */
#define BRRINX_START 32

/* ドラム定義テーブル長 */
#define DRUMTABLE_LEN 9

/* 変換系関数用のテンポラリバッファ長 */
#define TMP_BUFFER_SIZE 32

/* オクターブ範囲(SuperC seqcmd.inc内に定義) */
#define OCTAVE_MIN 0
#define OCTAVE_MAX 6
#define OCTAVE_RANGE (OCTAVE_MAX-OCTAVE_MIN + 1)

/* timebase MAX値 */
#define TIMEBASE_MAX 0x60

/* 音楽用音色 開始番号(seqcmd.inc内に定義) */
#define EXTONE_START 0x40

/**
 * コマンド定義
 */
#define TIE  ((12*OCTAVE_RANGE) | 0x80)
#define REST (TIE + 1)
#define DRUM_NOTE (REST + 1)
#define DRUM_NOTE_NUMS 7
enum commandlist{
	/* ※注 : SuperCのinclude/seqcmd.inc 内の */
	/*        コマンド定義順に合わせること    */
	CMD_SET_INST	= (DRUM_NOTE + DRUM_NOTE_NUMS),
	CMD_VOLUME,
	CMD_PAN,
	CMD_JUMP,
	CMD_TEMPO,
	CMD_GVOLUME,
	CMD_ECHO_ON,
	CMD_ECHO_OFF,
	CMD_ECHO_PARAM,
	CMD_ECHO_FIR,
	CMD_PORTAM_ON,
	CMD_PORTAM_OFF,
	CMD_MODURATION,
	CMD_MODURATION_OFF,
	CMD_TREMOLO,
	CMD_TREMOLO_OFF,
	CMD_SUBROUTINE,
	CMD_SUBROUTINE_RETURN,
	CMD_SUBROUTINE_BREAK,
	CMD_PITCHBEND,
	CMD_TEMPO_FADE,
	CMD_VOLUME_FADE,
	CMD_GVOLUME_FADE,
	CMD_TRANSPOSE,
	CMD_REL_TRANSPOSE,
	CMD_PAN_FADE,
	CMD_PAN_VIBRATION,
	CMD_PAN_VIBRATION_OFF,
	CMD_HARDPM_ON,
	CMD_HARDPM_OFF,
};

/**
 * getStringData関数戻り値
 */
typedef enum{
	GET_STRING_OK = 0,
	GET_STRING_NODATA,
	GET_STRING_ERROR,
}GetStringResult;

/**
 * 音階テーブル
 */
static const byte SCALE_TABLE[] = {9, 11, 0, 2, 4, 5, 7};
static const byte DRUM_TABLE[] = {5, 6, 0, 1, 2, 3, 4};

/**
 * 異常状態フラグ
 * (異常構文出現時のエラーメッセージの抑制の為に使います)
 */
bool isAbnormalState = false;

/* トラックデータ */
typedef struct stTrack
{
	byte data[TRACK_SIZE_MAX];
	int  ptr;
} Track;
typedef struct stTracksData
{
	word	loopAddr[TRACKS];
	Track	track[TRACKS + SUB_TRACKS];
	int	curTrack;

	/*----- トラック毎のMML解析データ -----*/

	/**
	 * ベロシティ
	 * 初期値は15(max)
	 */
	int velocity[TRACKS];
	int lastVelocity[TRACKS];

	/**
	 * ゲートタイム
	 * 初期値は7(max)
	 */
	int gatetime[TRACKS];
	int lastGatetime[TRACKS];

	/**
	 * オクターブ
	 * 初期値は3
	 */
	int curOctave[TRACKS];

	/**
	 * 音の長さ
	 */
	int ticks[TRACKS];
	int lastticks[TRACKS];
	bool forceTickOut[TRACKS];

	/**
	 * サラウンド指定
	 */
	bool lvolRev[TRACKS];
	bool rvolRev[TRACKS];

	/**
	 * ドラムセット指定
	 */
	bool drumPart[TRACKS];

	/**
	 * トランスポーズ指定
	 */
	int transpose[TRACKS];
} TracksData;

/**
 * ラベルのリスト
 */
typedef struct tag_stLabelNode {
	int label;	/* ラベル番号 */
	int depth;	/* ループ深度 */
	int addr;	/* サブルーチン開始アドレス */

	int lastoctave;		/* サブルーチンを抜ける際のオクターブ値 */
	int lastticks;		/* サブルーチンを抜ける際のtick値 */
	int lastvelocity;	/* サブルーチンを抜ける際のベロシティ値 */
	int lastgatetime;	/* サブルーチンを抜ける際のゲートタイム値 */
	bool noteExists;	/* 音符データを含むか */
	bool tieExists;		/* タイデータを含むか */

	struct tag_stLabelNode* left;
	struct tag_stLabelNode* right;
} stLabelNode;

/**
 * サブルーチンのリストデータ
 * データ一本化の際に、リストにあるアドレスを
 * 再計算するために使う
 */
typedef struct tag_stSubroutineListData {
	int depth;		/* ループ深度 */
	int track;		/* コマンドのあるトラック */
	int subaddr;		/* サブルーチンコマンドのアドレス */

	struct tag_stSubroutineListData* next;
} stSubroutineListData;

/**
 * サブルーチンのリスト
 */
typedef struct {
	stSubroutineListData *root;
	stSubroutineListData *cur;
} stSubroutineList;

/******************************************************************************/

/**
 * mmlエラー追加
 */
#define addError(err, list)\
	if(NULL == list)\
	{\
		list = err;\
	}\
	else\
	{\
		erroradd(err, list);\
	}\
	if(ERR_ERROR <= err->level)\
	{\
		isAbnormalState = true;\
	}\

/**
 * mmlエラー作成
 */
#define newError(mml, err, list)\
	err = createError(mml);\
	if(NULL == err)\
	{ /*エラーメモリ確保失敗*/ \
		addError(getmemerror(), list);\
	}

/**
 * mml文字列比較
 */
#define mmlCmpStr(mml, str)\
	mmlcmpstr(mml, str, strlen(str))

/**
 * 文字列データの取得
 * ※文字列読み取りエラーが発生した場合、destに格納される値は不定となります。
 */
GetStringResult getStringData(char* dest, const int maxLength, MmlMan* mml, ErrorNode* compileErrList)
{
	ErrorNode *compileErr = NULL;
	char readValue;
	int dataInx = 0;

	readValue = mmlgetch(mml);
	if('"' != readValue)
	{
		/* " で始まっていないので、文字列データではありません */
		return GET_STRING_NODATA;
	}

	/* " を読み飛ばします */
	mmlgetforward(mml);

	/* 文字列データを配列にコピーします */
	do
	{
		readValue = mmlgetforward(mml);
		/* 末端に到達した場合、読み出し終わり */
		if('"' == readValue)
		{
			break;
		}
		/* 途中で改行があった場合は、取得中止(エラー) */
		if(true == mml->isnewline)
		{
			newError(mml, compileErr, compileErrList);
			compileErr->type = SyntaxError;
			compileErr->level = ERR_ERROR;
			sprintf(compileErr->message, "Incorrect line break, string data not closed.");
			addError(compileErr, compileErrList);
			return GET_STRING_ERROR;
		}

		/* データ追加 */
		dest[dataInx++] = readValue;
	}while(maxLength-1 > dataInx);
	dest[dataInx] = '\0';

	if('"' != readValue)
	{
		/* 文字数が長すぎるか、'"' が見つからない */
		newError(mml, compileErr, compileErrList);
		compileErr->type = SyntaxError;
		compileErr->level = ERR_ERROR;
		sprintf(compileErr->message, "String is too long, or '\"' is missing (%s).", dest);
		addError(compileErr, compileErrList);
		return GET_STRING_ERROR;
	}

	return GET_STRING_OK;
}

/**
 * MD5値の取得
 */
bool getMDFile(char* fname, byte* digest)
{
	FILE *file;
	MD5_CTX context;
	int len;
	byte buffer[1024];

	file = fopen(fname, "rb");
	if(NULL == file)
	{
		return false;
	}

	MD5Init(&context);
	while(0 != (len = fread(buffer, 1, 1024, file)))
	{
		MD5Update(&context, buffer, len);
	}
	MD5Final(digest, &context);

	fclose(file);
	return true;
}


/**
 * BRRリストのデータを作成する
 */
stBrrListData* makeBrrListData(char *brrname, int *brrInx, byte *md5)
{
	stBrrListData *data = NULL;

	data = (stBrrListData *)malloc(sizeof(stBrrListData));
	if(NULL != data)
	{
		memset(data, 0, sizeof(stBrrListData));
		strcpy(data->fname, brrname);
		data->brrInx = (*brrInx)++;
		memcpy(data->md5digest, md5, 16);
		data->next = NULL;
	}

	return data;
}


/**
 * BRRリストのデータを解放する
 */
void deleteBrrListData(stBrrListData* bList)
{
	stBrrListData *s, *t;

	s = bList;

	while(NULL != s)
	{
		t = s->next;
		free(s);
		s = t;
	}
}

/**
 * BRRリストにデータを追加する
 */
stBrrListData* addBrrListData(stBrrListData* bList, char *fname, int *brrInx, byte* md5)
{
	/* データが無い場合は作る */
	if(NULL == bList)
	{
		bList = makeBrrListData(fname, brrInx, md5);
		if(NULL == bList)
		{
			return NULL;
		}

		return bList;
	}

	/* 同一のBRRデータかチェックする */
	if(0 == memcmp(bList->md5digest, md5, 16))
	{
		return bList;
	}

	if(bList->next != NULL)
	{
		/* 再帰呼び出しにより、子要素をチェック */
		return addBrrListData(bList->next, fname, brrInx, md5);
	}

	bList->next = makeBrrListData(fname, brrInx, md5);
	if(NULL == bList->next)
	{
		return NULL;
	}

	return bList->next;
}

/**
 * サブルーチンリストのデータを作成する
 */
stSubroutineListData* makeSubroutineListData(int depth, int track, int subaddr)
{
	stSubroutineListData *data = NULL;

	data = (stSubroutineListData *)malloc(sizeof(stSubroutineListData));
	if(NULL != data)
	{
		data->depth = depth;
		data->track = track;
		data->subaddr = subaddr;
		data->next = NULL;
	}

	return data;
}


/**
 * サブルーチンリストのデータを解放する
 */
void deleteSubroutineList(stSubroutineList* subList)
{
	stSubroutineListData *s, *t;

	s = subList->root;

	while(NULL != s)
	{
		t = s->next;
		free(s);
		s = t;
	}

	subList->root = NULL;
	subList->cur = NULL;
}

/**
 * サブルーチンリストにデータを追加する
 */
bool addSubroutineListData(stSubroutineList* subList, int depth, int track, int subaddr)
{
	/* 初回のデータ追加処理 */
	if(NULL == subList->root)
	{
		subList->root = makeSubroutineListData(depth, track, subaddr);
		if(NULL == subList->root)
		{
			return false;
		}

		subList->cur = subList->root;
		return true;
	}

	/* 2つ目以降のデータ追加処理 */
	subList->cur->next = makeSubroutineListData(depth, track, subaddr);
	if(NULL == subList->cur->next)
	{
		return false;
	}

	/* リストのインクリメント */
	subList->cur = subList->cur->next;
	return true;
}

/**
 * ラベルノードを作成する
 */
stLabelNode* makeLabelNode(int label, int depth, int addr)
{
	stLabelNode* node;

	node = (stLabelNode*)malloc(sizeof(stLabelNode));
	if(NULL != node)
	{
		node->label = label;
		node->depth = depth;
		node->addr = addr;
		node->left = NULL;
		node->right = NULL;

		node->lastoctave = -1;
		node->lastticks = -1;
		node->lastvelocity = -1;
		node->lastgatetime = -1;
		node->noteExists = false;
		node->tieExists = false;
	}
	return node;
}

/**
 * ラベルノードを挿入する
 */
stLabelNode* insertLabelNode(stLabelNode* node, int label, int depth, int addr, MmlMan* mml, ErrorNode* compileErrList)
{
	/**
	 * エラー
	 */
	ErrorNode* compileErr = NULL;

	/* 空ノードの場合、データ追加 */
	if(NULL == node)
	{
		return makeLabelNode(label, depth, addr);
	}

	/* 同名ラベルの場合、上書きする */
	if(node->label == label)
	{
		/* 警告表示 */
		newError(mml, compileErr, compileErrList);
		compileErr->type = SyntaxError;
		compileErr->level = ERR_WARN;
		sprintf(compileErr->message, "Label (%d) redefined.", label);
		addError(compileErr, compileErrList);

		/* データの上書き */
		node->depth = depth;
		node->addr = addr;
		return node;
	}

	if(node->label > label)
	{
		node->left = insertLabelNode(node->left, label, depth, addr, mml, compileErrList);
	}
	else
	{
		node->right = insertLabelNode(node->right, label, depth, addr, mml, compileErrList);
	}
	return node;
}

/**
 * ラベルノードを削除する
 */
void deleteLabelNode(stLabelNode* node)
{
	if(NULL == node)
	{
		return;
	}
	deleteLabelNode(node->left);
	deleteLabelNode(node->right);

	free(node);
}

/**
 * 定義済みラベルを検索する
 */
stLabelNode* searchLabelNode(stLabelNode* node, int label)
{
	while(node != NULL)
	{
		if(node->label == label)
		{
			return node;
		}

		if(node->label > label)
		{
			node = node->left;
		}
		else
		{
			node = node->right;
		}
	}
	return NULL;
}

/**
 * 16進数値を取得する
 * HEX値を取得できなかった場合は-1を返す
 */
int getHex(char c)
{
	c = tolower(c);
	if('0' <= c && c <= '9')
	{
		return (c - '0');
	}
	if('a' <= c && c <= 'f')
	{
		return (c - 'a' + 10);
	}

	return -1;
}

/**
 * 10進数値を取得する
 * DIGIT値を取得できなかった場合は-1を返す
 */
int getDigit(char c)
{
	c = tolower(c);
	if('0' <= c && c <= '9')
	{
		return (c - '0');
	}

	return -1;
}

/**
 * 値の取得
 *   mml     : mml構造体
 *   allowSp : 空白(半角SP/タブ)を許容するか
 *   Numbers : 解析した数値
 *   return  : 取得した引数個数
 */
int getNumbers(MmlMan* mml, bool allowSp, int *Numbers, ErrorNode* compileErrList)
{
	/**
	 * 変換後データの数
	 */
	int getCount = 0;

	/**
	 * エラー
	 */
	ErrorNode* compileErr = NULL;

	/**
	 * カンマ入力の後かどうか
	 */
	bool comma = false;

	/* 数値データの終わりか、バッファサイズ上限まで */
	/* データ読み込みを行う                         */
	while(getCount < TMP_BUFFER_SIZE)
	{
		bool sign = false;
		char readValue;
		int  cnvValue = 0;
		bool zero = false;

		/* 符号 */
		if(mmlgetch(mml) == '-')
		{
			sign = true;
			comma = false;
			mmlgetforward(mml);
		}

		/* "0001" や"0x11"等の、先頭の"0"を読み飛ばす */
		while(mmlgetch(mml) == '0')
		{
			mmlgetforward(mml);
			zero = true;
			comma = false;
			continue;
		}

		readValue = mmlgetch(mml);

		/* 入力モード毎に処理する */
		if(readValue == 'x')
		{
			/* 16進数モード */
			/**
			 * txt→hex変換した値
			 */
			int tmpHex;

			/**
			 * 変換後データ
			 */
			int hexValue = 0;

			comma = false;
			mmlgetforward(mml); /* x読み飛ばし */
			readValue = mmlgetforward(mml);
			tmpHex = getHex(readValue);
			if(0 > tmpHex)
			{
				/* 'x'の後に数値なし */
				newError(mml, compileErr, compileErrList);
				compileErr->type = SyntaxError;
				compileErr->level = ERR_ERROR;
				sprintf(compileErr->message, "HEX value convert error.");
				addError(compileErr, compileErrList);
				return 0;
			}

			while(0 <= tmpHex)
			{
				hexValue = (hexValue << 4) + tmpHex;
				/* 次の文字を読み出す */
				readValue = mmlgetforward(mml);
				tmpHex = getHex(readValue);
			}
			cnvValue = hexValue;
		}
		else if('0' <= readValue && readValue <= '9')
		{
			/* 10進数モード */
			/**
			 * txt→hex変換した値
			 */
			int tmpDigit;

			/**
			 * 変換後データ
			 */
			int digitValue = 0;

			comma = false;
			readValue = mmlgetch(mml);
			tmpDigit = getDigit(readValue);
			if(0 > tmpDigit)
			{
				/* '-'の後に数値なし */
				newError(mml, compileErr, compileErrList);
				compileErr->type = SyntaxError;
				compileErr->level = ERR_ERROR;
				sprintf(compileErr->message, "DIGIT value convert error.");
				addError(compileErr, compileErrList);
				return 0;
			}

			while(0 <= tmpDigit)
			{
				mmlgetforward(mml); /* 1文字進める */
				digitValue = (digitValue * 10) + tmpDigit;
				/* 次の文字を読み出す */
				readValue = mmlgetch(mml); /* 1文字読み出す */
				tmpDigit = getDigit(readValue);
			}
			cnvValue = digitValue;
		}
		else
		{
			/* 数値は無いんだけど、なぜか符号だけある場合はエラー */
			if(sign == true)
			{
				newError(mml, compileErr, compileErrList);
				compileErr->type = SyntaxError;
				compileErr->level = ERR_ERROR;
				sprintf(compileErr->message, "Unknown sign char.");
				addError(compileErr, compileErrList);
				return 0;
			}

			/* カンマの後が数値以外の場合エラーにします */
			if(true == comma)
			{
				newError(mml, compileErr, compileErrList);
				compileErr->type = SyntaxError;
				compileErr->level = ERR_ERROR;
				sprintf(compileErr->message, "Unknown comma.");
				addError(compileErr, compileErrList);
				return 0;
			}

			/* 0を読み飛ばしている場合、0が入力されたとみなします */
			if(false == zero)
			{
				return getCount;
			}
		}

		if(sign)
		{
			cnvValue = 0 - cnvValue;
		}
		Numbers[getCount++] = cnvValue;

		/* スペース読み飛ばしが許容されている場合、数字の後のスペースを読み飛ばす */
		if(true == allowSp)
		{
			while((readValue == ' ') || (readValue == '\t'))
			{
				mmlgetforward(mml);
				readValue = mmlgetch(mml);
			}
		}

		/* 次の数値読み込みがあるかどうか */
		if(readValue != ',')
		{
			break;
		}
		mmlgetforward(mml);
		comma = true;

		/* スペース読み飛ばしが許容されている場合、カンマの後のスペースを読み飛ばす */
		if(true == allowSp)
		{
			readValue = mmlgetch(mml);
			while((readValue == ' ') || (readValue == '\t'))
			{
				mmlgetforward(mml);
				readValue = mmlgetch(mml);
			}
		}
		/* Note: NULL文字/行末は次のwhileループの先頭でチェック */
	}
	return getCount;
}

/**
 * 音の長さの計算
 *   ticks       : 音の長さ格納用変数へのポインタ
 *   defaultTick : 音の長さ表記省略時のデフォルト長
 *   timebase    : Timebase値
 *   mml         : mml構造体
 *   return      : 音の長さの取得が正常に完了したか否か
 */
bool calcSteps(int* ticks, int defaultTicks, int timebase, MmlMan* mml, ErrorNode* compileErrList)
{
	int tmpNums[TMP_BUFFER_SIZE];
	int getNums;
	int noteLen;
	int tick_base;
	int divide_tick;
	int tick_ctr;
	ErrorNode *compileErr = NULL;

	/* 音長のticks直指定 */
	if('=' == mmlgetch(mml))
	{
		mmlgetforward(mml);
		getNums = getNumbers(mml, false, tmpNums, compileErr);
		if(0 == getNums || 1<getNums)
		{
			/* "c=", "c=24,12" 等の場合、エラー */
			return false;
		}

		*ticks = tmpNums[0];
		return true;
	}

	/* 通常のmmlの形式での音長指定                */
	/* "a32" "b" "cx4" 等は許容します。           */
	/* "cx4"はmml的に変なので、隠し仕様とします。 */
	getNums = getNumbers(mml, false, tmpNums, compileErr);
	if(0 == getNums)
	{
		if(0 < defaultTicks)
		{
			*ticks = defaultTicks;
			return true;
		}
		/* 前回stepを取得できていない場合は、エラー */
		if(0 > *ticks)
		{
			return false;
		}
		return true;
	}
	if(1 < getNums)
	{
		/* 引数が多い (c1,1)場合、エラー */
		return false;
	}

	noteLen = tmpNums[0];

	/* 全音譜(timebase * 4)が音長でで割り切れない場合、エラー */
	if( 0 != (timebase*4) % noteLen)
	{
		return false;
	}

	/* 長さを算出する */
	tick_base = ((timebase*4) / noteLen);
	tick_ctr = divide_tick = tick_base;
	*ticks = ((timebase*4) / noteLen);

	/* 付点の音の長さを算出する */
	while(mmlgetch(mml) == '.')
	{
		mmlgetforward(mml);
		if(0 != (divide_tick % 2))
		{
			newError(mml, compileErr, compileErrList);
			compileErr->type = SyntaxError;
			compileErr->level = ERR_ERROR;
			sprintf(compileErr->message, "This can't be any more finely \".\"");
			addError(compileErr, compileErrList);
			return false;
		}
		divide_tick /= 2;

		tick_ctr += divide_tick;
	}

	*ticks = tick_ctr;

	return true;
}

bool putSeq(TracksData *tracks, const byte data, int loopDepth, MmlMan *mml, ErrorNode* compileErrList)
{
	/**
	 * エラー
	 */
	ErrorNode* compileErr = NULL;

	if(tracks->curTrack < 0)
	{
		newError(mml, compileErr, compileErrList);
		compileErr->type = SyntaxError;
		compileErr->level = ERR_ERROR;
		sprintf(compileErr->message, "Track undefined.");
		addError(compileErr, compileErrList);
		return false;
	}

	if(0 <= loopDepth)
	{
		/**
		 * サブルーチンデータ解析中は、サブルーチンエリアに書き込む
		 */
		tracks->track[TRACKS + loopDepth].data[tracks->track[TRACKS + loopDepth].ptr++] = data;
	}
	else
	{
		/**
		 * 通常トラックデータを書き込む
		 */
		tracks->track[tracks->curTrack].data[tracks->track[tracks->curTrack].ptr++] = data;
	}

	return true;
}

/**
 * mmlをコンパイルして、バイナリデータ化します
 */
ErrorNode* compile(MmlMan* mml, BinMan *bin, stBrrListData** bl)
{
	/**
	 * トラックデータ
	 */
	TracksData tracks;

	/**
	 * 数値取得のテンポラリ
	 */
	int tempVal[TMP_BUFFER_SIZE] = {0};

	/**
	 * コンパイルエラー
	 */
	ErrorNode* compileErrList = NULL;

	/**
	 * エラー
	 */
	ErrorNode* compileErr = NULL;

	/**
	 * 行コメント解析中かどうか
	 */
	bool isLineComment = false;;

	/**
	 * 複数行コメント解析中かどうか
	 */
	bool isMutiLineComment = false;

	/**
	 * パンポット逆転モードかどうか
	 */
	bool isReverseStereo = false;

	/**
	 * mmlから読みだしたデータ
	 */
	char readValue;

	/**
	 * 音階
	 */
	int scale;

	/**
	 * オクターブ記号の反転
	 * 初期値は "'>' がUP, '<'がダウン"
	 */
	bool octaveSwap = false;

	/**
	 * 分解能値(4部音符のtick数)
	 * 初期値は48
	 */
	int timebase = 48;
	bool isSpecifyedTimebase = false;

	/**
	 * 音長省略時のデフォルト音長
	 */
	int defaultTicks = -1;

	/**
	 * 現在のループ深度
	 */
	int loopDepth = -1;

	/**
	 * 前回の無名サブルーチン
	 */
	stLabelNode lastSub[SUB_TRACKS];

	/**
	 * 現在解析しているサブルーチンコール元アドレス
	 */
	int subCallAddr[SUB_TRACKS];

	/**
	 * 最後の無名サブルーチン
	 */
	stLabelNode *latestSub;

	/**
	 * ラベルリスト
	 */
	stLabelNode* labels = NULL;

	/**
	 * サブルーチンのリスト
	 */
	stSubroutineList subs = {NULL, NULL};

	/**
	 * BRRデータのリスト
	 */
	stBrrListData* bList = *bl;

	/**
	 * BRRのインデックス
	 */
	int brrInx = BRRINX_START;

	/**
	 * 音色テーブル
	 */
	Track toneTable;

	/**
	 * 音色トランスポーズ
	 */
	Track toneTranspose;

	/**
	 * ドラムトーンテーブル
	 */
	byte drumTable[DRUMTABLE_LEN*DRUM_NOTE_NUMS];
	int drumTableCtr = 0;

#if 0
	/* Stack over test */
	while(true)
	{
		newError(mml, compileErr, compileErrList);
		compileErr->type = SyntaxError;
		compileErr->level = ERR_ERROR;
		sprintf(compileErr->message, "HEX value convert error.");
		addError(compileErr, compileErrList);
	}
#endif

	/* トラック初期化 */
	memset(&tracks, 0, sizeof(TracksData));
	tracks.curTrack = -1;
	{
		int i;
		/* Track毎データの初期化 */
		for(i=0; i<TRACKS; i++)
		{
			tracks.loopAddr[i] = 0xffff;
			tracks.velocity[i] = 15;
			tracks.lastVelocity[i] = -1;
			tracks.gatetime[i] = 7;
			tracks.lastGatetime[i] = -1;
			tracks.curOctave[i] = 3;
			tracks.ticks[i] = -1;
			tracks.lastticks[i] = -1;
			tracks.forceTickOut[i] = false;
			tracks.lvolRev[i] = false;
			tracks.rvolRev[i] = false;
			tracks.drumPart[i] = false;
			tracks.transpose[i] = 0;
		}
	}

	/* 音色テーブルの初期化 */
	memset(&toneTable, 0, sizeof(Track));
	memset(drumTable, 0, DRUMTABLE_LEN*DRUM_NOTE_NUMS);
	memset(&toneTranspose, 0, sizeof(Track));

	while(mml->iseof == false)
	{
		readValue = mmlgetusp(mml);

		/**************************************************************/
		/* MMLコメント処理                                            */
		/**************************************************************/

		/* 複数行のコメントがある場合は、      */
		/* コメント末尾までmmlを読み飛ばします */
		if(isMutiLineComment == true)
		{
			/* コメント末の記号以外は、      */
			/* コメントとみなし、無視します  */
			if(readValue != '*')
			{
				continue;
			}

			readValue = mmlgetch(mml);

			if('/' == readValue)
			{
				newError(mml, compileErr, compileErrList);
				compileErr->type = ErrorNone;
				compileErr->level = ERR_DEBUG;
				sprintf(compileErr->message, "Multi line comment end");
				addError(compileErr, compileErrList);

				mmlgetforward(mml);
				isMutiLineComment = false;
			}

			continue;

		} /* endif isMultiLineComment */

		/* 改行があった場合、行コメント処理を中断します */
		if(isLineComment == true)
		{
			/* 読み込み中の改行有無をチェック */
			if(mml->isnewline == true)
			{
				/* 改行があった場合、コメント状態を解除 */
				isLineComment = false;
			}
			else
			{
				/* 改行が無い場合は、読み飛ばし */
				continue;
			}
		} /* endif isLineComment */


		/**************************************************************/
		/* MML解析処理                                                */
		/**************************************************************/
		switch(readValue)
		{
			int octaveTmp;
			int remainTicks;

			/*************************************************************/

			/******************************************/
			/* 音長設定                               */
			/******************************************/
			case 'l':
				/* ログ出力 */
				newError(mml, compileErr, compileErrList);
				compileErr->type = ErrorNone;
				compileErr->level = ERR_DEBUG;
				sprintf(compileErr->message, "Length prefix");
				addError(compileErr, compileErrList);

				if(false == calcSteps(&defaultTicks, 0, timebase, mml, compileErr))
				{
					newError(mml, compileErr, compileErrList);
					compileErr->type = SyntaxError;
					compileErr->level = ERR_ERROR;
					sprintf(compileErr->message, "Invalid length prefix.");
					addError(compileErr, compileErrList);
					continue;
				}
				/*
				if(0x60 < tracks.ticks[tracks.curTrack])
				{
					newError(mml, compileErr, compileErrList);
					compileErr->type = SyntaxError;
					compileErr->level = ERR_ERROR;
					sprintf(compileErr->message, "Unable length prefix.");
					addError(compileErr, compileErrList);
					continue;
				}
				*/
				isAbnormalState = false;
				break;

			/******************************************/
			/* 音譜・休符・タイ                       */
			/******************************************/
			case 'a':
			case 'b':
			case 'c':
			case 'd':
			case 'e':
			case 'f':
			case 'g':
			case 'r':
			case '^':
				{
					byte note;

					if('r' == readValue)
					{
						/* ログ出力 */
						newError(mml, compileErr, compileErrList);
						compileErr->type = ErrorNone;
						compileErr->level = ERR_DEBUG;
						sprintf(compileErr->message, "Rest");
						addError(compileErr, compileErrList);

						note = REST;
					}
					else if('^' == readValue)
					{
						/* ログ出力 */
						newError(mml, compileErr, compileErrList);
						compileErr->type = ErrorNone;
						compileErr->level = ERR_DEBUG;
						sprintf(compileErr->message, "Tie");
						addError(compileErr, compileErrList);

						note = TIE;

						if(0 < loopDepth)
						{
							lastSub[loopDepth].tieExists = true;
						}
					}
					else
					{
						/* ログ出力 */
						newError(mml, compileErr, compileErrList);
						compileErr->type = ErrorNone;
						compileErr->level = ERR_DEBUG;
						sprintf(compileErr->message, "Note %c", readValue);
						addError(compileErr, compileErrList);

						/* 音階の算出 */
						if(false == tracks.drumPart[tracks.curTrack])
						{
							octaveTmp = tracks.curOctave[tracks.curTrack];
							scale = SCALE_TABLE[(readValue - 'a')];

							/* +/- による半音補正 */
							readValue = mmlgetch(mml);
							if(readValue == '+' || readValue == '#')  /* いちおう半角の#に対応させておく */
							{
								mmlgetforward(mml);
								scale++;
							}
							else if(readValue == '-')
							{
								mmlgetforward(mml);
								scale--;
							}

							/* トランスポーズ */
							scale += tracks.transpose[tracks.curTrack];

							/* scaleが 0 ～ 11 の値になるよう補正 */
							if(0 > scale || 11 < scale)
							{
								octaveTmp += (scale / 12);
								scale %= 12;
							}

							/* オクターブチェック */
							if(OCTAVE_MIN > octaveTmp || OCTAVE_MAX < octaveTmp)
							{
								/* ログ出力 */
								newError(mml, compileErr, compileErrList);
								compileErr->type = SyntaxError;
								compileErr->level = ERR_ERROR;
								sprintf(compileErr->message, "Octave range over : o%d", octaveTmp);
								addError(compileErr, compileErrList);
								continue;
							}
							note = ((octaveTmp-OCTAVE_MIN)*12 + scale) | 0x80;

							if(0 < loopDepth)
							{
								lastSub[loopDepth].noteExists = true;
							}
						}
						else
						{
							note = DRUM_TABLE[(readValue - 'a')] + DRUM_NOTE;
						}
					}

					/* Step算出 */
					if(false == calcSteps(&tracks.ticks[tracks.curTrack], defaultTicks, timebase, mml, compileErr))
					{
						newError(mml, compileErr, compileErrList);
						compileErr->type = SyntaxError;
						compileErr->level = ERR_ERROR;
						sprintf(compileErr->message, "Invalid length note.");
						addError(compileErr, compileErrList);
						continue;
					}

					/* 0x60tick以上の場合は分割する */
					remainTicks = 0;
					if(0x60 < tracks.ticks[tracks.curTrack])
					{
						remainTicks = tracks.ticks[tracks.curTrack] - 0x60;
						tracks.ticks[tracks.curTrack] = 0x60;
					}

					/* 音長挿入 */
					if((tracks.lastticks[tracks.curTrack] != tracks.ticks[tracks.curTrack]) || (tracks.gatetime[tracks.curTrack] != tracks.lastGatetime[tracks.curTrack]) || (tracks.velocity[tracks.curTrack] != tracks.lastVelocity[tracks.curTrack]) || (true == tracks.forceTickOut[tracks.curTrack]))
					{
						putSeq(&tracks, tracks.ticks[tracks.curTrack], loopDepth, mml, compileErr);
					}
					tracks.lastticks[tracks.curTrack] = tracks.ticks[tracks.curTrack];
					tracks.forceTickOut[tracks.curTrack] = false;

					/* ゲートタイム/ベロシティ挿入 */
					if(false == tracks.drumPart[tracks.curTrack])
					{
						if((tracks.gatetime[tracks.curTrack] != tracks.lastGatetime[tracks.curTrack]) || (tracks.velocity[tracks.curTrack] != tracks.lastVelocity[tracks.curTrack]))
						{
							byte qv = (tracks.gatetime[tracks.curTrack] << 4) | tracks.velocity[tracks.curTrack];
							putSeq(&tracks, qv, loopDepth, mml, compileErr);
						}
					}

					/* 音譜・休符・タイの挿入 */
					putSeq(&tracks, note, loopDepth, mml, compileErr);

					tracks.lastGatetime[tracks.curTrack] = tracks.gatetime[tracks.curTrack];
					tracks.lastVelocity[tracks.curTrack] = tracks.velocity[tracks.curTrack];

					/* 長い音の場合にタイを追加 */
					while(remainTicks != 0)
					{
						if(0x60 < remainTicks)
						{
							remainTicks -= 0x60;
							tracks.ticks[tracks.curTrack] = 0x60;
						}
						else
						{
							tracks.ticks[tracks.curTrack] = remainTicks;
							remainTicks = 0;
						}
						if(tracks.lastticks[tracks.curTrack] != tracks.ticks[tracks.curTrack])
						{
							putSeq(&tracks, tracks.ticks[tracks.curTrack], loopDepth, mml, compileErr);
						}
						tracks.lastticks[tracks.curTrack] = tracks.ticks[tracks.curTrack];
						putSeq(&tracks, TIE, loopDepth, mml, compileErr);
					}
				isAbnormalState = false;
				break;
				}

			/******************************************/
			/* ピッチベンド                           */
			/******************************************/
			case 'B':
				/* ログ出力 */
				newError(mml, compileErr, compileErrList);
				compileErr->type = ErrorNone;
				compileErr->level = ERR_DEBUG;
				sprintf(compileErr->message, "Pitchbend");
				addError(compileErr, compileErrList);

				if(3 != getNumbers(mml, false, tempVal, compileErrList))
				{
					newError(mml, compileErr, compileErrList);
					compileErr->type = SyntaxError;
					compileErr->level = ERR_ERROR;
					sprintf(compileErr->message, "Invalid pitchbend.");
					addError(compileErr, compileErrList);
					continue;
				}

				/* ピッチベンドコマンド挿入 */
				putSeq(&tracks, CMD_PITCHBEND, loopDepth, mml, compileErr);
				putSeq(&tracks, tempVal[0], loopDepth, mml, compileErr);
				putSeq(&tracks, tempVal[1], loopDepth, mml, compileErr);
				putSeq(&tracks, tempVal[2], loopDepth, mml, compileErr);
				isAbnormalState = false;
				break;

			/******************************************/
			/* トランスポーズ                         */
			/******************************************/
			case 'T':
				/* ログ出力 */
				newError(mml, compileErr, compileErrList);
				compileErr->type = ErrorNone;
				compileErr->level = ERR_DEBUG;
				sprintf(compileErr->message, "Transpose");
				addError(compileErr, compileErrList);

				if(1 != getNumbers(mml, false, tempVal, compileErrList))
				{
					newError(mml, compileErr, compileErrList);
					compileErr->type = SyntaxError;
					compileErr->level = ERR_ERROR;
					sprintf(compileErr->message, "Invalid transpose.");
					addError(compileErr, compileErrList);
					continue;
				}

				/* トランスポーズコマンド挿入 */
				putSeq(&tracks, CMD_TRANSPOSE, loopDepth, mml, compileErr);
				putSeq(&tracks, tempVal[0], loopDepth, mml, compileErr);
				isAbnormalState = false;
				break;

			/******************************************/
			/* エコーパラメータ設定                   */
			/******************************************/
			case 'E':
			{
				int edl;

				/* ログ出力 */
				newError(mml, compileErr, compileErrList);
				compileErr->type = ErrorNone;
				compileErr->level = ERR_DEBUG;
				sprintf(compileErr->message, "Echo parameter");
				addError(compileErr, compileErrList);

				if(3 != getNumbers(mml, false, tempVal, compileErrList))
				{
					newError(mml, compileErr, compileErrList);
					compileErr->type = SyntaxError;
					compileErr->level = ERR_ERROR;
					sprintf(compileErr->message, "Invalid echo parameter.");
					addError(compileErr, compileErrList);
					continue;
				}

				/* EDL最大値の更新 */
				edl = tempVal[1] & 0xf;
				if(mml->maxEDL < edl)
				{
					mml->maxEDL = edl;
				}

				/* エコーパラメータコマンド挿入 */
				putSeq(&tracks, CMD_ECHO_PARAM, loopDepth, mml, compileErr);
				putSeq(&tracks, tempVal[0], loopDepth, mml, compileErr);
				putSeq(&tracks, tempVal[1], loopDepth, mml, compileErr);
				putSeq(&tracks, tempVal[2], loopDepth, mml, compileErr);
				isAbnormalState = false;
				break;
			}

			/******************************************/
			/* エコーFIR係数設定                      */
			/******************************************/
			case 'F':
				/* ログ出力 */
				newError(mml, compileErr, compileErrList);
				compileErr->type = ErrorNone;
				compileErr->level = ERR_DEBUG;
				sprintf(compileErr->message, "FIR parameter");
				addError(compileErr, compileErrList);

				if(8 != getNumbers(mml, false, tempVal, compileErrList))
				{
					newError(mml, compileErr, compileErrList);
					compileErr->type = SyntaxError;
					compileErr->level = ERR_ERROR;
					sprintf(compileErr->message, "Invalid FIR parameter.");
					addError(compileErr, compileErrList);
					continue;
				}

				/* FIR係数コマンド挿入 */
				putSeq(&tracks, CMD_ECHO_FIR, loopDepth, mml, compileErr);
				{
					int i;
					for(i=0; i<8; i++){ putSeq(&tracks, tempVal[i], loopDepth, mml, compileErr); }
				}
				isAbnormalState = false;
				break;

			/******************************************/
			/* コントロール                           */
			/******************************************/
			case '#':
			{
				bool isDrum = false;

				if(false == isFirstChar(mml))
				{
					newError(mml, compileErr, compileErrList);
					compileErr->type = SyntaxError;
					compileErr->level = ERR_ERROR;
					sprintf(compileErr->message, "Invalid control \"#\".");
					addError(compileErr, compileErrList);
					continue;
				}
				/* トラック指定 */
				if(mmlCmpStr(mml, "track") == true)
				{
					skipspaces(mml);

					if(0 == getNumbers(mml, false, tempVal, compileErrList))
					{
						newError(mml, compileErr, compileErrList);
						compileErr->type = SyntaxError;
						compileErr->level = ERR_ERROR;
						sprintf(compileErr->message, "Invalid track assign.");
						addError(compileErr, compileErrList);
						continue;
					}
					if(0 >= tempVal[0] || TRACKS < tempVal[0])
					{
						newError(mml, compileErr, compileErrList);
						compileErr->type = SyntaxError;
						compileErr->level = ERR_ERROR;
						sprintf(compileErr->message, "Invalid track assign(%d).", tempVal[0]);
						addError(compileErr, compileErrList);
						continue;
					}

					/* サブルーチン解析中は切り替え不可 */
					if(0 < loopDepth)
					{
						newError(mml, compileErr, compileErrList);
						compileErr->type = SyntaxError;
						compileErr->level = ERR_ERROR;
						sprintf(compileErr->message, "Impossible track change until track end.");
						addError(compileErr, compileErrList);
						continue;
					}

					/* トラック切り替え */
					tracks.curTrack = tempVal[0]-1;

					/* ログ出力 */
					newError(mml, compileErr, compileErrList);
					compileErr->type = ErrorNone;
					compileErr->level = ERR_DEBUG;
					sprintf(compileErr->message, "Track Change => %d", tempVal[0]);
					addError(compileErr, compileErrList);

					isAbnormalState = false;
				} /* endif Track */
				/* 分解能指定 */
				else if(mmlCmpStr(mml, "timebase") == true)
				{
					if(tracks.curTrack != -1)
					{
						newError(mml, compileErr, compileErrList);
						compileErr->type = SyntaxError;
						compileErr->level = ERR_ERROR;
						sprintf(compileErr->message, "Impossible timebase specify in track.");
						addError(compileErr, compileErrList);
						continue;
					}
					if(isSpecifyedTimebase)
					{
						newError(mml, compileErr, compileErrList);
						compileErr->type = SyntaxError;
						compileErr->level = ERR_WARN;
						sprintf(compileErr->message, "Multiple timebase specify.");
						addError(compileErr, compileErrList);
					}

					isSpecifyedTimebase = true;
					skipspaces(mml);

					if(0 == getNumbers(mml, false, tempVal, compileErrList))
					{
						newError(mml, compileErr, compileErrList);
						compileErr->type = SyntaxError;
						compileErr->level = ERR_ERROR;
						sprintf(compileErr->message, "Invalid timebase assign.");
						addError(compileErr, compileErrList);
						continue;
					}
					if(TIMEBASE_MAX < tempVal[0])
					{
						newError(mml, compileErr, compileErrList);
						compileErr->type = SyntaxError;
						compileErr->level = ERR_ERROR;
						sprintf(compileErr->message, "Timebase is too large(%d).", tempVal[0]);
						addError(compileErr, compileErrList);
						continue;
					}

					/* timebase セット*/
					timebase = tempVal[0];

					/* ログ出力 */
					newError(mml, compileErr, compileErrList);
					compileErr->type = ErrorNone;
					compileErr->level = ERR_DEBUG;
					sprintf(compileErr->message, "Timebase Change => %d", tempVal[0]);
					addError(compileErr, compileErrList);

					isAbnormalState = false;
				} /* endif Timebase */
				else if(mmlCmpStr(mml, "swap<>") == true)
				{
					octaveSwap = !octaveSwap;

					/* ログ出力 */
					newError(mml, compileErr, compileErrList);
					compileErr->type = ErrorNone;
					compileErr->level = ERR_DEBUG;
					sprintf(compileErr->message, "Octave swap => '%c' is Up", octaveSwap ? '<' : '>');
					addError(compileErr, compileErrList);

					isAbnormalState = false;
				} /* endif swap<> */
				else if( (true == mmlCmpStr(mml, "tone")) || (true == (isDrum = mmlCmpStr(mml, "drum"))) )
				{
					GetStringResult gs_result;
					int pathLength;
					int validNums = 10;
					int nums = 0;
					int* arg = tempVal;
					bool isFile = false;
					bool isNoise = false;
					char fileName[MAX_PATH];
					int localBrrInx = 0;
					stBrrListData *localBlist = NULL;
					byte digest[16];

					if(true == isDrum)
					{
						validNums += 4;

						if( (DRUMTABLE_LEN*DRUM_NOTE_NUMS) <= drumTableCtr)
						{
							newError(mml, compileErr, compileErrList);
							compileErr->type = SyntaxError;
							compileErr->level = ERR_ERROR;
							sprintf(compileErr->message, "TOO MANY DRUM DEFINE.");
							addError(compileErr, compileErrList);
							continue;
						}
					}

					skipspaces(mml);

					/* mmlファイルパスをセット */
					strcpy(fileName, mml->fdir);
					pathLength = strlen(fileName);
					gs_result = getStringData((fileName+pathLength), (MAX_PATH-pathLength), mml, compileErrList);

					if(GET_STRING_NODATA != gs_result)
					{
						if(GET_STRING_ERROR == gs_result)
						{
							newError(mml, compileErr, compileErrList);
							compileErr->type = SyntaxError;
							compileErr->level = ERR_ERROR;
							sprintf(compileErr->message, "Failed to get BRR filepath.");
							addError(compileErr, compileErrList);
							continue;
						}

						skipspaces(mml);
						if(',' != mmlgetforward(mml))
						{
							/* カンマ区切りなし、文法エラー */
							newError(mml, compileErr, compileErrList);
							compileErr->type = SyntaxError;
							compileErr->level = ERR_ERROR;
							if(true == isDrum)
							{
								sprintf(compileErr->message, "Invalid drum define.");
							}
							else
							{
								sprintf(compileErr->message, "Invalid tone define.");
							}
							addError(compileErr, compileErrList);
							continue;
						}

						/* ファイルチェック */
						if(false == getMDFile(fileName, digest))
						{
							/* BRRファイルエラー */
							newError(mml, compileErr, compileErrList);
							compileErr->type = SyntaxError;
							compileErr->level = ERR_ERROR;
							sprintf(compileErr->message, "File read Error (%s).", fileName);
							addError(compileErr, compileErrList);
							continue;
						}

						/* BRRリスト追加 */
						localBlist = addBrrListData(bList, fileName, &brrInx, digest);
						if(NULL == localBlist)
						{
							/* メモリ確保エラー */
							newError(mml, compileErr, compileErrList); /* メモリ確保できなかった場合、たぶんこの処理もコケるのであまり意味ない */
							compileErr->type = SyntaxError;
							compileErr->level = ERR_FATAL;
							sprintf(compileErr->message, "addBrrListData : Memory alloc error.");
							addError(compileErr, compileErrList);
							deleteSubroutineList(&subs);
							deleteBrrListData(bList);
							bList = NULL;
							return false;
						}
						if(NULL == bList)
						{
							bList = localBlist;
						}

						localBrrInx = localBlist->brrInx;

						isFile = true;
						validNums--;
						skipspaces(mml);
					}
					else
					{
						readValue = mmlgetch(mml);
						if('n' == readValue)	/* ノイズクロック指定 */
						{
							/* 'n' の読み飛ばし */
							mmlgetforward(mml);
							isNoise = true;
						}
					}

					/* 引数取得 */
					nums = getNumbers(mml, true, tempVal, compileErrList);
					if(5>nums)
					{
						/* 引数の数がヘン、文法エラー */
						newError(mml, compileErr, compileErrList);
						compileErr->type = SyntaxError;
						compileErr->level = ERR_ERROR;
						if(true == isDrum)
						{
							sprintf(compileErr->message, "Invalid drum define(Invalid arguments).");
						}
						else
						{
							sprintf(compileErr->message, "Invalid tone define(Invalid arguments).");
						}
						addError(compileErr, compileErrList);
						continue;
					}

					/* BRRが波形番号指定の場合の波形情報を取得 */
					if(false == isFile)
					{
						localBrrInx = tempVal[0];
						if(true == isNoise)
						{
							localBrrInx &= 0x1f;
							localBrrInx |= 0x80;
						}
						arg++;
					}

					/* GAINモードの時は1つ引数が少ない */
					if(0 == arg[3])
					{
						validNums--;
					}
					if(nums != validNums)
					{
						/* 引数の数がヘン、文法エラー */
						newError(mml, compileErr, compileErrList);
						compileErr->type = SyntaxError;
						compileErr->level = ERR_ERROR;
						if(isDrum)
						{
							sprintf(compileErr->message, "Invalid drum define(Invalid arguments).");
						}
						else
						{
							sprintf(compileErr->message, "Invalid tone define(Invalid arguments).");
						}
						addError(compileErr, compileErrList);
						continue;
					}

					if(false == isDrum)
					{
						toneTranspose.data[toneTranspose.ptr++] = arg[1];
					}

					/* ADSRモード */
					if(arg[3] != 0)
					{
						byte adr, sr, rr, tmp;
						byte qv;
						byte pan;

						/* Decay */
						adr = arg[5];
						adr <<= 4;
						/* Attack */
						tmp = (arg[4] & 0xf);
						adr |= tmp;
						adr |= 0x80;
						/* Sustain Level */
						sr = arg[6];
						sr <<= 5;
						rr = sr;
						/* Sustain Rate */
						tmp = (arg[7] & 0x1f);
						sr |= tmp;
						/* Release Rate */
						tmp = (arg[8] & 0x1f);
						rr |= tmp;

						if(false == isDrum)
						{
							toneTable.data[toneTable.ptr++] = localBrrInx;	/* 波形番号 */
							toneTable.data[toneTable.ptr++] = arg[0];	/* ピッチ倍率 */
							toneTable.data[toneTable.ptr++] = arg[2];	/* デチューン */
							toneTable.data[toneTable.ptr++] = adr;		/* ADR */
							toneTable.data[toneTable.ptr++] = sr;		/* SR */
							toneTable.data[toneTable.ptr++] = rr;		/* RR */
							isAbnormalState = false;
							break;
						}

						qv =  ( ((arg[9]-1)&0x7) << 4);
						qv |= ((arg[10] - 1) & 0xf);

						pan = arg[11];
						if(true == isReverseStereo)
						{
							tmp = pan;
							pan ^= 0x3f;
							pan++;
							pan &= 0x3f;
							pan |= (tmp & 0xc0);
						}

						drumTable[drumTableCtr++] = localBrrInx;	/* 波形番号 */
						drumTable[drumTableCtr++] = arg[0];		/* ピッチ倍率 */
						drumTable[drumTableCtr++] = arg[2];		/* デチューン */
						drumTable[drumTableCtr++] = adr;		/* ADR */
						drumTable[drumTableCtr++] = sr;			/* SR */
						drumTable[drumTableCtr++] = rr;			/* RR */
						drumTable[drumTableCtr++] = qv;			/* ゲートタイム・ベロシティ */
						drumTable[drumTableCtr++] = pan;		/* パンポット */
						drumTable[drumTableCtr++] = arg[12];		/* 音階値 */
						isAbnormalState = false;
						break;
					}

					/* GAINモード */
					{
						byte gmode1, gmode2, gv1, gv2;
						byte gain1, gain2;
						gmode1 = arg[4];
						gv1 = arg[5];
						gmode2 = arg[6];
						gv2 = arg[7];
						byte qv;
						byte pan,tmp;

						/* 発音時のGAIN */
						switch(gmode1)
						{
							case 0:	/* 直接指定 */
								gv1 &= 0x7f;
								gain1 = gv1;
								break;
							case 1:	/* 減少（リニア） */
							case 2:	/* 減少（ 指数 ） */
							case 3:	/* 増加（リニア） */
							case 4:	/* 増加（折れ線） */
							default:
								gv1 &= 0x1f;
								gmode1--;
								gain1 = (gmode1 << 5);
								gain1 |= 0x80;
								gain1 |= gv1;
								break;
						}
						/* リリース時のGAIN */
						switch(gmode2)
						{
							case 0:	/* 直接指定 */
								gv2 &= 0x7f;
								gain2 = gv2;
								break;
							case 1:	/* 減少（リニア） */
							case 2:	/* 減少（ 指数 ） */
							case 3:	/* 増加（リニア） */
							case 4:	/* 増加（折れ線） */
							default:
								gv2 &= 0x1f;
								gmode2--;
								gain2 = (gmode2 << 5);
								gain2 |= 0x80;
								gain2 |= gv2;
								break;
						}

						if(false == isDrum)
						{
							toneTable.data[toneTable.ptr++] = localBrrInx;	/* 波形番号 */
							toneTable.data[toneTable.ptr++] = arg[0];	/* ピッチ倍率 */
							toneTable.data[toneTable.ptr++] = arg[2];	/* デチューン */
							toneTable.data[toneTable.ptr++] = 0;		/* ADR */
							toneTable.data[toneTable.ptr++] = gain1;	/* GAIN1 */
							toneTable.data[toneTable.ptr++] = gain2;	/* GAIN2 */
							isAbnormalState = false;
							break;
						}
						qv =  ( ((arg[8]-1)&0x7) << 4);
						qv |= ((arg[9] - 1) & 0xf);

						pan = arg[10];
						if(true == isReverseStereo)
						{
							tmp = pan;
							pan ^= 0x3f;
							pan++;
							pan &= 0x3f;
							pan |= (tmp & 0xc0);
						}

						drumTable[drumTableCtr++] = localBrrInx;	/* 波形番号 */
						drumTable[drumTableCtr++] = arg[0];		/* ピッチ倍率 */
						drumTable[drumTableCtr++] = arg[2];		/* デチューン */
						drumTable[drumTableCtr++] = 0;			/* ADR */
						drumTable[drumTableCtr++] = gain1;		/* GAIN1 */
						drumTable[drumTableCtr++] = gain2;		/* GAIN2 */
						drumTable[drumTableCtr++] = qv;			/* ゲートタイム・ベロシティ */
						drumTable[drumTableCtr++] = pan;		/* パンポット */
						drumTable[drumTableCtr++] = arg[11];		/* 音階値 */
						isAbnormalState = false;
						break;
					}
				} /* endif tone / drum */
				else if(mmlCmpStr(mml, "macro") == true)
				{
					newError(mml, compileErr, compileErrList);
					if(false == addmacro(mml))
					{
						compileErr->type = SyntaxError;
						compileErr->level = ERR_ERROR;
						sprintf(compileErr->message, "Invalid or multiple macro.");
						addError(compileErr, compileErrList);
						continue;
					}

					compileErr->type = ErrorNone;
					compileErr->level = ERR_DEBUG;
					sprintf(compileErr->message, "Define macro => \"%s\".", mml->macroTail->name);
					addError(compileErr, compileErrList);
					continue;
				} /* endif macro */
				else if(mmlCmpStr(mml, "reversestereo") == true)
				{
					isReverseStereo = !isReverseStereo;

					/* ログ出力 */
					newError(mml, compileErr, compileErrList);
					compileErr->type = ErrorNone;
					compileErr->level = ERR_DEBUG;
					sprintf(compileErr->message, "ReverseStereo => left is %i", isReverseStereo ? 0 : 63);
					addError(compileErr, compileErrList);

					isAbnormalState = false;
				} /* endif reversestereo */
				else if(mmlCmpStr(mml, "title") == true)
				{
					char songTitle[32];

					skipspaces(mml);
					if(GET_STRING_OK != getStringData(songTitle, 32, mml, compileErrList))
					{
						isAbnormalState = true;
						continue;
					}

					strcpy(mml->spcTitle, songTitle);
				} /* endif title */
				else if(mmlCmpStr(mml, "game") == true)
				{
					char songGame[32];

					skipspaces(mml);
					if(GET_STRING_OK != getStringData(songGame, 32, mml, compileErrList))
					{
						isAbnormalState = true;
						continue;
					}

					strcpy(mml->spcGame, songGame);
				} /* endif game */
				else if(mmlCmpStr(mml, "composer") == true)
				{
					char songComposer[32];

					skipspaces(mml);
					if(GET_STRING_OK != getStringData(songComposer, 32, mml, compileErrList))
					{
						isAbnormalState = true;
						continue;
					}

					strcpy(mml->spcComposer, songComposer);
				} /* endif composer */
				else if(mmlCmpStr(mml, "dumper") == true)
				{
					char songDumper[16];

					skipspaces(mml);
					if(GET_STRING_OK != getStringData(songDumper, 16, mml, compileErrList))
					{
						isAbnormalState = true;
						continue;
					}

					strcpy(mml->spcDumper, songDumper);
				} /* endif dumper */
				else if(mmlCmpStr(mml, "comment") == true)
				{
					char songComment[32];

					skipspaces(mml);
					if(GET_STRING_OK != getStringData(songComment, 32, mml, compileErrList))
					{
						isAbnormalState = true;
						continue;
					}

					strcpy(mml->spcComment, songComment);
				} /* endif comment */
				else if(mmlCmpStr(mml, "length") == true)
				{
					int nums;
					int sec;

					skipspaces(mml);

					nums = getNumbers(mml, true, tempVal, compileErrList);
					if(2 < nums || 0 == nums)
					{
						newError(mml, compileErr, compileErrList);
						compileErr->type = SyntaxError;
						compileErr->level = ERR_ERROR;
						sprintf(compileErr->message, "Invalid length.");
						addError(compileErr, compileErrList);
						continue;
					}

					/* sec,fade 指定 */
					if(2 == nums)
					{
						mml->playingTime = tempVal[0];
						mml->fadeTime = tempVal[1];
						break;
					}

					/* sec のみ指定 */
					if(':' != mmlgetch(mml))
					{
						mml->playingTime = tempVal[0];
						break;
					}

					mmlgetforward(mml);
					sec = tempVal[0]*60;

					nums = getNumbers(mml, true, tempVal, compileErrList);
					/* min:sec,fade 指定 */
					if(2 == nums)
					{
						mml->playingTime = sec + tempVal[0];
						mml->fadeTime = tempVal[1];
						break;
					}

					/* min:sec 指定 */
					if(1 == nums)
					{
						mml->playingTime = sec + tempVal[0];
						break;
					}

					/* ここに来たらエラー */
					newError(mml, compileErr, compileErrList);
					compileErr->type = SyntaxError;
					compileErr->level = ERR_ERROR;
					sprintf(compileErr->message, "Invalid length.");
					addError(compileErr, compileErrList);
					continue;

				} /* endif length */
				else
				{
					newError(mml, compileErr, compileErrList);
					compileErr->type = SyntaxError;
					compileErr->level = ERR_ERROR;
					sprintf(compileErr->message, "Invalid control \"#\".");
					addError(compileErr, compileErrList);
					continue;
				}
				isAbnormalState = false;
				break;
			}

			/******************************************/
			/* サブコマンド定義                       */
			/******************************************/
			case '%':
				readValue = mmlgetch(mml);
				switch(readValue)
				{
					/******************************************/
					/* 逆位相サラウンド                       */
					/******************************************/
					case 'r':
						mmlgetforward(mml);
						if(2 != getNumbers(mml, false, tempVal, compileErrList))
						{
							newError(mml, compileErr, compileErrList);
							compileErr->type = SyntaxError;
							compileErr->level = ERR_ERROR;
							sprintf(compileErr->message, "Invalid Surround specify.");
							addError(compileErr, compileErrList);
							continue;
						}
						tracks.lvolRev[tracks.curTrack] = tempVal[0];
						tracks.rvolRev[tracks.curTrack] = tempVal[1];
						isAbnormalState = false;
						break;

					/******************************************/
					/* 内部トランスポーズ                     */
					/******************************************/
					case 't':
						mmlgetforward(mml);
						if(1 != getNumbers(mml, false, tempVal, compileErrList))
						{
							newError(mml, compileErr, compileErrList);
							compileErr->type = SyntaxError;
							compileErr->level = ERR_ERROR;
							sprintf(compileErr->message, "Invalid internal transpose.");
							addError(compileErr, compileErrList);
							continue;
						}
						tracks.transpose[tracks.curTrack] = tempVal[0];
						isAbnormalState = false;
						break;

					/******************************************/
					/* 相対トランスポーズ                     */
					/******************************************/
					case 'T':
						mmlgetforward(mml);
						/* ログ出力 */
						newError(mml, compileErr, compileErrList);
						compileErr->type = ErrorNone;
						compileErr->level = ERR_DEBUG;
						sprintf(compileErr->message, "Transpose");
						addError(compileErr, compileErrList);

						if(1 != getNumbers(mml, false, tempVal, compileErrList))
						{
							newError(mml, compileErr, compileErrList);
							compileErr->type = SyntaxError;
							compileErr->level = ERR_ERROR;
							sprintf(compileErr->message, "Invalid relative transpose.");
							addError(compileErr, compileErrList);
							continue;
						}

						/* トランスポーズコマンド挿入 */
						putSeq(&tracks, CMD_REL_TRANSPOSE, loopDepth, mml, compileErr);
						putSeq(&tracks, tempVal[0], loopDepth, mml, compileErr);
						isAbnormalState = false;
						break;

					/******************************************/
					/* エコー有無効切替                       */
					/******************************************/
					case 'e':
						{
							int nums;
							mmlgetforward(mml);
							nums = getNumbers(mml, false, tempVal, compileErrList);
							if(1 < nums)
							{
								newError(mml, compileErr, compileErrList);
								compileErr->type = SyntaxError;
								compileErr->level = ERR_ERROR;
								sprintf(compileErr->message, "Invalid echo specify.");
								addError(compileErr, compileErrList);
								continue;
							}
							if(1 == nums && tempVal[0] == 0)
							{
								putSeq(&tracks, CMD_ECHO_OFF, loopDepth, mml, compileErr);
								isAbnormalState = false;
								break;
							}
							putSeq(&tracks, CMD_ECHO_ON, loopDepth, mml, compileErr);
						}
						isAbnormalState = false;
						break;

					/******************************************/
					/* H/W ピッチモジュレーション有無効切替   */
					/******************************************/
					case 'm':
						{
							int nums;
							mmlgetforward(mml);
							nums = getNumbers(mml, false, tempVal, compileErrList);
							if(1 < nums)
							{
								newError(mml, compileErr, compileErrList);
								compileErr->type = SyntaxError;
								compileErr->level = ERR_ERROR;
								sprintf(compileErr->message, "Invalid pmon specify.");
								addError(compileErr, compileErrList);
								continue;
							}
							if(1 == nums && tempVal[0] == 0)
							{
								putSeq(&tracks, CMD_HARDPM_OFF, loopDepth, mml, compileErr);
								isAbnormalState = false;
								break;
							}
							putSeq(&tracks, CMD_HARDPM_ON, loopDepth, mml, compileErr);
						}
						isAbnormalState = false;
						break;


					/******************************************/
					/* ポルタメント有無効切替                 */
					/******************************************/
					case 'p':
						{
							int nums;
							mmlgetforward(mml);
							nums = getNumbers(mml, false, tempVal, compileErrList);
							if(1 < nums)
							{
								newError(mml, compileErr, compileErrList);
								compileErr->type = SyntaxError;
								compileErr->level = ERR_ERROR;
								sprintf(compileErr->message, "Invalid portament specify.");
								addError(compileErr, compileErrList);
								continue;
							}
							if(1 == nums && tempVal[0] == 0)
							{
								putSeq(&tracks, CMD_PORTAM_OFF, loopDepth, mml, compileErr);
								isAbnormalState = false;
								break;
							}
							putSeq(&tracks, CMD_PORTAM_ON, loopDepth, mml, compileErr);
						}
						isAbnormalState = false;
						break;

					/******************************************/
					/* ドラムセット有無効切替                 */
					/******************************************/
					case 'd':
						{
							int nums;
							mmlgetforward(mml);
							nums = getNumbers(mml, false, tempVal, compileErrList);
							if(1 < nums)
							{
								newError(mml, compileErr, compileErrList);
								compileErr->type = SyntaxError;
								compileErr->level = ERR_ERROR;
								sprintf(compileErr->message, "Invalid drumset specify.");
								addError(compileErr, compileErrList);
								continue;
							}
							if(1 == nums && tempVal[0] == 0)
							{
								tracks.drumPart[tracks.curTrack] = false;
								isAbnormalState = false;
								break;
							}
							tracks.drumPart[tracks.curTrack] = true;
						}
						isAbnormalState = false;
						break;

					/******************************************/
					/* トレモロ                               */
					/******************************************/
					case 'v':
						{
							int nums;
							int delay;
							int baseptr;

							delay = 0;
							baseptr = 0;

							mmlgetforward(mml);
							nums = getNumbers(mml, false, tempVal, compileErrList);
							if((nums == 1 && tempVal[0] == 0))
							{
								/* トレモロOFFコマンド挿入 */
								putSeq(&tracks, CMD_TREMOLO_OFF, loopDepth, mml, compileErr);
								isAbnormalState = false;
								break;
							}
							if(2 > nums || 3 < nums)
							{
								newError(mml, compileErr, compileErrList);
								compileErr->type = SyntaxError;
								compileErr->level = ERR_ERROR;
								sprintf(compileErr->message, "Invalid modulation specifyed.");
								addError(compileErr, compileErrList);
								continue;
							}
							if(nums == 3)
							{
								delay = tempVal[0];
								baseptr = 1;
							}
							/* トレモロコマンド挿入 */
							putSeq(&tracks, CMD_TREMOLO, loopDepth, mml, compileErr);
							putSeq(&tracks, delay, loopDepth, mml, compileErr);
							putSeq(&tracks, tempVal[baseptr], loopDepth, mml, compileErr);
							putSeq(&tracks, tempVal[baseptr+1], loopDepth, mml, compileErr);
							isAbnormalState = false;
							break;
						}

					default:
						newError(mml, compileErr, compileErrList);
						compileErr->type = SyntaxError;
						compileErr->level = ERR_ERROR;
						sprintf(compileErr->message, "Invalid sub-cmd(%%%c).", readValue);
						addError(compileErr, compileErrList);
						break;
				}
				break;

			/******************************************/
			/* コメント／ループ                       */
			/******************************************/
			case '/':
				readValue = mmlgetch(mml);
				switch(readValue)
				{
					case '/':  /* 行コメント */
						/* ログ出力 */
						newError(mml, compileErr, compileErrList);
						compileErr->type = ErrorNone;
						compileErr->level = ERR_DEBUG;
						sprintf(compileErr->message, "Line comment");
						addError(compileErr, compileErrList);

						isLineComment = true;
						mmlgetforward(mml);
						isAbnormalState = false;
						break;

					case '*':  /* 複数行コメント */
						/* ログ出力 */
						newError(mml, compileErr, compileErrList);
						compileErr->type = ErrorNone;
						compileErr->level = ERR_DEBUG;
						sprintf(compileErr->message, "Multi line comment start");
						addError(compileErr, compileErrList);

						isMutiLineComment = true;
						mmlgetforward(mml);
						isAbnormalState = false;
						break;

					default:  /* ループポイント */
						/* ログ出力 */
						newError(mml, compileErr, compileErrList);
						compileErr->type = ErrorNone;
						compileErr->level = ERR_DEBUG;
						sprintf(compileErr->message, "LoopPoint");
						addError(compileErr, compileErrList);

						if(0 > tracks.curTrack)
						{
							newError(mml, compileErr, compileErrList);
							compileErr->type = SyntaxError;
							compileErr->level = ERR_ERROR;
							sprintf(compileErr->message, "Track undefined.");
							addError(compileErr, compileErrList);
							continue;
						}

						if(0xffff != tracks.loopAddr[tracks.curTrack])
						{
							newError(mml, compileErr, compileErrList);
							compileErr->type = SyntaxError;
							compileErr->level = ERR_ERROR;
							sprintf(compileErr->message, "Loop point is already exists.");
							addError(compileErr, compileErrList);
							continue;
						}
						tracks.loopAddr[tracks.curTrack] = tracks.track[tracks.curTrack].ptr;

						/* 次ループ時に音の長さが必ず */
						/* 一致するようにする為、     */
						/* gatetime値をリセットする   */
						tracks.lastGatetime[tracks.curTrack] = -1;

						isAbnormalState = false;
						break;
				}
				break;

			/******************************************/
			/* オクターブ指定                         */
			/******************************************/
			case 'o':
				/* ログ出力 */
				newError(mml, compileErr, compileErrList);
				compileErr->type = ErrorNone;
				compileErr->level = ERR_DEBUG;
				sprintf(compileErr->message, "Set octave");
				addError(compileErr, compileErrList);

				if(1 != getNumbers(mml, false, tempVal, compileErrList))
				{
					newError(mml, compileErr, compileErrList);
					compileErr->type = SyntaxError;
					compileErr->level = ERR_ERROR;
					sprintf(compileErr->message, "Invalid octave specifyed.");
					addError(compileErr, compileErrList);
					continue;
				}
				
				tracks.curOctave[tracks.curTrack] = tempVal[0];
				isAbnormalState = false;
				break;

			/******************************************/
			/* オクターブDOWN/UP                      */
			/******************************************/
			case '<':
				/* ログ出力 */
				newError(mml, compileErr, compileErrList);
				compileErr->type = ErrorNone;
				compileErr->level = ERR_DEBUG;
				sprintf(compileErr->message, "octave down(up)");
				addError(compileErr, compileErrList);

				if(octaveSwap)
				{
					tracks.curOctave[tracks.curTrack]++;
				}
				else
				{
					tracks.curOctave[tracks.curTrack]--;
				}
				isAbnormalState = false;
				break;
			/******************************************/
			case '>':
				/* ログ出力 */
				newError(mml, compileErr, compileErrList);
				compileErr->type = ErrorNone;
				compileErr->level = ERR_DEBUG;
				sprintf(compileErr->message, "octave up(down)");
				addError(compileErr, compileErrList);

				if(octaveSwap)
				{
					tracks.curOctave[tracks.curTrack]--;
				}
				else
				{
					tracks.curOctave[tracks.curTrack]++;
				}
				isAbnormalState = false;
				break;

			/******************************************/
			/* 音色変更                               */
			/******************************************/
			case '@':
			{
				int tone;
				/* ログ出力 */
				newError(mml, compileErr, compileErrList);
				compileErr->type = ErrorNone;
				compileErr->level = ERR_DEBUG;
				sprintf(compileErr->message, "Set instrument");
				addError(compileErr, compileErrList);

				readValue = mmlgetch(mml);
				switch(readValue)
				{
					/* プリセット音色 */
					case '@':
						mmlgetforward(mml);
						if(1 != getNumbers(mml, false, tempVal, compileErrList))
						{
							newError(mml, compileErr, compileErrList);
							compileErr->type = SyntaxError;
							compileErr->level = ERR_ERROR;
							sprintf(compileErr->message, "Invalid tone specifyed.");
							addError(compileErr, compileErrList);
							continue;
						}
						if(EXTONE_START <= tempVal[0] || 0 > tempVal[0])
						{
							newError(mml, compileErr, compileErrList);
							compileErr->type = SyntaxError;
							compileErr->level = ERR_ERROR;
							sprintf(compileErr->message, "Invalid default tone specifyed.");
							addError(compileErr, compileErrList);
							continue;
						}
						tone = tempVal[0];
						break;

					/* シーケンス付随音色 */
					default:
						if(1 != getNumbers(mml, false, tempVal, compileErrList))
						{
							newError(mml, compileErr, compileErrList);
							compileErr->type = SyntaxError;
							compileErr->level = ERR_ERROR;
							sprintf(compileErr->message, "Invalid tone specifyed.");
							addError(compileErr, compileErrList);
							continue;
						}
						if((0xe0 - EXTONE_START) <= tempVal[0] || 0 > tempVal[0])
						{
							newError(mml, compileErr, compileErrList);
							compileErr->type = SyntaxError;
							compileErr->level = ERR_ERROR;
							sprintf(compileErr->message, "Invalid tone specifyed.");
							addError(compileErr, compileErrList);
							continue;
						}
						tone = tempVal[0] + EXTONE_START;
						tracks.transpose[tracks.curTrack] = (char)toneTranspose.data[tempVal[0]];
						break;
				}

				/* コマンド配置 */
				putSeq(&tracks, CMD_SET_INST, loopDepth, mml, compileErr);
				putSeq(&tracks, tone, loopDepth, mml, compileErr);
				isAbnormalState = false;
				break;
			}

			/******************************************/
			/* ゲートタイム                           */
			/******************************************/
			case 'q':
				/* ログ出力 */
				newError(mml, compileErr, compileErrList);
				compileErr->type = ErrorNone;
				compileErr->level = ERR_DEBUG;
				sprintf(compileErr->message, "Gatetime");
				addError(compileErr, compileErrList);

				if(1 != getNumbers(mml, false, tempVal, compileErrList))
				{
					newError(mml, compileErr, compileErrList);
					compileErr->type = SyntaxError;
					compileErr->level = ERR_ERROR;
					sprintf(compileErr->message, "Invalid track assign.");
					addError(compileErr, compileErrList);
					continue;
				}
				if(0 >= tempVal[0] || 8 < tempVal[0])
				{
					newError(mml, compileErr, compileErrList);
					compileErr->type = SyntaxError;
					compileErr->level = ERR_ERROR;
					sprintf(compileErr->message, "Invalid gatetime(%d).", tempVal[0]);
					addError(compileErr, compileErrList);
					continue;
				}
				tracks.gatetime[tracks.curTrack] = tempVal[0] - 1;
				tracks.lastGatetime[tracks.curTrack] = -1;
				isAbnormalState = false;
				break;

			/******************************************/
			/* パンポット                             */
			/******************************************/
			case 'p':
				{
					int nums;
					byte pan;

					/* ログ出力 */
					newError(mml, compileErr, compileErrList);
					compileErr->type = ErrorNone;
					compileErr->level = ERR_DEBUG;
					sprintf(compileErr->message, "Panpot");
					addError(compileErr, compileErrList);
					
					nums = getNumbers(mml, false, tempVal, compileErrList);
					if(0 >= nums || 2 < nums)
					{
						newError(mml, compileErr, compileErrList);
						compileErr->type = SyntaxError;
						compileErr->level = ERR_ERROR;
						sprintf(compileErr->message, "Invalid panpot specifyed.");
						addError(compileErr, compileErrList);
						continue;
					}

					if(nums == 1)
					{
						pan = (tempVal[0] & 0x3f);
						if(true == isReverseStereo)
						{
							pan ^= 0x3f;
							pan++;
							pan &= 0x3f;
						}
						/* サラウンド */
						if(true == tracks.lvolRev[tracks.curTrack])
						{
							pan |= 0x80;
						}
						if(true == tracks.rvolRev[tracks.curTrack])
						{
							pan |= 0x40;
						}

						/* パンコマンド挿入 */
						putSeq(&tracks, CMD_PAN, loopDepth, mml, compileErr);
						putSeq(&tracks, pan, loopDepth, mml, compileErr);
						isAbnormalState = false;
						break;
					}

					/* パンフェードコマンド挿入 */
					pan = (tempVal[1] & 0x3f);
					if(true == isReverseStereo)
					{
						pan ^= 0x3f;
						pan++;
						pan &= 0x3f;
					}
					putSeq(&tracks, CMD_PAN_FADE, loopDepth, mml, compileErr);
					putSeq(&tracks, tempVal[0], loopDepth, mml, compileErr);
					putSeq(&tracks, pan, loopDepth, mml, compileErr);
					isAbnormalState = false;
					break;
				}

			/******************************************/
			/* パン振動                               */
			/******************************************/
			case 'P':
				{
					int nums;
					/* ログ出力 */
					newError(mml, compileErr, compileErrList);
					compileErr->type = ErrorNone;
					compileErr->level = ERR_DEBUG;
					sprintf(compileErr->message, "Panpot vibration");
					addError(compileErr, compileErrList);
					
					nums = getNumbers(mml, false, tempVal, compileErrList);
					if(1 > nums || 2 < nums)
					{
						newError(mml, compileErr, compileErrList);
						compileErr->type = SyntaxError;
						compileErr->level = ERR_ERROR;
						sprintf(compileErr->message, "Invalid panpot vibration specifyed.");
						addError(compileErr, compileErrList);
						continue;
					}

					if(nums == 1)
					{
						if(0 != tempVal[0])
						{
							newError(mml, compileErr, compileErrList);
							compileErr->type = SyntaxError;
							compileErr->level = ERR_ERROR;
							sprintf(compileErr->message, "Invalid panpot vibration specifyed.");
							addError(compileErr, compileErrList);
							continue;
						}

						/* パンコマンド挿入 */
						putSeq(&tracks, CMD_PAN_VIBRATION_OFF, loopDepth, mml, compileErr);
						isAbnormalState = false;
						break;
					}

					/* パン振動コマンド挿入 */
					putSeq(&tracks, CMD_PAN_VIBRATION, loopDepth, mml, compileErr);
					putSeq(&tracks, tempVal[0], loopDepth, mml, compileErr);
					putSeq(&tracks, tempVal[1], loopDepth, mml, compileErr);
					isAbnormalState = false;
					break;
				}



			/******************************************/
			/* 音量                                   */
			/******************************************/
			case 'v':  /* チャンネル音量 */
			case 'V':  /* マスタ音量 */
				{
					int nums;
					/* ログ出力 */
					newError(mml, compileErr, compileErrList);
					compileErr->type = ErrorNone;
					compileErr->level = ERR_DEBUG;
					if(readValue == 'v')
					{
						sprintf(compileErr->message, "Volume");
					}
					else
					{
						sprintf(compileErr->message, "Master volume");
					}
					addError(compileErr, compileErrList);

					nums = getNumbers(mml, false, tempVal, compileErrList);
					if(0 >= nums || 2 < nums)
					{
						newError(mml, compileErr, compileErrList);
						compileErr->type = SyntaxError;
						compileErr->level = ERR_ERROR;
						if(readValue == 'v')
						{
							sprintf(compileErr->message, "Invalid volume specifyed.");
						}
						else
						{
							sprintf(compileErr->message, "Invalid master volume specifyed.");
						}
						addError(compileErr, compileErrList);
						continue;
					}

					if(nums == 1)
					{
						/* ボリュームコマンド挿入 */
						if(readValue == 'v')
						{
							putSeq(&tracks, CMD_VOLUME, loopDepth, mml, compileErr);
						}
						else
						{
							putSeq(&tracks, CMD_GVOLUME, loopDepth, mml, compileErr);
						}
						putSeq(&tracks, tempVal[0], loopDepth, mml, compileErr);
						break;
					}

					/* ボリュームフェードコマンド挿入 */
					if(readValue == 'v')
					{
						putSeq(&tracks, CMD_VOLUME_FADE, loopDepth, mml, compileErr);
					}
					else
					{
						putSeq(&tracks, CMD_GVOLUME_FADE, loopDepth, mml, compileErr);
					}
					putSeq(&tracks, tempVal[0], loopDepth, mml, compileErr);
					putSeq(&tracks, tempVal[1], loopDepth, mml, compileErr);
					isAbnormalState = false;
					break;
				}

			/******************************************/
			/* ベロシティ                             */
			/******************************************/
			case 'k':
				/* ログ出力 */
				newError(mml, compileErr, compileErrList);
				compileErr->type = ErrorNone;
				compileErr->level = ERR_DEBUG;
				sprintf(compileErr->message, "Velocity");
				addError(compileErr, compileErrList);

				if(1 != getNumbers(mml, false, tempVal, compileErrList))
				{
					newError(mml, compileErr, compileErrList);
					compileErr->type = SyntaxError;
					compileErr->level = ERR_ERROR;
					sprintf(compileErr->message, "Invalid track assign.");
					addError(compileErr, compileErrList);
					continue;
				}
				if(0 >= tempVal[0] || 16 < tempVal[0])
				{
					newError(mml, compileErr, compileErrList);
					compileErr->type = SyntaxError;
					compileErr->level = ERR_ERROR;
					sprintf(compileErr->message, "Invalid gatetime(%d).", tempVal[0]);
					addError(compileErr, compileErrList);
					continue;
				}
				tracks.velocity[tracks.curTrack] = tempVal[0] - 1;
				tracks.lastVelocity[tracks.curTrack] = -1;
				isAbnormalState = false;
				break;

			/******************************************/
			/* モジュレーション                       */
			/******************************************/
			case 'm':
				{
					int nums;
					int delay;
					int baseptr;

					delay = 0;
					baseptr = 0;

					/* ログ出力 */
					newError(mml, compileErr, compileErrList);
					compileErr->type = ErrorNone;
					compileErr->level = ERR_DEBUG;
					sprintf(compileErr->message, "Modulation");
					addError(compileErr, compileErrList);

					nums = getNumbers(mml, false, tempVal, compileErrList);
					if((nums == 1 && tempVal[0] == 0))
					{
						/* モジュレーションOFFコマンド挿入 */
						putSeq(&tracks, CMD_MODURATION_OFF, loopDepth, mml, compileErr);
						break;
					}
					if(2 > nums || 3 < nums)
					{
						newError(mml, compileErr, compileErrList);
						compileErr->type = SyntaxError;
						compileErr->level = ERR_ERROR;
						sprintf(compileErr->message, "Invalid modulation specifyed.");
						addError(compileErr, compileErrList);
						continue;
					}
					if(nums == 3)
					{
						delay = tempVal[0];
						baseptr = 1;
					}
					/* モジュレーションコマンド挿入 */
					putSeq(&tracks, CMD_MODURATION, loopDepth, mml, compileErr);
					putSeq(&tracks, delay, loopDepth, mml, compileErr);
					putSeq(&tracks, tempVal[baseptr], loopDepth, mml, compileErr);
					putSeq(&tracks, tempVal[baseptr+1], loopDepth, mml, compileErr);
					isAbnormalState = false;
					break;
				}

			/******************************************/
			/* テンポ                                 */
			/******************************************/
			case 't':
				{
					int nums;
					double t;
					byte tempo;

					/* ログ出力 */
					newError(mml, compileErr, compileErrList);
					compileErr->type = ErrorNone;
					compileErr->level = ERR_DEBUG;
					sprintf(compileErr->message, "Tempo");
					addError(compileErr, compileErrList);

					nums = getNumbers(mml, false, tempVal, compileErrList);
					if(0 >= nums || 2 < nums)
					{
						newError(mml, compileErr, compileErrList);
						compileErr->type = SyntaxError;
						compileErr->level = ERR_ERROR;
						sprintf(compileErr->message, "Invalid tempo specifyed.");
						addError(compileErr, compileErrList);
						continue;
					}

					if(nums == 1)
					{
						/* bpm -> テンポ値変換                      */
						/* SuperC N-SPCstyleの場合、1cycle = 2ms    */
						/* timer 1cycle 125us * 16count             */
						t = 60.0 / tempVal[0];     /* 4分音符時間 */
						t /= timebase;           /* 1tick時間   */
						t /= (0.000125 * TIMER); /* 割合        */
						tempo = 256.0 / t;

						/* テンポコマンド挿入 */
						putSeq(&tracks, CMD_TEMPO, loopDepth, mml, compileErr);
						putSeq(&tracks, tempo, loopDepth, mml, compileErr);
						isAbnormalState = false;
						break;
					}

					/* テンポフェードコマンド挿入 */

					/* bpm -> テンポ値変換                      */
					/* SuperC N-SPCstyleの場合、1cycle = 2ms    */
					/* timer 1cycle 125us * 16count             */
					t = 60.0 / tempVal[1];     /* 4分音符時間 */
					t /= timebase;           /* 1tick時間   */
					t /= (0.000125 * TIMER); /* 割合        */
					tempo = 256.0 / t;

					putSeq(&tracks, CMD_TEMPO_FADE, loopDepth, mml, compileErr);
					putSeq(&tracks, tempVal[0], loopDepth, mml, compileErr);
					putSeq(&tracks, tempo, loopDepth, mml, compileErr);
					isAbnormalState = false;

					break;
				}

			/******************************************/
			/* マクロ展開                             */
			/******************************************/
			case '$':
			{
				char macroName[MACRO_NAME_MAX];
				int macroNameInx = 0;
				Macro* macSearch = NULL;
				Macro* macAdd = NULL;

				/* マクロが"("で始まっているか確認する */
				readValue = mmlgetforward(mml);
				if('(' != readValue)
				{
					newError(mml, compileErr, compileErrList);
					compileErr->type = SyntaxError;
					compileErr->level = ERR_ERROR;
					sprintf(compileErr->message, "Invalid macro call.");
					addError(compileErr, compileErrList);
					continue;
				}

				/* マクロ名を取得する */
				do
				{
					readValue = mmlgetforward(mml);
					if(')' == readValue)
					{
						break;
					}
					macroName[macroNameInx++] = readValue;
				}while(MACRO_NAME_MAX-1 > macroNameInx);
				macroName[macroNameInx] = '\0';
				if(')' != readValue)
				{
					/* マクロ名が長すぎるか、")"が見つからない */
					newError(mml, compileErr, compileErrList);
					compileErr->type = SyntaxError;
					compileErr->level = ERR_ERROR;
					sprintf(compileErr->message, "Macro name is too long, or ')' is missing (%s).", macroName);
					addError(compileErr, compileErrList);
					continue;
				}

				/* 指定のマクロが定義済みか検索する */
				macSearch = mml->macroRoot;
				while(macSearch != NULL)
				{
					if(0 == strcmp(macroName, macSearch->name))
					{
						break;
					}
					macSearch = macSearch->next;
				}
				if(macSearch == NULL)
				{
					/* 指定されたマクロは定義されていない */
					newError(mml, compileErr, compileErrList);
					compileErr->type = SyntaxError;
					compileErr->level = ERR_ERROR;
					sprintf(compileErr->message, "Macro is not defined(%s).", macroName);
					addError(compileErr, compileErrList);
					continue;
				}

				/* 実行マクロを追加する */
				macAdd = (Macro*)malloc(sizeof(Macro));
				memcpy(macAdd, macSearch, sizeof(Macro));
				if(mml->macroExecRoot == NULL)
				{
					/* 実行マクロをセット */
					mml->macroExecRoot = mml->macroExecCur = macAdd;

					/* ログ出力 */
					newError(mml, compileErr, compileErrList);
					compileErr->type = ErrorNone;
					compileErr->level = ERR_DEBUG;
					sprintf(compileErr->message, "Macro : %s", macroName);
					addError(compileErr, compileErrList);
					break;
				}

				/* マクロをセット */
				mml->macroExecCur->child = macAdd;
				macAdd->parrent = mml->macroExecCur;
				mml->macroExecCur = macAdd;

				/* ログ出力 */
				newError(mml, compileErr, compileErrList);
				compileErr->type = ErrorNone;
				compileErr->level = ERR_DEBUG;
				sprintf(compileErr->message, "Macro : %s", macroName);
				addError(compileErr, compileErrList);

				isAbnormalState = false;
				break;
			}

			/******************************************/
			/* コマンド値の直接入力                   */
			/******************************************/
			case 'X':
				/* ログ出力 */
				newError(mml, compileErr, compileErrList);
				compileErr->type = ErrorNone;
				compileErr->level = ERR_DEBUG;
				sprintf(compileErr->message, "Direct insert value");
				addError(compileErr, compileErrList);

				if(1 != getNumbers(mml, false, tempVal, compileErrList))
				{
					newError(mml, compileErr, compileErrList);
					compileErr->type = SyntaxError;
					compileErr->level = ERR_ERROR;
					sprintf(compileErr->message, "Invalid direct insert value.");
					addError(compileErr, compileErrList);
					continue;
				}
				if(255 < tempVal[0])
				{
					newError(mml, compileErr, compileErrList);
					compileErr->type = SyntaxError;
					compileErr->level = ERR_ERROR;
					sprintf(compileErr->message, "Invalid direct insert value.");
					addError(compileErr, compileErrList);
					continue;
				}
				putSeq(&tracks, tempVal[0], loopDepth, mml, compileErr);
				isAbnormalState = false;
				break;

			/******************************************/
			/* 無名サブルーチン開始                   */
			/******************************************/
			case '[':
				{
					int subaddr;
					int trk;

					/* ログ出力 */
					newError(mml, compileErr, compileErrList);
					compileErr->type = ErrorNone;
					compileErr->level = ERR_DEBUG;
					sprintf(compileErr->message, "Noname-Subroutine Start");
					addError(compileErr, compileErrList);

					/* サブルーチン深度が2より大きいものはNG */
					if((SUB_TRACKS-1) < loopDepth)
					{
						newError(mml, compileErr, compileErrList);
						compileErr->type = SyntaxError;
						compileErr->level = ERR_ERROR;
						sprintf(compileErr->message, "Loop depth allows till 2.");
						addError(compileErr, compileErrList);
						continue;
					}

					/* サブルーチンコマンド挿入 */
					subaddr = tracks.track[TRACKS+1+loopDepth].ptr;
					putSeq(&tracks, CMD_SUBROUTINE, loopDepth, mml, compileErr);
					putSeq(&tracks, 0, loopDepth, mml, compileErr);
					putSeq(&tracks, subaddr, loopDepth, mml, compileErr);
					putSeq(&tracks, (subaddr>>8), loopDepth, mml, compileErr);

					/* サブルーチン深度アップ */
					loopDepth++;
					trk = (0>=loopDepth ? tracks.curTrack : TRACKS);

					/* サブルーチン開始アドレス記憶 */
					lastSub[loopDepth].label = -1; /* -1 = ラベルなし */
					lastSub[loopDepth].depth = loopDepth;
					lastSub[loopDepth].addr = subaddr;
					subCallAddr[loopDepth] = tracks.track[trk].ptr-4;

					/* サブルーチン呼び元記憶 */
					if(false == addSubroutineListData(&subs, loopDepth, trk, tracks.track[trk].ptr-4))
					{
						/* メモリ確保エラー */
						newError(mml, compileErr, compileErrList); /* メモリ確保できなかった場合、たぶんこの処理もコケるのであまり意味ない */
						compileErr->type = SyntaxError;
						compileErr->level = ERR_FATAL;
						sprintf(compileErr->message, "addSubroutineListData : Memory alloc error.");
						addError(compileErr, compileErrList);
						deleteSubroutineList(&subs);
						deleteBrrListData(bList);
						bList = NULL;
						deleteLabelNode(labels);
						return false;
					}

					/* サブルーチンを抜けた後の音の長さが */
					/* 一定になるようにする               */
					tracks.forceTickOut[tracks.curTrack] = true;
					isAbnormalState = false;
					break;
				}

			/******************************************/
			/* サブルーチン末端                       */
			/******************************************/
			case ']':
				{
					int trk;

					/* ログ出力 */
					newError(mml, compileErr, compileErrList);
					compileErr->type = ErrorNone;
					compileErr->level = ERR_DEBUG;
					sprintf(compileErr->message, "Noname-Subroutine Start");
					addError(compileErr, compileErrList);

					/* サブルーチン解析中ではない場合、NG */
					if(0 > loopDepth)
					{
						newError(mml, compileErr, compileErrList);
						compileErr->type = SyntaxError;
						compileErr->level = ERR_ERROR;
						sprintf(compileErr->message, "']' : Subroutine is not started.");
						addError(compileErr, compileErrList);
						continue;
					}

					/* ループ回数の入力チェック */
					if(1 != getNumbers(mml, false, tempVal, compileErrList))
					{
						newError(mml, compileErr, compileErrList);
						compileErr->type = SyntaxError;
						compileErr->level = ERR_ERROR;
						sprintf(compileErr->message, "Invalid loop num.");
						addError(compileErr, compileErrList);
						continue;
					}
					if(256 < tempVal[0] || 1 > tempVal[0])
					{
						newError(mml, compileErr, compileErrList);
						compileErr->type = SyntaxError;
						compileErr->level = ERR_ERROR;
						sprintf(compileErr->message, "Invalid loop num.");
						addError(compileErr, compileErrList);
						continue;
					}

					/* ループ回数の書き出し */
					trk = (0>=loopDepth ? tracks.curTrack : TRACKS);
					tracks.track[trk].data[subCallAddr[loopDepth]+1] = tempVal[0]-1;

					/* サブルーチンの使いまわしをセット */
					if(0 > lastSub[loopDepth].label)
					{
						latestSub = &lastSub[loopDepth];
					}
					else
					{
						stLabelNode* node;
						node = insertLabelNode(labels, lastSub[loopDepth].label, lastSub[loopDepth].depth, lastSub[loopDepth].addr, mml, compileErrList);
						if(NULL == node)
						{
							newError(mml, compileErr, compileErrList);
							compileErr->type = MemoryError;
							compileErr->level = ERR_FATAL;
							sprintf(compileErr->message, "Label insert failed.");
							addError(compileErr, compileErrList);
							return false;
						}
						if(NULL == labels)
						{
							labels = node;
						}
					}

					/* サブルーチン終了時の値を記憶 */
					if(0 > lastSub[loopDepth].lastoctave)
					{
						lastSub[loopDepth].lastoctave = tracks.curOctave[tracks.curTrack];
						lastSub[loopDepth].lastticks = tracks.ticks[tracks.curTrack];
						lastSub[loopDepth].lastvelocity = tracks.lastVelocity[tracks.curTrack];
						lastSub[loopDepth].lastgatetime = tracks.lastGatetime[tracks.curTrack];
					}

					/* サブルーチン末端コマンド書き込み */
					putSeq(&tracks, CMD_SUBROUTINE_RETURN, loopDepth, mml, compileErr);

					/* サブルーチン深度ダウン */
					loopDepth--;

					isAbnormalState = false;
					break;
				}

			/******************************************/
			/* サブルーチンの途中終了                 */
			/******************************************/
			case '|':
				/* ログ出力 */
				newError(mml, compileErr, compileErrList);
				compileErr->type = ErrorNone;
				compileErr->level = ERR_DEBUG;
				sprintf(compileErr->message, "Noname-Subroutine Start");
				addError(compileErr, compileErrList);

				/* サブルーチン解析中ではない場合、NG */
				if(0 > loopDepth)
				{
					newError(mml, compileErr, compileErrList);
					compileErr->type = SyntaxError;
					compileErr->level = ERR_ERROR;
					sprintf(compileErr->message, "']' : Subroutine is not started.");
					addError(compileErr, compileErrList);
					continue;
				}

				/* サブルーチン終了時の値を記憶 */
				if(0 > lastSub[loopDepth].lastoctave)
				{
					lastSub[loopDepth].lastoctave = tracks.curOctave[tracks.curTrack];
					lastSub[loopDepth].lastticks = tracks.ticks[tracks.curTrack];
					lastSub[loopDepth].lastvelocity = tracks.lastVelocity[tracks.curTrack];
					lastSub[loopDepth].lastgatetime = tracks.lastGatetime[tracks.curTrack];
				}

				/* コマンド挿入 */
				putSeq(&tracks, CMD_SUBROUTINE_BREAK, loopDepth, mml, compileErr);
				isAbnormalState = false;
				break;

			/******************************************/
			/* サブルーチンの再利用                   */
			/******************************************/
			case '*':
				{
					int trk;

					/* ログ出力 */
					newError(mml, compileErr, compileErrList);
					compileErr->type = ErrorNone;
					compileErr->level = ERR_DEBUG;
					sprintf(compileErr->message, "reuse subroutine");
					addError(compileErr, compileErrList);

					/* 未定義チェック */
					if(NULL == latestSub)
					{
						newError(mml, compileErr, compileErrList);
						compileErr->type = SyntaxError;
						compileErr->level = ERR_ERROR;
						sprintf(compileErr->message, "'*' Noname-Subroutine not defined.");
						addError(compileErr, compileErrList);
						continue;
					}

					/* ループ回数の入力チェック */
					if(1 != getNumbers(mml, false, tempVal, compileErrList))
					{
						newError(mml, compileErr, compileErrList);
						compileErr->type = SyntaxError;
						compileErr->level = ERR_ERROR;
						sprintf(compileErr->message, "Invalid loop num.");
						addError(compileErr, compileErrList);
						continue;
					}
					if(256 < tempVal[0] || 1 > tempVal[0])
					{
						newError(mml, compileErr, compileErrList);
						compileErr->type = SyntaxError;
						compileErr->level = ERR_ERROR;
						sprintf(compileErr->message, "Invalid loop num.");
						addError(compileErr, compileErrList);
						continue;
					}

					/* サブルーチンコマンド挿入 */
					putSeq(&tracks, CMD_SUBROUTINE, loopDepth, mml, compileErr);
					putSeq(&tracks, (tempVal[0]-1), loopDepth, mml, compileErr);
					putSeq(&tracks, latestSub->addr, loopDepth, mml, compileErr);
					putSeq(&tracks, (latestSub->addr>>8), loopDepth, mml, compileErr);

					/* サブルーチン終了後の各種値の復元 */
					if(true == latestSub->noteExists)
					{
						tracks.curOctave[tracks.curTrack] = latestSub->lastoctave;
					}
					tracks.ticks[tracks.curTrack] = latestSub->lastticks;
					if( (true == latestSub->noteExists) || (true == latestSub->tieExists) )
					{
						tracks.velocity[tracks.curTrack] = latestSub->lastvelocity;
						tracks.lastVelocity[tracks.curTrack] = latestSub->lastvelocity;
						tracks.gatetime[tracks.curTrack] = latestSub->lastgatetime;
						tracks.lastGatetime[tracks.curTrack] = latestSub->lastgatetime;
					}

					/* サブルーチン呼び元記憶　*/
					trk = (0>loopDepth ? tracks.curTrack : TRACKS);
					if(false == addSubroutineListData(&subs, latestSub->depth, trk, tracks.track[trk].ptr-4))
					{
						/* メモリ確保エラー */
						newError(mml, compileErr, compileErrList); /* メモリ確保できなかった場合、たぶんこの処理もコケるのであまり意味ない */
						compileErr->type = SyntaxError;
						compileErr->level = ERR_FATAL;
						sprintf(compileErr->message, "addSubroutineListData : Memory alloc error.");
						addError(compileErr, compileErrList);
						deleteSubroutineList(&subs);
						deleteBrrListData(bList);
						deleteLabelNode(labels);
						bList = NULL;
						return false;
					}

					/* サブルーチンを抜けた後の音の長さが */
					/* 一定になるようにする               */
					tracks.forceTickOut[tracks.curTrack] = true;

					isAbnormalState = false;
					break;
				}

			/******************************************/
			/* ラベルループ / リモートコマンド        */
			/******************************************/
			case '(':
			{
				int nums;
				int subaddr;
				int trk;
				int label;

				if('!' == mmlgetch(mml)) /* リモートコマンド */
				{
					newError(mml, compileErr, compileErrList);
					compileErr->type = SyntaxError;
					compileErr->level = ERR_ERROR;
					sprintf(compileErr->message, "REMOTE COMMAND IS NOT IMPLEMENT.");
					addError(compileErr, compileErrList);
					break;
				}

				/* ラベルループ */
				newError(mml, compileErr, compileErrList);
				compileErr->type = ErrorNone;
				compileErr->level = ERR_DEBUG;
				sprintf(compileErr->message, "Label loop");
				addError(compileErr, compileErrList);

				/* ラベルの数字チェック */
				if(1 != getNumbers(mml, false, tempVal, compileErrList))
				{
					newError(mml, compileErr, compileErrList);
					compileErr->type = SyntaxError;
					compileErr->level = ERR_ERROR;
					sprintf(compileErr->message, "Invalid label number.");
					addError(compileErr, compileErrList);
					continue;
				}
				/* ラベルが閉じられているかチェック */
				if(')' != mmlgetch(mml))
				{
					newError(mml, compileErr, compileErrList);
					compileErr->type = SyntaxError;
					compileErr->level = ERR_ERROR;
					sprintf(compileErr->message, "Label is not closed.");
					addError(compileErr, compileErrList);
					continue;
				}
				/* 負数ラベルはNG */
				if(0 > tempVal[0])
				{
					newError(mml, compileErr, compileErrList);
					compileErr->type = SyntaxError;
					compileErr->level = ERR_ERROR;
					sprintf(compileErr->message, "Invalid label number.");
					addError(compileErr, compileErrList);
					continue;
				}
				mmlgetforward(mml);
				label = tempVal[0];

				/* ラベルの後の数字入力をチェック */
				nums = getNumbers(mml, false, tempVal, compileErrList);
				if(1 < nums)
				{
					/* "(1)1,1" 等はNG */
					newError(mml, compileErr, compileErrList);
					compileErr->type = SyntaxError;
					compileErr->level = ERR_ERROR;
					sprintf(compileErr->message, "Invalid label number.");
					addError(compileErr, compileErrList);
					continue;
				}

				if(nums == 1)
				{
					/* ラベルループの再利用 */
					stLabelNode* node;

					/* ラベルの定義があるかチェックする */
					node = searchLabelNode(labels, label);
					if(NULL == node)
					{
						newError(mml, compileErr, compileErrList);
						compileErr->type = SyntaxError;
						compileErr->level = ERR_ERROR;
						sprintf(compileErr->message, "label '(%d)' not defined.", label);
						addError(compileErr, compileErrList);
						continue;
					}
					/* ループ回数の入力チェック */
					if(256 < tempVal[0] || 1 > tempVal[0])
					{
						newError(mml, compileErr, compileErrList);
						compileErr->type = SyntaxError;
						compileErr->level = ERR_ERROR;
						sprintf(compileErr->message, "Invalid loop num.");
						addError(compileErr, compileErrList);
						continue;
					}

					/* サブルーチン終了後の各種値の復元 */
					if(true == node->noteExists)
					{
						tracks.curOctave[tracks.curTrack] = node->lastoctave;
					}
					tracks.ticks[tracks.curTrack] = node->lastticks;
					if( (true == node->noteExists) || (true == node->tieExists) )
					{
						tracks.velocity[tracks.curTrack] = node->lastvelocity;
						tracks.lastVelocity[tracks.curTrack] = node->lastvelocity;
						tracks.gatetime[tracks.curTrack] = node->lastgatetime;
						tracks.lastGatetime[tracks.curTrack] = node->lastgatetime;
					}

					/* サブルーチンコマンド挿入 */
					putSeq(&tracks, CMD_SUBROUTINE, loopDepth, mml, compileErr);
					putSeq(&tracks, (tempVal[0]-1), loopDepth, mml, compileErr);
					putSeq(&tracks, node->addr, loopDepth, mml, compileErr);
					putSeq(&tracks, (node->addr>>8), loopDepth, mml, compileErr);

					/* サブルーチン呼び元記憶　*/
					trk = (0>loopDepth ? tracks.curTrack : TRACKS);
					if(false == addSubroutineListData(&subs, node->depth, trk, tracks.track[trk].ptr-4))
					{
						/* メモリ確保エラー */
						newError(mml, compileErr, compileErrList); /* メモリ確保できなかった場合、たぶんこの処理もコケるのであまり意味ない */
						compileErr->type = SyntaxError;
						compileErr->level = ERR_FATAL;
						sprintf(compileErr->message, "addSubroutineListData : Memory alloc error.");
						addError(compileErr, compileErrList);
						deleteSubroutineList(&subs);
						deleteBrrListData(bList);
						deleteLabelNode(labels);
						bList = NULL;
						return false;
					}
					/* サブルーチンを抜けた後の音の長さが */
					/* 一定になるようにする               */
					tracks.forceTickOut[tracks.curTrack] = true;
					isAbnormalState = false;
					break;
				}

				/* あまりないと思うが、"(1) [cccc]" 等、ラベル後のスペース入力に対応する */
				skipspaces(mml);

				/* ラベルが開始されているかチェック */
				if('[' != mmlgetch(mml))
				{
					newError(mml, compileErr, compileErrList);
					compileErr->type = SyntaxError;
					compileErr->level = ERR_ERROR;
					sprintf(compileErr->message, "Invalid label.");
					addError(compileErr, compileErrList);
					continue;
				}
				mmlgetforward(mml);

				/* サブルーチン深度が2より大きいものはNG */
				if((SUB_TRACKS-1) < loopDepth)
				{
					newError(mml, compileErr, compileErrList);
					compileErr->type = SyntaxError;
					compileErr->level = ERR_ERROR;
					sprintf(compileErr->message, "Loop depth allows till 2.");
					addError(compileErr, compileErrList);
					continue;
				}

				/* サブルーチンコマンド挿入 */
				subaddr = tracks.track[TRACKS+1+loopDepth].ptr;
				putSeq(&tracks, CMD_SUBROUTINE, loopDepth, mml, compileErr);
				putSeq(&tracks, 0, loopDepth, mml, compileErr);
				putSeq(&tracks, subaddr, loopDepth, mml, compileErr);
				putSeq(&tracks, (subaddr>>8), loopDepth, mml, compileErr);

				/* サブルーチン深度アップ */
				loopDepth++;
				trk = (0>=loopDepth ? tracks.curTrack : TRACKS);

				/* サブルーチン開始アドレス記憶 */
				lastSub[loopDepth].label = tempVal[0];
				lastSub[loopDepth].depth = loopDepth;
				lastSub[loopDepth].addr = subaddr;
				subCallAddr[loopDepth] = tracks.track[trk].ptr-4;

				/* サブルーチン呼び元記憶 */
				if(false == addSubroutineListData(&subs, loopDepth, trk, tracks.track[trk].ptr-4))
				{
					/* メモリ確保エラー */
					newError(mml, compileErr, compileErrList); /* メモリ確保できなかった場合、たぶんこの処理もコケるのであまり意味ない */
					compileErr->type = SyntaxError;
					compileErr->level = ERR_FATAL;
					sprintf(compileErr->message, "addSubroutineListData : Memory alloc error.");
					addError(compileErr, compileErrList);
					deleteSubroutineList(&subs);
					deleteBrrListData(bList);
					bList = NULL;
					deleteLabelNode(labels);
					return false;
				}

				/* サブルーチンを抜けた後の音の長さが */
				/* 一定になるようにする               */
				tracks.forceTickOut[tracks.curTrack] = true;
				isAbnormalState = false;
				break;
			}

			/******************************************/
			/* 解析の中断                             */
			/******************************************/
			case '!':
				/* ログ出力 */
				newError(mml, compileErr, compileErrList);
				compileErr->type = ErrorNone;
				compileErr->level = ERR_DEBUG;
				sprintf(compileErr->message, "break");
				addError(compileErr, compileErrList);
				goto endAna;

			/******************************************/
			/* 空白文字                               */
			/* 見つけても無視する                     */
			/******************************************/
			case '\n':
			case '\r':
			case ' ':
			case '\t':
			case '\0':
				break;

			/******************************************/
			/* その他コマンド未定義文字               */
			/* 見つけたらSyntax Errorにする           */
			/******************************************/
			default:
				if(false == isAbnormalState)
				{
					newError(mml, compileErr, compileErrList);
					compileErr->type = SyntaxError;
					compileErr->level = ERR_ERROR;
					sprintf(compileErr->message, "Unknown character(%c : 0x%x).", readValue, readValue);
					addError(compileErr, compileErrList);
				}
				break;
		}
	}
	endAna:

	/* 複数行コメントが閉じているかチェックします */
	if(isMutiLineComment == true)
	{
		/* エラーログ出力 */
		newError(mml, compileErr, compileErrList);
		compileErr->type = SyntaxError;
		compileErr->level = ERR_ERROR;
		sprintf(compileErr->message, "Unterminated comment(\"*/\".");
		addError(compileErr, compileErrList);
	}

	/* 文法エラーが無い場合は、バイナリデータとしてまとめる */
	if(ERR_ERROR > getErrorLevel(compileErrList))
	{
		int i;
		word dataptr;
		word sub1, sub2;

		dataptr = (2*TRACKS);
		dataptr += DRUMTABLE_LEN*DRUM_NOTE_NUMS;
		dataptr += toneTable.ptr;

		/* ヘッダ作成 */
		for(i=0; i<TRACKS; i++)
		{
			word empty = 0;
			if(0 == tracks.track[i].ptr)
			{
				/* 空トラックヘッダ書き込み */
				bindataadd(bin, (const byte*)&empty, 2);
				continue;
			}

			/* トラックヘッダ書き込み */
			bindataadd(bin, (const byte*)&dataptr, 2);
			dataptr += tracks.track[i].ptr;

			/* 末端コマンド分移動 */
			dataptr++;
			/* シーケンスがループする場合は更に2バイト移動 */
			if(0xffff != tracks.loopAddr[i])
			{
				dataptr+=2;
			}
		}

		/* サブルーチントラックの開始位置取得 */
		sub1 = dataptr;
		sub2 = dataptr + tracks.track[TRACKS].ptr;

		/* サブルーチンアドレスの変換 */
		if(NULL != subs.root)
		{
			stSubroutineListData *cur;

			cur = subs.root;
			while(cur != NULL)
			{
				*((word *)&tracks.track[cur->track].data[cur->subaddr+2]) += (0 == cur->depth ? sub1 : sub2);
				cur = cur->next;
			}
		}

		/* 波形定義テーブルの書き出し */
		bindataadd(bin, drumTable, DRUMTABLE_LEN*DRUM_NOTE_NUMS);
		bindataadd(bin, (const byte*)toneTable.data, toneTable.ptr);

		/* 変換データのまとめこみ */
		for(i=0; i<TRACKS; i++)
		{
			byte cmd = 0x00;
			word lp = 0x0000;	/* ループポイント */
			word trkst = 0x0000;	/* トラックのスタート位置 */

			/* 空データ判定 */
			if(0 == tracks.track[i].ptr)
			{
				continue;
			}

			/* メインのデータ書き込み */
			trkst = bin->dataInx;
			bindataadd(bin, (const byte*)tracks.track[i].data, tracks.track[i].ptr);
			if(0xffff == tracks.loopAddr[i])
			{
				/* ループなし : シーケンス末端の書き込み */
				bindataadd(bin, (const byte*)&cmd, 1);
				continue;
			}

			/* ループコマンドの書き込み */
			cmd = CMD_JUMP;
			bindataadd(bin, (const byte*)&cmd, 1); 

			/* ループアドレスの算出 */
			lp = trkst + tracks.loopAddr[i];
			/* ループアドレスの書き込み */
			bindataadd(bin, (const byte*)&lp, 2); 
		}
		/* サブルーチントラックの書き出し */
		bindataadd(bin, (const byte*)tracks.track[TRACKS].data, tracks.track[TRACKS].ptr);
		bindataadd(bin, (const byte*)tracks.track[TRACKS+1].data, tracks.track[TRACKS+1].ptr);
	}

	deleteSubroutineList(&subs);
	deleteLabelNode(labels);
	*bl = bList;
	return compileErrList;
}

