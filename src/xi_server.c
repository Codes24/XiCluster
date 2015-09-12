/*****************************************************************************
*  <システム>   分散処理フレームワーク
*  <名称>	サーバ制御API
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

int server_mem(){
	int i;
	time_t tm;
	char ipbuf[128];

	if ( shmOpen(G_ShmKey) != 0 ){
		printf("XICLUSTER not running\n");
		exit(0);
	}
	if ( cacheSetAddress(NULL) != 0 ){
		printf("shared memory attach error\n");
		exit(0);
	}

	//共有メモリ情報
	dispMemInfo(1,G_IndexTbl);

	return 0;
}

int server_cache(){
	int i;
	time_t tm;
	char ipbuf[128];

	if ( shmOpen(G_ShmKey) != 0 ){
		printf("XICLUSTER not running\n");
		exit(0);
	}
	if ( cacheSetAddress(NULL) != 0 ){
		printf("shared memory attach error\n");
		exit(0);
	}

	//キャッシュ情報
	dispCacheInfo(1,G_MetaTbl, G_IndexTbl);

	return 0;
}

int server_volume(){
	int i;
	time_t tm;
	char ipbuf[128];

	if ( shmOpen(G_ShmKey) != 0 ){
		printf("XICLUSTER not running\n");
		exit(0);
	}
	if ( cacheSetAddress(NULL) != 0 ){
		printf("shared memory attach error\n");
		exit(0);
	}

	//キャッシュ情報
	dispVolumeInfo(1,G_NodeTbl, G_DiskTbl);

	return 0;
}

int server_parameter(){
	int i;
	time_t tm;
	char ipbuf[128];

	if ( shmOpen(G_ShmKey) != 0 ){
		printf("XICLUSTER not running\n");
		exit(0);
	}
	if ( cacheSetAddress(NULL) != 0 ){
		printf("shared memory attach error\n");
		exit(0);
	}

	paraDispPara(G_ConfTbl);

	return 0;
}

int server_status(){
	int i;
	time_t tm;
	char ipbuf[128];

	if ( shmOpen(G_ShmKey) != 0 ){
		printf("XICLUSTER not running\n");
		exit(0);
	}
	if ( cacheSetAddress(NULL) != 0 ){
		printf("shared memory attach error\n");
		exit(0);
	}

	//クラスタ情報
	dispNodeInfo(1,G_NodeTbl);

	return 0;
}

int server_status2(){
	int i;
	time_t tm;
	char ipbuf[128];

	if ( shmOpen(G_ShmKey) != 0 ){
		printf("XICLUSTER not running\n");
		exit(0);
	}
	if ( cacheSetAddress(NULL) != 0 ){
		printf("shared memory attach error\n");
		exit(0);
	}

	//クラスタ情報
	dispNodeInfoDetail(1,G_NodeTbl);

	return 0;
}

int server_process(){
	int i;
	time_t tm;
	char ipbuf[128];

	if ( shmOpen(G_ShmKey) != 0 ){
		printf("XICLUSTER not running\n");
		exit(0);
	}
	if ( cacheSetAddress(NULL) != 0 ){
		printf("shared memory attach error\n");
		exit(0);
	}

	//常駐プロセス
	dispProcessInfo(1,G_ProcTbl);

	return 0;
}

int server_perf(){
	int i;
	time_t tm;

	if ( shmOpen(G_ShmKey) != 0 ){
		printf("XICLUSTER not running\n");
		exit(0);
	}
	if ( cacheSetAddress(NULL) != 0 ){
		printf("shared memory attach error\n");
		exit(0);
	}

	//パフォーマンス情報
	dispPerformance(1,G_NodeTbl,G_DiskTbl);

	return 0;
}

int server_memhex(){
	int i;

	if ( shmOpen(G_ShmKey) != 0 ){
		printf("XICLUSTER not running\n");
		exit(0);
	}
	if ( cacheSetAddress(NULL) != 0 ){
		printf("shared memory attach error\n");
		exit(0);
	}

	for(i=0; i<G_ConfTbl->cache_blocks; i++){
		dispMemDataDump(i);
	}

	return 0;
}

int server_start(){
	int i,j;
	int ret;
	int ck_flg;
	int pno=0;
	time_t sv_time;
	time_t tm;
	T_SHM_HEADER hwk;
	struct hostent *hserv;
	char ipbuf1[128];
	char ipbuf2[128];
	char buff1[128];
	char buff2[128];
	char para[128];
	char l_ConfFile[MAX_FILE_PATH+1];
	char wk_dir[MAX_FILE_PATH+1];
	T_CONFIG_TABLE l_ConfFileData;
	char *wk_dirs[]={"META", "IDX", "DATA","LOG",""};

    //ログON
    logPrintInit(1, DEBUG_DEBUG, DEBUG_DEBUG);
	logPrint(DEBUG_INFO,"XICLUSTER server starting...");

	//パラメータ初期化
	paraSetDefault(&l_ConfFileData);
	//paraDispPara(&l_ConfFileData);

	//設定ファイル読込み
	sprintf(l_ConfFile,"%s/conf/xicluster.conf",G_BaseDir);
	if ( filisFile(l_ConfFile) != 1 ){
		logPrint(DEBUG_ERROR,"No File(%s)",l_ConfFile);
		return -1;
	}
	//設定ファイル読込み
	logPrint(DEBUG_INFO,"%s read",l_ConfFile);
	if ( paraReadConfFile(l_ConfFile,&l_ConfFileData) != 0 ){
		logPrint(DEBUG_ERROR,"File Read error(%s)",l_ConfFile);
		return -2;
	}
	//paraDispPara(&l_ConfFileData);

	//パラメータチェック
	if ( paraCheckPara(&l_ConfFileData) != 0 ){
		logPrint(DEBUG_ERROR,"Parameter error(%s)",l_ConfFile);
		return -3;
	}
	logPrint(DEBUG_INFO,"Parameter Check OK");

    //セマフォ作成
	ret=semCreat(G_SemKey,IPC_SEM_NSEMS);
	if ( ret == 1 ){
		printf("XICLUSTER server running\n");
		exit(0);
	}
	if ( ret != 0 ){
		logPrint(DEBUG_ERROR,"semCreat(%d)",G_SemKey);
		return -4;
	}
	logPrint(DEBUG_INFO,"semaphore create (key=%X num=%d) => %d",G_SemKey,IPC_SEM_NSEMS,ret);


    //メッセージキュー作成
	/*
	ret=mquCreat(G_MquKey);
	if ( ret == 1 ){
		printf("XICLUSTER server running\n");
		exit(0);
	}
	if ( ret != 0 ){
		logPrint(DEBUG_ERROR,"mquCreat(%d)",G_MquKey);
		return -5;
	}
	logPrint(DEBUG_INFO,"message queues create (key=%X) => %d",G_MquKey,ret);
	*/

	//共有メモリのサイズ算出
	hwk.size_header     = VALUP1024( sizeof(T_SHM_HEADER) );
	hwk.size_conf       = VALUP1024( sizeof(T_CONFIG_TABLE) );
	hwk.size_node       = VALUP1024( sizeof(T_NODE_TABLE) * l_ConfFileData.max_node);
	hwk.size_proc       = VALUP1024( sizeof(T_PROCESS_TABLE) * l_ConfFileData.max_process );
	hwk.size_disk       = VALUP1024( sizeof(T_DISK_TABLE) * G_Data_Disks );
	hwk.size_meta       = VALUP1024( sizeof(T_META_TABLE) * l_ConfFileData.cache_files );
	hwk.size_index      = VALUP1024( sizeof(T_INDEX_TABLE)  * l_ConfFileData.cache_blocks );
	hwk.size_data       = VALUP1024( sizeof(T_DATA_TABLE) * l_ConfFileData.cache_blocks );
	hwk.size_total		= hwk.size_header + hwk.size_conf + hwk.size_node + hwk.size_proc
							+ hwk.size_meta + hwk.size_disk + hwk.size_index + hwk.size_data;
    hwk.offset_header   = 0;
    hwk.offset_conf     = hwk.size_header;
    hwk.offset_node     = hwk.offset_conf + hwk.size_conf;
    hwk.offset_proc     = hwk.offset_node + hwk.size_node;
    hwk.offset_disk     = hwk.offset_proc + hwk.size_proc;
    hwk.offset_meta     = hwk.offset_disk + hwk.size_disk;
    hwk.offset_index    = hwk.offset_meta + hwk.size_meta;
    hwk.offset_data     = hwk.offset_index + hwk.size_index;

	logPrint(DEBUG_INFO,"shared memory size");
	logPrint(DEBUG_INFO,"  memory header Area       = %dbyte",hwk.size_header);
	logPrint(DEBUG_INFO,"  system configration Area = %dbyte",hwk.size_conf);
	logPrint(DEBUG_INFO,"  cluster information Area = %dbyte * %drecord",sizeof(T_NODE_TABLE), l_ConfFileData.max_node);
	logPrint(DEBUG_INFO,"  process information Area = %dbyte * %drecord",sizeof(T_PROCESS_TABLE), l_ConfFileData.max_process);
	logPrint(DEBUG_INFO,"  Disk Management Area     = %dbyte * %drecord",sizeof(T_DISK_TABLE), G_Data_Disks);
	logPrint(DEBUG_INFO,"  Meta Cache Area          = %dbyte * %drecord",sizeof(T_META_TABLE), l_ConfFileData.cache_files);
	logPrint(DEBUG_INFO,"  Index Cache Area         = %dbyte * %drecord",sizeof(T_INDEX_TABLE), l_ConfFileData.cache_blocks);
	logPrint(DEBUG_INFO,"  Data Cache Area          = %dbyte * %drecord",sizeof(T_DATA_TABLE), l_ConfFileData.cache_blocks);
	logPrint(DEBUG_INFO,"  TOTAL                    = %dbyte",hwk.size_total);
	
    //共有メモリ作成
	ret=shmCreat(G_ShmKey,hwk.size_total);
	if ( ret == 1 ){
		printf("XICLUSTER server running\n");
		exit(0);
	}
	if ( ret != 0 ){
		logPrint(DEBUG_ERROR,"shmCreat(%d,%d)",G_ShmKey,hwk.size_total);
		return -6;
	}
	logPrint(DEBUG_INFO,"shared memory create (key=%X size=%dbyte) => %d",G_ShmKey,hwk.size_total,ret);

	//////////////////////////////////////////////////////////////
	//共有メモリ設定
	//////////////////////////////////////////////////////////////
	if ( cacheSetAddress(&hwk) != 0 ){
		printf("shared memory attach error\n");
		exit(0);
	}
	memcpy(G_ConfTbl, &l_ConfFileData, sizeof(T_CONFIG_TABLE));
	time(&sv_time);

	//サーバ情報
	cacheSetNodeStatus(0, SYSTEM_STATUS_INIT);
	G_NodeTbl[0].stime=sv_time;
	G_NodeTbl[0].alv=sv_time;
	G_NodeTbl[0].run_userid=getuid();
	G_NodeTbl[0].run_groupid=getgid();
	G_NodeTbl[0].task_process=G_ConfTbl->task_process;
	G_NodeTbl[0].cache_process=G_ConfTbl->cache_process;
	G_NodeTbl[0].msyn_process=G_ConfTbl->msyn_process;
	G_NodeTbl[0].dsyn_process=G_ConfTbl->dsyn_process;
	G_NodeTbl[0].cache_meta_blocks=G_ConfTbl->cache_files;
	G_NodeTbl[0].cache_data_blocks=G_ConfTbl->cache_blocks;
	cacheSetMasterFlag(0,0);
	gethostname(G_NodeTbl[0].run_hostname, sizeof(G_NodeTbl[0].run_hostname) );
	hserv=gethostbyname(G_NodeTbl[0].run_hostname);
	G_NodeTbl[0].svr_ip=(u_int32_t)utlGetIPAddress(G_ConfTbl->network_if_svr);
	G_NodeTbl[0].clt_ip=(u_int32_t)utlGetIPAddress(G_ConfTbl->network_if_clt);
	if ( G_NodeTbl[0].svr_ip == 0 ){ memcpy(&G_NodeTbl[0].svr_ip,hserv->h_addr,sizeof(long)); }
	if ( G_NodeTbl[0].clt_ip == 0 ){ memcpy(&G_NodeTbl[0].clt_ip,hserv->h_addr,sizeof(long)); }
	G_NodeTbl[0].sga_size=hwk.size_total;
	utlDisplyIP((u_char*)&G_NodeTbl[0].svr_ip,ipbuf1);
	utlDisplyIP((u_char*)&G_NodeTbl[0].clt_ip,ipbuf2);
	if ( G_NodeTbl[0].node_id[0] == (char)NULL){
		utlGetSHA1(G_NodeTbl[0].run_hostname, G_NodeTbl[0].node_id);
	}
	if ( G_NodeTbl[0].node_id[0] == (char)NULL){
		utlGetSHA1(G_NodeTbl[0].run_hostname, G_NodeTbl[0].node_id);
	}

	logPrint(DEBUG_INFO,"node_id=%s",utlSHA1_to_HEX(G_NodeTbl[0].node_id));
	logPrint(DEBUG_INFO,"uid=%d gid=%d host=%s IP=%s,%s",
		G_NodeTbl[0].run_userid,
		G_NodeTbl[0].run_groupid,
		G_NodeTbl[0].run_hostname,
		ipbuf1,
		ipbuf2
		);

	//ノード情報初期化
	daemon_NodeTableRead();
	daemon_NodeTableInit();


	//////////////////////////////////////////////////////////////
	//ディスク関連の初期化処理
	//////////////////////////////////////////////////////////////
	//Dataディスク情報
	if ( G_Data_Disks == 0 ){
		sprintf(G_DataDir[0].dir,"%s/data",G_BaseDir);
		G_Data_Disks++;
	}
	if ( G_Data_Disks > l_ConfFileData.max_data_dir ){
		logPrint(DEBUG_ERROR,"Parameter error(data_dir over)");
		return -7;
	}
	for(i=0; i<G_Data_Disks; i++){
		cacheSetVolumeStatus(i,VOLUME_STATUS_INIT);
		if ( filisDirectory(G_DataDir[i].dir) != 1 ){
			cacheSetVolumeStatus(i,VOLUME_STATUS_ERROR);
			logPrint(DEBUG_ERROR,"No Diretory(%s)",G_DataDir[i].dir);
			return -8;
		}
		strcpy(G_DataDir[i].dev,filGetDeviceName(G_DataDir[i].dir));
		//logPrint(DEBUG_INFO,"Data Disk = [%s][%s]",G_DataDir[i].dir, G_DataDir[i].dev);

		//サブディレクトリ作成
		for(j=0; wk_dirs[j][0]!=(char)NULL ; j++){
			sprintf(wk_dir,"%s/%s",G_DataDir[i].dir,wk_dirs[j]);
			if ( filisDirectory(wk_dir) != 1 ){
				ret=mkdir(wk_dir,0777);
				logPrint(DEBUG_INFO,"mkdir(%s,%d)=%d",wk_dir,0777,ret);
				if ( ret  != 0 ){
					cacheSetVolumeStatus(i,VOLUME_STATUS_ERROR);
					logPrint(DEBUG_ERROR,"Make Diretory Error (%s)",wk_dir);
					return -9;
				}
			}
		}

		//Volume-id
		ret=diskrdGetVolumeSCN(i, G_DataDir[i].dir, &(G_DataDir[i].vol_scn));
		if ( ret <= 0 ){
			ret=diskwtWriteVolumeSCN(i, G_DataDir[i].dir, &(G_DataDir[i].vol_scn));
			if ( ret < 0 ){
				cacheSetVolumeStatus(i,VOLUME_STATUS_ERROR);
				logPrint(DEBUG_ERROR,"diskwtWriteVolumeSCN(%d,%s,%s)=%d",i,G_DataDir[i].dir, dispSCN(&(G_DataDir[i].vol_scn)), ret );
			}
		}

	}

	//ディスク情報
	T_SCN c_scn;
	memset(&c_scn, 0, sizeof(T_SCN));
	G_NodeTbl[0].disks=G_Data_Disks;
	memcpy(G_DiskTbl,G_DataDir, sizeof(T_DISK_TABLE) * G_Data_Disks);
	for(i=0; i<G_NodeTbl[0].disks; i++){
		ret=memcmp( &(G_DiskTbl[i].vol_scn), &c_scn, sizeof(T_SCN));
		if ( ret > 0 ){ memcpy(&c_scn, &(G_DiskTbl[i].vol_scn), sizeof(T_SCN)); }
	}
	memcpy(&(G_NodeTbl[0].node_scn), &c_scn, sizeof(T_SCN));

	for(i=0; i<G_NodeTbl[0].disks; i++){
		ret=memcmp( &(G_DiskTbl[i].vol_scn), &(G_NodeTbl[0].node_scn), sizeof(T_SCN));
		if ( ret == 0 ){
			cacheSetVolumeStatus(i,VOLUME_STATUS_SERVICE);
		}else{
			cacheSetVolumeStatus(i,VOLUME_STATUS_SYNC);
		}
		logPrint(DEBUG_INFO,"[%d]volume=%s device=%s scn=%s sts=%d",
			i,G_DiskTbl[i].dir, G_DiskTbl[i].dev, dispSCN(&(G_DiskTbl[i].vol_scn)), G_DiskTbl[i].sts );
	}

	//ルートDIR
	T_FILE_ID new_id;
	memset((char*)&new_id, 0, sizeof(T_FILE_ID));
	ret=diskwtWriteMetaCreateTop(new_id, new_id);
	if ( ret < 0 ){
		logPrint(DEBUG_ERROR,"diskwtWriteMetaCreateTop(%s,%s)=%d",dispFileID(&new_id,buff1), dispFileID(&new_id,buff2), ret);
		return -10; 
	}
	logPrint(DEBUG_INFO,"diskwtWriteMetaCreateTop(%s,%s)=%d",dispFileID(&new_id,buff1), dispFileID(&new_id,buff2), ret);


	//////////////////////////////////////////////////////////////
	//デーモン起動
	//////////////////////////////////////////////////////////////
	int process_cnt=0;
	ret=daemon_exec(process_cnt,PROCESS_TYPE_SMAN,  "SMAN","");
	if ( ret != 0 ){ return -21; }else{ process_cnt++; }

	sprintf(para,"%s:%d",ipbuf1,G_ConfTbl->network_port_svr);
	ret=daemon_exec(process_cnt, PROCESS_TYPE_NLSR, "NLSR",para);
	if ( ret != 0 ){ return -22; }else{ process_cnt++; }

	sprintf(para,"%s:%d",ipbuf2,G_ConfTbl->network_port_clt);
	ret=daemon_exec(process_cnt, PROCESS_TYPE_CLSR, "CLSR",para);
	if ( ret != 0 ){ return -23; }else{ process_cnt++; }

	ret=daemon_exec(process_cnt, PROCESS_TYPE_CACH, "CACH","");
	if ( ret != 0 ){ return -24; }else{ process_cnt++; }

	sprintf(para,"%s:%d",ipbuf1,G_ConfTbl->network_port_cache);
	ret=daemon_exec(process_cnt, PROCESS_TYPE_CRCV, "CRCV",para);
	if ( ret != 0 ){ return -25; }else{ process_cnt++; }

	for(i=0; i<G_ConfTbl->cache_process; i++){
		ret=daemon_exec(process_cnt, PROCESS_TYPE_CSND, "CSND","");
		if ( ret != 0 ){ return -26; }else{ process_cnt++; }
	}

	for(i=0; i<G_ConfTbl->msyn_process; i++){
		ret=daemon_exec(process_cnt, PROCESS_TYPE_MSYN, "MSYN","");
		if ( ret != 0 ){ return -27; }else{ process_cnt++; }
	}

	for(i=0; i<G_ConfTbl->dsyn_process; i++){
		ret=daemon_exec(process_cnt, PROCESS_TYPE_DSYN, "DSYN","");
		if ( ret != 0 ){ return -28; }else{ process_cnt++; }
	}

	for(i=0; i<G_Data_Disks; i++){
		ret=daemon_exec(process_cnt, PROCESS_TYPE_DISK, "DISK",G_DiskTbl[i].dir);
		if ( ret != 0 ){ return -29; }else{ process_cnt++; }
	}

	ret=daemon_exec(process_cnt, PROCESS_TYPE_DMNT, "DMNT", "");
	if ( ret != 0 ){ return -30; }else{ process_cnt++; }

	for(i=0; i<G_ConfTbl->task_process; i++){
		ret=daemon_exec(process_cnt, PROCESS_TYPE_TASK, "TASK","");
		if ( ret != 0 ){ return -31; }else{ process_cnt++; }
	}

	//////////////////////////////////////////////////////////////
	//デーモン起動待ち
	//////////////////////////////////////////////////////////////
	cacheSetNodeStatus(0, SYSTEM_STATUS_SYNC);
	logPrint(DEBUG_INFO,"waiting process start (timeout=%d)",G_ConfTbl->start_wait);
	time(&sv_time);
	while(1){
		ck_flg=0;
		for ( i=0; i<G_ConfTbl->max_process; i++){
			if ( G_ProcTbl[i].pid == 0 ){ continue; }
			if ( G_ProcTbl[i].sts == PROCESS_STATUS_INIT ){ ck_flg++; }
		}
		//printf("%d %d %d  (%d,%d)\n",G_ProcTbl[i].pid,G_ProcTbl[i].sts,ck_flg,sv_time, G_ConfTbl->start_wait);
		if ( ck_flg == 0 ){ break; }

		//タイムアウトチェック
		time(&tm);
		if ( tm > (sv_time + G_ConfTbl->start_wait) ){
			logPrint(DEBUG_ERROR,"daemon start timeout");
			return 1; 
		}
		sleep(1);
		logPrint(DEBUG_INFO,"XICLUSTER child process wainting (%d/%d)", tm - sv_time, G_ConfTbl->start_wait);
	}

	cacheSetNodeStatus(0, SYSTEM_STATUS_RUNNING);
	logPrint(DEBUG_INFO,"XICLUSTER server start");

    logPrintInit(1, G_ConfTbl->debug_level_file, G_ConfTbl->debug_level_stdout);

//for ( int i=0; i<=G_ConfTbl->max_node; i++){
//if ( G_NodeTbl[i].run_hostname[0]==NULL){ break; }
//printf("[%d] %s %X %s\n",i,G_NodeTbl[i].run_hostname, G_NodeTbl[i].svr_ip, utlSHA1_to_HEX(G_NodeTbl[i].node_id));
//}

	return 0;
}


int server_stop(){
	int ret;
	int i;
	int ck_flg;
	time_t sv_time;
	time_t tm;

    logPrintInit(1, DEBUG_DEBUG, DEBUG_DEBUG);
	logPrint(DEBUG_INFO,"XICLUSTER server stopping...");

	if ( shmOpen(G_ShmKey) == 0 ){
		if ( cacheSetAddress(NULL) != 0 ){
			printf("shared memory attach error\n");
			exit(0);
		}

		//常駐プロセス停止
		for ( i=0; i<G_ConfTbl->max_process; i++){
			if ( G_ProcTbl[i].pid == 0 ){ continue; }
			if ( kill(G_ProcTbl[i].pid,0) == 0 ){
				kill(G_ProcTbl[i].pid, 15 );
				logPrint(DEBUG_INFO,"process kill pno=%d pid=%d sig=15", i, G_ProcTbl[i].pid);
			}
		}	

		//停止待ち
		logPrint(DEBUG_INFO,"waiting process stop (timeout=%d)",G_ConfTbl->stop_wait);
		time(&sv_time);
		while(1){
			ck_flg=0;
			for ( i=0; i<G_ConfTbl->max_process; i++){
				if ( G_ProcTbl[i].pid == 0 ){ continue; }
				if ( kill(G_ProcTbl[i].pid,0) == 0 ){ ck_flg++; }
			}
			if ( ck_flg == 0 ){ break; }

			//タイムアウトチェック
			time(&tm);
			if ( tm > (sv_time + G_ConfTbl->stop_wait) ){
				logPrint(DEBUG_ERROR,"daemon stop timeout");
				break;
			}
			sleep(1);
			logPrint(DEBUG_INFO,"XICLUSTER child process wainting (%d/%d)", tm - sv_time, G_ConfTbl->stop_wait);
		}

		//プロセス強制停止
		for ( i=0; i<G_ConfTbl->max_process; i++){
			if ( G_ProcTbl[i].pid == 0 ){ continue; }
			if ( kill(G_ProcTbl[i].pid,0) == 0 ){
				kill(G_ProcTbl[i].pid, 9 );
				logPrint(DEBUG_INFO,"process kill pno=%d pid=%d sig=9", i, G_ProcTbl[i].pid);
			}
		}	

		//メモリ情報出力
		daemon_NodeTableWrite();

		//共有メモリ削除
		ret=shmClose();
		logPrint(DEBUG_INFO,"shared memory delete (key=%X) => %d",G_ShmKey,ret);
	}

	//セマフォ削除
	if ( semOpen(G_SemKey) == 0 ){
		ret=semClose();
		logPrint(DEBUG_INFO,"semaphore delete (key=%X) => %d",G_SemKey,ret);
	}

	//メッセージキュー削除
	//if ( mquOpen(G_MquKey) == 0 ){
	//	ret=mquClose();
	//	logPrint(DEBUG_INFO,"message queues delete (key=%X) => %d",G_MquKey,ret);
	//}

	logPrint(DEBUG_INFO,"XICLUSTER server stop");

	return 0;
}

