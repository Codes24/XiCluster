/*****************************************************************************
*  <システム>   分散処理フレームワーク
*  <名称>       共通関数／変数定義
*  <目的>
*  <機能>
*  <開発環境>   UNIX
*  <特記事項>
*
*  VERSION  DATE        BY              CHANGE/COMMENT
*  -----------------------------------------------------------------------------
*  V0.00      2014/01/18  Takakusaki        新規作成
******************************************************************************/
#include "xi_common.h"

T_DISK_TABLE    *G_DataDir;     /* 一時領域 */
int G_Data_Disks=0;

/*****************************************************************************
 グローバル変数
******************************************************************************/
char *commGetBaseDir(char *wk){
	char path[MAX_FILE_PATH + 1];
	char linkname[MAX_FILE_PATH + 1];
	int my_pid=getpid();
	ssize_t r;

	//カレントパス取得
	sprintf(path,"/proc/%d/exe",my_pid);
	r = readlink(path, linkname, MAX_FILE_PATH);

	strcpy(wk, filGetDirName(linkname));
	strcpy(linkname,wk);
	strcpy(wk, filGetDirName(linkname));

	return wk;
}

uint16_t commCRC16(char* data, int length)
{
    int i, ii;
    uint16_t crc = 0xFFFF;
    for ( i=0; i<length; i++ ) {
        crc ^= (u_int)(*data++ << 8);
        for ( ii=0; ii<8; ii++ ) {
            if ( crc & 0x8000 ) { crc <<= 1; crc ^= CRC_CCITT16; }
            else { crc <<= 1; }
        }
    }
    return crc;
}

int commReqCheckSumSet(T_TRANS_SEC *ck){
    time( &(ck->dt) );
    ck->crc=commCRC16( (char*)ck, sizeof(time_t));
    return 0;
}

int commReqCheckSumJudge(T_TRANS_SEC *ck){
	time_t t;
	uint16_t c;

	time(&t);
	if ( abs(t - ck->dt) > 300 ){ return -1; }

	c=commCRC16((char*)ck, sizeof(time_t));
	if ( c != ck->crc ){ return -2; }

    return 0;
}


