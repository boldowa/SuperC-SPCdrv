/**
 * SPC関連処理
 */

/**
 * ID666ヘッダ
 */
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
	char date[11];
	char musicLength[3];
	char fadeTime[5];
	char composer[32];
	char defaultChannelDisable;
	char emulator;
	unsigned char unused[45];
} stID666_Text;

typedef struct {
	/**
         * メモリ解放要フラグ
	 * ※直接更新(ex.u bin->reqfree = false) は、
	 *   spc.c以外禁止
	 */
	bool reqfree;

	/**
	 * SPCコア実データバッファのポインタ
	 */
	byte* data;

	/**
	 * SPCコアサイズ
	 */
	int size;

	/**
	 * SPCコアバージョン
	 */
	byte ver;

	/**
	 * SPCコア マイナーバージョン
	 */
	byte verMiner;

	/**
	 * シーケンスデータベースアドレスのある場所
	 */
	word seqBasePoint;

	/**
	 * DIRアドレス
	 */
	word dir;

	/**
	 * ESAデータ格納アドレス
	 */
	word esaLoc;

	/**
	 * コアの読み込み先
	 */
	word location;

} stSpcCore;

bool coreread(stSpcCore*, char*);
void freecore(stSpcCore*);
bool makeSPC(byte*, stSpcCore*, MmlMan*, BinMan*, stBrrListData*);
int makeBin(byte*, stSpcCore*, MmlMan*, BinMan*, stBrrListData*);

