/*****************************************************************************
*  <システム>   分散処理フレームワーク
*  <名称>       ログ関連API
*  <目的>       
*  <機能>       
*  <開発環境>   UNIX
*  <特記事項>
*
*  VERSION      DATE	    BY		      CHANGE/COMMENT
*  -----------------------------------------------------------------------------
*  V0.0.00      2014/01/18      Takakusaki	      新規作成
******************************************************************************/
#include "xi_server.h"

char G_LogDir[MAX_FILE_PATH+1];
static double sv_usec;
static int debug_flg=0;
static int debug_flg_file=0;
static int debug_flg_stdout=0;

static char *logLevel2Name(int lvl){
	if ( lvl == DEBUG_DEBUG ){ return "DEB"; }
	if ( lvl == DEBUG_INFO ){ return "INF"; }
	if ( lvl == DEBUG_NOTICE ){ return "WAR"; }
	if ( lvl == DEBUG_WARNING ){ return "WAR"; }
	if ( lvl == DEBUG_ERROR ){ return "ERR"; }
	if ( lvl == DEBUG_CRIT ){ return "ERR"; }
	if ( lvl == DEBUG_ALERT ){ return "ERR"; }
	return "";
}

int logPrintInit(int flg, int flg_file, int flg_stdout){
	debug_flg=flg;
	debug_flg_file=flg_file;
	debug_flg_stdout=flg_stdout;
}

/*****************************************************************************
*  <関数名>     logPrint
*  <機能>       デバック文出力
*  <説明>   	デバックレベルに応じてデバック文のSTDOUT,FILE出力を行う
*  <引数>   
*		lvl:I:ログレベル
*       msgfmt:I:フォーマット
*       ...:I:可変引数
*  <リターン値> 
*		0:正常
*       -1:異常
*  <備考>
******************************************************************************/
int logPrint(int lvl, char *msgfmt, ...)
{
    char mbuf[10240];
    struct tm *dt;
    time_t nowtime;
    va_list argptr;
	char filename[MAX_FILE_PATH+1];
	FILE *fp;
	char logtype;

	if ( debug_flg == 0 ){ return 0; }

    /* 可変引数文字列の編集  */
    va_start(argptr,msgfmt);
    vsprintf(mbuf,msgfmt,argptr);
    va_end(argptr);

    /* 現在時刻 */
	time(&nowtime);
	dt = localtime(&nowtime);

	/* ファイル名 */
	sprintf(filename,"%s/%04d%02d%02d.log", G_LogDir, dt->tm_year+1900, dt->tm_mon+1, dt->tm_mday);

	//標準出力
	if ( lvl >= debug_flg_stdout ){
		printf("%04d/%02d/%02d %02d:%02d:%02d [%03d]%s:%s\n",
			dt->tm_year+1900,
			dt->tm_mon+1,
			dt->tm_mday,
			dt->tm_hour,
			dt->tm_min,
			dt->tm_sec,
			G_ProcNo,
			logLevel2Name(lvl),
			mbuf);
	}


	/* ファイル出力 */
	if ( lvl >= debug_flg_file ){
		if ( (fp=fopen(filename, "a")) == NULL ){ return -1; }
		fprintf(fp,"%04d/%02d/%02d %02d:%02d:%02d [%03d]%s:%s\n",
			dt->tm_year+1900,
			dt->tm_mon+1,
			dt->tm_mday,
			dt->tm_hour,
			dt->tm_min,
			dt->tm_sec,
			G_ProcNo,
			logLevel2Name(lvl),
			mbuf);
		fclose(fp);	
	}

	return 0;
}

/*****************************************************************************
*  <関数名>     logDebug
*  <機能>       デバック文出力
*  <説明>   	デバックレベルに応じてデバック文のSTDOUT,FILE出力を行う
*  <引数>   
*		lvl:I:ログレベル
*       msgfmt:I:フォーマット
*       ...:I:可変引数
*  <リターン値> 
*		0:正常
*       -1:異常
*  <備考>
******************************************************************************/
int logDebug(int lvl, char *msgfmt, ...)
{
    char mbuf[10240];
    struct tm *dt;
	struct timeval tv;
    time_t nowtime;
    int mlen;
    va_list argptr;
	double wk_usec, now_usec;

    /* 可変引数文字列の編集  */
    va_start(argptr,msgfmt);
    vsprintf(mbuf,msgfmt,argptr);
    va_end(argptr);

    /* 現在時刻 */
	gettimeofday(&tv,NULL);

	if ( sv_usec==0 ){
		sv_usec=(tv.tv_sec * 1000000) + tv.tv_usec;
	}
	now_usec=(tv.tv_sec * 1000000) + tv.tv_usec;
	wk_usec=(now_usec - sv_usec)/1000000;
	if ( wk_usec < 0 ) wk_usec=0;
	if ( wk_usec >= 1000 ) wk_usec=999.999999;
	printf("%010.6f %s %s\n", wk_usec, logLevel2Name(lvl), mbuf);

	return 0;
}


/*****************************************************************************
*  <関数名>     logSysLog
*  <機能>       SYSLOG出力
*  <説明>   	SYSLOGへの出力を行う
*  <引数>
*  <リターン値> 常に０
*  <備考>
******************************************************************************/
int logSysLog(char *msgfmt, ...)
{
	char mbuf[4096];
	va_list argptr;

	/* 可変引数文字列の編集  */
	va_start(argptr,msgfmt);
	vsprintf(mbuf,msgfmt,argptr);
	va_end(argptr);

    if ( strncmp(msgfmt,"ERR",3)==0){
		syslog( LOG_ERR, mbuf);
    }else if ( strncmp(msgfmt,"WAR",3)==0){
		syslog( LOG_WARNING, mbuf);
    }else{
		syslog( LOG_INFO, mbuf);
    }

    return 0;
}

