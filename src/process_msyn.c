/*****************************************************************************
*  <システム>   分散処理フレームワーク
*  <名称>	META同期デーモン
*  <目的>	   
*  <機能>	   
*  <開発環境>   UNIX
*  <特記事項>
*
*  VERSION	DATE		BY				CHANGE/COMMENT
*  -----------------------------------------------------------------------------
*  V0.00	  2014/03/29  Takakusaki		新規作成
******************************************************************************/
#include "xi_server.h"

int process_msyn_init(){
	return 0;
}

int process_msyn(){
	int ret;
	int i;
	int fd;
	int master_node=(-1);
	char buff1[32];
	char buff2[32];
	u_long out_cnt=0;
	u_long upd_cnt=0;
	T_META_SYNC_RESULT out_buff[G_ConfTbl->meta_sync_max_rec];
	T_SCN scn;
	static time_t last_check_dt=0;
	time_t t;

	//チェック間隔
	time(&t);
	if ( t <= (G_ConfTbl->meta_check_interval + last_check_dt) ){ return 0; }
	last_check_dt=t;

	//Volume内での最小SCNを求める
	memcpy(&scn, &(G_NodeTbl[0].node_scn), sizeof(T_SCN) );
	for(i=0; i<G_NodeTbl[0].disks; i++){
		if ( memcmp(&(G_DiskTbl[i].vol_scn), &scn, sizeof(T_SCN)) >= 0 ){ continue; }
		memcpy(&scn, &(G_DiskTbl[i].vol_scn), sizeof(T_SCN) );
	}

	//MASTERノード
	master_node=daemon_GetMasterIdx();
	if ( master_node < 0 ){ return 0; }
	if ( memcmp(&scn, &(G_NodeTbl[master_node].node_scn), sizeof(T_SCN)) == 0 ){ return 0; }
	//logPrint(DEBUG_INFO,"MASTER_NODE=%d MASTER_SCN=%s MY_SCN=%s",
	//	master_node, dispSCN(&(G_NodeTbl[master_node].node_scn),buff1), dispSCN(&(scn),buff2) );

	//META同期要求
	ret=transCacheSyncMeta(scn, &out_cnt, out_buff);
	if ( ret < 0 ){
		logPrint(DEBUG_ERROR,"transCacheSyncMeta(%s,%d,*)=%d",dispSCN(&scn),out_cnt,ret);
		return -1;
	}
	logPrint(DEBUG_DEBUG,"transCacheSyncMeta(%s,%d,*)=%d",dispSCN(&scn),out_cnt,ret);
	if ( ret == 0 ){ return 0; }
	if ( out_cnt == 0 ){ return 0; }

	//受信データで自ノードの情報を更新
	for(i=0; i<out_cnt; i++){

		//journalLogとMETA書込み
		if ( out_buff[i].jlog.flg == JOURNAL_TYPE_META ){
			ret=disksyncWriteMeta( &(out_buff[i]) );
			if ( ret < 0 ){
				logPrint(DEBUG_ERROR,"disksyncWriteMeta(%d)=%d",i, ret);
				return -2;
			}
			if ( ret > 0 ){ upd_cnt++; }
			logPrint(DEBUG_DEBUG,"disksyncWriteMeta(%d)=%d",i,ret);
		}

		//journalLogとINDEXヘッダ
		if ( out_buff[i].jlog.flg == JOURNAL_TYPE_INDEX_CRE ){
			ret=disksyncWriteIndexHeader( &(out_buff[i]) );
			if ( ret < 0 ){
				logPrint(DEBUG_ERROR,"disksyncWriteIndexHeader(%d)=%d",i, ret);
				return -3;
			}
			if ( ret > 0 ){ upd_cnt++; }
			logPrint(DEBUG_DEBUG,"disksyncWriteIndexHeader(%d)=%d",i,ret);
		}

		//journalLogとINDEXレコード
		if ( out_buff[i].jlog.flg == JOURNAL_TYPE_INDEX_ADD ){
			ret=disksyncWriteIndexRecord( &(out_buff[i]) );
			if ( ret < 0 ){
				logPrint(DEBUG_ERROR,"disksyncWriteIndexRecord(%d)=%d",i, ret);
				return -3;
			}
			if ( ret > 0 ){ upd_cnt++; }
			logPrint(DEBUG_DEBUG,"disksyncWriteIndexRecord(%d)=%d",i,ret);
		}

		//SCN更新
		if ( out_buff[i].jlog.flg == JOURNAL_TYPE_SCN_UP ){
			ret=disksyncWriteScnCount( &(out_buff[i]) );
			if ( ret < 0 ){
				logPrint(DEBUG_ERROR,"disksyncWriteScnCount(%d)=%d",i, ret);
				return -3;
			}
			if ( ret > 0 ){ upd_cnt++; }
			logPrint(DEBUG_DEBUG,"disksyncWriteScnCount(%d)=%d",i,ret);
		}
	}

	//全VolumeがActiveとなった
	if ( upd_cnt == 0 ){
		for(i=0; i<G_NodeTbl[0].disks; i++){
			cacheSetVolumeStatus(i,VOLUME_STATUS_SERVICE);
		}
		return 0; 
	}

	last_check_dt = 0;
	return 0;
}

int process_msyn_fin(){
	return 0;
}
