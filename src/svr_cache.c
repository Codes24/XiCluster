/*****************************************************************************
*  <システム>   分散処理フレームワーク
*  <名称>	メモリアクセスAPI
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

/*****************************************************************************
*  <関数名> cacheSetAddress
*  <機能>   共有メモリのアドレスを割り当て
*  <説明>   グローバルポインター変数へ共有メモリのアドレスを割り当てる
*  <引数>   なし
*  <リターン値>
*	   0:正常
*	   -1:異常
*  <備考>
******************************************************************************/
int cacheSetAddress(T_SHM_HEADER *hwk)
{

	/* 共有メモリのアドレス割り当て／グローバル変数設定 */
	G_ShmHeader = (T_SHM_HEADER *)(G_shmaddr);
	if ( G_ShmHeader == NULL ){ return -1; }
	if ( hwk != NULL ){
		memcpy((char*)G_ShmHeader, (char*)hwk, sizeof(T_SHM_HEADER));
	}
	G_ConfTbl   = (T_CONFIG_TABLE *)(G_shmaddr + G_ShmHeader->offset_conf);
	G_NodeTbl   = (T_NODE_TABLE *)(G_shmaddr + G_ShmHeader->offset_node);
	G_ProcTbl   = (T_PROCESS_TABLE *)(G_shmaddr + G_ShmHeader->offset_proc);
	G_MetaTbl   = (T_META_TABLE *)(G_shmaddr + G_ShmHeader->offset_meta);
	G_DiskTbl   = (T_DISK_TABLE *)(G_shmaddr + G_ShmHeader->offset_disk);
	G_IndexTbl  = (T_INDEX_TABLE *)(G_shmaddr + G_ShmHeader->offset_index);
	G_DataTbl   = (T_DATA_TABLE *)(G_shmaddr + G_ShmHeader->offset_data);
	return 0;
}

int cacheSetNodeStatus(int idx, int sts){
	if ( G_NodeTbl[idx].sts != sts ){
		logPrint(DEBUG_INFO,"G_NodeTbl[%d].sts %d -> %d",idx, G_NodeTbl[idx].sts, sts);
		G_NodeTbl[idx].sts=sts;
	}
	return 0;
}

int cacheSetProcStatus(int pno, int sts){
	if ( G_ProcTbl[pno].sts != sts ){
		//logPrint(DEBUG_DEBUG,"G_ProcTbl[%d].sts %d -> %d",pno, G_ProcTbl[pno].sts, sts);
		G_ProcTbl[pno].sts=sts;
	}
	return 0;
}

int cacheSetMasterFlag(int idx, int sts){
	if ( G_NodeTbl[idx].master_flg != sts ){
		logPrint(DEBUG_INFO,"G_NodeTbl[%d].master_flg %d -> %d",idx, G_NodeTbl[idx].master_flg, sts);
		G_NodeTbl[idx].master_flg=sts;
	}
	return 0;
}

int cacheSetFlagIndexStatus(int idx, int sts){
	logPrint(DEBUG_DEBUG,"G_IndexTbl[%d].sts %d -> %d",idx, G_IndexTbl[idx].sts, sts);
	G_IndexTbl[idx].sts=sts;
	return 0;
}

int cacheSetFlagIndexDisk(int idx, int sts){
	logPrint(DEBUG_DEBUG,"G_IndexTbl[%d].disk_flg %d -> %d",idx, G_IndexTbl[idx].disk_flg, sts);
	G_IndexTbl[idx].disk_flg=sts;
	return 0;
}

int cacheSetFlagIndexNode(int idx, int sts){
	logPrint(DEBUG_DEBUG,"G_IndexTbl[%d].node_flg %d -> %d",idx, G_IndexTbl[idx].node_flg, sts);
	G_IndexTbl[idx].node_flg=sts;
	return 0;
}

int cacheSetFlagMetaStatus(int idx, int sts){
	logPrint(DEBUG_DEBUG,"G_MetaTbl[%d].sts %d -> %d",idx, G_MetaTbl[idx].sts, sts);
	G_MetaTbl[idx].sts=sts;
	return 0;
}

int cacheSetFlagMetaDisk(int idx, int sts){
	logPrint(DEBUG_DEBUG,"G_MetaTbl[%d].disk_flg %d -> %d",idx, G_MetaTbl[idx].disk_flg, sts);
	G_MetaTbl[idx].disk_flg=sts;
	return 0;
}

int cacheSetFlagMetaNode(int idx, int sts){
	logPrint(DEBUG_DEBUG,"G_MetaTbl[%d].node_flg %d -> %d",idx, G_MetaTbl[idx].node_flg, sts);
	G_MetaTbl[idx].node_flg=sts;
	return 0;
}

int cacheSetVolumeStatus(int idx, int sts){
	if ( G_DiskTbl[idx].sts != sts ){
		logPrint(DEBUG_INFO,"G_DiskTbl[%d].sts %d -> %d",idx, G_DiskTbl[idx].sts, sts);
		G_DiskTbl[idx].sts=sts;
	}
	return 0;
}

int cacheMetaCached(int idx){

	logPrint(DEBUG_DEBUG,"cacheMetaCached(%d)",idx);

	if ( semLock(SEM_LOCK_IDXO001,  G_ConfTbl->enospc_retry, G_ConfTbl->enospc_retrans) < 0 ){ return -1; }
	G_MetaTbl[idx].req=REQUEST_CLIENT_NONE;
	G_MetaTbl[idx].sts=CACHE_META_STS_CACHE;
	if ( semUnLock(SEM_LOCK_IDXO001,G_ConfTbl->enospc_retry, G_ConfTbl->enospc_retrans) < 0 ){ return -3; }
	return 0;
}

int cacheMetaRelease(int idx){

	logPrint(DEBUG_DEBUG,"cacheMetaRelease(%d)",idx);

	if ( semLock(SEM_LOCK_IDXO001,  G_ConfTbl->enospc_retry, G_ConfTbl->enospc_retrans) < 0 ){ return -1; }
	G_MetaTbl[idx].req=REQUEST_CLIENT_NONE;
	G_MetaTbl[idx].pid=0;
	G_MetaTbl[idx].w_tm.tv_sec=0;
	G_MetaTbl[idx].w_tm.tv_usec=0;
	G_MetaTbl[idx].disk_wcnt=0;
	G_MetaTbl[idx].node_wcnt=0;
	G_MetaTbl[idx].disk_flg=CACHE_META_STS_NONE;
	G_MetaTbl[idx].node_flg=CACHE_META_STS_NONE;
	G_MetaTbl[idx].read_flg=CACHE_META_STS_NONE;
	G_MetaTbl[idx].sts=CACHE_META_STS_NONE;
	if ( semUnLock(SEM_LOCK_IDXO001,G_ConfTbl->enospc_retry, G_ConfTbl->enospc_retrans) < 0 ){ return -3; }
	return 0;
}

int cacheDataCached(int idx){

	logPrint(DEBUG_DEBUG,"cacheDataCached(%d)",idx);

	if ( semLock(SEM_LOCK_IDXO002,  G_ConfTbl->enospc_retry, G_ConfTbl->enospc_retrans) < 0 ){ return -1; }
	G_IndexTbl[idx].sts=CACHE_DATA_STS_CACHE;
	if ( semUnLock(SEM_LOCK_IDXO002,G_ConfTbl->enospc_retry, G_ConfTbl->enospc_retrans) < 0 ){ return -3; }
	return 0;
}

int cacheDataRelease(int idx){

	logPrint(DEBUG_DEBUG,"cacheDataRelease(%d)",idx);

	if ( semLock(SEM_LOCK_IDXO002,  G_ConfTbl->enospc_retry, G_ConfTbl->enospc_retrans) < 0 ){ return -1; }
	G_IndexTbl[idx].pid=0;
	G_IndexTbl[idx].size=0;
	G_IndexTbl[idx].w_tm.tv_sec=0;
	G_IndexTbl[idx].w_tm.tv_usec=0;
	G_IndexTbl[idx].disk_flg=CACHE_DATA_STS_NONE;
	G_IndexTbl[idx].node_flg=CACHE_DATA_STS_NONE;
	G_IndexTbl[idx].read_flg=CACHE_DATA_STS_NONE;
	G_IndexTbl[idx].sts=CACHE_DATA_STS_NONE;
	if ( semUnLock(SEM_LOCK_IDXO002,G_ConfTbl->enospc_retry, G_ConfTbl->enospc_retrans) < 0 ){ return -3; }
	return 0;
}

int cacheGetFreeFileCache(){
	int i;
	int free_no=(-1);
	int loop_cnt;
	int free_no2=(-1);
	struct timeval dt;

	gettimeofday(&dt,NULL);

	for(loop_cnt=0; loop_cnt<G_ConfTbl->lock_retry; loop_cnt++){

		if ( semLock(SEM_LOCK_IDXO001,  G_ConfTbl->enospc_retry, G_ConfTbl->enospc_retrans) < 0 ){ return -2; }
		for ( i=0; i<G_ConfTbl->cache_files; i++){
			if ( G_MetaTbl[i].read_flg > 0 ){ continue; }
			if ( G_MetaTbl[i].sts == CACHE_META_STS_NONE ){
				free_no=i;
				break;
			}
			if ( G_MetaTbl[i].sts == CACHE_META_STS_CACHE ){
				if ( tmCompareMicroSec(G_MetaTbl[i].w_tm, dt) < 0 ){
					memcpy(&dt, &(G_MetaTbl[i].w_tm), sizeof(dt));
					free_no2=i;
				}
			}
		}

		if ( free_no==(-1) && free_no2 >= 0 ){
			free_no = free_no2;
		}
		if ( free_no >= 0 ){
			G_MetaTbl[free_no].req=REQUEST_CLIENT_NONE;
			G_MetaTbl[free_no].sts=CACHE_META_STS_MEM_S;
			G_MetaTbl[free_no].disk_flg=CACHE_META_STS_NONE;
			G_MetaTbl[free_no].node_flg=CACHE_META_STS_NONE;
			G_MetaTbl[free_no].read_flg=CACHE_META_STS_NONE;
			G_MetaTbl[free_no].pid=getpid();
			G_MetaTbl[free_no].disk_wcnt=0;
			G_MetaTbl[free_no].node_wcnt=0;
			gettimeofday(&(G_MetaTbl[free_no].w_tm),NULL);
		}
		if ( semUnLock(SEM_LOCK_IDXO001,G_ConfTbl->enospc_retry, G_ConfTbl->enospc_retrans) < 0 ){ return -3; }
		if ( free_no >= 0 ){ return free_no; }

		usleep(G_ConfTbl->lock_sleep);
	}

	return -1;
}

int cacheGetFreeDiskCache(){
	int i;
	int free_no=(-1);
	int loop_cnt;
	int free_no2=(-1);
	struct timeval dt;

	gettimeofday(&dt,NULL);

	for(loop_cnt=0; loop_cnt<G_ConfTbl->lock_retry; loop_cnt++){

		if ( semLock(SEM_LOCK_IDXO002,  G_ConfTbl->enospc_retry, G_ConfTbl->enospc_retrans) < 0 ){ return -2; }

		for ( i=0; i<G_ConfTbl->cache_blocks; i++){
			if ( G_IndexTbl[i].read_flg > 0 ){ continue; }
			if ( G_IndexTbl[i].sts == CACHE_DATA_STS_NONE ){
				free_no=i;
				break;
			}
			if ( G_IndexTbl[i].sts == CACHE_DATA_STS_CACHE ){
				if ( tmCompareMicroSec(G_IndexTbl[i].w_tm, dt) < 0 ){
					memcpy(&dt, &(G_IndexTbl[i].w_tm), sizeof(dt));
					free_no2=i;
				}
			}
		}

		if ( free_no==(-1) && free_no2 >= 0 ){
			free_no = free_no2;
		}
		if ( free_no >= 0 ){
			G_IndexTbl[free_no].sts=CACHE_DATA_STS_MEM_S;
			G_IndexTbl[free_no].size=0;
			G_IndexTbl[free_no].disk_flg=CACHE_DATA_STS_NONE;
			G_IndexTbl[free_no].node_flg=CACHE_DATA_STS_NONE;
			G_IndexTbl[free_no].read_flg=CACHE_DATA_STS_NONE;
			G_IndexTbl[free_no].pid=getpid();
			gettimeofday(&(G_IndexTbl[free_no].w_tm),NULL);
		}

		if ( semUnLock(SEM_LOCK_IDXO002,G_ConfTbl->enospc_retry, G_ConfTbl->enospc_retrans) < 0 ){ return -3; }
		if ( free_no >= 0 ){ return free_no; }

		usleep(G_ConfTbl->lock_sleep);
	}

	return -1;
}

int cacheGetFreeTaskNo(){
	int i;
	int free_no=(-1);

	for ( i=1; i<G_ConfTbl->max_process; i++){
		if ( G_ProcTbl[i].name[0] == (char)NULL ){ continue; }
		if ( G_ProcTbl[i].ptype != PROCESS_TYPE_TASK ){ continue; }
		if ( G_ProcTbl[i].req != 0){ continue; }
		if ( G_ProcTbl[i].sts != PROCESS_STATUS_IDLE){ continue; }
		G_ProcTbl[i].req = REQUEST_CLIENT_REQUEST;
		free_no=i;
		break;
	}

	return free_no;
}

int cacheMetaWrite(T_META_HEADER hd, T_META_INFO fi, int req_no){
	int ret;
	int free_no;

	//空きMETAキャッシュ検索
	free_no=cacheGetFreeFileCache();
	if ( free_no < 0 ){
		logPrint(DEBUG_ERROR,"cacheGetFreeFileCache()=%d",free_no);
		return -1;
	}

	//METAキャッシュ設定
	memcpy( &(G_MetaTbl[free_no].inf), &(fi), sizeof(T_META_INFO));
	memcpy( &(G_MetaTbl[free_no].row), &(hd.row), sizeof(T_DIR_ID));
	G_MetaTbl[free_no].req=req_no;
	cacheSetFlagMetaDisk(free_no, CACHE_META_STS_USE);
	cacheSetFlagMetaNode(free_no, CACHE_META_STS_USE);
	cacheSetFlagMetaStatus(free_no, CACHE_META_STS_MEM_E);

	return 0;
}

int cacheCheckBlockWrite(T_FILE_ID *file_id, u_long block_no){
	int i;

	for ( i=0; i<G_ConfTbl->cache_blocks; i++){
		if ( G_IndexTbl[i].sts == CACHE_DATA_STS_NONE ){ continue; }
		if ( G_IndexTbl[i].sts == CACHE_DATA_STS_CACHE ){ continue; }
		if ( G_IndexTbl[i].block_no == block_no &&
				memcmp( &(G_MetaTbl[ G_IndexTbl[i].file_idx ].inf.file_id), file_id, sizeof(T_FILE_ID)) == 0 ){
			return 1;
		}
	}
	return 0;
}

int cacheCheckCachedBlock(T_FILE_ID *file_id, u_long block_no){
	int i;
	int idx=-1;
	int meta_idx;

	if ( semLock(SEM_LOCK_IDXO002,  G_ConfTbl->enospc_retry, G_ConfTbl->enospc_retrans) < 0 ){ return -2; }
	for ( i=0; i<G_ConfTbl->cache_blocks; i++){
		if ( G_IndexTbl[i].sts != CACHE_DATA_STS_CACHE ){ continue; }
		if ( G_IndexTbl[i].block_no != block_no ){ continue; }
		meta_idx=G_IndexTbl[i].file_idx;
		if ( memcmp( &(G_MetaTbl[ meta_idx ].inf.file_id), file_id, sizeof(T_FILE_ID)) != 0 ){ continue; }
		G_IndexTbl[i].read_flg++;
		idx=i;
		break;
	}
	if ( semUnLock(SEM_LOCK_IDXO002,G_ConfTbl->enospc_retry, G_ConfTbl->enospc_retrans) < 0 ){ return -3; }

	G_ProcTbl[G_ProcNo].cache_idx=idx;
//printf("S:G_ProcTbl[%d].cache_idx=%d\n",G_ProcNo,idx);

	return idx;
}

int cacheCheckCachedRelease(int idx){

	G_ProcTbl[G_ProcNo].cache_idx=(-1);
//printf("R:G_ProcTbl[%d].cache_idx=%d\n",G_ProcNo,idx);

	if ( semLock(SEM_LOCK_IDXO002,  G_ConfTbl->enospc_retry, G_ConfTbl->enospc_retrans) < 0 ){ return -2; }
	G_IndexTbl[idx].read_flg--;
	if ( G_IndexTbl[idx].read_flg < 0 ){ G_IndexTbl[idx].read_flg=0; }
	if ( semUnLock(SEM_LOCK_IDXO002,G_ConfTbl->enospc_retry, G_ConfTbl->enospc_retrans) < 0 ){ return -3; }
	return 0;
}
