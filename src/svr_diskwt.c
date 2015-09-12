/*****************************************************************************
*  <システム>   分散処理フレームワーク
*  <名称>	DISK書込みAPI
*  <目的> 
*  <機能>	   
*  <開発環境>   UNIX
*  <特記事項>
*
*  VERSION	DATE		BY				CHANGE/COMMENT
*  -----------------------------------------------------------------------------
*  V0.00	  2014/02/10  Takakusaki		新規作成
******************************************************************************/
#include "xi_server.h"

static int diskwtMetaHeaderInfo(T_FILE_ID dir_id, T_META_HEADER *hd){
	int i;
	int fd;
	int ret;
	char filename[MAX_FILE_PATH+1];

	for(i=0; i<G_NodeTbl[0].disks; i++){

		if ( G_DiskTbl[i].sts != VOLUME_STATUS_SERVICE ) { continue; }

		disknmGetMetaFileName(G_DiskTbl[i].dir, dir_id, filename);
		if ( (fd=open(filename, O_RDONLY)) < 0 ){
			logPrint(DEBUG_ERROR,"open(%s)=%d",filename,fd);
			return -1; 
		}
		ret=read(fd, (char*)hd,sizeof(T_META_HEADER));
		close(fd);
		if ( ret != sizeof(T_META_HEADER) ){ return -2; }
		break;
	}

	return 0;
}

static int diskwtMetaCreate(T_FILE_ID up_dir_id, T_FILE_ID out_dir_id, int flg){
	int i;
	int ret;
	int fd;
	T_META_HEADER hd;
	char filename[MAX_FILE_PATH+1];

	//METAヘッダ情報
	memset(&hd,0,sizeof(T_META_HEADER));
	memcpy(hd.file_type, FILE_TYPE_META, sizeof(hd.file_type));
	memcpy((char*)&(hd.row.up_dir_id), (char*)&(up_dir_id), sizeof(T_FILE_ID));
	memcpy((char*)&(hd.row.dir_id), (char*)&(out_dir_id), sizeof(T_FILE_ID));

	//SCN取得
	daemon_GetSCN(&(G_NodeTbl[0].node_scn), flg);

	//ディスク書込み
	for(i=0; i<G_NodeTbl[0].disks; i++){

		if ( G_DiskTbl[i].sts != VOLUME_STATUS_SERVICE ) { continue; }

		//ディレクトリチェック
		disknmGetMetaDirName(G_DiskTbl[i].dir, hd.row.dir_id, filename);
		if ( filCreateDirectory(filename) < 0 ){ return -1; }

		//ファイルチェック
		disknmGetMetaFileName(G_DiskTbl[i].dir, hd.row.dir_id, filename);
		if ( filisFile(filename) == 1 ){ continue; }

		//ジャーナルログ
		ret=diskwtWriteJournalLog(JOURNAL_TYPE_META, &(G_NodeTbl[0].node_scn), G_DiskTbl[i].dir, &(hd.row.dir_id), 0);
		if ( ret < 0 ){ return -2; }

		//METAヘッダ情報出力
		if ( (fd=open(filename,O_CREAT|O_RDWR|O_TRUNC,0622)) < 0 ){
			logPrint(DEBUG_ERROR,"open(%s)=%d",filename,fd);
			return -3; 
		}
		ret=write(fd,(char *)&hd,sizeof(T_META_HEADER));	
		close(fd);
		if ( ret != sizeof(T_META_HEADER) ){ return -4; }
		logPrint(DEBUG_INFO,"C:W:[%s]",filename);

        //METAリスト
        ret=diskwtWriteMetaList(G_DiskTbl[i].dir, filename);
        if ( ret < 0 ){ return -5; }

		//Volumeヘッダ更新
		ret=diskwtWriteVolumeSCN(i, G_DiskTbl[i].dir, &(G_NodeTbl[0].node_scn));
		if ( ret < 0 ){ return -6; }

	}
	return 1;
}

int diskwtWriteMetaList(char *dir, char *filename){
	FILE *fp;
	char ifilename[MAX_FILE_PATH+1];

	sprintf(ifilename,"%s/META/.index",dir);
	if ( (fp=fopen(ifilename,"a+")) == NULL ){
		logPrint(DEBUG_ERROR,"fopen(%s)",ifilename);
		return -1;
	}
	fprintf(fp,"%s\n",filename);
	fclose(fp);
	return 0;
}

int diskwtWriteMetaRecord(T_FILE_ID dir_id, T_META_INFO fi, int flg){
	int i;
	int fd;
	int ret;
	int rec_no;
	u_long file_size;
	T_META_HEADER hd;
	char filename[MAX_FILE_PATH+1];

	if ( fi.dup_cnt <= 0 ){ fi.dup_cnt=1; }
	gettimeofday(&(fi.st_mtm),NULL);

	//SCN取得
	daemon_GetSCN(&(G_NodeTbl[0].node_scn), flg);

	//ディスク書込み
	for(i=0; i<G_NodeTbl[0].disks; i++){

		if ( G_DiskTbl[i].sts != VOLUME_STATUS_SERVICE ) { continue; }

		//ファイル名
		disknmGetMetaFileName(G_DiskTbl[i].dir, dir_id, filename);
		if ( filisFile(filename) != 1 ){ return -1; }

		//ジャーナルログ
		if ( (file_size=filGetFileSize(filename)) == 0 ){ return -2; }
		rec_no=(u_long)((file_size - sizeof(T_META_HEADER)) / sizeof(T_META_INFO)) + 1;
		ret=diskwtWriteJournalLog(JOURNAL_TYPE_META, &(G_NodeTbl[0].node_scn), G_DiskTbl[i].dir, &dir_id, rec_no);
		if ( ret < 0 ){ return -3; }

		//METAレコード追加
		if ( (fd=open(filename,O_RDWR|O_APPEND)) < 0 ){
			logPrint(DEBUG_ERROR,"open(%s)=%d",filename,fd);
			return -4; 
		}
		ret=write(fd,(char *)&fi,sizeof(T_META_INFO));	
		close(fd);
		if ( ret != sizeof(T_META_INFO) ){ return -5; }
		logPrint(DEBUG_INFO,"C:W:[%s] %s",filename,dispFileID(&(fi.file_id)) );

		//Volumeヘッダ更新
		ret=diskwtWriteVolumeSCN(i, G_DiskTbl[i].dir, &(G_NodeTbl[0].node_scn));
		if ( ret < 0 ){ return -6; }
	}
	return 1;
}

int diskwtWriteMetaCreateTop(T_FILE_ID up_dir_id, T_FILE_ID dir_id){
	int i;
	int ret;
	T_META_INFO fi;
	T_META_HEADER hd;
	char filename[MAX_FILE_PATH+1];

	//ルートMETAがある場合は初期化不要
	for(i=0; i<G_NodeTbl[0].disks; i++){
		if ( G_DiskTbl[i].sts != VOLUME_STATUS_SERVICE ) { continue; }
		disknmGetMetaFileName(G_DiskTbl[i].dir, dir_id, filename);
		if ( filisFile(filename) == 1 ){ return 0; }
	}

	//META初期化
	ret=diskwtMetaCreate(up_dir_id, dir_id, 0);
	if ( ret < 0 ){ return -2; }

	//METAレコード追加
	memset((char*)&fi,0,sizeof(T_META_INFO));
	memcpy((char*)&(fi.file_id), (char*)&(dir_id), sizeof(T_FILE_ID));
	fi.st_type=FILE_TYPE_DIR;
	fi.st_uid=getuid();
	fi.st_gid=getgid();
	fi.st_mode=G_ConfTbl->def_perm_dir;
	fi.st_size=0;
	fi.st_blksize=BLOCK_SIZE;
	fi.st_blocks=0;
	fi.dup_cnt=1;
	gettimeofday(&(fi.st_atm),NULL);
	gettimeofday(&(fi.st_mtm),NULL);
	gettimeofday(&(fi.st_ctm),NULL);
	fi.st_dtm.tv_sec=0;
	fi.st_dtm.tv_usec=0;
	strcpy(fi.name, ".");
	ret=diskwtWriteMetaRecord(dir_id, fi, 0);
	if ( ret < 0 ){ return -4; }

	return 1;
}


int diskwtWriteMetaCreate(T_FILE_ID dir_id, T_META_INFO fi){
	int i;
	int ret;

	//META新規作成
	ret=diskwtMetaCreate(dir_id, fi.file_id, 1);
	if ( ret < 0 ){ return -1; }

	//METAコレード追加
	ret=diskwtWriteMetaRecord(dir_id, fi, 1);
	if ( ret < 0 ){ return -2; }

	return 1;
}


int diskwtWriteDataCache(char *in_addr, T_INDEX_TABLE in_idx, char *out_base_dir, T_FILE_ID out_file_id){
	int fd;
	gzFile zp;
	int ret;
	int wret;
	char dirname[MAX_FILE_PATH+1];
	char filename1[MAX_FILE_PATH+1];
	char filename2[MAX_FILE_PATH+1];
	char *addr;

	if ( in_idx.size <= 0 ){ return 0; }

	//DATAディレクトリ作成
	disknmGetDataDirName(out_base_dir, out_file_id, dirname);
	ret=filCreateDirectory(dirname);
	if ( ret < 0 ){
		logPrint(DEBUG_ERROR,"filCreateDirectory(%s) => %d",dirname,ret); 
		return -1;
	}
	if ( filisDirectory(dirname) != 1 ){
		logPrint(DEBUG_ERROR,"not found directory (%s)",dirname); 
		return -1; 
	}


	//DATAファイル出力
	sprintf(filename1,"%s/%d.%d.tmp",dirname, in_idx.block_no,getpid());
	int comp_type=daemon_CompressType(in_idx.size);
	if ( comp_type == 0 ){
		sprintf(filename2,"%s/%d",dirname, in_idx.block_no);
		if ( (fd=open(filename1,O_CREAT|O_RDWR|O_TRUNC,0622)) < 0 ){
			logPrint(DEBUG_ERROR,"open(%s)=%d",filename1,fd);
			return -2; 
		}
		wret=write(fd, in_addr, in_idx.size);
		close(fd);
		if ( wret != in_idx.size ){
			logPrint(DEBUG_ERROR,"write(%s)=%d",filename1,wret);
			return -3; 
		}
	}else{
		sprintf(filename2,"%s/%d.gz",dirname, in_idx.block_no);
		if ( (zp = gzopen(filename1, "wb9")) == NULL ){
			logPrint(DEBUG_ERROR,"open(%s)=%d",filename1,fd);
			return -2; 
		}
		wret=gzwrite(zp, in_addr, in_idx.size);
		gzclose(zp);
		if ( wret != in_idx.size ){
			logPrint(DEBUG_ERROR,"gzwrite(%s)=%d",filename1,wret);
			return -3; 
		}
	}	

	//終了処理
	ret=rename(filename1, filename2);
	if ( ret != 0 ){
		logPrint(DEBUG_ERROR,"rename(%s,%s)=%d",filename1,filename2,ret);
	}
	logPrint(DEBUG_INFO,"C:W:[%s] %dbyte",filename2, wret);

	return 1;
}

int diskwtWriteIndexHeader(char *filename, T_FILE_ID dir_id, u_long block_num){
    int ret;
    int fd;
    T_IDX_HEADER ihd;

    memcpy(ihd.file_type, FILE_TYPE_INDX, sizeof(ihd.file_type));
    memcpy(&(ihd.dir_id), &dir_id, sizeof(T_FILE_ID));
	ihd.block_num = block_num;

    if ( (fd=open(filename,O_CREAT|O_RDWR|O_TRUNC,0622)) < 0 ){
        logPrint(DEBUG_ERROR,"open(%s)=%d",filename,fd);
        return -1;
    }
    ret=write(fd, &ihd, sizeof(T_IDX_HEADER));
    if ( ret != sizeof(T_IDX_HEADER) ){
        logPrint(DEBUG_ERROR,"write(%s)=%d",filename,ret);
        return -2;
    }
    close(fd);
    return 0;
}

int diskwtWriteIndexAppend(char *filename, T_FILE_ID file_id, T_IDX_INFO idx){
	int ret;
	int fd;

	if ( (fd=open(filename,O_RDWR|O_APPEND)) < 0 ){
		logPrint(DEBUG_ERROR,"open(%s)=%d",filename,fd);
		return -1;
	}
	ret=write(fd,(char *)&idx,sizeof(T_IDX_INFO));
	if ( ret != sizeof(T_IDX_INFO) ){ close(fd); return -2; }
	logPrint(DEBUG_INFO,"C:W:[%s] %s",filename,dispFileID(&file_id) );
	close(fd);
	return 0;
}

int diskwtWriteIndexList(char *dir, char *filename){
    FILE *fp;
    char ifilename[MAX_FILE_PATH+1];

    sprintf(ifilename,"%s/IDX/.index",dir);
    if ( (fp=fopen(ifilename,"a+")) == NULL ){
        logPrint(DEBUG_ERROR,"fopen(%s)",ifilename);
        return -1;
    }
    fprintf(fp,"%s\n",filename);
    fclose(fp);
    return 0;
}

int diskwtWriteIndexInit(T_FILE_ID dir_id, T_FILE_ID file_id, u_long block_num){
	int i;
	int ret;
	int fd;
	T_IDX_HEADER ihd;
	char filename[MAX_FILE_PATH+1];

	daemon_GetSCN(&(G_NodeTbl[0].node_scn), 1);

	for(i=0; i<G_NodeTbl[0].disks; i++){

		if ( G_DiskTbl[i].sts != VOLUME_STATUS_SERVICE ) { continue; }

		//INDEXディレクトリ作成
		disknmGetIdxDirName(G_DiskTbl[i].dir, file_id, filename);
		ret=filCreateDirectory(filename);
		if ( ret < 0 ){ logPrint(DEBUG_ERROR,"filCreateDirectory(%s) => %d",filename,ret); }
		if ( filisDirectory(filename) != 1 ){ return -1; }

		//INDEXファイル名
		disknmGetIdxFileName(G_DiskTbl[i].dir, file_id, filename);

		//ジャーナルログ
		ret=diskwtWriteJournalLog(JOURNAL_TYPE_INDEX_CRE, &(G_NodeTbl[0].node_scn), G_DiskTbl[i].dir, &file_id, 0);
		if ( ret < 0 ){ return -2; }

		//INDEXヘッダ出力
		ret=diskwtWriteIndexHeader(filename, dir_id, block_num);
		if ( ret < 0 ){ return -3; }

		//INDEXリスト出力
		ret=diskwtWriteIndexList(G_DiskTbl[i].dir, filename);
		if ( ret < 0 ){ return -4; }

		//Volumeヘッダ更新
		ret=diskwtWriteVolumeSCN(i, G_DiskTbl[i].dir, &(G_NodeTbl[0].node_scn));
		if ( ret < 0 ){ return -5; }
	}

	return 0;
}

int diskwtWriteIndexCache(T_FILE_ID dir_id, T_FILE_ID file_id, u_long block_no, u_long size, u_char *node_id){
	int i;
	int fd;
	int ret;
	u_long file_size;
	u_long rec_no;
	char filename[MAX_FILE_PATH+1];
	char ifilename[MAX_FILE_PATH+1];
	T_IDX_INFO c_idx;
	FILE *fp;

	daemon_GetSCN(&(G_NodeTbl[0].node_scn), 1);

	//書込み情報
	memcpy(c_idx.node_id, node_id, NODE_ID_LEN );
	c_idx.block_no = block_no;
	c_idx.size = size;
	gettimeofday(&(c_idx.w_tm),NULL);
	//logPrint(DEBUG_INFO,"node_id=%s block_no=%d size=%d", utlSHA1_to_HEX(c_idx.node_id), c_idx.block_no, c_idx.size);

	for(i=0; i<G_NodeTbl[0].disks; i++){

		if ( G_DiskTbl[i].sts != VOLUME_STATUS_SERVICE ) { continue; }

		//サイズチェク
		disknmGetIdxFileName(G_DiskTbl[i].dir, file_id, filename);
		file_size=filGetFileSize(filename);
		if ( file_size < sizeof(T_IDX_HEADER) ){ return -3;  }
		rec_no=(u_long)((file_size - sizeof(T_IDX_HEADER)) / sizeof(T_IDX_INFO)) + 1;

		//ジャーナルログ
		ret=diskwtWriteJournalLog(JOURNAL_TYPE_INDEX_ADD, &(G_NodeTbl[0].node_scn), G_DiskTbl[i].dir, &file_id, rec_no);
		if ( ret < 0 ){ return -4; }

		//INDEXレコード追加
		ret=diskwtWriteIndexAppend(filename, file_id, c_idx);
		if ( ret < 0 ){ return -5; }

		//Volumeヘッダ更新
		ret=diskwtWriteVolumeSCN(i, G_DiskTbl[i].dir, &(G_NodeTbl[0].node_scn));
		if ( ret < 0 ){ return -6; }
	}

    //DUP数算出
    ret=diskrepRepNumReCal(file_id, block_no);
    if ( ret < 0 ){
        logPrint(DEBUG_ERROR,"diskrepRepNumReCal()=%d",ret);
        return -10;
    }

	return 1;
}

int diskwtWriteScnCount(){
	int i;
	int ret;
	T_FILE_ID file_id;

	memset(&file_id, 0, sizeof(T_FILE_ID));
	for(i=0; i<G_NodeTbl[0].disks; i++){

		if ( G_DiskTbl[i].sts != VOLUME_STATUS_SERVICE ) { continue; }

		//ジャーナルログ
		ret=diskwtWriteJournalLog(JOURNAL_TYPE_SCN_UP, &(G_NodeTbl[0].node_scn), G_DiskTbl[i].dir, &file_id, 0);
		if ( ret < 0 ){ return -1; }

		//Volumeヘッダ更新
		ret=diskwtWriteVolumeSCN(i, G_DiskTbl[i].dir, &(G_NodeTbl[0].node_scn));
		if ( ret < 0 ){ return -2; }
	}

	return 0;
}


int diskwtWriteVolumeSCN(int idx, char *base_dir, T_SCN *scn){
	int ret;
	int fd;
	char filename[MAX_FILE_PATH+1];
	T_VOLUME_HEADER vh;

	//メモリ更新
	memcpy( &(G_DiskTbl[idx].vol_scn), scn, sizeof(T_SCN) );

	sprintf(filename,"%s/.volume",base_dir);
	memcpy(vh.file_type, FILE_TYPE_VOLUME, sizeof(vh.file_type));
	utlGetSHA1(filename, vh.volume_id);
	memcpy(&(vh.vol_scn), scn, sizeof(T_SCN));
	gettimeofday(&vh.w_tm, NULL);

	if ( (fd=open(filename,O_CREAT|O_RDWR|O_TRUNC,0622)) < 0 ){
		logPrint(DEBUG_ERROR,"open(%s)=%d",filename,fd);
		return -1; 
	}
	ret=write(fd, (char*)&vh, sizeof(T_VOLUME_HEADER));
	if ( ret != sizeof(T_VOLUME_HEADER) ){
		logPrint(DEBUG_ERROR,"write(%s)=%d",filename,ret);
		return -2; 
	}
	close(fd);
	return 0;
}

int diskwtWriteJournalLog(int flg, T_SCN *scn, char *out_dir, T_FILE_ID *dir_id, u_long rec_no){
	int fd;
	int ret;
	u_long fsize;
	time_t nowtime;
	struct tm *dt;
	char filename[MAX_FILE_PATH+1];
	T_JLOG_HEADER jhd;
	T_JLOG_INFO jlog;

	//ジャーナル
	memcpy(jhd.file_type, FILE_TYPE_JLOG, sizeof(jhd.file_type) );
	memcpy(&(jlog.scn), scn, sizeof(T_SCN));
	memcpy(&(jlog.dir_id), dir_id, sizeof(T_FILE_ID));
	jlog.flg = flg;
	jlog.rec_no=rec_no;

	//ファイル名
	//time(&nowtime);
	//dt = localtime(&nowtime);
	//sprintf(filename,"%s/LOG/%04d%02d%02d",out_dir,dt->tm_year+1900, dt->tm_mon+1, dt->tm_mday);
	sprintf(filename,"%s/LOG/%08d",out_dir,jlog.scn.dt);
	if ( filisDirectory(filename) <= 0 ){ mkdir(filename,0777); }
	sprintf(filename,"%s/LOG/%08d/%d-%d",
			out_dir, jlog.scn.dt, jlog.scn.seq1, (jlog.scn.seq2/G_ConfTbl->jlog_max_rec) );
	logPrint(DEBUG_DEBUG,"J:[%s] %s:%d",filename, dispFileID(dir_id), rec_no );

	//新規ならヘッダ出力
	ret=filisFile(filename);
	if ( ret < 0 ){
		if ( (fd=open(filename,O_CREAT|O_RDWR|O_TRUNC,0622)) < 0 ){
			logPrint(DEBUG_ERROR,"open(%s)=%d",filename,fd);
			return -1; 
		}
		ret=write(fd, &jhd, sizeof(T_JLOG_HEADER));
		if ( ret != sizeof(T_JLOG_HEADER) ){
			logPrint(DEBUG_ERROR,"write(%s)=%d",filename,ret);
			return -2; 
		}
		close(fd);
	}

	//ジャナルログの追加
	if ( (fd=open(filename,O_RDWR|O_APPEND)) < 0 ){
		logPrint(DEBUG_ERROR,"open(%s)=%d",filename,fd);
		return -3; 
	}
	ret=write(fd, &jlog, sizeof(T_JLOG_INFO));
	if ( ret != sizeof(T_JLOG_INFO) ){
		logPrint(DEBUG_ERROR,"write(%s)=%d",filename,ret);
		return -4; 
	}
	close(fd);

	return 0;
}

