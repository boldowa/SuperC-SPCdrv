/**
 * 時間関係処理
 */

#include "gstdafx.h"
#include "timefunc.h"

#ifdef _WIN32
/**
 * 現在日時文字列の取得
 * (Windows向け)
 */
void setDateStr(char* sDate)
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

/**
 * 現在日時文字列の取得(SPC用)
 * (Windows向け)
 */
void setDateStrForSPC(char* sDate)
{
	SYSTEMTIME tm;
	GetLocalTime(&tm);

	sprintf(sDate, "%02hu/%02hu/%4hu",
		tm.wMonth,
		tm.wDay,
		tm.wYear
	       );
}

/**
 * 現在年の取得(SNSF用)
 * (Windows向け)
 */
void setYear(char* sYear)
{
	SYSTEMTIME tm;
	GetLocalTime(&tm);

	sprintf(sYear, "%4hu",
		tm.wYear
	       );
}

/**
 * 現在時刻の取得
 * (Windows向け)
 */
void getTime(TIME *tm)
{
	GetLocalTime(tm);
}

/**
 * 差分時間の取得
 * (Windows向け)
 */
double getTimeToPass(TIME *start, TIME *end)
{
	FILETIME st, et, difft;
	ULONGLONG timediff;
	double timeToPass;

	SystemTimeToFileTime(start, &st);
	SystemTimeToFileTime(end, &et);

	difft.dwHighDateTime = et.dwHighDateTime - st.dwHighDateTime;
	difft.dwLowDateTime = et.dwLowDateTime - st.dwLowDateTime;

	timediff = ((ULONGLONG)difft.dwHighDateTime << 32) + difft.dwLowDateTime;

	timeToPass = timediff * 1.0e-7;
	return timeToPass;
}

#else
/**
 * 現在日時文字列の取得
 * (Linux向け)
 */
void setDateStr(char* sDate)
{
	struct timeval time;
	struct tm* time_st;

	gettimeofday(&time, NULL); /* 時間取得 */
	time_st = localtime(&time.tv_sec); /* 現地時間に変換する */

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

/**
 * 現在日時文字列の取得(SPC用)
 * (Linux向け)
 */
void setDateStrForSPC(char* sDate)
{
	struct timeval time;
	struct tm* time_st;

	gettimeofday(&time, NULL); /* 時間取得 */
	time_st = localtime(&time.tv_sec); /* 現地時間に変換する */

	sprintf(sDate, "%02d/%02d/%4d",
		time_st->tm_mon+1,
		time_st->tm_mday,
	        time_st->tm_year+1900
		);
}

/**
 * 現在年の取得(SNSF用)
 * (Linux向け)
 */
void setYear(char* sYear)
{
	struct timeval time;
	struct tm* time_st;

	gettimeofday(&time, NULL); /* 時間取得 */
	time_st = localtime(&time.tv_sec); /* 現地時間に変換する */

	sprintf(sYear, "%4d",
	        time_st->tm_year+1900
		);
}

/**
 * 現在時刻の取得
 * (Linux向け)
 */
void getTime(TIME *tm)
{
	gettimeofday(tm, NULL); /* 時間取得 */
}

/**
 * 差分時間の取得
 * (Linux向け)
 */
double getTimeToPass(TIME *start, TIME *end)
{
	TIME tm;
	double timeToPass;

	tm.tv_sec = end->tv_sec - start->tv_sec;
	tm.tv_usec = end->tv_usec - start->tv_usec;
	timeToPass  = tm.tv_usec * 1.0e-6;
	timeToPass += tm.tv_sec;

	return timeToPass;
}

#endif

