/*****************************************************************************
*  <システム>   分散処理フレームワーク
*  <名称>       ディスク書込みデーモン
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

static int process_disk_get_data(){
    int i;
    int ret=(-1);

    if ( semLock(SEM_LOCK_IDXO002,  G_ConfTbl->enospc_retry, G_ConfTbl->enospc_retrans) < 0 ){ return -2; }
    for ( i=0; i<G_ConfTbl->cache_blocks; i++){
        if ( G_IndexTbl[i].sts != CACHE_DATA_STS_MEM_E ){ continue; }
        if ( G_IndexTbl[i].disk_flg != CACHE_DATA_STS_NONE ){ continue; }
		cacheSetFlagIndexDisk(i, CACHE_DATA_STS_WRITE_S);
        ret=i;
        break;
    }
    if ( semUnLock(SEM_LOCK_IDXO002,G_ConfTbl->enospc_retry, G_ConfTbl->enospc_retrans) < 0 ){ return -3; }
    return ret;
}

int process_disk_init(){
	return 0;
}

int process_disk(){
    int i;
	int ret;
	int file_idx;
	char *addr;
	T_CLIENT_REQUEST req;

	//DATAキャッシュ書込み
	i=process_disk_get_data();
	if ( i >= 0 ){
		file_idx=G_IndexTbl[i].file_idx;
		addr=G_DataTbl[i].data;
		ret=diskwtWriteDataCache(addr, G_IndexTbl[i], G_ProcTbl[G_ProcNo].para, G_MetaTbl[file_idx].inf.file_id);
		if ( ret >= 0 ){
			logPrint(DEBUG_DEBUG,"diskwtWriteDataCache(%s)=%d",dispFileID(&(G_MetaTbl[file_idx].inf.file_id)),ret);
			cacheSetFlagIndexDisk(i,CACHE_DATA_STS_WRITE_E);
		}else{
			logPrint(DEBUG_ERROR,"diskwtWriteDataCache(%s)=%d",dispFileID(&(G_MetaTbl[file_idx].inf.file_id)),ret);
			cacheSetFlagIndexDisk(i,CACHE_DATA_STS_ERROR);
		}
    }

	return 0;
}

int process_disk_fin(){
	return 0;
}
