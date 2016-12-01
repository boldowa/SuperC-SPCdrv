/**
 * SNESアドレス変換関連処理
 */
#include "gstdafx.h"
#include "address.h"

int pc2snes(int ad)
{
	return ( ((ad<<1) & 0x7f0000) | ((ad & 0x7fff)|0x8000) );
}

int snes2pc(int ad)
{
	return ( ((ad&0x7f0000)>>1) | (ad & 0x7fff) );
}

