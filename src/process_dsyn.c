/*****************************************************************************
*  <システム>   分散処理フレームワーク
*  <名称>	DATA同期デーモン
*  <目的>	自ノードのINDEXを読み込んで、別ノードよりデータblockデータを取得する
*  <機能>	   
*  <開発環境>   UNIX
*  <特記事項>
*
*  VERSION	DATE		BY				CHANGE/COMMENT
*  -----------------------------------------------------------------------------
*  V0.00	  2014/03/29  Takakusaki		新規作成
******************************************************************************/
#include "xi_server.h"

typedef struct{
	short node_no;
	short wflg;
	time_t w_tm;
	u_int cnt;
	u_int size;
} T_DSYNC_TARGET;

int process_dsyn_init(){
	return 0;
}

int process_dsyn(){
	int i,j;
	int ret;
	int fd;
	int sidx;
	char filename[MAX_FILE_PATH+1];
	char metafilename[MAX_FILE_PATH+1];
	char buff[MAX_FILE_PATH+1+4];
	static FILE *fp=NULL;
	static time_t last_check_dt=0;
	time_t now_dt;
	char *rbuff;
	T_IDX_HEADER ihd;
	T_IDX_INFO   idata;
	T_FILE_ID file_id;

	//データ同期のチェック間隔
	time(&now_dt);
	if ( last_check_dt == 0 ){ last_check_dt=now_dt; }
	if ( now_dt <= (last_check_dt + G_ConfTbl->dsync_trans_interval) ){ return 0; }

	//データ同期-初期化
	if ( fp == NULL ){
		sidx=disknmChoiceServer(2);
		sprintf(filename,"%s/IDX/.index", G_DiskTbl[sidx].dir);
		if ( (fp=fopen(filename,"r")) == NULL ){ last_check_dt=now_dt; return -1; }
	}
	rbuff=fgets(buff,MAX_FILE_PATH,fp);

	//データ同期-終了処理
	if ( rbuff == NULL ){
		fclose(fp);
		last_check_dt=now_dt;
		fp=NULL;
		return 0;
	}

	//ファイル名
	chomp(buff);
	disknmFileName2FileID(buff, &file_id);

	//データ分散情報取得
	T_DSYNC_TARGET tgt[G_ConfTbl->max_blocks];
	memset(tgt, 0, sizeof(T_DSYNC_TARGET) * G_ConfTbl->max_blocks );
	if ( (fd=open(buff,O_RDONLY)) < 0 ){ return -3; }
	ret=read(fd, (char*)&ihd,sizeof(T_IDX_HEADER));
	while( read(fd, (char*)&idata, sizeof(T_IDX_INFO)) == sizeof(T_IDX_INFO) ){
		(tgt[ idata.block_no ].cnt)++;
		tgt[ idata.block_no ].size = idata.size;
		if ( idata.w_tm.tv_sec > tgt[ idata.block_no ].w_tm ){
			tgt[ idata.block_no ].w_tm = idata.w_tm.tv_sec;
		}

		//自ノードの場合はwflgを立てる＝リカバリのみ実施
		if ( memcmp(idata.node_id, G_NodeTbl[0].node_id, sizeof(idata.node_id)) == 0 ){
			tgt[ idata.block_no ].wflg=1;
			continue;
		}

		//別ノードから取得
		for (i=1; i<G_ConfTbl->max_node; i++){
			if ( G_NodeTbl[i].sts != SYSTEM_STATUS_RUNNING ){ continue; }
			if ( memcmp(idata.node_id, G_NodeTbl[i].node_id, sizeof(idata.node_id)) == 0 ){
				tgt[ idata.block_no ].node_no=i;
			}
		}
	}
	close(fd);

	//データBlock同期
	int ck=0;
	int block_dup_cnt=daemon_DuplicateNum();
	for(i=1; i<G_ConfTbl->max_blocks; i++){
		if ( tgt[i].cnt == 0 ){ break; }
		if ( tgt[i].node_no == 0 ){ continue; }
		if ( now_dt <= (tgt[i].w_tm + G_ConfTbl->dsync_trans_wt_elap) ){ continue; }

		//キャッシュチェック
		ck=diskrdCheckDataFile(file_id, i);
		if ( ck >= 0 ){ continue; }
		if ( tgt[i].wflg == 1 ){
			logPrint(DEBUG_WARNING,"data file recovery(%s:%d)",dispFileID(&file_id), i, tgt[i].node_no);
		}else{
			if ( tgt[i].cnt >= block_dup_cnt ){ continue; }
		}

		//対象ブロックのファイル存在チェック
		int file_exist_flg=0;
		for(j=0; j<G_NodeTbl[0].disks; j++){
			if ( G_DiskTbl[j].sts != VOLUME_STATUS_SERVICE){ continue; }
			ret=disknmSearchDataFileName(G_DiskTbl[j].dir, file_id, i, filename);
			if ( ret == 1 ){ file_exist_flg++; break; }
		}

		//対象ブロックのキャッシュ存在チェック
		int cache_exist_flg=0;
		for ( j=0; j<G_ConfTbl->cache_blocks; j++){
			if ( G_IndexTbl[j].sts == CACHE_DATA_STS_NONE ){ continue; }
			if ( G_IndexTbl[j].sts == CACHE_DATA_STS_CACHE ){ continue; }
			int idx=G_IndexTbl[j].file_idx;
//printf("%s:%d %s:%d\n",dispFileID(&file_id),i,  dispFileID(&(G_MetaTbl[idx].inf.file_id)), G_IndexTbl[j].block_no);
			if ( memcmp(&(G_MetaTbl[idx].inf.file_id), &file_id, sizeof(T_FILE_ID)) == 0 && G_IndexTbl[j].block_no == i ){
				cache_exist_flg++;
				break;
			}
		}

//logPrint(DEBUG_INFO,"file_id=%s:%d size=%d node_no=%d tm=%s  dup=%d/%d exist=%d,%d",
//dispFileID(&file_id),i, tgt[i].size, tgt[i].node_no, tmVal2YYYYMMDDhhmmss(tgt[i].w_tm),
//tgt[i].cnt,block_dup_cnt, file_exist_flg, cache_exist_flg);
		//キャッシュ処理中はパス
		if ( cache_exist_flg > 0 ){
			logPrint(DEBUG_DEBUG,"Replication Waiting (file_id=%s:%d size=%d node_no=%d)",
				dispFileID(&file_id),i, tgt[i].size, tgt[i].node_no);
			continue; 
		}

		//DATAブロックのレプリケーション
		ret=diskrepReplecateBlock(G_ProcNo,file_id,i,tgt[i].size,tgt[i].node_no,tgt[i].wflg);
		if ( ret < 0 ){
			logPrint(DEBUG_ERROR,"diskrepReplecateBlock(%d,%s,%d,%d,%d,%d)=%d",
					G_ProcNo,dispFileID(&file_id), i, tgt[i].size, tgt[i].node_no, tgt[i].wflg, ret);
			continue;
		}
		if ( ret == 0 ){
			logPrint(DEBUG_WARNING,"diskrepReplecateBlock(%d,%s,%d,%d,%d,%d)=%d",
					G_ProcNo,dispFileID(&file_id), i, tgt[i].size, tgt[i].node_no, tgt[i].wflg, ret);
			continue;
		}
		logPrint(DEBUG_DEBUG,"diskrepReplecateBlock(%d,%s,%d,%d,%d,%d)=%d",
				G_ProcNo,dispFileID(&file_id), i, tgt[i].size, tgt[i].node_no, tgt[i].wflg, ret);
	}

	return 1;
}

int process_dsyn_fin(){
	return 0;
}
