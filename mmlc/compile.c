/**
 * SuperC mml compile main code
 */

#include "gstdafx.h"
#include "mmlman.h"
#include "compile.h"

/****************************************
 * 非公開定数
 ****************************************/

/* タイマ設定値 (SuperC アセンブリ定義に合わせる) */
#define TIMER 16

/* トラック数 (SuperC アセンブリ定義に合わせる) */
#define TRACK_MAX 10

/* 変換系関数用のテンポラリバッファ長 */
#define TMP_BUFFER_SIZE 32

/* オクターブ範囲 */
#define OCTAVE_MIN 1
#define OCTAVE_MAX 6

/* timebase MAX値 */
#define TIMEBASE_MAX 0x60

/**
 * コマンド定義
 */
#define TIE  0xc8
#define REST 0xc9
enum commandlist{
	/* ※注 : SuperCのinclude/seqcmd.inc 内の */
	/*        コマンド定義順に合わせること    */
	CMD_SET_INST	= 0xda,
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
};

/**
 * 音階テーブル
 */
static const byte SCALE_TABLE[] = {9, 11, 0, 2, 4, 5, 7};
/*                                {a.  b, c, d, e, f, g} */

/* トラックデータ */
typedef struct stTrack
{
	byte data[TRACK_SIZE_MAX];
	int  ptr;
} Track;
typedef struct stTracksData
{
	word	loopAddr[TRACKS];
	Track	track[TRACKS];
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

	/**
	 * サラウンド指定
	 */
	bool lvolRev[TRACKS];
	bool rvolRev[TRACKS];

	/**
	 * ドラムセット指定
	 */
	bool drumPart[TRACKS];
} TracksData;

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
 * 16進数値を取得する
 * HEX値を取得できなかった場合は-1を返す
 */
int getHex(char c)
{
	c = tolower(c);
	if('0' <= c || c <= '9')
	{
		return (c - '0');
	}
	if('a' <= c || c <= 'f')
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
 *   Numbers : 解析した数値
 *   return  : 取得した引数個数
 */
int getNumbers(MmlMan* mml, int *Numbers, ErrorNode* compileErrList)
{
	/**
	 * 変換後データの数
	 */
	int getCount = 0;

	/**
	 * エラー
	 */
	ErrorNode* compileErr = NULL;

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
			mmlgetforward(mml);
		}

		/* "0001" や"0x11"等の、先頭の"0"を読み飛ばす */
		while(mmlgetch(mml) == '0')
		{
			mmlgetforward(mml);
			zero = true;
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

		/* 次の数値読み込みがあるかどうか */
		if(readValue != ',')
		{
			break;
		}
		mmlgetforward(mml);
	}
	return getCount;
}

/**
 * 音の長さの計算
 *   ticks    : 音の長さ格納用変数へのポインタ
 *   timebase : Timebase値
 *   mml      : mml構造体
 *   return   : 音の長さの取得が正常に完了したか否か
 */
bool calcSteps(int* ticks, int timebase, MmlMan* mml, ErrorNode* compileErrList)
{
	int tmpNums[TMP_BUFFER_SIZE];
	int getNums;
	int noteLen;
	int tick_base;
	int divide_tick;
	int tick_ctr;
	ErrorNode *compileErr = NULL;

	/* "a32" "b" "cx4" 等は許容します。           */
	/* "cx4"はmml的に変なので、隠し仕様とします。 */
	getNums = getNumbers(mml, tmpNums, compileErr);
	if(0 == getNums)
	{
		/* 前回stepを取得できていない場合は、エラー */
		if(255 < *ticks)
		{
			return false;
		}
		return true;
	}
	if(1 < getNums)
	{
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

bool putSeq(TracksData *tracks, const byte data, MmlMan *mml, ErrorNode* compileErrList)
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

	tracks->track[tracks->curTrack].data[tracks->track[tracks->curTrack].ptr++] = data;

	return true;
}

/**
 * mmlをコンパイルして、バイナリデータ化します
 */
ErrorNode* compile(MmlMan* mml, BinMan *bin)
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
	 * mmlから読みだしたデータ
	 */
	char readValue;

	/**
	 * 音階
	 */
	byte scale;

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
		/* 個別のデータ初期化 */
		for(i=0; i<TRACK_MAX; i++)
		{
			tracks.loopAddr[i] = 0xffff;
			tracks.velocity[i] = 15;
			tracks.lastVelocity[i] = -1;
			tracks.gatetime[i] = 7;
			tracks.lastGatetime[i] = -1;
			tracks.curOctave[i] = 3;
			tracks.ticks[i] = 0xffff;
			tracks.lastticks[i] = -1;
			tracks.lvolRev[i] = false;
			tracks.rvolRev[i] = false;
			tracks.drumPart[i] = false;
		}
	}

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
			byte note;

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

				if(false == calcSteps(&tracks.ticks[tracks.curTrack], timebase, mml, compileErr))
				{
					newError(mml, compileErr, compileErrList);
					compileErr->type = SyntaxError;
					compileErr->level = ERR_ERROR;
					sprintf(compileErr->message, "Invalid length prefix.");
					addError(compileErr, compileErrList);
					continue;
				}
				if(0x60 < tracks.ticks[tracks.curTrack])
				{
					newError(mml, compileErr, compileErrList);
					compileErr->type = SyntaxError;
					compileErr->level = ERR_ERROR;
					sprintf(compileErr->message, "Unable length prefix.");
					addError(compileErr, compileErrList);
					continue;
				}
				break;

			/******************************************/
			/* 音階                                   */
			/******************************************/
			case 'a':
			case 'b':
			case 'c':
			case 'd':
			case 'e':
			case 'f':
			case 'g':
				/* ログ出力 */
				newError(mml, compileErr, compileErrList);
				compileErr->type = ErrorNone;
				compileErr->level = ERR_DEBUG;
				sprintf(compileErr->message, "Note %c", readValue);
				addError(compileErr, compileErrList);

				/* 音階の算出 */
				octaveTmp = tracks.curOctave[tracks.curTrack];
				scale = SCALE_TABLE[(readValue - 'a')];

				/* +/- の算出 */
				readValue = mmlgetch(mml);
				if(readValue == '+' || readValue == '#')  /* いちおう半角の#に対応させておく */
				{
					mmlgetforward(mml);
					scale++;
					if(12 <= scale)
					{
						octaveTmp++;
						scale = 0;
					}
				}
				else if(readValue == '-')
				{
					mmlgetforward(mml);
					scale++;
					if(0 > scale)
					{
						octaveTmp--;
						scale = 11;
					}
				}

				/* オクターブチェック */
				if(OCTAVE_MIN > octaveTmp && octaveTmp < OCTAVE_MAX)
				{
					/* ログ出力 */
					newError(mml, compileErr, compileErrList);
					compileErr->type = SyntaxError;
					compileErr->level = ERR_ERROR;
					sprintf(compileErr->message, "Octave range over : o%d", octaveTmp);
					addError(compileErr, compileErrList);
				}

				/* Step算出 */
				tracks.lastticks[tracks.curTrack] = tracks.ticks[tracks.curTrack];
				if(false == calcSteps(&tracks.ticks[tracks.curTrack], timebase, mml, compileErr))
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
				if((tracks.lastticks[tracks.curTrack] != tracks.ticks[tracks.curTrack]) || (tracks.gatetime[tracks.curTrack] != tracks.lastGatetime[tracks.curTrack]) || (tracks.velocity[tracks.curTrack] != tracks.lastVelocity[tracks.curTrack]))
				{
					putSeq(&tracks, tracks.ticks[tracks.curTrack], mml, compileErr);
				}

				/* ゲートタイム/ベロシティ挿入 */
				if((tracks.gatetime[tracks.curTrack] != tracks.lastGatetime[tracks.curTrack]) || (tracks.velocity[tracks.curTrack] != tracks.lastVelocity[tracks.curTrack]))
				{
					byte qv = (tracks.gatetime[tracks.curTrack] << 4) | tracks.velocity[tracks.curTrack];
					putSeq(&tracks, qv, mml, compileErr);
				}

				/* 音譜挿入 */
				note = ((octaveTmp-1)*12 + scale) | 0x80;
				putSeq(&tracks, note, mml, compileErr);

			tie_shared:
				tracks.lastGatetime[tracks.curTrack] = tracks.gatetime[tracks.curTrack];
				tracks.lastVelocity[tracks.curTrack] = tracks.velocity[tracks.curTrack];

				/* 長い音の場合にタイを追加 */
				if(remainTicks != 0)
				{
					tracks.lastticks[tracks.curTrack] = tracks.ticks[tracks.curTrack];
					tracks.ticks[tracks.curTrack] = remainTicks;
					if(tracks.lastticks[tracks.curTrack] != tracks.ticks[tracks.curTrack])
					{
						putSeq(&tracks, tracks.ticks[tracks.curTrack], mml, compileErr);
					}
					putSeq(&tracks, TIE, mml, compileErr);
				}

				break;

			/******************************************/
			/* タイ                                   */
			/******************************************/
			case '^':
				/* ログ出力 */
				newError(mml, compileErr, compileErrList);
				compileErr->type = ErrorNone;
				compileErr->level = ERR_DEBUG;
				sprintf(compileErr->message, "Tie %c ", readValue);
				addError(compileErr, compileErrList);

				/* Step算出 */
				tracks.lastticks[tracks.curTrack] = tracks.ticks[tracks.curTrack];
				if(false == calcSteps(&tracks.ticks[tracks.curTrack], timebase, mml, compileErr))
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
				if((tracks.lastticks[tracks.curTrack] != tracks.ticks[tracks.curTrack]) || (tracks.gatetime[tracks.curTrack] != tracks.lastGatetime[tracks.curTrack]) || (tracks.velocity[tracks.curTrack] != tracks.lastVelocity[tracks.curTrack]))
				{
					putSeq(&tracks, tracks.ticks[tracks.curTrack], mml, compileErr);
				}

				/* ゲートタイム/ベロシティ挿入 */
				if(tracks.gatetime[tracks.curTrack] != tracks.lastGatetime[tracks.curTrack] || tracks.velocity[tracks.curTrack] != tracks.lastVelocity[tracks.curTrack])
				{
					byte qv = (tracks.gatetime[tracks.curTrack] << 4) | tracks.velocity[tracks.curTrack];
					putSeq(&tracks, qv, mml, compileErr);
				}

				/* タイ挿入 */
				putSeq(&tracks, TIE, mml, compileErr);
				goto tie_shared;

			/******************************************/
			/* 休符                                   */
			/******************************************/
			case 'r':
				/* ログ出力 */
				newError(mml, compileErr, compileErrList);
				compileErr->type = ErrorNone;
				compileErr->level = ERR_DEBUG;
				sprintf(compileErr->message, "Rest %c ", readValue);
				addError(compileErr, compileErrList);

				tracks.lastticks[tracks.curTrack] = tracks.ticks[tracks.curTrack];
				if(false == calcSteps(&tracks.ticks[tracks.curTrack], timebase, mml, compileErr))
				{
					newError(mml, compileErr, compileErrList);
					compileErr->type = SyntaxError;
					compileErr->level = ERR_ERROR;
					sprintf(compileErr->message, "Invalid length note.");
					addError(compileErr, compileErrList);
					continue;
				}

				/* 音長挿入 */
				remainTicks = 0;
				if(0x60 < tracks.ticks[tracks.curTrack])
				{
					remainTicks = tracks.ticks[tracks.curTrack] - 0x60;
					tracks.ticks[tracks.curTrack] = 0x60;
				}

				if((tracks.lastticks[tracks.curTrack] != tracks.ticks[tracks.curTrack]) || (tracks.gatetime[tracks.curTrack] != tracks.lastGatetime[tracks.curTrack]) || (tracks.velocity[tracks.curTrack] != tracks.lastVelocity[tracks.curTrack]))
				{
					putSeq(&tracks, tracks.ticks[tracks.curTrack], mml, compileErr);
				}
				/* ゲートタイム/ベロシティ挿入 */
				if(tracks.gatetime[tracks.curTrack] != tracks.lastGatetime[tracks.curTrack] || tracks.velocity[tracks.curTrack] != tracks.lastVelocity[tracks.curTrack])
				{
					byte qv = (tracks.gatetime[tracks.curTrack] << 4) | tracks.velocity[tracks.curTrack];
					putSeq(&tracks, qv, mml, compileErr);
				}
				putSeq(&tracks, REST, mml, compileErr);
				goto tie_shared;


			/******************************************/
			/* コントロール                           */
			/******************************************/
			case '#':
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

					if(0 == getNumbers(mml, tempVal, compileErrList))
					{
						newError(mml, compileErr, compileErrList);
						compileErr->type = SyntaxError;
						compileErr->level = ERR_ERROR;
						sprintf(compileErr->message, "Invalid track assign.");
						addError(compileErr, compileErrList);
						continue;
					}
					if(0 >= tempVal[0] || TRACK_MAX < tempVal[0])
					{
						newError(mml, compileErr, compileErrList);
						compileErr->type = SyntaxError;
						compileErr->level = ERR_ERROR;
						sprintf(compileErr->message, "Invalid track assign(%d).", tempVal[0]);
						addError(compileErr, compileErrList);
						continue;
					}

					/* トラック切り替え */
					tracks.curTrack = tempVal[0]-1;

					/* ログ出力 */
					newError(mml, compileErr, compileErrList);
					compileErr->type = ErrorNone;
					compileErr->level = ERR_DEBUG;
					sprintf(compileErr->message, "Track Change => %d (TODO: Implement)", tempVal[0]);
					addError(compileErr, compileErrList);
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

					if(0 == getNumbers(mml, tempVal, compileErrList))
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
				} /* endif swap<> */
				else if(mmlCmpStr(mml, "drum") == true)
				{
					/* TODO: 実装 */
				}
				else if(mmlCmpStr(mml, "tone") == true)
				{
					/* TODO: 実装 */
				}
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
				else
				{
					newError(mml, compileErr, compileErrList);
					compileErr->type = SyntaxError;
					compileErr->level = ERR_ERROR;
					sprintf(compileErr->message, "Invalid control \"#\".");
					addError(compileErr, compileErrList);
					continue;
				}
				break;

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
						if(2 != getNumbers(mml, tempVal, compileErrList))
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
						break;

					/******************************************/
					/* エコー有無効切替                       */
					/******************************************/
					case 'e':
						{
							int nums;
							mmlgetforward(mml);
							nums = getNumbers(mml, tempVal, compileErrList);
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
								putSeq(&tracks, CMD_ECHO_OFF, mml, compileErr);
								break;
							}
							putSeq(&tracks, CMD_ECHO_ON, mml, compileErr);
						}
						break;

					/******************************************/
					/* ポルタメント有無効切替                 */
					/******************************************/
					case 'p':
						{
							int nums;
							mmlgetforward(mml);
							nums = getNumbers(mml, tempVal, compileErrList);
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
								putSeq(&tracks, CMD_PORTAM_OFF, mml, compileErr);
								break;
							}
							putSeq(&tracks, CMD_PORTAM_ON, mml, compileErr);
						}
						break;

					/******************************************/
					/* ドラムセット有無効切替                 */
					/******************************************/
					case 'd':
						{
							int nums;
							mmlgetforward(mml);
							nums = getNumbers(mml, tempVal, compileErrList);
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
								tracks.drumPart[tracks.curTrack] = true;
								break;
							}
							tracks.drumPart[tracks.curTrack] = false;
						}
						break;

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

				if(1 != getNumbers(mml, tempVal, compileErrList))
				{
					newError(mml, compileErr, compileErrList);
					compileErr->type = SyntaxError;
					compileErr->level = ERR_ERROR;
					sprintf(compileErr->message, "Invalid octave specifyed.");
					addError(compileErr, compileErrList);
					continue;
				}
				
				tracks.curOctave[tracks.curTrack] = tempVal[0];
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
				break;

			/******************************************/
			/* 音色変更                               */
			/******************************************/
			case '@':
				/* ログ出力 */
				newError(mml, compileErr, compileErrList);
				compileErr->type = ErrorNone;
				compileErr->level = ERR_DEBUG;
				sprintf(compileErr->message, "Set instrument");
				addError(compileErr, compileErrList);

				if(1 != getNumbers(mml, tempVal, compileErrList))
				{
					newError(mml, compileErr, compileErrList);
					compileErr->type = SyntaxError;
					compileErr->level = ERR_ERROR;
					sprintf(compileErr->message, "Invalid tone specifyed.");
					addError(compileErr, compileErrList);
					continue;
				}

				/* コマンド配置 */
				putSeq(&tracks, CMD_SET_INST, mml, compileErr);
				putSeq(&tracks, tempVal[0], mml, compileErr);
				break;

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

				if(1 != getNumbers(mml, tempVal, compileErrList))
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
				break;

			/******************************************/
			/* パンポット                             */
			/******************************************/
			case 'p':
				{
					int nums;
					/* ログ出力 */
					newError(mml, compileErr, compileErrList);
					compileErr->type = ErrorNone;
					compileErr->level = ERR_DEBUG;
					sprintf(compileErr->message, "Panpot");
					addError(compileErr, compileErrList);
					
					nums = getNumbers(mml, tempVal, compileErrList);
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
						/* サラウンド */
						byte pan = tempVal[0];
						if(true == tracks.lvolRev[tracks.curTrack])
						{
							pan |= 0x80;
						}
						if(true == tracks.rvolRev[tracks.curTrack])
						{
							pan |= 0x40;
						}

						/* パンコマンド挿入 */
						putSeq(&tracks, CMD_PAN, mml, compileErr);
						putSeq(&tracks, pan, mml, compileErr);
						break;
					}

					/* パンフェードコマンド挿入 */
					/* TODO: ドライバに実装なし */
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

					nums = getNumbers(mml, tempVal, compileErrList);
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
							putSeq(&tracks, CMD_VOLUME, mml, compileErr);
						}
						else
						{
							putSeq(&tracks, CMD_GVOLUME, mml, compileErr);
						}
						putSeq(&tracks, tempVal[0], mml, compileErr);
						break;
					}

					/* ボリュームフェードコマンド挿入 */
					/* TODO: ドライバに実装なし */
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

				if(1 != getNumbers(mml, tempVal, compileErrList))
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
				break;

			/******************************************/
			/* モジュレーション                       */
			/******************************************/
			case 'm':
				/* TODO: 実装 */
				/* ログ出力 */
				newError(mml, compileErr, compileErrList);
				compileErr->type = ErrorNone;
				compileErr->level = ERR_DEBUG;
				sprintf(compileErr->message, "Modulation (TODO: Implement)");
				addError(compileErr, compileErrList);
				break;

			/******************************************/
			/* テンポ                                 */
			/******************************************/
			case 't':
				{
					int nums;

					/* ログ出力 */
					newError(mml, compileErr, compileErrList);
					compileErr->type = ErrorNone;
					compileErr->level = ERR_DEBUG;
					sprintf(compileErr->message, "Tempo");
					addError(compileErr, compileErrList);

					nums = getNumbers(mml, tempVal, compileErrList);
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
						double t;
						byte tempo;
						t = 60.0 / tempVal[0];     /* 4分音符時間 */
						t /= timebase;           /* 1tick時間   */
						t /= (0.000125 * TIMER); /* 割合        */
						tempo = 256.0 / t;

						/* テンポコマンド挿入 */
						putSeq(&tracks, CMD_TEMPO, mml, compileErr);
						putSeq(&tracks, tempo, mml, compileErr);
						break;
					}

					/* テンポフェードコマンド挿入 */
					/* TODO: ドライバに実装なし */

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
				}while(MACRO_NAME_MAX > macroNameInx);
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

				if(1 != getNumbers(mml, tempVal, compileErrList))
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
				putSeq(&tracks, tempVal[0], mml, compileErr);
				break;

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
				newError(mml, compileErr, compileErrList);
				compileErr->type = SyntaxError;
				compileErr->level = ERR_ERROR;
				sprintf(compileErr->message, "Unknown character(%c : 0x%x).", readValue, readValue);
				addError(compileErr, compileErrList);
				break;
		}
	}

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

		dataptr = BASEPOINT + (2*TRACK_MAX);

		/* ヘッダ作成 */
		for(i=0; i<TRACK_MAX; i++)
		{
			word empty = 0;
			if(0 == tracks.track[i].ptr)
			{
				bindataadd(bin, (const byte*)&empty, 2);
				continue;
			}
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

		/* 変換データのまとめこみ */
		for(i=0; i<TRACK_MAX; i++)
		{
			byte cmd = 0x00;
			word lp = 0x0000;
			word rlp = 0x0000;
			word trkst = 0x0000;

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

			/* ループアドレスの相対アドレス算出 */
			lp = trkst + tracks.loopAddr[i];
			rlp = lp - bin->dataInx - 2;
			/* ループ相対アドレスの書き込み */
			bindataadd(bin, (const byte*)&rlp, 2); 
		}
	}

	return compileErrList;
}

