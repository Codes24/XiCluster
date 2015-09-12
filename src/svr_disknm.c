/*****************************************************************************
*  <システム>   分散処理フレームワーク
*  <名称>	ファイル名生成API
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

static char *disknmSubDirName(suseconds_t t){
	char w[32];
	static char ret[32];

	sprintf(w,"%06X",t);
	sprintf(ret,"%c%c/%c%c/%c%c",
		w[0],
		w[1],
		w[2],
		w[3],
		w[4],
		w[5]
	);
	return ret;
}

int disknmChoiceServer(int flg){
	int i;
	int w;
	u_int sv_idx=0;
	u_int sv_io=0;
	u_int sv_free=0;

	if ( flg == 1 ){

		//ディスク使用量優先
		for(i=0; i<G_NodeTbl[0].disks; i++){
			if ( G_DiskTbl[i].dir[0] == NULL ){ break; }
			if ( G_DiskTbl[i].sts != VOLUME_STATUS_SERVICE){ continue; }
			if ( G_DiskTbl[i].f_free > sv_free ){
				sv_free=G_DiskTbl[i].f_free;
				sv_idx=i;
			}
		}

	}else{

		//パフォーマンス優先
		for(i=0; i<G_NodeTbl[0].disks; i++){
			if ( G_DiskTbl[i].dir[0] == NULL ){ break; }
			if ( G_DiskTbl[i].sts != VOLUME_STATUS_SERVICE){ continue; }
			w=G_DiskTbl[i].stat_s.rd_ticks + G_DiskTbl[i].stat_s.wr_ticks;
			if ( sv_io != 0 ){
				if ( w >= sv_io ){ continue; }
			}
			sv_io=w;
			sv_idx=i;
		}
	}

	return sv_idx;
}

int disknmGetMetaDirName(char *data_base_dir, T_FILE_ID id, char *filename){

	if ( data_base_dir[0] == (char)NULL ){
		int sidx=disknmChoiceServer(1);
		sprintf(filename,"%s/META/%s", G_DiskTbl[sidx].dir, disknmSubDirName(id.id.tv_usec), id.id.tv_sec);
	}else{
		sprintf(filename,"%s/META/%s", data_base_dir, disknmSubDirName(id.id.tv_usec), id.id.tv_sec);
	}
	return 0;
}

/* ファイル名取得 */
int disknmGetMetaFileName(char *data_base_dir, T_FILE_ID id, char *filename){

	if ( data_base_dir[0] == (char)NULL ){
		int sidx=disknmChoiceServer(1);
		sprintf(filename,"%s/META/%s/%08X.%08X%06X", G_DiskTbl[sidx].dir, disknmSubDirName(id.id.tv_usec), id.id.tv_sec, id.ver.tv_sec, id.ver.tv_usec);
	}else{
		sprintf(filename,"%s/META/%s/%08X.%08X%06X", data_base_dir, disknmSubDirName(id.id.tv_usec), id.id.tv_sec, id.ver.tv_sec, id.ver.tv_usec);
	}
	return 0;
}

int disknmGetIdxDirName(char *data_base_dir, T_FILE_ID id, char *filename){
	if ( data_base_dir[0] == (char)NULL ){
		int sidx=disknmChoiceServer(1);
		sprintf(filename,"%s/IDX/%s", G_DiskTbl[sidx].dir, disknmSubDirName(id.id.tv_usec));
	}else{
		sprintf(filename,"%s/IDX/%s", data_base_dir, disknmSubDirName(id.id.tv_usec));
	}
	return 0;
}

int disknmGetIdxFileName(char *data_base_dir, T_FILE_ID id, char *filename){
	if ( data_base_dir[0] == (char)NULL ){
		int sidx=disknmChoiceServer(1);
		sprintf(filename,"%s/IDX/%s/%08X.%08X%06X",
			G_DiskTbl[sidx].dir, disknmSubDirName(id.id.tv_usec), id.id.tv_sec, id.ver.tv_sec, id.ver.tv_usec);
	}else{
		sprintf(filename,"%s/IDX/%s/%08X.%08X%06X",
			data_base_dir, disknmSubDirName(id.id.tv_usec), id.id.tv_sec, id.ver.tv_sec, id.ver.tv_usec);
	}
	return 0;
}

int disknmGetDataDirName(char *data_base_dir, T_FILE_ID id, char *filename){
	if ( data_base_dir[0] == (char)NULL ){
		int sidx=disknmChoiceServer(1);
		sprintf(filename,"%s/DATA/%s/%08X.%08X%06X",
			G_DiskTbl[sidx].dir, disknmSubDirName(id.id.tv_usec), id.id.tv_sec, id.ver.tv_sec, id.ver.tv_usec);
	}else{
		sprintf(filename,"%s/DATA/%s/%08X.%08X%06X",
			data_base_dir, disknmSubDirName(id.id.tv_usec), id.id.tv_sec, id.ver.tv_sec, id.ver.tv_usec);
	}
	return 0;
}

int disknmGetDataFileName(char *data_base_dir, T_FILE_ID id, int block_no, char *filename){
	if ( data_base_dir[0] == (char)NULL ){
		int sidx=disknmChoiceServer(1);
		sprintf(filename,"%s/DATA/%s/%08X.%08X%06X/%d",
			G_DiskTbl[sidx].dir, disknmSubDirName(id.id.tv_usec), id.id.tv_sec, id.ver.tv_sec, id.ver.tv_usec, block_no);
	}else{
		sprintf(filename,"%s/DATA/%s/%08X.%08X%06X/%d",
			data_base_dir, disknmSubDirName(id.id.tv_usec), id.id.tv_sec, id.ver.tv_sec, id.ver.tv_usec, block_no);
	}
	return 0;
}

int disknmSearchDataFileName(char *data_base_dir, T_FILE_ID id, int block_no, char *filename){
	char wdir[MAX_FILE_PATH+1];

	if ( data_base_dir[0] == (char)NULL ){
		int sidx=disknmChoiceServer(1);
		strcpy(wdir,G_DiskTbl[sidx].dir);
	}else{
		strcpy(wdir,data_base_dir);
	}

	sprintf(filename,"%s/DATA/%s/%08X.%08X%06X/%d",
		wdir, disknmSubDirName(id.id.tv_usec), id.id.tv_sec, id.ver.tv_sec, id.ver.tv_usec, block_no);
	if ( filisFile(filename) == 1 ){ return 1; }

	sprintf(filename,"%s/DATA/%s/%08X.%08X%06X/%d.gz",
		wdir, disknmSubDirName(id.id.tv_usec), id.id.tv_sec, id.ver.tv_sec, id.ver.tv_usec, block_no);
	if ( filisFile(filename) == 1 ){ return 1; }

	sprintf(filename,"%s/DATA/%s/%08X.%08X%06X/%d.zip",
		wdir, disknmSubDirName(id.id.tv_usec), id.id.tv_sec, id.ver.tv_sec, id.ver.tv_usec, block_no);
	if ( filisFile(filename) == 1 ){ return 1; }

	sprintf(filename,"%s/DATA/%s/%08X.%08X%06X/%d.deflate",
		wdir, disknmSubDirName(id.id.tv_usec), id.id.tv_sec, id.ver.tv_sec, id.ver.tv_usec, block_no);
	if ( filisFile(filename) == 1 ){ return 1; }

	filename[0]=NULL;
	return 0;
}

int disknmGetLogDirName(char *data_base_dir, char *filename){
	if ( data_base_dir[0] == (char)NULL ){
		int sidx=disknmChoiceServer(1);
		sprintf(filename,"%s/LOG", G_DiskTbl[sidx].dir);
	}else{
		sprintf(filename,"%s/LOG", data_base_dir);
	}
	return 0;
}

char *disknmFileName2FileID(char *filenm){
	static char wk[32];
	int i;
	u_long sec,usec;
	char w[MAX_FILE_PATH+1];
	char w1[32];
	char w2[32];
	char w3[32];

	//ID部分のみ抽出
	strcpy(w,filenm);
	utlStrCut(w,"/.");
	utlStrCutRear(w,28);

	//入れ替える
	utlStrStr(w,6,8,w1);
	utlStrStr(w,0,6,w2);
	utlStrStr(w,14,14,w3);
	sprintf(wk,"%s%s.%s",w1,w2,w3);

	return wk;
}

void disknmFileName2FileID(char *filenm, T_FILE_ID *file_id){
	static char wk[32];
	int i;
	u_long sec,usec;
	char w[MAX_FILE_PATH+1];
	char w1[32];
	char w2[32];
	char w3[32];
	char w4[32];

	//ID部分のみ抽出
	strcpy(w,filenm);
	utlStrCut(w,"/.");
	utlStrCutRear(w,28);

	//入れ替える
	utlStrStr(w,6,8,w1);
	utlStrStr(w,0,6,w2);
	utlStrStr(w,14,8,w3);
	utlStrStr(w,22,6,w4);
	sscanf(w1,"%X",&(file_id->id.tv_sec));
	sscanf(w2,"%X",&(file_id->id.tv_usec));
	sscanf(w3,"%X",&(file_id->ver.tv_sec));
	sscanf(w4,"%X",&(file_id->ver.tv_usec));
	//printf("%X %X %X %X\n",file_id->id.tv_sec, file_id->id.tv_usec, file_id->ver.tv_sec, file_id->ver.tv_usec);
}

void disknmFileID2FileID(char *w, T_FILE_ID *file_id){
	char w1[32];
	char w2[32];
	char w3[32];
	char w4[32];

	utlStrStr(w,0,8,w1);
	utlStrStr(w,8,6,w2);
	utlStrStr(w,14,8,w3);
	utlStrStr(w,22,6,w4);
	sscanf(w1,"%X",&(file_id->id.tv_sec));
	sscanf(w2,"%X",&(file_id->id.tv_usec));
	sscanf(w3,"%X",&(file_id->ver.tv_sec));
	sscanf(w4,"%X",&(file_id->ver.tv_usec));
}
