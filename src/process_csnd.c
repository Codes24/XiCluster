/*****************************************************************************
*  <システム>	分散処理フレームワーク
*  <名称>	DATAキャッシュ転送デーモン
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

static int DiskIndexUpdate(int i){
	int file_idx;

	//情報更新
	file_idx=G_IndexTbl[i].file_idx;
	G_MetaTbl[file_idx].node_wcnt += 1;
	logPrint(DEBUG_DEBUG,"DiskIndexUpdate() idx[%d]=(block_no=%d sts=%d,%d,%d size=%d) meta[%d]=(sts=%d,%d,%d write=%d/%d)",
			i,
			G_IndexTbl[i].block_no,
			G_IndexTbl[i].sts,
			G_IndexTbl[i].disk_flg,
			G_IndexTbl[i].node_flg,
			G_IndexTbl[i].size,
			file_idx,
			G_MetaTbl[file_idx].sts,
			G_MetaTbl[file_idx].disk_flg,
			G_MetaTbl[file_idx].node_flg,
			G_MetaTbl[file_idx].node_wcnt,
			G_MetaTbl[file_idx].inf.st_blocks
		);

	//最終ブロックの場合はMETAを更新
	if ( G_MetaTbl[file_idx].node_wcnt >= G_MetaTbl[file_idx].inf.st_blocks ){
		cacheSetFlagMetaNode(file_idx, CACHE_META_STS_USE);
	}
	return 0;
}

static int NodeIndexUpdate(int i){
	int file_idx;

	//情報更新
	file_idx=G_IndexTbl[i].file_idx;
	G_MetaTbl[file_idx].disk_wcnt += 1;
	logPrint(DEBUG_DEBUG,"NodeIndexUpdate() idx[%d]=(block_no=%d sts=%d,%d,%d size=%d) meta[%d]=(sts=%d,%d,%d write=%d/%d)",
			i,
			G_IndexTbl[i].block_no,
			G_IndexTbl[i].sts,
			G_IndexTbl[i].disk_flg,
			G_IndexTbl[i].node_flg,
			G_IndexTbl[i].size,
			file_idx,
			G_MetaTbl[file_idx].sts,
			G_MetaTbl[file_idx].disk_flg,
			G_MetaTbl[file_idx].node_flg,
			G_MetaTbl[file_idx].disk_wcnt,
			G_MetaTbl[file_idx].inf.st_blocks
	);

	//最終ブロックの場合はMETAを更新
	if ( G_MetaTbl[file_idx].disk_wcnt >= G_MetaTbl[file_idx].inf.st_blocks ){
		cacheSetFlagMetaDisk(file_idx,CACHE_META_STS_USE);
	}
	return 0;
}

static int process_csnd_get_data_trans(){
	int i;
	int ret=(-1);

	if ( semLock(SEM_LOCK_IDXO002,  G_ConfTbl->enospc_retry, G_ConfTbl->enospc_retrans) < 0 ){ return -2; }
	for ( i=0; i<G_ConfTbl->cache_blocks; i++){
		if ( G_IndexTbl[i].sts != CACHE_DATA_STS_MEM_E ){ continue; }
		if ( G_IndexTbl[i].node_flg != CACHE_DATA_STS_NONE ){ continue; }
		cacheSetFlagIndexNode(i,CACHE_DATA_STS_WRITE_S);
		ret=i;
		break;
	}
	if ( semUnLock(SEM_LOCK_IDXO002,G_ConfTbl->enospc_retry, G_ConfTbl->enospc_retrans) < 0 ){ return -3; }
	return ret;
}

static int process_csnd_get_index_node(){
	int i;
	int ret=(-1);

	if ( semLock(SEM_LOCK_IDXO002,  G_ConfTbl->enospc_retry, G_ConfTbl->enospc_retrans) < 0 ){ return -2; }
	for ( i=0; i<G_ConfTbl->cache_blocks; i++){
		if ( G_IndexTbl[i].sts != CACHE_DATA_STS_MEM_E ){ continue; }
		if ( G_IndexTbl[i].node_flg != CACHE_DATA_STS_WRITE_E ){ continue; }
		cacheSetFlagIndexNode(i,CACHE_DATA_STS_SEND);
		ret=i;
		break;
	}
	if ( semUnLock(SEM_LOCK_IDXO002,G_ConfTbl->enospc_retry, G_ConfTbl->enospc_retrans) < 0 ){ return -3; }
	return ret;
}

static int process_csnd_get_index_disk(){
	int i;
	int ret=(-1);

	if ( semLock(SEM_LOCK_IDXO002,  G_ConfTbl->enospc_retry, G_ConfTbl->enospc_retrans) < 0 ){ return -2; }
	for ( i=0; i<G_ConfTbl->cache_blocks; i++){
		if ( G_IndexTbl[i].sts != CACHE_DATA_STS_MEM_E ){ continue; }
		if ( G_IndexTbl[i].disk_flg != CACHE_DATA_STS_WRITE_E ){ continue; }
		cacheSetFlagIndexDisk(i,CACHE_DATA_STS_SEND);
		ret=i;
		break;
	}
	if ( semUnLock(SEM_LOCK_IDXO002,G_ConfTbl->enospc_retry, G_ConfTbl->enospc_retrans) < 0 ){ return -3; }
	return ret;
}

static int process_csnd_get_meta(){
	int i;
	int ret=(-1);

	if ( semLock(SEM_LOCK_IDXO001,  G_ConfTbl->enospc_retry, G_ConfTbl->enospc_retrans) < 0 ){ return -2; }
	for ( i=0; i<G_ConfTbl->cache_files; i++){
		if ( G_MetaTbl[i].sts != CACHE_META_STS_MEM_E ){ continue; }
		if ( G_MetaTbl[i].disk_flg != CACHE_META_STS_USE ){ continue; }
		if ( G_MetaTbl[i].node_flg != CACHE_META_STS_USE ){ continue; }
		cacheSetFlagMetaStatus(i, CACHE_META_STS_SEND);
		ret=i;
		break;
	}
	if ( semUnLock(SEM_LOCK_IDXO001,G_ConfTbl->enospc_retry, G_ConfTbl->enospc_retrans) < 0 ){ return -3; }
	return ret;
}

int process_csnd_init(){
	int ret;
	return 0;
}

int process_csnd(){
	int i,j;
	int ret;

	///////////////////////////////////////////////////////////////////////////////
	//DATAキャッシュ転送
	///////////////////////////////////////////////////////////////////////////////
	i=process_csnd_get_data_trans();
	if ( i >= 0 ){

		//DATAキャッシュ転送
		ret=transCacheSendData(G_ProcNo,i);
		if ( ret < 0 ){
			logPrint(DEBUG_ERROR,"transCacheSendData(%d,%d)=%d",G_ProcNo,i,ret);
			cacheSetFlagIndexNode(i,CACHE_DATA_STS_ERROR);
			return 0;
		}
		if ( ret == 0 ){
			logPrint(DEBUG_DEBUG,"transCacheSendData(%d,%d)=%d (NO_REMOTE_HOST)",G_ProcNo,i,ret);
			cacheSetFlagIndexNode(i,CACHE_DATA_STS_USE);
			DiskIndexUpdate(i);
			return 0;
		}
		logPrint(DEBUG_DEBUG,"transCacheSendData(%d,%d)=%d",G_ProcNo,i,ret);

		//フラグ更新
		cacheSetFlagIndexNode(i,CACHE_DATA_STS_WRITE_E);
	}

	///////////////////////////////////////////////////////////////////////////////
	//INDEXキャッシュのマスタ転送の完了通知
	///////////////////////////////////////////////////////////////////////////////
	i=process_csnd_get_index_node();
	if ( i >= 0 ){
		//フラグ更新
		cacheSetFlagIndexNode(i,CACHE_DATA_STS_USE);

		//転送完了後始末
		DiskIndexUpdate(i);
	}

	///////////////////////////////////////////////////////////////////////////////
	//INDEXキャッシュLocalDisk書込み完了通知
	///////////////////////////////////////////////////////////////////////////////
	i=process_csnd_get_index_disk();
	if ( i >= 0 ){

		//INDEXキャッシュ転送
		int file_idx=G_IndexTbl[i].file_idx;
		ret=transCacheSendIndexAdd(G_ProcNo, G_MetaTbl[file_idx].row.dir_id, G_MetaTbl[file_idx].inf.file_id, G_IndexTbl[i].block_no, G_IndexTbl[i].size);
		if ( ret < 0 ){
			logPrint(DEBUG_ERROR,"transCacheSendIndexAdd(%d,%s,%s,%d,%d)=%d", G_ProcNo,
				dispFileID( &(G_MetaTbl[file_idx].row.dir_id) ),
				dispFileID( &(G_MetaTbl[file_idx].inf.file_id) ),
				G_IndexTbl[i].block_no, G_IndexTbl[i].size,ret);
			cacheSetFlagIndexDisk(i,CACHE_DATA_STS_ERROR);
		}else{
			logPrint(DEBUG_INFO,"transCacheSendIndexAdd(%d,%s,%s,%d,%d)=%d",G_ProcNo,
				dispFileID( &(G_MetaTbl[file_idx].row.dir_id) ),
				dispFileID( &(G_MetaTbl[file_idx].inf.file_id) ),
				G_IndexTbl[i].block_no, G_IndexTbl[i].size,ret);
			cacheSetFlagIndexDisk(i,CACHE_DATA_STS_USE);
		}

		//転送完了後始末
		NodeIndexUpdate(i);
	}

	///////////////////////////////////////////////////////////////////////////////
	//METAキャッシュ出力の完了通知
	///////////////////////////////////////////////////////////////////////////////
	i=process_csnd_get_meta();
	if ( i >= 0 ){

		//METAキャッシュ出力の完了通知
		ret=transCacheSendMeta(G_ProcNo,i);
		if ( ret == 0 ){
			logPrint(DEBUG_DEBUG,"transCacheSendMeta(%d,%d)=%d",G_ProcNo,i,ret);
			cacheSetFlagMetaStatus(i, CACHE_META_STS_USE);
		}else{
			logPrint(DEBUG_ERROR,"transCacheSendMeta(%d,%d)=%d",G_ProcNo,i,ret);
			cacheSetFlagMetaStatus(i, CACHE_META_STS_ERROR);
		}
	}

	return 0;
}

int process_csnd_fin(){
	int ret;
	return 0;
}
