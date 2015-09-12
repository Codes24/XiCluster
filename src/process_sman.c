/*****************************************************************************
*  <システム>   分散処理フレームワーク
*  <名称>       システム監視デーモン
*  <目的>       常駐プロセスの異常監視と再起動を行う
*  <機能>       
*  <開発環境>   UNIX
*  <特記事項>
*
*  VERSION    DATE        BY                CHANGE/COMMENT
*  -----------------------------------------------------------------------------
*  V0.00      2014/01/18  Takakusaki        新規作成
******************************************************************************/
#include "xi_server.h"

/* プロセス障害時に確保中キャッシュをリリースする */
static int CacheReleaseCheck(int pno, int pid){
	int i;
	int ret;

	logPrint(DEBUG_INFO,"CacheReleaseCheck(%d,%d)",pno,pid);

	//METAキャッシュ
	for ( i=0; i<G_ConfTbl->cache_files; i++){
		if ( G_MetaTbl[i].pid != pid ){ continue; }
		ret=cacheMetaRelease(i);
		logPrint(DEBUG_INFO,"cacheMetaRelease(%d)=%d",i,ret);
	}

	//DATAキャッシュ
	for ( i=0; i<G_ConfTbl->cache_blocks; i++){
		if ( G_IndexTbl[i].pid != pid ){ continue; }
		ret=cacheDataRelease(i);
		logPrint(DEBUG_INFO,"cacheDataRelease(%d)=%d",i,ret);
	}

	//DATAキャッシュ参照中
	if ( G_ProcTbl[pno].cache_idx >= 0 ){
		ret=cacheCheckCachedRelease(G_ProcTbl[pno].cache_idx);
		logPrint(DEBUG_INFO,"cacheCheckCachedRelease(%d)=%d",G_ProcTbl[pno].cache_idx,ret);
		G_ProcTbl[pno].cache_idx=(-1);
	}

	return 0;
}

/* 再起動 */
static int cluster_restart(){
    int cld_pid;
    char command[MAX_FILE_PATH+1];

    //子プロセス起動
    if ( (cld_pid=fork()) == 0 ){
        sprintf(command,"%s/bin/xicluster_server",G_BaseDir);
        char *newargv[] = { command,"restart", NULL };

        execv(command, newargv);
        logPrint(DEBUG_WARNING,"child execute error");
        exit(0);
    }

    return 0;
}

int process_sman_init(){
	return 0;
}

int process_sman(){
	time_t tm;
	int i;
	int ret;
	int status;
	int pid;
	char p1[1024];
	char p2[1024];
	char p3[1024];

	/* シグナル刈り取り */
	while(1){
		pid = wait3(&status, WNOHANG ,NULL);
		if ( pid <= 0 ) break;
		logPrint(DEBUG_INFO,"SIGCLD (pno=%d pid=%d)", G_ProcNo, pid);
	}

	/* プロセス存在監視 */
	for ( i=1; i<G_ConfTbl->max_process; i++){
		if ( G_ProcTbl[i].name[0] == (char)NULL ){ continue; }
		if ( G_ProcTbl[i].pid == 0 ){ continue; }
		if ( G_ProcTbl[i].sts == PROCESS_STATUS_NONE ){ continue; }
		if ( kill(G_ProcTbl[i].pid,0) == 0 ){ continue; }
		logPrint(DEBUG_WARNING,"process down (pno=%d pid=%d pname=%s)", 
				i, G_ProcTbl[i].pid, G_ProcTbl[i].name);
		cacheSetProcStatus(i,PROCESS_STATUS_NONE);
		time(&G_ProcTbl[i].etime);
		CacheReleaseCheck(i, G_ProcTbl[i].pid);
		G_ProcTbl[i].pid = 0;
	}

	/* プロセスBUSY/HUNG監視 */
	time(&tm);
	for ( i=1; i<G_ConfTbl->max_process; i++){
		if ( G_ProcTbl[i].name[0] == (char)NULL ){ continue; }
		if ( G_ProcTbl[i].pid == 0 ){ continue; }
		if ( G_ProcTbl[i].sts == PROCESS_STATUS_NONE ){ continue; }
		if ( (tm - G_ProcTbl[i].alv) <= G_ConfTbl->proc_ka ){ continue; }
		cacheSetProcStatus(i,PROCESS_STATUS_NONE);
		logPrint(DEBUG_WARNING,"process busy (pno=%d pid=%d pname=%s)", i, G_ProcTbl[i].pid, G_ProcTbl[i].name);

		time(&G_ProcTbl[i].etime);
		ret=kill(G_ProcTbl[i].pid,15);
		logPrint(DEBUG_INFO,"kill(%d,15)=%d",G_ProcTbl[i].pid,ret);
		CacheReleaseCheck(i, G_ProcTbl[i].pid);
		G_ProcTbl[i].pid = 0;
	}

	/* 再起動回数クリア */
	for ( i=1; i<G_ConfTbl->max_process; i++){
		if ( G_ProcTbl[i].name[0] == (char)NULL ){ continue; }
		if ( G_ProcTbl[i].fork_try <= 1 ){ continue; }
		if ( tm < (G_ProcTbl[i].stime + G_ConfTbl->fork_clear) ){ continue; }
		G_ProcTbl[i].fork_try=1;
		logPrint(DEBUG_INFO,"fork counter clear (pno=%d pid=%d pname=%s)", i, G_ProcTbl[i].pid, G_ProcTbl[i].name);
	}

	/* プロセス起動 */
	for ( i=1; i<G_ConfTbl->max_process; i++){
		if ( G_ProcTbl[i].name[0] == (char)NULL ){ continue; }
		if ( G_ProcTbl[i].sts != PROCESS_STATUS_NONE ){ continue; }
		if ( tm < (G_ProcTbl[i].stime + G_ConfTbl->rerun_wait) ){ continue; }
		if ( G_ProcTbl[i].fork_try >= G_ConfTbl->fork_max ){
			logPrint(DEBUG_INFO,"cluster restart retry over (pno=%d)",G_ProcNo);
			cluster_restart();
			continue; 
		}
		utlStringSep(G_ProcTbl[i].name,"_",1,p1);
		utlStringSep(G_ProcTbl[i].name,"_",2,p2);
		utlStringSep(G_ProcTbl[i].name,"_",3,p3);
		daemon_exec(i, G_ProcTbl[i].ptype, p3, G_ProcTbl[i].para);
	}

	/* 負荷状況監視 */
	int task_busy=0;
	for ( i=1; i<G_ConfTbl->max_process; i++){
		if ( G_ProcTbl[i].name[0] == (char)NULL ){ continue; }
		if ( G_ProcTbl[i].ptype != PROCESS_TYPE_TASK ){ continue; }
		if ( G_ProcTbl[i].sts == PROCESS_STATUS_BUSY or G_ProcTbl[i].sts == PROCESS_STATUS_WAIT ){ 
			task_busy++;
		}
	}
	G_NodeTbl[0].task_busy = task_busy;

	return 0;
}

int process_sman_fin(){
	return 0;
}
