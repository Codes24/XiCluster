/*****************************************************************************
*  <システム>   分散処理フレームワーク
*  <名称>       時間関連API
*  <目的>       
*  <機能>       
*  <開発環境>   UNIX
*  <特記事項>
*
*  VERSION      DATE            BY                      CHANGE/COMMENT
*  -----------------------------------------------------------------------------
*  V0.0.00      2014/01/18      Takakusaki              新規作成
******************************************************************************/
#include "xi_common.h"

#define INBUFSIZE 64

static char *mdat[]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

static int MonthToValue(char *s){
    static char b[INBUFSIZE];
    int i,v;

    for ( i=0; i<=11; i++){
        if ( strcmp(mdat[i],s) == 0 ){ v=i+1; break; }
    }

    return v;
}

static int TimeSplit(char *w5,char *w5_1,char *w5_2,char *w5_3){
    int i;
    int cnt,wcnt;

    w5_1[0]=(char)NULL;
    w5_2[0]=(char)NULL;
    w5_3[0]=(char)NULL;
    for ( i=0,cnt=0,wcnt=0; w5[i]!=(char)NULL; i++){
        if ( w5[i] == ':' ){
            cnt++;
            wcnt=0;
            if ( cnt >= 3) return 1;
            continue;
        }
        if ( cnt == 0 ){
            w5_1[wcnt++]=w5[i];
            w5_1[wcnt+1]=(char)NULL;
        }
        if ( cnt == 1 ){
            w5_2[wcnt++]=w5[i];
            w5_2[wcnt+1]=(char)NULL;
        }
        if ( cnt == 2 ){
            w5_3[wcnt++]=w5[i];
            w5_3[wcnt+1]=(char)NULL;
        }
    }
    return 0;
}

char *tmVal2YYYYMMDD(time_t tm)
{
	struct tm *tt;
	static char tmbuff[INBUFSIZE];

	tt=localtime(&tm);
	sprintf(tmbuff,"%04d/%02d/%02d",
		tt->tm_year + 1900,
		tt->tm_mon + 1,
		tt->tm_mday);

	return tmbuff;
}

char *tmVal2YYYYMMDDhhmm(time_t tm)
{
	struct tm *tt;
	static char tmbuff[INBUFSIZE];

	tt=localtime(&tm);
	sprintf(tmbuff,"%04d/%02d/%02d %02d:%02d",
		tt->tm_year + 1900,
		tt->tm_mon + 1,
		tt->tm_mday,
		tt->tm_hour,
		tt->tm_min);

	return tmbuff;
}

char *tmVal2YYYYMMDDhhmmss(time_t tm)
{
	struct tm *tt;
	static char tmbuff[INBUFSIZE];

	tt=localtime(&tm);
	sprintf(tmbuff,"%04d/%02d/%02d %02d:%02d:%02d",
		tt->tm_year + 1900,
		tt->tm_mon + 1,
		tt->tm_mday,
		tt->tm_hour,
		tt->tm_min,
		tt->tm_sec
	);

	return tmbuff;
}

char *tmVal2YYYYMMDDhhmmssss(struct timeval tm)
{
	struct tm *tt;
	static char tmbuff[INBUFSIZE];

	if ( tm.tv_sec == 0 ){
		strcpy(tmbuff,"0000/00/00 00:00:00.000000");
	}else{
		tt=localtime(&tm.tv_sec);
		sprintf(tmbuff,"%04d/%02d/%02d %02d:%02d:%02d.%06d",
		tt->tm_year + 1900,
		tt->tm_mon + 1,
		tt->tm_mday,
		tt->tm_hour,
		tt->tm_min,
		tt->tm_sec,
		tm.tv_usec
		);
	}

	return tmbuff;
}


void tmTimeToVal(char *in, char *out)
{
	int type=0;
    char w1[INBUFSIZE];
    char w2[INBUFSIZE];
    char w3[INBUFSIZE];
    char w4[INBUFSIZE];
    char w5[INBUFSIZE];
    char w6[INBUFSIZE];
    char w5_1[INBUFSIZE];
    char w5_2[INBUFSIZE];
    char w5_3[INBUFSIZE];

	out[0]=(char)NULL;

	/* フォーマット判定 */
	if ( strlen(in) == 10 && in[4]=='-' && in[7]=='-' ){
		type=1;
	}else if ( in[4]=='-' && in[7]=='-' && in[10]=='T' && in[13]==':' && in[16]==':' && in[19]=='+'){
		type=2;
	}else if ( in[4]=='-' && in[7]=='-' && in[10]=='T' && in[13]==':' && in[16]==':' && in[19]=='Z'){
		type=3;
	}else if ( in[3]==','){
		type=4;
	}

	/* 2007-10-25 */
	if ( type==1 ){
		sscanf(in,"%4s-%2s-%2s",w1,w2,w3);
		sprintf(out,"%04d%02d%02d%02d%02d%02d", atoi(w1),atoi(w2),atoi(w3),0,0,0);
	}

	/* 2007-10-26T06:32:12+09:00 */
	if ( type==2){
		sscanf(in,"%4s-%2s-%2sT%2s:%2s:%2s",w1,w2,w3,w4,w5,w6);
		sprintf(out,"%04d%02d%02d%02d%02d%02d",
			atoi(w1),atoi(w2),atoi(w3),atoi(w4),atoi(w5),atoi(w6));
	}

	/* 2007-10-26T00:19:51Z */
	if ( type==3){
		sscanf(in,"%4s-%2s-%2sT%2s:%2s:%2s",w1,w2,w3,w4,w5,w6);
		sprintf(out,"%04d%02d%02d%02d%02d%02d",
			atoi(w1),atoi(w2),atoi(w3),atoi(w4),atoi(w5),atoi(w6));
	}

	/* Fri, 26 Oct 2007 14:38:00 +0900 */
	if ( type==4 ){
		sscanf(in,"%s %s %s %s %s",w1,w2,w3,w4,w5);
		TimeSplit(w5,w5_1,w5_2,w5_3);
		sprintf(out,"%04d%02d%02d%02d%02d%02d",
			atoi(w4),MonthToValue(w3),atoi(w2),atoi(w5_1),atoi(w5_2),atoi(w5_3));
	}

}

int tmCompareMicroSec(struct timeval t1, struct timeval t2){
//printf("[%d.%d][%d.%d]\n",t1.tv_sec, t1.tv_usec, t2.tv_sec, t2.tv_usec);
	if ( t1.tv_sec > t2.tv_sec ){ return 1; }
	if ( t1.tv_sec == t2.tv_sec ){
		if ( t1.tv_usec == t2.tv_usec ){ return 0; }
		if ( t1.tv_usec > t2.tv_usec ){ return 1; }
	}
	return -1;
}

double tmTimevalElapsed(struct timeval t1, struct timeval t2){
	double elap;

	elap=t2.tv_sec - t1.tv_sec;
    if ( t1.tv_usec <= t2.tv_usec ){
        elap += (double)(t2.tv_usec - t1.tv_usec) / 1000000;
    }else{
        elap++;
        elap += (double)(t2.tv_usec - t1.tv_usec) / 1000000;
    }
	return elap;
}

double tmTransSpeedMB(u_long size, double dt){
	if ( dt == 0 ){ return 0; }
	return (double)size / dt / 1024 / 1024;
}
