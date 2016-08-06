/**
 * パス取得関連処理
 */

#include "gstdafx.h"
#include "pathfunc.h"

/**
 * 指定ファイルのあるディレクトリを取得する
 */
void getFileDir(char* dirname, const char* filename)
{
	int cplen;
	char* search;
#ifdef _WIN32
	search = strrchr(filename, '\\');
	if(NULL == search)
	{
		search = strrchr(filename, '/');
	}
#else
	search = strrchr(filename, '/');
#endif
	if(NULL == search)
	{
		dirname[0] = '\0';
		return;
	}

	cplen = search - filename+1;
	memcpy(dirname, filename, cplen);
	dirname[cplen] = '\0';
	return;
}

