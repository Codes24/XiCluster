 /*****************************************************************************
*  <システム>   分散処理フレームワーク
*  <名称>	ディスクアクセス要求API
*  <目的>	   
*  <機能>	   
*  <開発環境>   UNIX
*  <特記事項>
*
*  VERSION	DATE		BY				CHANGE/COMMENT
*  -----------------------------------------------------------------------------
*  V0.00	  2014/03/31  Takakusaki		新規作成
******************************************************************************/
#include "xi_server.h"

int diskreqMetaMkDir(T_CLIENT_REQUEST req, char *msg){
	int ret;
	T_FILE_ID new_id;
	T_META_HEADER hd;
	T_META_HEADER cur_hd;
	T_META_INFO fi;
	char buff1[32];
	char buff2[32];
	char out_path[MAX_FILE_PATH+1];
	
	msg[0]=(char)NULL;
	//ディレクトリチェック
	if ( req.clt.para1[0] != '/' ){
		sprintf(msg, "directory error (%s)",req.clt.para1);
		return -1;
	}

	//ディレクトリ検索
	ret=diskrdSearchDir(req.clt.para1, &hd, &fi, out_path);
	logPrint(DEBUG_DEBUG,"diskrdSearchDir(%s,*,*,%s)=%d",req.clt.para1,out_path,ret);
	if ( ret == 1 ){
		sprintf(msg, "directory exist (%s)",req.clt.para1);
		return -2;
	}
	if ( ret < 0 ){
		sprintf(msg, "not found directory (%s)",req.clt.para1);
		return -3;
	}

	//ディレクトリ情報抽出
	ret=diskrdGetDirInfo(fi.file_id, &cur_hd);
	//logPrint(DEBUG_DEBUG,"diskrdGetDirInfo(%s,*)=%d",dispFileID(&(fi.file_id)), ret);
	if ( ret < 0 ){
		sprintf(msg, "not found directory (%s)",req.clt.para1);
		return -4;
	}

	//パーミッションチェック
	ret=daemon_CheckPerm(req, fi);
	//logPrint(DEBUG_DEBUG,"daemon_CheckPerm()=%d",ret);
	if ( ret < 0 ){
		sprintf(msg, "permission error (%s)",req.clt.para1);
		return -5;
	}

	//ディレクトリ新規作成
	daemon_GetNewFileID(&new_id,0);
    memset((char*)&fi,0,sizeof(T_META_INFO));
    memcpy((char*)&(fi.file_id), (char*)&(new_id), sizeof(T_FILE_ID));
    fi.st_type=FILE_TYPE_DIR;
    //fi.st_uid=getuid();
    //fi.st_gid=getgid();
    fi.st_uid=req.uid;
    fi.st_gid=req.gid;
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
    strcpy(fi.name, out_path);

	ret=cacheMetaWrite(cur_hd, fi, req.req);
	if ( ret < 0 ){
		sprintf(msg, "create directory error (%s)",req.clt.para1);
		return -6;
	}

	return 0;
}

int diskreqMetaRmDir(T_CLIENT_REQUEST req, char *msg){
	int ret;
	int file_cnt;
	u_long out_oya_id=0;
	u_long out_dir_id=0;
	T_META_HEADER hd;
	T_META_INFO fid;

	msg[0]=(char)NULL;

	//ディレクトリチェック
	if ( req.clt.para1[0] != '/' ){
		sprintf(msg, "directory error (%s)",req.clt.para1);
		return -1;
	}

	//ディレクトリ情報検索
	ret=diskrdSearchMeta(req.clt.para1, &hd, &fid);
	//logPrint(DEBUG_DEBUG,"diskrdSearchMeta(%s,*,*)=%d",req.clt.para1,ret);
	if ( ret == 0 ){
		sprintf(msg, "not found directory (%s)",req.clt.para1);
		return -2;
	}
	if ( ret < 0 ){
		sprintf(msg, "meta search error (%s)",req.clt.para1);
		return -3;
	}
	if ( fid.st_type != FILE_TYPE_DIR ){
		sprintf(msg, "not directory (%s)",req.clt.para1);
		return -4;
	}

	//ディレクトリ情報
	ret=diskrdGetDirInFiles(fid.file_id, &file_cnt);
	if ( ret < 0 ){
		sprintf(msg, "not found directory (%s)",req.clt.para1);
		return -5;
	}
	if ( file_cnt > 0 ){
		sprintf(msg, "directory is not empty (%s)",req.clt.para1);
		return -6;
	}

	//パーミッションチェック
	ret=daemon_CheckPerm(req, fid);
	//logPrint(DEBUG_DEBUG,"daemon_CheckPerm()=%d",ret);
	if ( ret < 0 ){
		sprintf(msg, "permission error (%s)",req.clt.para1);
		return -7;
	}

	//ディレクトリ削除
	gettimeofday(&(fid.st_dtm),NULL);
	ret=cacheMetaWrite(hd, fid, req.req);
	if ( ret < 0 ){
		sprintf(msg, "delete directory error (%s)",req.clt.para1);
		return -8;
	}

	return 0;
}

int diskreqMetaRmFile(T_CLIENT_REQUEST req, char *msg){
	int ret;
	int wk;
	T_META_HEADER hd;
	T_META_HEADER cur_hd;
	T_META_INFO fid;
	T_META_INFO fif;
	char out_path[MAX_FILE_PATH+1];

	msg[0]=(char)NULL;

	//ディレクトリチェック
	if ( req.clt.para1[0] != '/' ){
		sprintf(msg, "directory error (%s)",req.clt.para1);
		return -1;
	}

	//ディレクトリ検索
	ret=diskrdSearchDir(req.clt.para1, &hd, &fid, out_path);
	//logPrint(DEBUG_DEBUG,"diskrdSearchDir(%s,*,*,%s)=%d",req.clt.para1,out_path,ret);
	if ( ret <= 0 ){
		sprintf(msg, "No such directory (%s)",req.clt.para1);
		return -2;
	}

	//ファイル情報検索
	ret=diskrdSearchMeta(req.clt.para1, &hd, &fif);
	//logPrint(DEBUG_DEBUG,"diskrdSearchMeta(%s,*,*)=%d",req.clt.para1,ret);
	if ( ret <= 0 ){
		sprintf(msg, "No such directory (%s)",req.clt.para1);
		return -3;
	}
	if ( fif.st_type != FILE_TYPE_FILE ){
		sprintf(msg, "not file (%s)",req.clt.para1);
		return -4;
	}

	//パーミッションチェック
	ret=daemon_CheckPerm(req, fid);
	//logPrint(DEBUG_DEBUG,"daemon_CheckPerm()=%d",ret);
	if ( ret < 0 ){
		sprintf(msg, "permission error (%s)",req.clt.para1);
		return -5;
	}

	//ファイル削除
	gettimeofday(&(fif.st_dtm),NULL);
	ret=cacheMetaWrite(hd, fif, req.req);
	if ( ret < 0 ){
		sprintf(msg, "delete file error (%s)",req.clt.para1);
		return -6;
	}

	return 0;
}

int diskreqMetaStat(T_CLIENT_REQUEST req, char *msg, struct xi_stat *stat){
	int ret;
	int wk;
	T_META_HEADER hd;
	T_META_INFO fid;

	msg[0]=(char)NULL;
	memset((char*)stat,0,sizeof(struct xi_stat));

	//ディレクトリチェック
	if ( req.clt.para1[0] != '/' ){
		sprintf(msg, "directory error (%s)",req.clt.para1);
		return -1;
	}

	//ファイル情報検索
	ret=diskrdSearchMeta(req.clt.para1, &hd, &fid);
	//logPrint(DEBUG_DEBUG,"diskrdSearchMeta(%s,*,*)=%d",req.clt.para1,ret);
	if ( ret == 0 ){
		sprintf(msg, "No such file or directory (%s)",req.clt.para1);
		return -2;
	}
	if ( ret < 0 ){
		sprintf(msg, "No such file or directory (%s)",req.clt.para1);
		return -3;
	}

	//パーミッションチェック
	ret=daemon_CheckPerm(req, fid);
	logPrint(DEBUG_INFO,"daemon_CheckPerm()=%d",ret);
	if ( ret < 0 ){
		sprintf(msg, "permission error (%s)",req.clt.para1);
		return -4;
	}

	//ファイル種類を返す
	//if ( fid.st_type == FILE_TYPE_FILE ){
	//	sprintf(msg, "FILE");
	//}else if ( fid.st_type == FILE_TYPE_DIR ){
	//	sprintf(msg, "DIR");
	//}else{
	//	sprintf(msg, "UNKNOWN");
	//}

	//META情報を返す
	stat->st_type = fid.st_type;
	stat->st_mode = fid.st_mode;
	stat->st_uid = fid.st_uid;
	stat->st_gid = fid.st_gid;
	stat->st_size = fid.st_size;
	stat->st_blksize = fid.st_blksize;
	stat->st_blocks = fid.st_blocks;
	stat->dup_cnt = fid.dup_cnt;
	memcpy(&stat->st_atm, &fid.st_atm, sizeof(struct timeval));
	memcpy(&stat->st_mtm, &fid.st_mtm, sizeof(struct timeval));
	memcpy(&stat->st_ctm, &fid.st_ctm, sizeof(struct timeval));

	return 0;
}

int diskreqMetaChmod(T_CLIENT_REQUEST req, char *msg){
	int ret;
	int wk;
	T_META_HEADER hd;
	T_META_INFO fid;

	msg[0]=(char)NULL;

	//ディレクトリチェック
	if ( req.clt.para2[0] != '/' ){
		sprintf(msg, "directory error (%s)",req.clt.para2);
		return -1;
	}

	//ファイル情報検索
	ret=diskrdSearchMeta(req.clt.para2, &hd, &fid);
	//logPrint(DEBUG_DEBUG,"diskrdSearchMeta(%s,*,*)=%d",req.clt.para2,ret);
	if ( ret == 0 ){
		sprintf(msg, "No such file or directory (%s)",req.clt.para2);
		return -2;
	}
	if ( ret < 0 ){
		sprintf(msg, "No such file or directory (%s)",req.clt.para2);
		return -3;
	}

	//パーミッションチェック
	ret=daemon_CheckPerm(req, fid);
	//logPrint(DEBUG_DEBUG,"daemon_CheckPerm()=%d",ret);
	if ( ret < 0 ){
		sprintf(msg, "permission error (%s)",req.clt.para2);
		return -4;
	}

	//パーミッション更新
	sscanf(req.clt.para1,"%X",&wk);
	fid.st_mode = wk;
	ret=cacheMetaWrite(hd, fid, req.req);
	if ( ret < 0 ){
		sprintf(msg, "chmod error (%s)",req.clt.para2);
		return -5;
	}

	return 0;
}

int diskreqMetaChown(T_CLIENT_REQUEST req, char *msg){
	int ret;
	T_META_HEADER hd;
	T_META_INFO fid;

	msg[0]=(char)NULL;

	//ディレクトリチェック
	if ( req.clt.para2[0] != '/' ){
		sprintf(msg, "directory error (%s)",req.clt.para2);
		return -1;
	}

	//ファイル情報検索
	ret=diskrdSearchMeta(req.clt.para2, &hd, &fid);
	//logPrint(DEBUG_DEBUG,"diskrdSearchMeta(%s,*,*)=%d",req.clt.para2,ret);
	if ( ret == 0 ){
		sprintf(msg, "No such file or directory (%s)",req.clt.para2);
		return -2;
	}
	if ( ret < 0 ){
		sprintf(msg, "No such file or directory (%s)",req.clt.para2);
		return -3;
	}

	//パーミッションチェック
	ret=daemon_CheckPerm(req, fid);
	//logPrint(DEBUG_DEBUG,"daemon_CheckPerm()=%d",ret);
	if ( ret < 0 ){
		sprintf(msg, "permission error (%s)",req.clt.para2);
		return -4;
	}

	//所有者変更
	fid.st_uid = atol(req.clt.para1);
	ret=cacheMetaWrite(hd, fid, req.req);
	if ( ret < 0 ){
		sprintf(msg, "chown error (%s)",req.clt.para2);
		return -5;
	}

	return 0;
}

int diskreqMetaChgrp(T_CLIENT_REQUEST req, char *msg){
	int ret;
	T_META_HEADER hd;
	T_META_INFO fid;

	msg[0]=(char)NULL;

	//ディレクトリチェック
	if ( req.clt.para2[0] != '/' ){
		sprintf(msg, "directory error (%s)",req.clt.para2);
		return -1;
	}

	//ファイル情報検索
	ret=diskrdSearchMeta(req.clt.para2, &hd, &fid);
	//logPrint(DEBUG_DEBUG,"diskrdSearchMeta(%s,*,*)=%d",req.clt.para2,ret);
	if ( ret == 0 ){
		sprintf(msg, "No such file or directory (%s)",req.clt.para2);
		return -2;
	}
	if ( ret < 0 ){
		sprintf(msg, "No such file or directory (%s)",req.clt.para2);
		return -3;
	}

	//パーミッションチェック
	ret=daemon_CheckPerm(req, fid);
	if ( ret < 0 ){
		sprintf(msg, "permission error (%s)",req.clt.para2);
		return -4;
	}

	//所有グループ変更
	fid.st_gid = atol(req.clt.para1);
	ret=cacheMetaWrite(hd, fid, req.req);
	if ( ret < 0 ){
		sprintf(msg, "chgrp error (%s)",req.clt.para2);
		return -5;
	}

	return 0;
}

