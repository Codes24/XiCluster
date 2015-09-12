/*****************************************************************************
*  <システム>   分散処理フレームワーク
*  <名称>	キャッシュ管理デーモン
*  <目的>	
*  <機能>	   
*  <開発環境>   UNIX
*  <特記事項>
*
*  VERSION	DATE		BY				CHANGE/COMMENT
*  -----------------------------------------------------------------------------
*  V0.00	  2014/01/18  Takakusaki		新規作成
******************************************************************************/
#include "xi_server.h"

static int  l_server_fd;
static int  l_client_fd;
static char l_sock_file[MAX_FILE_PATH+1];
static int  l_gate_fd;

int process_cach_init(){
	return 0;
}

int process_cach(){
	int i,j;
	int ret;
	int file_idx;
	time_t tm;
	static time_t last_watch=0;
	int free_cnt;
	int use_cnt;

	///////////////////////////////////////////////////////////////////////////////
	//METAキャッシュクリア
	///////////////////////////////////////////////////////////////////////////////
	free_cnt=0;
	use_cnt=0;
	for ( i=0; i<G_ConfTbl->cache_files; i++){
		if ( G_MetaTbl[i].disk_flg == CACHE_META_STS_ERROR ){
			cacheSetFlagMetaDisk(i, CACHE_META_STS_USE);
		}
		if ( G_MetaTbl[i].node_flg == CACHE_META_STS_ERROR ){
			cacheSetFlagMetaNode(i, CACHE_META_STS_USE);
		}
		if ( G_MetaTbl[i].sts == CACHE_META_STS_USE ){
			ret=cacheMetaCached(i);
		}
		if ( G_MetaTbl[i].sts == CACHE_META_STS_ERROR){
			ret=cacheMetaRelease(i);
		}
		if ( G_MetaTbl[i].sts == CACHE_META_STS_NONE || G_MetaTbl[i].sts == CACHE_META_STS_CACHE ){
			free_cnt++;
		}else{
			use_cnt++;
		}
	}
	G_NodeTbl[0].cache_meta_use=use_cnt;

	for ( i=0; i<G_ConfTbl->cache_files; i++){
		if ( G_MetaTbl[i].sts != CACHE_META_STS_MEM_S){ continue; }
		if ( G_MetaTbl[i].disk_flg != CACHE_META_STS_USE){ continue; }
		if ( G_MetaTbl[i].node_flg != CACHE_META_STS_USE){ continue; }
		cacheSetFlagMetaStatus(i, CACHE_META_STS_USE);
	}

	///////////////////////////////////////////////////////////////////////////////
	//DATAキャッシュクリア
	///////////////////////////////////////////////////////////////////////////////
	free_cnt=0;
	use_cnt=0;
	for ( i=0; i<G_ConfTbl->cache_blocks; i++){

		file_idx=G_IndexTbl[i].file_idx;

		if ( G_IndexTbl[i].disk_flg == CACHE_DATA_STS_ERROR ){
			cacheSetFlagIndexDisk(i,CACHE_DATA_STS_USE);
			cacheSetFlagMetaDisk(file_idx, CACHE_META_STS_USE);
		}
		if ( G_IndexTbl[i].node_flg == CACHE_DATA_STS_ERROR ){
			cacheSetFlagIndexNode(i,CACHE_DATA_STS_USE);
			cacheSetFlagMetaNode(file_idx, CACHE_META_STS_USE);
		}
		if ( G_IndexTbl[i].sts == CACHE_DATA_STS_MEM_E ){
			if ( G_IndexTbl[i].disk_flg == CACHE_DATA_STS_USE && G_IndexTbl[i].node_flg == CACHE_DATA_STS_USE ){
				ret=cacheDataCached(i);
			}
		}
		if ( G_IndexTbl[i].sts == CACHE_DATA_STS_ERROR){
		   	ret=cacheDataRelease(i);
		}
		if ( G_IndexTbl[i].sts == CACHE_DATA_STS_NONE || G_IndexTbl[i].sts == CACHE_DATA_STS_CACHE ){
			free_cnt++;
		}else{
			use_cnt++;
		}
	}
	G_NodeTbl[0].cache_data_use=use_cnt;

	time(&tm);
	if ( tm < last_watch + G_ConfTbl->node_watch_interval ){ return 0; }
	last_watch=tm;

	//////////////////////////////////////////////////////////////////
	//ノード無応答監視
	//////////////////////////////////////////////////////////////////
	for (i=1; i<G_ConfTbl->max_node; i++){
		if ( G_NodeTbl[i].run_hostname[0] == (char)NULL ){ continue; }
		if ( G_NodeTbl[i].sts != SYSTEM_STATUS_RUNNING ){ continue; }
		if ( tm > G_NodeTbl[i].alv + G_ConfTbl->node_hang_timeout ){
			logPrint(DEBUG_WARNING,"node check timeout(pno=%d idx=%d)",G_ProcNo,i);
			cacheSetNodeStatus(i,SYSTEM_STATUS_NORES);
		}
	}

	//////////////////////////////////////////////////////////////////
	//ノード排除監視
	//////////////////////////////////////////////////////////////////
	for (i=1; i<G_ConfTbl->max_node; i++){
		if ( G_NodeTbl[i].run_hostname[0] == (char)NULL ){ continue; }
		if ( tm > G_NodeTbl[i].alv + G_ConfTbl->node_removal_time ){
			logPrint(DEBUG_WARNING,"node removal(pno=%d idx=%d)",G_ProcNo,i);
			memset((char*)&G_NodeTbl[i], 0, sizeof(T_NODE_TABLE) );
		}
	}

	//////////////////////////////////////////////////////////////////
	//パフォーマンス情報更新
	//////////////////////////////////////////////////////////////////
	//CPU使用率
	perfGetCPUavg(
		&G_NodeTbl[0].cpu_cnt,
		&G_NodeTbl[0].cpu_user,
		&G_NodeTbl[0].cpu_nice,
		&G_NodeTbl[0].cpu_system,
		&G_NodeTbl[0].cpu_idle,
		&G_NodeTbl[0].cpu_iowait
	);

	//メモリ使用量
	perfGetMEM(
		&G_NodeTbl[0].mem_size,
		&G_NodeTbl[0].mem_free,
		&G_NodeTbl[0].mem_buffer,
		&G_NodeTbl[0].mem_cache,
		&G_NodeTbl[0].mem_active,
		&G_NodeTbl[0].mem_inactive
	);

	//ディスク使用量
	int	dev_cnt=0;
	u_long disk_total=0;
	u_long disk_use=0;
	u_long disk_free=0;
	int cmp_flg=0;
	for(i=0; i<G_NodeTbl[0].disks; i++){
		perfGetDISK(
			G_DiskTbl[i].dir,
			&G_DiskTbl[i].f_bsize,
			&G_DiskTbl[i].f_blocks,
			&G_DiskTbl[i].f_use,
			&G_DiskTbl[i].f_free
		);
		perfGetDISKSTAT(
			G_DiskTbl[i].dev,
			&G_DiskTbl[i].stat_a,
			&G_DiskTbl[i].stat_s
		);
	}

	//ディスク負荷状況
	for(i=0; i<G_NodeTbl[0].disks; i++){
		cmp_flg=0;
		for(j=0; j<i; j++){
			if ( strcmp(G_DiskTbl[i].dev, G_DiskTbl[j].dev) == 0 ){ cmp_flg++; }
		}
		if ( cmp_flg != 0 ){ continue; }
		dev_cnt++;
		disk_total += (u_long)((float)G_DiskTbl[i].f_bsize * G_DiskTbl[i].f_blocks / 1024/1024);
		disk_use += (u_long)((float)G_DiskTbl[i].f_bsize * G_DiskTbl[i].f_use / 1024/1024);
		disk_free += (u_long)((float)G_DiskTbl[i].f_bsize * G_DiskTbl[i].f_free / 1024/1024);
	}
	G_NodeTbl[0].devices=dev_cnt;
	G_NodeTbl[0].disk_size = disk_total;
	G_NodeTbl[0].disk_use = disk_use;
	G_NodeTbl[0].disk_free = disk_free;


	//////////////////////////////////////////////////////////////////
	//マスタノード判定
	//////////////////////////////////////////////////////////////////
	ret=daemon_CheckMaster();
	if ( ret == MASTER_FLG_MASTER ){
		if ( G_NodeTbl[0].master_flg != MASTER_FLG_MASTER ){
			daemon_GetSCN(&(G_NodeTbl[0].node_scn), 2);

			ret=diskwtWriteScnCount();
			if ( ret != 0 ){
				logPrint(DEBUG_ERROR,"diskwtWriteScnCount() => %d",ret);
			}
		}
		cacheSetMasterFlag(0,MASTER_FLG_MASTER);
	}else if ( ret == MASTER_FLG_SLAVE ){
		cacheSetMasterFlag(0,MASTER_FLG_SLAVE);
	}else{
		cacheSetMasterFlag(0,0);
	}
	for (i=1; i<G_ConfTbl->max_node; i++){
		if ( G_NodeTbl[i].run_hostname[0] == (char)NULL ){ continue; }
		if ( G_NodeTbl[i].sts == SYSTEM_STATUS_RUNNING ){ continue; }
		if ( G_NodeTbl[i].master_flg != MASTER_FLG_MASTER ){ continue; }
		cacheSetMasterFlag(i,0);
	}


	return 0;
}

int process_cach_fin(){
	return 0;
}
