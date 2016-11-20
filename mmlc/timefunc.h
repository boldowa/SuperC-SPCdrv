/**
 * 時間関係処理
 */

#ifndef _TIMEFUNC_H
#define _TIMEFUNC_H

/**
 * 型
 */
#ifndef TIME
#ifdef _WIN32
  typedef SYSTEMTIME TIME;
#else
  typedef struct timeval TIME;
#endif /* _WIN32 */
#endif /* TIME */

/**
 * 関数
 */
void setDateStr(char*);
void setDateStrForSPC(char*);
void getTime(TIME*);
double getTimeToPass(TIME*, TIME*);

#endif /* _TIMEFUNC_H */

