/*****************************************************************************
*  <システム>   分散処理フレームワーク
*  <名称>	ファイル読込みAPI
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

int diskrdGetAllMetas(T_FILE_ID file_id, T_META_HEADER *hd, T_META_INFO *buff_arr){
	int ifd;
	int i_cnt;
	int m_cnt=0;
	int ret;
	int i,ck;
	T_META_INFO fid;
	char filename[MAX_FILE_PATH+1];

    //ファイル読込み
    disknmGetMetaFileName("", file_id, filename);
    if ( (ifd=open(filename,O_RDONLY)) < 0 ){
        return -2;
    }
    ret=read(ifd, (char*)hd,sizeof(T_META_HEADER));
    if ( ret != sizeof(T_META_HEADER) ){
        close(ifd);
        return -3;
    }


    //読込み
    while ( 1 ){
        ret=read(ifd, (char*)&fid, sizeof(T_META_INFO) );
        if ( ret < sizeof(T_META_INFO) ){ break; }

        //重複チェック
        i_cnt=-1;
        for(i=0; i<m_cnt; i++){
            //if ( buff_arr[i].file_id.id.tv_sec == 0 ){ continue; }
            ck=tmCompareMicroSec(fid.file_id.id, buff_arr[i].file_id.id);
            if ( ck != 0 ){ continue; }
            ck=tmCompareMicroSec(fid.file_id.ver, buff_arr[i].file_id.ver);
            if ( ck >= 0 ){ i_cnt=i; }
        }
        if ( i_cnt < 0 ){
            i_cnt=m_cnt;
            m_cnt++;
        }

		//printf("m_cnt=%d i_cnt=%d id=%d:%d mode=%d del=%d %s\n",
		//m_cnt, i_cnt, fid.st_uid, fid.st_gid, fid.st_mode, fid.st_dtm.tv_sec, fid.name);

        memcpy((char*)&(buff_arr[i_cnt]), (char*)&fid, sizeof(T_META_INFO) );

        if ( m_cnt >= G_ConfTbl->max_file_per_dir ){
            close(ifd);
            return -5;
        }
    }
    close(ifd);

	return m_cnt;
}

int diskrdCheckDataFile(T_FILE_ID file_id, u_long block_no){
	int ret;
	int i;
	u_long rsize;
	char filename[MAX_FILE_PATH+1];

	for(i=0; i<G_NodeTbl[0].disks; i++){
		if ( G_DiskTbl[i].dir[0] == NULL ){ break; }
		if ( G_DiskTbl[i].sts != VOLUME_STATUS_SERVICE){ continue; }

		ret=disknmSearchDataFileName(G_DiskTbl[i].dir, file_id, block_no, filename);
		if ( ret != 1 ){ continue; }

		rsize=filGetFileSize(filename);
		if ( rsize > 0 ){ return 0; }
	}

	return -1;
}

int diskrdGetVolumeSCN(int idx, char *base_dir, T_SCN *scn){
	int ret;
    int fd;
    char filename[MAX_FILE_PATH+1];
    T_VOLUME_HEADER vh;

	memset(scn, 0, sizeof(T_SCN) );
    sprintf(filename,"%s/.volume",base_dir);

    if ( (fd=open(filename,O_RDONLY)) < 0 ){
        //logPrint(DEBUG_ERROR,"open(%s)=%d",filename,fd);
        return 0;
    }
    ret=read(fd, (char*)&vh, sizeof(T_VOLUME_HEADER));
    if ( ret != sizeof(T_VOLUME_HEADER) ){
        logPrint(DEBUG_ERROR,"read(%s)=%d",filename,ret);
        return -1;
    }
    close(fd);

    memcpy(scn, &(vh.vol_scn), sizeof(T_SCN));

    return 1;
}

int diskrdGetDataBlockDup(T_FILE_ID file_id, u_long block_no, T_IDX_HEADER *ihd){
	int i;
	int ret;
	int fd;
    char filename[MAX_FILE_PATH+1];
    T_IDX_INFO   idata;

    //INDEXヘッダ読込み
    disknmGetIdxFileName("",file_id,filename);
    //logPrint(DEBUG_INFO,"%s %s", filename, dispFileID(&file_id));
    if ( (fd=open(filename,O_RDONLY)) < 0 ){ return -1; }
    ret=read(fd, (char*)ihd,sizeof(T_IDX_HEADER));
    if ( ret != sizeof(T_IDX_HEADER) ){
        close(fd);
        return -2;
    }

    if ( block_no != ihd->block_num ){ close(fd); return 0; }

    //DUP数算出
    u_short tgt[G_ConfTbl->max_blocks];
    memset(tgt, 0, sizeof(tgt) );
    while( read(fd, (char*)&idata, sizeof(T_IDX_INFO)) == sizeof(T_IDX_INFO) ){
        (tgt[ idata.block_no ])++;
    }
    close(fd);

    u_short dup_cnt=0xFF;
    for(i=1; i<G_ConfTbl->max_blocks; i++){
        if ( tgt[i] == 0 ){ break; }
        if ( tgt[i] < dup_cnt ){ dup_cnt=tgt[i]; }
    }
    if ( dup_cnt >= 0xFF ){ return 0; }

	return dup_cnt;
}

int diskrdGetDataBlockSize(T_FILE_ID file_id, u_long block_no){
	int ret;
	int ifd;
	int block_size=0;
	char filename[MAX_FILE_PATH+1];
	T_IDX_HEADER hd;
	T_IDX_INFO idata;

	//読込み
	disknmGetIdxFileName("", file_id, filename);
	if ( (ifd=open(filename,O_RDONLY)) < 0 ){ return -1;	}
	ret=read(ifd, (char*)&hd,sizeof(T_IDX_HEADER));
	if ( ret != sizeof(T_IDX_HEADER) ){
		close(ifd);
		return -2;
	}

	while(1){
		ret=read(ifd, &idata, sizeof(T_IDX_INFO) );
		if ( ret < sizeof(T_IDX_INFO) ){ break; }
		if ( idata.block_no == block_no ){
			block_size=idata.size;
			break;
		}
	}

	close(ifd);
	return block_size;
}

int diskrdGetDirInfo(T_FILE_ID dir_id, T_META_HEADER *hd){
	int ret;
	int ifd;
	char filename[MAX_FILE_PATH+1];

	//読込み
	disknmGetMetaFileName("", dir_id, filename);
	if ( (ifd=open(filename,O_RDONLY)) < 0 ){ return -1;	}
	ret=read(ifd, (char*)hd,sizeof(T_META_HEADER));
	if ( ret != sizeof(T_META_HEADER) ){
		close(ifd);
		return -2;
	}

	close(ifd);
	return 0;
}

int diskrdGetDirInFiles(T_FILE_ID dir_id, int *file_cnt){
	int i;
	int m_cnt;
	T_META_HEADER hd;
	T_META_INFO buff_arr[G_ConfTbl->max_file_per_dir + 1];	

	(*file_cnt)=0;

	//META情報取得
	m_cnt=diskrdGetAllMetas(dir_id, &hd, buff_arr);
	if ( m_cnt < 0 ){ return -1; }

	//件数カウント
	for(i=0; i<m_cnt; i++){
		if ( buff_arr[i].st_dtm.tv_sec != 0 ){ continue; }
		(*file_cnt)++;
	}

	return 0;
}

int diskrdGetFileInfo(T_FILE_ID dir_id, T_FILE_ID fil_id, T_META_HEADER *hd, T_META_INFO *fi){
	int i;
	int m_cnt;
	T_META_INFO buff_arr[G_ConfTbl->max_file_per_dir + 1];

	memset((char*)hd,0,sizeof(T_META_HEADER));
	memset((char*)fi, 0, sizeof(T_META_INFO) );

	//META情報取得
	m_cnt=diskrdGetAllMetas(dir_id, hd, buff_arr);
	if ( m_cnt <= 0 ){ return m_cnt; }

	//検索
	for (i=0; i<m_cnt; i++){
		if ( buff_arr[i].file_id.id.tv_sec != fil_id.id.tv_sec ){ continue; }
		if ( buff_arr[i].file_id.id.tv_usec != fil_id.id.tv_usec ){ continue; }
		memcpy((char*)fi, (char*)&buff_arr[i], sizeof(T_META_INFO) );
		return 1;
	}

	return 0;
}

int diskrdGetMetaInfo(T_FILE_ID dir_id, char *srch, T_META_HEADER *hd, T_META_INFO *fi){
	int i;
	int m_cnt;
	T_META_INFO buff_arr[G_ConfTbl->max_file_per_dir + 1];

	memset((char*)hd,0,sizeof(T_META_HEADER));
	memset((char*)fi,0,sizeof(T_META_INFO));

	//META情報取得
	m_cnt=diskrdGetAllMetas(dir_id, hd, buff_arr);
	if ( m_cnt <= 0 ){ return m_cnt; }

	//検索
	for (i=0; i<m_cnt; i++){
		if ( buff_arr[i].st_dtm.tv_sec != 0 ){ continue; }
		if ( strcmp(buff_arr[i].name, srch) != 0 ){ continue; }
		memcpy((char*)fi, (char*)&buff_arr[i], sizeof(T_META_INFO) );
		return 1;
	}

	return 0;
}

int diskrdSerchMetaFilePtn(T_FILE_ID dir_id, char *srch){
	int i;
	int m_cnt=0;
	T_META_HEADER hd;
	T_META_INFO buff_arr[G_ConfTbl->max_file_per_dir + 1];

	//META情報取得
	m_cnt=diskrdGetAllMetas(dir_id, &hd, buff_arr);
	if ( m_cnt <= 0 ){ return m_cnt; }

	//検索
	for (i=0; i<m_cnt; i++){
		if ( buff_arr[i].st_dtm.tv_sec != 0 ){ continue; }
		if ( strncmp(buff_arr[i].name, srch, strlen(srch)) != 0 ){ continue; }
		strcpy(srch,buff_arr[i].name);
		return 1;
	}

	return 0;
}

int diskrdSearchMeta(char *file_path, T_META_HEADER *hd, T_META_INFO *fi){
	int ret;
	int i;
	int kaiso_cnt=0;
	char resbuf[MAX_FILE_NAME+1];
	T_FILE_ID dir_id;

	memset((char*)&dir_id,0,sizeof(T_FILE_ID));
	//logPrint(DEBUG_INFO,"S:[%s]",file_path);

	//ルート読込み
	ret=diskrdGetMetaInfo(dir_id, ".", hd, fi);
	//logPrint(DEBUG_INFO,"diskrdSearchMeta(%s,%s,*,*)=%d", dispFileID(&dir_id), ".", ret);
	if ( ret != 1 ){ return -1; }
	if ( strcmp(file_path,"/") == 0 ){
		return 1; 
	}

	//階層数
	for(i=0; file_path[i]!=(char)NULL; i++){
		if ( file_path[i]=='/' ){ kaiso_cnt++; }
	} 

	//２階層目以降
	for(i=2; i<=(kaiso_cnt+1); i++){
		utlStringSep(file_path,"/",i,resbuf);
		if ( resbuf[0] == (char)NULL ){ break; }
		ret=diskrdGetMetaInfo(dir_id, resbuf, hd, fi);
		//logPrint(DEBUG_INFO,"diskrdGetMetaInfo(%s,%s,*,*)=%d", dispFileID(&dir_id), resbuf, ret);
		if ( ret == 0 ){ return 0; }
		if ( ret < 0 ){ return -2; }
		if ( fi->st_type != FILE_TYPE_DIR ){
			return 1;
		}
		memcpy((char*)&(dir_id), (char*)&(fi->file_id), sizeof(T_FILE_ID));
	}

	return 1;
}

int diskrdSearchDir(char *file_path, T_META_HEADER *hd, T_META_INFO *fi, char *out_path){
	int ret;
	int i;
	int kaiso_cnt=0;
	char resbuf[MAX_FILE_NAME+1];
	T_META_HEADER hd2;
	T_META_INFO fi2;
	T_FILE_ID dir_id;
	char b1[256];

	memset((char*)&dir_id,0,sizeof(T_FILE_ID));
	//logPrint(DEBUG_INFO,"S:[%s]",file_path);

	//ルート読込み
	ret=diskrdGetMetaInfo(dir_id, ".", hd, fi);
	//logPrint(DEBUG_INFO,"diskrdGetMetaInfo(%s,%s,*,*)=%d", dispFileID(&dir_id), ".", ret);
	if ( ret != 1 ){ return -1; }
	if ( strcmp(file_path,"/") == 0 ){ return 1; }

	//階層数
	for(i=0; file_path[i]!=(char)NULL; i++){
		if ( file_path[i]=='/' ){ kaiso_cnt++; }
	} 

	//２階層目以降
	for(i=2; i<=kaiso_cnt; i++){
		utlStringSep(file_path,"/",i,resbuf);
		if ( resbuf[0] == (char)NULL ){ break; }
		ret=diskrdGetMetaInfo(dir_id, resbuf, hd, fi);
		//logPrint(DEBUG_INFO,"diskrdGetMetaInfo(%s,%s,*,*)=%d", dispFileID(&dir_id), resbuf, ret);
		if ( ret <= 0 ){ return -2; }
		memcpy((char*)&(dir_id), (char*)&(fi->file_id), sizeof(T_FILE_ID));
	}

	//ディレクトリチェック
	utlStringSep(file_path, "/", kaiso_cnt+1,out_path);
	ret=diskrdGetMetaInfo(dir_id, out_path, &hd2, &fi2);
	//logPrint(DEBUG_INFO,"diskrdGetMetaInfo(%s,%s,*,*)=%d", dispFileID(&dir_id), out_path, ret);
	if ( ret == 1 ){ return 1; }
	if ( ret < 0 ){ return -3; }

	return 0;
}


