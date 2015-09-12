/*****************************************************************************
*  <システム>   分散処理フレームワーク
*  <名称>	DISK同期API
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

int disksyncFileNameSort(const struct dirent **s1, const struct dirent **s2){
	char buff1[32];
	char buff2[32];
	long a1,a2;

	a1=atol( utlStringSep((char*)(*s1)->d_name,"-",1,buff1) );
	a2=atol( utlStringSep((char*)(*s2)->d_name,"-",1,buff2) );
	if ( a1 < a2 ){ return -1; }
	if ( a1 > a2 ){ return 1; }

	a1=atol( utlStringSep((char*)(*s1)->d_name,"-",2,buff1) );
	a2=atol( utlStringSep((char*)(*s2)->d_name,"-",2,buff2) );
	if ( a1 < a2 ){ return -1; }
	if ( a1 > a2 ){ return 1; }
	return 0;
}

int disksyncFileNameSort(const void *v1, const void *v2){
	struct dirent   *s1, *s2;
	char buff1[32];
	char buff2[32];
	long a1,a2;

	s1 = *(struct dirent **)v1 ;
	s2 = *(struct dirent **)v2 ;

	a1=atol( utlStringSep((char*)s1->d_name,"-",1,buff1) );
	a2=atol( utlStringSep((char*)s2->d_name,"-",1,buff2) );
	if ( a1 < a2 ){ return -1; }
	if ( a1 > a2 ){ return 1; }

	a1=atol( utlStringSep((char*)s1->d_name,"-",2,buff1) );
	a2=atol( utlStringSep((char*)s2->d_name,"-",2,buff2) );
	if ( a1 < a2 ){ return -1; }
	if ( a1 > a2 ){ return 1; }
	return 0;
}

int disksyncReadMETArow(char *dirname, u_long yyyymmdd, T_SCN scn, T_META_SYNC_RESULT *outbuff, u_long *r_size){
	int i;
	int d_ret;
	int ret;
	int oret=0;
	int ifd;
	int seek_byte=0;
	struct dirent **namelist;
	char fulldirname[MAX_FILE_PATH+1];
	char fullfilename[MAX_FILE_PATH+1];
	T_JLOG_HEADER jhd;
	T_JLOG_INFO in;
	T_META_HEADER mhd;
	T_IDX_HEADER ihd;
	char buff1[32];
	char buff2[32];

	//JounalLogを読み込んで対象レコードを取得
	sprintf(fulldirname,"%s/%08d",dirname,yyyymmdd);
	d_ret=scandir(fulldirname, &namelist, NULL, disksyncFileNameSort);
	if ( d_ret < 0 ){ return -1; }

	for (i = 0; i < d_ret; i++) {
		if ( namelist[i]->d_name[0] == '.'){ continue; }
		sprintf(fullfilename,"%s/%s",fulldirname,namelist[i]->d_name);
		utlStringSep(namelist[i]->d_name,"-",1,buff1);
		utlStringSep(namelist[i]->d_name,"-",2,buff2);
		free(namelist[i]);

//printf("    cnt=%d %s  scn=%ld:%ld:%ld  %ld:%ld max=%d r_size=%d oret=%d\n",
//		i,fullfilename,  scn.dt,scn.seq1,scn.seq2, yyyymmdd,atol(buff1), G_ConfTbl->meta_sync_max_rec, *r_size, oret);

		if ( yyyymmdd < scn.dt ){ continue; }
		if ( yyyymmdd == scn.dt && atol(buff1) < scn.seq1 ){ continue; }
		if ( *r_size >= G_ConfTbl->meta_sync_max_rec ){ continue; }
		if ( oret != 0 ){ continue; }

		//JounalLog読込み
		if ( (ifd=open(fullfilename,O_RDONLY)) < 0 ){ oret=-1; break; }
		ret=read(ifd, (char*)&(jhd), sizeof(T_JLOG_HEADER) );
		if ( ret < sizeof(T_JLOG_HEADER) ){ close(ifd); oret=-2; break; }

		while ( 1 ){
			ret=read(ifd, (char*)&(in), sizeof(T_JLOG_INFO) );
			if ( ret < sizeof(T_JLOG_INFO) ){ break; }
			if ( in.scn.dt < scn.dt ){ continue; }
			if ( in.scn.dt == scn.dt and in.scn.seq1 == scn.seq1 and in.scn.seq2 <= scn.seq2 ){ continue; }

			//logPrint(DEBUG_DEBUG,"[%d] scn=%s %s:%d",
			//  *r_size, dispSCN(in.scn,buff1), dispFileID(&in.dir_id), in.rec_no );

			memcpy(&(outbuff[*r_size].jlog), &in, sizeof(T_JLOG_INFO) );
			(*r_size)++;
			if ( *r_size >= G_ConfTbl->meta_sync_max_rec ){ break; }
		}
		close(ifd);

	}
	free(namelist);
	if ( oret < 0 ){ return oret; }
	if ( (*r_size) <= 0 ){ return oret; }

	//転送データ生成
	for(i=0; i<*r_size; i++){

		//printf("[%d] flg=%d scn=%s %s:%d\n",
		//	i, outbuff[i].jlog.flg, dispSCN( &(outbuff[i].jlog.scn)), 
		//	dispFileID(&(outbuff[i].jlog.dir_id)), outbuff[i].jlog.rec_no);

		//META読込み
		if ( outbuff[i].jlog.flg == JOURNAL_TYPE_META ){

			disknmGetMetaFileName("", outbuff[i].jlog.dir_id, fullfilename);
			if ( (ifd=open(fullfilename,O_RDONLY)) < 0 ){ oret=-2; break; }
			ret=read(ifd, (char*)&mhd, sizeof(T_META_HEADER) );
			if ( ret < sizeof(T_META_HEADER) ){ close(ifd); oret=-3; break; }
			memcpy((char*)&(outbuff[i].meta.row), &(mhd.row), sizeof(T_DIR_ID));

			if ( outbuff[i].jlog.rec_no > 0 ){
				seek_byte = sizeof(T_META_HEADER) + (outbuff[i].jlog.rec_no - 1) * sizeof(T_META_INFO);
				ret=lseek(ifd, seek_byte, SEEK_SET);
				if ( ret < 0 ){ close(ifd); oret=-4; break; }
				ret=read(ifd, (char*)&(outbuff[i].meta.inf), sizeof(T_META_INFO) );
				if ( ret < sizeof(T_META_INFO) ){ close(ifd); oret=-5; break; }
			}
			close(ifd);
			continue;
		}

		//INDEXディレクトリ作成
		if ( outbuff[i].jlog.flg == JOURNAL_TYPE_INDEX_CRE ){
			disknmGetIdxFileName("", outbuff[i].jlog.dir_id, fullfilename);
			if ( (ifd=open(fullfilename,O_RDONLY)) < 0 ){ oret=-11; break; }
			ret=read(ifd, (char*)&ihd, sizeof(T_IDX_HEADER) );
			if ( ret < sizeof(T_IDX_HEADER) ){ close(ifd); oret=-12; break; }
			close(ifd);
			memcpy((char*)&(outbuff[i].row.dir_id), &(ihd.dir_id), sizeof(T_FILE_ID));
			outbuff[i].row.block_num = ihd.block_num;

			//logPrint(DEBUG_INFO,"%s dir_id=%s block_num=%d",
			//		fullfilename, dispFileID(&(outbuff[i].row.dir_id)), outbuff[i].row.block_num);
			continue;
		}

		//該当blockが処理中の場合はパス
		ret=cacheCheckBlockWrite(&(outbuff[i].jlog.dir_id), outbuff[i].idx.inf.block_no);
		//logPrint(DEBUG_INFO,"cacheCheckBlockWrite(%s:%d)=%d", 
		//	dispFileID(&(outbuff[i].jlog.dir_id)), outbuff[i].idx.inf.block_no, ret);
		if ( ret == 1 ){
			*r_size=i;
			logPrint(DEBUG_DEBUG,"%s:%d writing", 
				dispFileID(&(outbuff[i].jlog.dir_id)), outbuff[i].idx.inf.block_no, ret);
			break;
		}


		//INDEXレコード書込み
		if ( outbuff[i].jlog.flg == JOURNAL_TYPE_INDEX_ADD ){
			disknmGetIdxFileName("", outbuff[i].jlog.dir_id, fullfilename);
			if ( (ifd=open(fullfilename,O_RDONLY)) < 0 ){ oret=-21; break; }
			seek_byte = sizeof(T_IDX_HEADER) + (outbuff[i].jlog.rec_no - 1) * sizeof(T_IDX_INFO);
			//logPrint(DEBUG_INFO,"%s rec_no=%d seek_byte=%d",fullfilename, outbuff[i].jlog.rec_no, seek_byte);
			ret=lseek(ifd, seek_byte, SEEK_SET);
			if ( ret < 0 ){ close(ifd); oret=-22; break; }
			ret=read(ifd, (char*)&(outbuff[i].idx.inf), sizeof(T_IDX_INFO) );
			if ( ret < sizeof(T_IDX_INFO) ){ close(ifd); oret=-23; break; }
			close(ifd);
			continue;
		}
	}

	/*
	printf("[REQ]\n");
	printf("scn     =%s\n",dispSCN(&scn));
	printf("dir     =%s\n",dirname);
	printf("yyyymmdd=%d\n",yyyymmdd);
	for(i=0; i<*r_size; i++){
		printf("[%d] flg=%d\n",i,outbuff[i].jlog.flg);
		printf("jlog	  =%s rec_no=%d\n",dispFileID(&(outbuff[i].jlog.dir_id)), outbuff[i].jlog.rec_no);
		if ( outbuff[i].jlog.flg ==  JOURNAL_TYPE_META ){
			printf("up_dir_id =%s\n",dispFileID(&(outbuff[i].meta.row.up_dir_id)));
			printf("dir_id	  =%s\n",dispFileID(&(outbuff[i].meta.row.dir_id)));
			printf("file_id   =%s\n",dispFileID(&(outbuff[i].meta.inf.file_id)));
		}
		if ( outbuff[i].jlog.flg ==  JOURNAL_TYPE_INDEX_CRE ){
			printf("dir_id	  =%s\n",dispFileID(&(outbuff[i].dir_id)));
		}
		if ( outbuff[i].jlog.flg ==  JOURNAL_TYPE_INDEX_ADD ){
			printf("node_id   =%s\n",utlSHA1_to_HEX(outbuff[i].idx.node_id));
			printf("block_no  =%d\n",outbuff[i].idx.block_no);
			printf("size      =%d\n",outbuff[i].idx.size);
		}
	}
	printf("\n");
	*/

	return oret;
}

int disksyncReadMeta(T_CLIENT_REQUEST req, T_META_SYNC_RESULT *outbuff, u_long *r_cnt, u_long *r_size){
	int i;
	int ret;
	int d_ret;
	struct dirent **namelist;
	char dirname[MAX_FILE_PATH+1];
	u_long wYYYYMMDD[G_ConfTbl->meta_sync_max_rec];

    memset(wYYYYMMDD,0,sizeof(wYYYYMMDD));

    //対象ディレクトリ(YYYYMMDD)を取得
    disknmGetLogDirName("", dirname);
    if ( (d_ret=scandir(dirname, &namelist, NULL, alphasort)) < 0 ){
        logPrint(DEBUG_ERROR,"scandir()=%d",ret);
        return -1;
    }

    for (i = 0; i < d_ret; i++) {
        if ( namelist[i]->d_name[0] == '.'){ continue; }
        if ( atol(namelist[i]->d_name) == 0 ){ continue; }
        if ( atol(namelist[i]->d_name) >= req.scn.scn.dt ){
            if ( *r_cnt < G_ConfTbl->meta_sync_max_rec ){
                wYYYYMMDD[*r_cnt]=atol(namelist[i]->d_name);
            }
            (*r_cnt)++;
        }
        free(namelist[i]);
    }
    free(namelist);

    //対象ファイル読込み
    for (i = 0; i < d_ret; i++) {
        if ( wYYYYMMDD[i] == 0 ){ break; }
        ret=disksyncReadMETArow(dirname, wYYYYMMDD[i], req.scn.scn, outbuff, r_size);
        if ( ret < 0 ){
            logPrint(DEBUG_ERROR,"disksyncReadMETA()=%d",ret);
            return -2;
        }
    }

	return 0;
}

int disksyncWriteIndexHeader(T_META_SYNC_RESULT *out_buff){
	int i;
	int ret;
	char filename[MAX_FILE_PATH+1];
	int w_cnt=0;

	//自ノードのSCNが古い場合
	if ( memcmp(&(G_NodeTbl[0].node_scn), &(out_buff->jlog.scn), sizeof(T_SCN)) < 0 ){
		memcpy(&(G_NodeTbl[0].node_scn), &(out_buff->jlog.scn), sizeof(T_SCN));
	}

	//ディスク書込み
	for(i=0; i<G_NodeTbl[0].disks; i++){

		if ( memcmp(&(G_DiskTbl[i].vol_scn), &(out_buff->jlog.scn), sizeof(T_SCN)) >= 0 ){ continue; }
		w_cnt++;

		//INDEXディレクトリ
		disknmGetIdxDirName(G_DiskTbl[i].dir, out_buff->jlog.dir_id, filename);
		ret=filCreateDirectory(filename);
		if ( ret < 0 ){
			logPrint(DEBUG_ERROR,"filCreateDirectory(%s)=%d",filename,ret); 
			return -1;
		}

		//INDEXファイル名
		disknmGetIdxFileName(G_DiskTbl[i].dir, out_buff->jlog.dir_id, filename);

		//ジャーナルログ
		ret=diskwtWriteJournalLog(JOURNAL_TYPE_INDEX_CRE, &(out_buff->jlog.scn), G_DiskTbl[i].dir, &(out_buff->jlog.dir_id), out_buff->jlog.rec_no);
		if ( ret < 0 ){
			logPrint(DEBUG_ERROR,"diskwtWriteJournalLog()=%d",ret); 
			return -2;
		}

        //INDEXヘッダ出力
        ret=diskwtWriteIndexHeader(filename, out_buff->row.dir_id, out_buff->row.block_num);
        if ( ret < 0 ){
			logPrint(DEBUG_ERROR,"diskwtWriteIndexHeader()=%d",ret); 
			return -3; 
		}

        //INDEXリスト出力
        ret=diskwtWriteIndexList(G_DiskTbl[i].dir, filename);
        if ( ret < 0 ){
			logPrint(DEBUG_ERROR,"diskwtWriteIndexList()=%d",ret); 
			return -4; 
		}

        //Volumeヘッダ更新
        ret=diskwtWriteVolumeSCN(i, G_DiskTbl[i].dir, &(out_buff->jlog.scn));
        if ( ret < 0 ){
			logPrint(DEBUG_ERROR,"diskwtWriteVolumeSCN()=%d",ret); 
			return -5; 
		}

	}

	return w_cnt;
}

int disksyncWriteIndexRecord(T_META_SYNC_RESULT *out_buff){
	int i;
	int fd;
	int ret;
	int file_size;
	int rec_no;
	char filename[MAX_FILE_PATH+1];
	char ifilename[MAX_FILE_PATH+1];
	char buff1[32];
	char buff2[32];
	FILE *fp;
	int w_cnt=0;

	//自ノードのSCNが古い場合
	if ( memcmp(&(G_NodeTbl[0].node_scn), &(out_buff->jlog.scn), sizeof(T_SCN)) < 0 ){
		memcpy(&(G_NodeTbl[0].node_scn), &(out_buff->jlog.scn), sizeof(T_SCN));
	}

	//ディスク書込み
	for(i=0; i<G_NodeTbl[0].disks; i++){

		memcpy(&(G_NodeTbl[0].node_scn), &(out_buff->jlog.scn), sizeof(T_SCN));
		if ( memcmp(&(G_DiskTbl[i].vol_scn), &(out_buff->jlog.scn), sizeof(T_SCN)) >= 0 ){ continue; }
		w_cnt++;

		//INDEXディレクトリ
		disknmGetIdxDirName(G_DiskTbl[i].dir, out_buff->jlog.dir_id, filename);
		ret=filCreateDirectory(filename);
		if ( ret < 0 ){
			logPrint(DEBUG_ERROR,"filCreateDirectory(%s)=%d",filename,ret); 
			return -1;
		}

		//INDEXファイル名
		disknmGetIdxFileName(G_DiskTbl[i].dir, out_buff->jlog.dir_id, filename);

		//ジャーナルログ
		ret=diskwtWriteJournalLog(JOURNAL_TYPE_INDEX_ADD, &(out_buff->jlog.scn), G_DiskTbl[i].dir, &(out_buff->jlog.dir_id), out_buff->jlog.rec_no);
		if ( ret < 0 ){
			logPrint(DEBUG_ERROR,"diskwtWriteJournalLog()=%d",ret); 
			return -2;
		}

		//INDEXレコード追加
		ret=diskwtWriteIndexAppend(filename, out_buff->jlog.dir_id, out_buff->idx.inf);
        if ( ret < 0 ){
			logPrint(DEBUG_ERROR,"diskwtWriteIndexAppend()=%d",ret); 
			return -3; 
		}

        //Volumeヘッダ更新
        ret=diskwtWriteVolumeSCN(i, G_DiskTbl[i].dir, &(out_buff->jlog.scn));
        if ( ret < 0 ){
			logPrint(DEBUG_ERROR,"diskwtWriteVolumeSCN()=%d",ret); 
			return -4; 
		}

	}

	return w_cnt;
}

int disksyncWriteMeta(T_META_SYNC_RESULT *out_buff){
	int i;
	int fd;
	int ret;
	int file_size;
	int rec_no;
	char filename[MAX_FILE_PATH+1];
	char buff1[32];
	char buff2[32];
	T_META_HEADER hd;
	int w_cnt=0;

	/*
	printf("[%s]\n",dispSCN(&(out_buff->jlog.scn)));
	printf("jlog	 =%s rec_no=%d\n",dispFileID(&(out_buff->jlog.dir_id)), out_buff->jlog.rec_no);
	printf("up_dir_id=%s\n", dispFileID(&(out_buff->meta.row.up_dir_id)));
	printf("dir_id   =%s\n",dispFileID(&(out_buff->meta.row.dir_id)));
	printf("file_id  =%s\n", dispFileID(&(out_buff->meta.inf.file_id)));
	printf("\n");
	*/

	//自ノードのSCNが古い場合
	if ( memcmp(&(G_NodeTbl[0].node_scn), &(out_buff->jlog.scn), sizeof(T_SCN)) < 0 ){
		memcpy(&(G_NodeTbl[0].node_scn), &(out_buff->jlog.scn), sizeof(T_SCN));
	}

	//ディスク書込み
	for(i=0; i<G_NodeTbl[0].disks; i++){

		if ( memcmp(&(G_DiskTbl[i].vol_scn), &(out_buff->jlog.scn), sizeof(T_SCN)) >= 0 ){ continue; }
		w_cnt++;

		//ジャーナルログ
		ret=diskwtWriteJournalLog(JOURNAL_TYPE_META, &(out_buff->jlog.scn), G_DiskTbl[i].dir, &(out_buff->jlog.dir_id), out_buff->jlog.rec_no);
		if ( ret < 0 ){
			logPrint(DEBUG_ERROR,"diskwtWriteJournalLog()=%d",ret); 
			return -1;
		}

		//METAディレクトリ作成
		disknmGetMetaDirName(G_DiskTbl[i].dir, out_buff->jlog.dir_id, filename);
		if ( filisDirectory(filename) != 1 ){
			ret=filCreateDirectory(filename);
			if ( ret < 0 ){
				logPrint(DEBUG_ERROR,"filCreateDirectory(%s)=%d",filename,ret);
				return -2; 
			}
		}

		//METAファイル作成
		disknmGetMetaFileName(G_DiskTbl[i].dir, out_buff->jlog.dir_id, filename);
		if ( filisFile(filename) != 1 ){
			memset(&hd,0,sizeof(T_META_HEADER));
			memcpy(hd.file_type, FILE_TYPE_META, sizeof(hd.file_type));
			memcpy((char*)&(hd.row.up_dir_id), (char*)&(out_buff->meta.row.up_dir_id), sizeof(T_FILE_ID));
			memcpy((char*)&(hd.row.dir_id), (char*)&(out_buff->meta.row.dir_id), sizeof(T_FILE_ID));

            //METAファイル新規作成
			if ( (fd=open(filename,O_CREAT|O_RDWR|O_TRUNC,0622)) < 0 ){
				logPrint(DEBUG_ERROR,"open(%s)=%d",filename,fd);
				return -3; 
			}
			ret=write(fd,(char *)&hd,sizeof(T_META_HEADER));	
			if ( ret != sizeof(T_META_HEADER) ){ close(fd); return -4; }
			logPrint(DEBUG_INFO,"T:W:[%s]",filename);
			close(fd);

            //METAリスト出力
            ret=diskwtWriteMetaList(G_DiskTbl[i].dir, filename);
            if ( ret < 0 ){
                logPrint(DEBUG_ERROR,"diskwtWriteMetaList()=%d",ret);
                return -5;
            }
		}

		if ( out_buff->jlog.rec_no == 0 ){ continue; }

		//METAレコード追加
		disknmGetMetaFileName(G_DiskTbl[i].dir, out_buff->meta.row.dir_id, filename);
		if ( (fd=open(filename,O_RDWR|O_APPEND)) < 0 ){
			logPrint(DEBUG_ERROR,"open(%s)=%d",filename,fd);
			return -6; 
		}
		ret=write(fd,(char *)&(out_buff->meta.inf),sizeof(T_META_INFO));	
		if ( ret != sizeof(T_META_INFO) ){ close(fd); return -7; }
		logPrint(DEBUG_INFO,"T:W:[%s] %s",filename,dispFileID(&(out_buff->meta.inf.file_id)) );
		close(fd);

        //Volumeヘッダ更新
        ret=diskwtWriteVolumeSCN(i, G_DiskTbl[i].dir, &(out_buff->jlog.scn));
        if ( ret < 0 ){
			logPrint(DEBUG_ERROR,"diskwtWriteVolumeSCN()=%d",ret);
			return -8; 
		}

	}
	return w_cnt;
}

int disksyncWriteScnCount(T_META_SYNC_RESULT *out_buff){
	int i;
	int ret;
	char filename[MAX_FILE_PATH+1];
	int w_cnt=0;
	T_FILE_ID file_id;

	//自ノードのSCNが古い場合
	if ( memcmp(&(G_NodeTbl[0].node_scn), &(out_buff->jlog.scn), sizeof(T_SCN)) < 0 ){
		memcpy(&(G_NodeTbl[0].node_scn), &(out_buff->jlog.scn), sizeof(T_SCN));
	}

	memset(&file_id, 0, sizeof(T_FILE_ID));

	//ディスク書込み
	for(i=0; i<G_NodeTbl[0].disks; i++){

		if ( memcmp(&(G_DiskTbl[i].vol_scn), &(out_buff->jlog.scn), sizeof(T_SCN)) >= 0 ){ continue; }
		w_cnt++;

        //ジャーナルログ
        ret=diskwtWriteJournalLog(JOURNAL_TYPE_SCN_UP, &(out_buff->jlog.scn), G_DiskTbl[i].dir, &file_id, 0);
        if ( ret < 0 ){
			logPrint(DEBUG_ERROR,"diskwtWriteJournalLog()=%d",ret); 
			return -1; 
		}

        //Volumeヘッダ更新
        ret=diskwtWriteVolumeSCN(i, G_DiskTbl[i].dir, &(out_buff->jlog.scn));
        if ( ret < 0 ){
			logPrint(DEBUG_ERROR,"diskwtWriteVolumeSCN()=%d",ret); 
			return -2; 
		}
	}

	return w_cnt;
}

