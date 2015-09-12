/*****************************************************************************
*  <システム>   分散処理フレームワーク
*  <名称>       デーモン共通メイン処理
*  <目的>       
*  <機能>       
*  <開発環境>   UNIX
*  <特記事項>
*
*  VERSION    DATE        BY                CHANGE/COMMENT
*  -----------------------------------------------------------------------------
*  V0.00      2014/01/18  Takakusaki        新規作成
******************************************************************************/
#include "xi_server.h"

/*****************************************************************************
 ローカル変数
******************************************************************************/
static int  l_ptype=-1;
static int  l_loop_sleep=0;
static int  l_EndFlg=0;
static char l_pname[1024];

static void xicluster_intsig(int);

/*****************************************************************************
*  <関数名> xicluster_intsig()
*  <機能>   シグナル受信処理
*  <説明>   シグナル受信により終了フラグを設定する
*  <引数>   signo:I:シグナル番号
*  <リターン値> なし
*  <備考>
******************************************************************************/
static void xicluster_intsig(int signo)
{
	l_EndFlg=signo;
	signal(signo, xicluster_intsig);
}

main(int argc, char **argv){
	int ret;
	time_t tm;
	char p1[32];
	char p2[32];
	char p3[32];

	//パラメータ解析
	if ( argc < 1 ){
		printf("ERROR:parameter error cnt=%d\n",argc);
		exit(1);
	}
	utlStringSep(argv[0],"_",1,p1);
	utlStringSep(argv[0],"_",2,p2);
	utlStringSep(argv[0],"_",3,p3);
	if ( strcmp(p1,"XICLUSTER") != 0 ){
		printf("ERROR:I cannot carry it out from a command-line\n");
		exit(1);
	}
	G_ProcNo=atoi(p2);
	if ( strcmp(p3,"SMAN") == 0){ l_ptype=PROCESS_TYPE_SMAN; }
	if ( strcmp(p3,"NLSR") == 0){ l_ptype=PROCESS_TYPE_NLSR; }
	if ( strcmp(p3,"CLSR") == 0){ l_ptype=PROCESS_TYPE_CLSR; }
	if ( strcmp(p3,"CACH") == 0){ l_ptype=PROCESS_TYPE_CACH; }
	if ( strcmp(p3,"CRCV") == 0){ l_ptype=PROCESS_TYPE_CRCV; }
	if ( strcmp(p3,"CSND") == 0){ l_ptype=PROCESS_TYPE_CSND; }
	if ( strcmp(p3,"MSYN") == 0){ l_ptype=PROCESS_TYPE_MSYN; }
	if ( strcmp(p3,"DSYN") == 0){ l_ptype=PROCESS_TYPE_DSYN; }
	if ( strcmp(p3,"DISK") == 0){ l_ptype=PROCESS_TYPE_DISK; }
	if ( strcmp(p3,"DMNT") == 0){ l_ptype=PROCESS_TYPE_DMNT; }
	if ( strcmp(p3,"TASK") == 0){ l_ptype=PROCESS_TYPE_TASK; }
	strcpy(l_pname,p3);
	logPrint(DEBUG_INFO, "pname=%s ptype=%d",l_pname,l_ptype);
	if ( l_ptype == -1 ){
        logPrint(DEBUG_ERROR,"Process type error");
        exit(1);
	}

    /* 割り込み処理設定 */
    signal(SIGINT, xicluster_intsig);          /* [Ctrl]+[c]シグナル捕獲  */
    signal(SIGQUIT,xicluster_intsig);          /* シグナル                */
    signal(SIGTERM,xicluster_intsig);          /* シグナル                */

    //ベースディレクトリ
	commGetBaseDir(G_BaseDir);
    sprintf(G_TempDir,"%s/temp",G_BaseDir);
    if ( filisDirectory(G_TempDir) != 1 ){
        logPrint(DEBUG_ERROR,"No Diretory(%s)",G_TempDir);
        exit(1);
    }
    sprintf(G_LogDir,"%s/log",G_BaseDir);
    if ( filisDirectory(G_LogDir) != 1 ){
        logPrint(DEBUG_ERROR,"No Diretory(%s)",G_LogDir);
        exit(1);
    }
    sprintf(G_HLogDir,"%s/hlog",G_BaseDir);
    if ( filisDirectory(G_HLogDir) != 1 ){
        logPrint(DEBUG_ERROR,"No Diretory(%s)",G_HLogDir);
        exit(1);
    }


    //IPCキー
    G_ShmKey=ftok(G_TempDir, 1);
    G_SemKey=ftok(G_TempDir, 2);
    G_MquKey=ftok(G_TempDir, 3);

	//セマフォ
	if ( semOpen(G_SemKey) != 0 ){
        logPrint(DEBUG_ERROR,"semOpen(%d)",G_SemKey);
        exit(1);
	}

	//メッセージキュー
	//if ( mquOpen(G_MquKey) != 0 ){
    //    logPrint(DEBUG_ERROR,"mquOpen(%d)",G_MquKey);
    //    exit(1);
	//}

	//共有メモリオープン
	if ( shmOpen(G_ShmKey) != 0 ){
        logPrint(DEBUG_ERROR,"shmOpen(%d)",G_ShmKey);
        exit(1);
	}
    if ( cacheSetAddress(NULL) != 0 ){
        printf("shared memory attach error\n");
        exit(1);
    }

	//ログレベル
	logPrintInit(1, G_ConfTbl->debug_level_file, G_ConfTbl->debug_level_stdout);

	////////////////////////////////////////////////////////////
	//各子プロセスの初期化処理
	////////////////////////////////////////////////////////////
	ret=0;
	switch(l_ptype){
	case PROCESS_TYPE_SMAN: 
		l_loop_sleep=G_ConfTbl->sleep_loop_sman;
		ret=process_sman_init();
		break;
	case PROCESS_TYPE_NLSR: 
		l_loop_sleep=G_ConfTbl->sleep_loop_nlsr;
		if ( G_ConfTbl->network_prot_svr == REQUEST_NODE_PROT_TCP ){
			ret=process_nlsr_tcp_init();
		}else{
			ret=process_nlsr_udp_init();
		}
		break;
	case PROCESS_TYPE_CLSR: 
		l_loop_sleep=G_ConfTbl->sleep_loop_clsr;
		ret=process_clsr_init();
		break;
	case PROCESS_TYPE_CACH: 
		l_loop_sleep=G_ConfTbl->sleep_loop_cach;
		ret=process_cach_init();
		break;
	case PROCESS_TYPE_CRCV: 
		l_loop_sleep=G_ConfTbl->sleep_loop_crcv;
		ret=process_crcv_init();
		break;
	case PROCESS_TYPE_CSND: 
		l_loop_sleep=G_ConfTbl->sleep_loop_csnd;
		ret=process_csnd_init();
		break;
	case PROCESS_TYPE_MSYN: 
		l_loop_sleep=G_ConfTbl->sleep_loop_msyn;
		ret=process_msyn_init();
		break;
	case PROCESS_TYPE_DSYN: 
		l_loop_sleep=G_ConfTbl->sleep_loop_dsyn;
		ret=process_dsyn_init();
		break;
	case PROCESS_TYPE_DISK: 
		l_loop_sleep=G_ConfTbl->sleep_loop_disk;
		ret=process_disk_init();
		break;
	case PROCESS_TYPE_DMNT: 
		l_loop_sleep=G_ConfTbl->sleep_loop_dmnt;
		ret=process_disk_init();
		break;
	case PROCESS_TYPE_TASK: 
		l_loop_sleep=G_ConfTbl->sleep_loop_task;
		ret=process_task_init();
		break;
	default:
        logPrint(DEBUG_ERROR,"process type unknown (TYPE=%d)",l_ptype);
        exit(1);
	}


	////////////////////////////////////////////////////////////
	//メインループ
	////////////////////////////////////////////////////////////
	G_ProcTbl[G_ProcNo].sts=PROCESS_STATUS_SYNC;
	while(1){

		//親プロセスチェック
		if ( kill(G_ProcTbl[0].pid,0) != 0 ){
        	logPrint(DEBUG_INFO,"detected by SYSMAN daemon down");
			break;
		}

		//終了シグナル受信
		if ( l_EndFlg != 0 ){
        	logPrint(DEBUG_INFO,"signal signo=%d",l_EndFlg);
			break;
		}

		//各子プロセスの処理
		if ( G_NodeTbl[0].sts == SYSTEM_STATUS_RUNNING || G_NodeTbl[0].sts == SYSTEM_STATUS_SYNC ){
			time(&G_ProcTbl[G_ProcNo].alv);
			if ( l_ptype == PROCESS_TYPE_SMAN ){ process_sman(); }
			if ( l_ptype == PROCESS_TYPE_NLSR ){ 
				if ( G_ConfTbl->network_prot_svr == REQUEST_NODE_PROT_TCP ){
					process_nlsr_tcp(); 
				}else{
					process_nlsr_udp(); 
				}
			}
			if ( l_ptype == PROCESS_TYPE_CLSR ){ process_clsr(); }
			if ( l_ptype == PROCESS_TYPE_CACH ){ process_cach(); }
			if ( l_ptype == PROCESS_TYPE_CRCV ){ process_crcv(); }
			if ( l_ptype == PROCESS_TYPE_CSND ){ process_csnd(); }
			if ( l_ptype == PROCESS_TYPE_MSYN ){ process_msyn(); }
			if ( l_ptype == PROCESS_TYPE_DSYN ){ process_dsyn(); }
			if ( l_ptype == PROCESS_TYPE_DISK ){ process_disk(); }
			if ( l_ptype == PROCESS_TYPE_DMNT ){ process_dmnt(); }
			if ( l_ptype == PROCESS_TYPE_TASK ){ process_task(); }
		}

		//アイドル状態に遷移
		G_ProcTbl[G_ProcNo].sts=PROCESS_STATUS_IDLE;
		time(&G_ProcTbl[G_ProcNo].alv);
		usleep(l_loop_sleep);
	}

	G_ProcTbl[G_ProcNo].sts=PROCESS_STATUS_FIN;

	////////////////////////////////////////////////////////////
	//各子プロセスの終了処理
	////////////////////////////////////////////////////////////
	if ( l_ptype == PROCESS_TYPE_SMAN ){ process_sman_fin(); }
	if ( l_ptype == PROCESS_TYPE_NLSR ){
		if ( G_ConfTbl->network_prot_svr == REQUEST_NODE_PROT_TCP ){
			process_nlsr_tcp_fin(); 
		}else{
			process_nlsr_udp_fin(); 
		}
	}
	if ( l_ptype == PROCESS_TYPE_CLSR ){ process_clsr_fin(); }
	if ( l_ptype == PROCESS_TYPE_CACH ){ process_cach_fin(); }
	if ( l_ptype == PROCESS_TYPE_CRCV ){ process_crcv_fin(); }
	if ( l_ptype == PROCESS_TYPE_CSND ){ process_csnd_fin(); }
	if ( l_ptype == PROCESS_TYPE_MSYN ){ process_msyn_fin(); }
	if ( l_ptype == PROCESS_TYPE_DSYN ){ process_dsyn_fin(); }
	if ( l_ptype == PROCESS_TYPE_DISK ){ process_disk_fin(); }
	if ( l_ptype == PROCESS_TYPE_DMNT ){ process_dmnt_fin(); }
	if ( l_ptype == PROCESS_TYPE_TASK ){ process_task_fin(); }

	G_ProcTbl[G_ProcNo].sts=PROCESS_STATUS_NONE;

	exit(0);
}
