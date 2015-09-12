/*****************************************************************************
*  <システム>   分散処理フレームワーク
*  <名称>	データ管理デーモン
*  <目的>	META/INDEX/DATAの期限切れデータ削除やマージ処理を行う。
*  <機能>	   
*  <開発環境>   UNIX
*  <特記事項>
*
*  VERSION	DATE		BY				CHANGE/COMMENT
*  -----------------------------------------------------------------------------
*  V0.00	  2014/03/29  Takakusaki		新規作成
******************************************************************************/
#include "xi_server.h"

int process_dmnt_init(){
	return 0;
}

int process_dmnt(){
	time_t t;
    int sidx;
    static FILE *fp=NULL;
    char filename[MAX_FILE_PATH+1];
    char buff[MAX_FILE_PATH+1+4];
	static time_t last_check_dt=0;
    char *rbuff;

	//マスター以外
	if ( G_NodeTbl[0].master_flg != MASTER_FLG_MASTER ){ return 0; }

	//データメンテナンス間隔
    time(&t);
    if ( last_check_dt == 0 ){ last_check_dt=t; }
    if ( t <= (last_check_dt + G_ConfTbl->disk_mente_interval) ){ return 0; }

    //データ同期-初期化
    if ( fp == NULL ){
        sidx=disknmChoiceServer(2);
        sprintf(filename,"%s/META/.index", G_DiskTbl[sidx].dir);
        if ( (fp=fopen(filename,"r")) == NULL ){ last_check_dt=t; return -1; }
    }
    rbuff=fgets(buff,MAX_FILE_PATH,fp);

    //データ同期-終了処理
    if ( rbuff == NULL ){
        fclose(fp);
        last_check_dt=t;
        fp=NULL;
        return 0;
    }

    //ファイル名
    chomp(buff);
	//logPrint(DEBUG_INFO,"%s",buff);


	return 0;
}

int process_dmnt_fin(){
	return 0;
}
