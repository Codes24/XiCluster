/*****************************************************************************
*  <システム>   分散処理フレームワーク
*  <名称>       JOBログアクセスAPI
*  <目的>       
*  <機能>       
*  <開発環境>   UNIX
*  <特記事項>
*
*  VERSION      DATE	    BY		      CHANGE/COMMENT
*  -----------------------------------------------------------------------------
*  V0.0.00      2014/05/06      Takakusaki	      新規作成
******************************************************************************/
#include "xi_server.h"

static struct timeval l_str_dt;
static struct timeval l_end_dt;

int hlogJobHistoryLogStr(T_CLIENT_REQUEST req){
    gettimeofday(&l_str_dt,NULL);
    return 0;
}

char *hlogCommand(int req, char *para1, char *para2){
	static char command[10240];
	char cmd[1024];

	sprintf(cmd,"%d",req);
	if ( req == REQUEST_CLIENT_MKDIR ){ strcpy(cmd,"mkdir"); }
	if ( req == REQUEST_CLIENT_RMDIR ){ strcpy(cmd,"rmdir"); }
	if ( req == REQUEST_CLIENT_RM ){ strcpy(cmd,"rm"); }
	if ( req == REQUEST_CLIENT_CHMOD ){ strcpy(cmd,"chmod"); }
	if ( req == REQUEST_CLIENT_CHOWN ){ strcpy(cmd,"chown"); }
	if ( req == REQUEST_CLIENT_CHGRP ){ strcpy(cmd,"chgrp"); }
	if ( req == REQUEST_CLIENT_GET ){ strcpy(cmd,"get"); }
	if ( req == REQUEST_CLIENT_PUT ){ strcpy(cmd,"put"); }

	sprintf(command,"%s %s %s",cmd,para1,para2);
	return command;
}

int hlogJobHistoryLogEnd(T_CLIENT_REQUEST req, int rescode){
    char filename[MAX_FILE_PATH+1];
    FILE *fp;
    struct tm *str_dt;
    struct tm *end_dt;

    gettimeofday(&l_end_dt,NULL);
	str_dt = localtime(&l_str_dt.tv_sec);
	end_dt = localtime(&l_end_dt.tv_sec);

    /* ファイル名 */
    sprintf(filename,"%s/%04d%02d%02d.log", G_HLogDir, str_dt->tm_year+1900, str_dt->tm_mon+1, str_dt->tm_mday);

    /* ファイル出力 */
    if ( (fp=fopen(filename, "a")) == NULL ){ return -1; }
    fprintf(fp,"%04d/%02d/%02d %02d:%02d:%02d.%06d\t%.06f\t%d\t%d\t%d\t%s\n",
            str_dt->tm_year+1900,
            str_dt->tm_mon+1,
            str_dt->tm_mday,
            str_dt->tm_hour,
            str_dt->tm_min,
            str_dt->tm_sec,
            l_str_dt.tv_usec,
			tmTimevalElapsed(l_str_dt, l_end_dt),
            rescode,
            req.uid,
            req.gid,
			hlogCommand(req.req, req.clt.para1, req.clt.para2)
            );
    fclose(fp);

    return 0;
}



