/*****************************************************************************
*  <システム>   分散処理フレームワーク
*  <名称>	デーモン共通API
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
 グローバル変数
******************************************************************************/
/* 共有メモリ上のアドレス */
T_SHM_HEADER	*G_ShmHeader;   /* SGA(ヘッダ領域) */
T_CONFIG_TABLE  *G_ConfTbl;	 /* SGA(パラメータ領域) */
T_NODE_TABLE	*G_NodeTbl;	 /* SGA(ノード管理領域) */
T_PROCESS_TABLE *G_ProcTbl;	 /* SGA(プロセス管理領域) */
T_DISK_TABLE	*G_DiskTbl;	 /* SGA(ディスク管理領域) */
T_META_TABLE	*G_MetaTbl;	 /* SGA(METAキャッシュ管理領域) */
T_INDEX_TABLE   *G_IndexTbl;	/* SGA(INDEXキャッシュ管理領域) */
T_DATA_TABLE	*G_DataTbl;	 /* SGA(DATAキャッシュ領域) */

int G_ShmKey;
int G_SemKey;
int G_MquKey;
int G_ProcNo=0;

char G_BaseDir[MAX_FILE_PATH+1]=".";
char G_TempDir[MAX_FILE_PATH+1]="./temp";
char G_ConfDir[MAX_FILE_PATH+1]="./conf";
char G_HLogDir[MAX_FILE_PATH+1];


int daemon_CompressType(int out_size){
	if ( G_ConfTbl->compress_type==0 or out_size<=G_ConfTbl->compress_threshold ){
		return 0;
	}
	return G_ConfTbl->compress_type;
}

int daemon_DataFileType(char *filename){
	int len=strlen(filename);

	if ( strcmp("gz",filename + len - 2) == 0 ){ return 1; }
	if ( strcmp("zip",filename + len - 3) == 0 ){ return 2; }
	if ( strcmp("deflate",filename + len - 7) == 0 ){ return 3; }
	
	return 0;
}

int daemon_ActiveNodeNum(){
	int i;
	int ret=0;

	for (i=0; i<G_ConfTbl->max_node; i++){
		if ( G_NodeTbl[i].sts != SYSTEM_STATUS_RUNNING ){ continue; }
		ret++;
	}
	return ret;
}

int daemon_DuplicateNum(){
	int i;
	int ret=0;

	for (i=0; i<G_ConfTbl->max_node; i++){
		if ( G_NodeTbl[i].sts != SYSTEM_STATUS_RUNNING ){ continue; }
		ret++;
	}
	if ( ret >= G_ConfTbl->block_dup_cnt ){
		return G_ConfTbl->block_dup_cnt;
	}
	return ret;
}

int daemon_GetMasterIdx(){
	int i;
	int ret=-1;

	for(i=0; i<=G_ConfTbl->max_node; i++){
		if ( G_NodeTbl[i].run_hostname[0] == (char)NULL ){ break; }
		if ( G_NodeTbl[i].master_flg == MASTER_FLG_MASTER ){
			if ( ret != (-1)){ return -2; }
			ret=i;
		}
	}
	return ret;
}

u_long daemon_GetMasterSvrIP(){
	int idx=daemon_GetMasterIdx();
	if (idx < 0 ){
		return 0;
	}else{
		return G_NodeTbl[idx].svr_ip;
	}
}
u_long daemon_GetMasterCltIP(){
	int idx=daemon_GetMasterIdx();
	if (idx < 0 ){
		return 0;
	}else{
		return G_NodeTbl[idx].clt_ip;
	}
}

int daemon_CheckMaster(){
	int i;
	int ck_id_max_node=-1;
	int ck_all_cnt=0;
	int ck_act_cnt=0;
	int ck_mst_cnt=0;
	int ck_msyn_process=0;
	u_char ck_node[20];
	T_SCN ck_scn;
	time_t t;
	static time_t no_master_dt=0;

	//META同期プロセスが０の場合はマスタになれない
	if ( G_ConfTbl->msyn_process <= 0 ){ return -1; }

	//起動してからしばらくは待つ（ノード情報受信待ち）
	time(&t);
	if ( t <= (G_NodeTbl[0].stime + G_ConfTbl->data_trans_interval) ){ return -1; }
	
	//最新SCNの検索
	ck_scn.dt=G_NodeTbl[0].node_scn.dt;
	ck_scn.seq1=G_NodeTbl[0].node_scn.seq1;
	ck_scn.seq2=G_NodeTbl[0].node_scn.seq2;

	for(i=0; i<=G_ConfTbl->max_node; i++){
		if ( G_NodeTbl[i].run_hostname[0] == (char)NULL ){ continue; }
		ck_all_cnt++;
		if ( G_NodeTbl[i].msyn_process > 0 ){ ck_msyn_process++; }

		//アクティブノード数/マスター数
		if ( G_NodeTbl[i].sts == SYSTEM_STATUS_RUNNING ){
			ck_act_cnt++;
			if ( G_NodeTbl[i].master_flg == MASTER_FLG_MASTER ){ ck_mst_cnt++; }
		}

		//最大SCN
		if ( G_NodeTbl[i].node_scn.dt < ck_scn.dt ){ continue; }
		if ( G_NodeTbl[i].node_scn.dt == ck_scn.dt and G_NodeTbl[i].node_scn.seq1 < ck_scn.seq1 ){ continue; }
		if ( G_NodeTbl[i].node_scn.dt == ck_scn.dt 
				and G_NodeTbl[i].node_scn.seq1 == ck_scn.seq1 
				and G_NodeTbl[i].node_scn.seq2 <= ck_scn.seq2 ){ continue; }

		ck_scn.dt=G_NodeTbl[i].node_scn.dt;
		ck_scn.seq1=G_NodeTbl[i].node_scn.seq1;
		ck_scn.seq2=G_NodeTbl[i].node_scn.seq2;

	}

	//最大SCNの中で最大Node番号は
	memcpy((void*)ck_node, (void*)G_NodeTbl[0].node_id, NODE_ID_LEN);
	for(i=0; i<=G_ConfTbl->max_node; i++){
		if ( G_NodeTbl[i].run_hostname[0] == (char)NULL ){ continue; }
		if ( G_NodeTbl[i].node_scn.dt < ck_scn.dt ){ continue; }
		if ( G_NodeTbl[i].node_scn.dt == ck_scn.dt and G_NodeTbl[i].node_scn.seq1 < ck_scn.seq1 ){ continue; }
		if ( G_NodeTbl[i].node_scn.dt == ck_scn.dt 
				and G_NodeTbl[i].node_scn.seq1 == ck_scn.seq1 
				and G_NodeTbl[i].node_scn.seq2 < ck_scn.seq2 ){ continue; }

//logPrint(DEBUG_INFO,"[%d] sts=%d",i,G_NodeTbl[i].sts);
		if ( G_NodeTbl[i].sts == SYSTEM_STATUS_INIT || G_NodeTbl[i].sts == SYSTEM_STATUS_SYNC || G_NodeTbl[i].sts == SYSTEM_STATUS_RUNNING ){
			if ( memcmp(G_NodeTbl[i].node_id, ck_node, NODE_ID_LEN) >= 0 ){
				memcpy(ck_node,  G_NodeTbl[i].node_id, NODE_ID_LEN);
				ck_id_max_node=i;
			}
		}
	}

	//デバック表示
	static int sv_all_cnt=-1;
	static int sv_msyn_process=-1;
	static int sv_act_cnt=-1;
	static int sv_mst_cnt=-1;
	int debug_out_flg=DEBUG_INFO;
	if ( sv_all_cnt==ck_all_cnt && sv_msyn_process==ck_msyn_process && sv_act_cnt==ck_act_cnt && sv_mst_cnt==ck_mst_cnt){
		debug_out_flg=DEBUG_DEBUG;
	}else{
		debug_out_flg=DEBUG_INFO;
	}
	logPrint(debug_out_flg,"MASTER_CHECK CNT(all=%d/%d master=%d/%d) MAX_NODE=%d SCN(new=%d:%d:%d my=%d:%d:%d)",
			ck_act_cnt,
			ck_all_cnt,
			ck_mst_cnt,
			ck_msyn_process,
			ck_id_max_node,
			ck_scn.dt,
			ck_scn.seq1, 
			ck_scn.seq2, 
			G_NodeTbl[0].node_scn.dt,
			G_NodeTbl[0].node_scn.seq1,
			G_NodeTbl[0].node_scn.seq2
	);
	sv_all_cnt=ck_all_cnt;
	sv_msyn_process=ck_msyn_process;
	sv_act_cnt=ck_act_cnt;
	sv_mst_cnt=ck_mst_cnt;

	//重複マスター排除
	if ( G_NodeTbl[0].master_flg == MASTER_FLG_MASTER ){
		if ( ck_mst_cnt>1 && ck_id_max_node!=0 ){
			return MASTER_FLG_SLAVE;
		}
		return MASTER_FLG_MASTER;
	}

	//マスタ判定
	if ( ck_mst_cnt > 0 ){ no_master_dt=0; return MASTER_FLG_SLAVE; }
	if ( no_master_dt == 0 ){ no_master_dt=t; }
	if ( t <= (no_master_dt + G_ConfTbl->nodetbl_write_interval + 1) ){ return MASTER_FLG_SLAVE; }
	if ( ck_id_max_node == 0 ){ return MASTER_FLG_MASTER; }
	return MASTER_FLG_SLAVE;
}

int daemon_GetSCN(T_SCN *scn, int flg){
	time_t t;
	struct tm *tt;
	int yyyymmdd;

	//時間取得
	time(&t);
	tt=localtime(&t);
	yyyymmdd = (tt->tm_year + 1900) * 10000 + (tt->tm_mon + 1) * 100 + tt->tm_mday;

	//初期SCN
	if ( flg == 0 ){
		scn->dt = 0;
		scn->seq1 = 0;
		scn->seq2 = scn->seq2 + 1;
		return 0;
	}

	//マスター起動時の通番インクリメント
	if ( flg == 2 ){
		if ( scn->dt == 0 ){
			scn->dt = yyyymmdd;
			scn->seq1 = 0;
		}
		scn->seq1++;
		scn->seq2 = 0;
		return 0;
	}

	//通常のSCN番号発行
	if ( yyyymmdd > scn->dt ){
		scn->dt = yyyymmdd;
		scn->seq1 = 0;
		scn->seq2 = 0;
	}else{
		scn->seq2 = scn->seq2 + 1;
	}

	return 0;
}

int daemon_GetNewFileID(T_FILE_ID *id, int flg){
	struct timeval t;

	//ID取得
	if ( semLock(SEM_LOCK_IDXO003, G_ConfTbl->enospc_retry, G_ConfTbl->enospc_retrans) < 0 ){
		id->id.tv_sec=(time_t)999999999;
		id->id.tv_usec=(suseconds_t)999999;
		id->ver.tv_sec=(time_t)999999999;
		id->ver.tv_usec=(suseconds_t)999999;
		return -1; 
	}
	gettimeofday(&t,NULL);
	if ( semUnLock(SEM_LOCK_IDXO003,G_ConfTbl->enospc_retry, G_ConfTbl->enospc_retrans) < 0 ){
		id->id.tv_sec=(time_t)999999999;
		id->id.tv_usec=(suseconds_t)999999;
		id->ver.tv_sec=(time_t)999999999;
		id->ver.tv_usec=(suseconds_t)999999;
		return -2; 
	}

	//新規ID
	if ( flg==1 && id->id.tv_sec != 0 ){
		id->ver.tv_sec=(time_t)t.tv_sec;
		id->ver.tv_usec=(suseconds_t)t.tv_usec;
	}else{
		id->id.tv_sec=(time_t)t.tv_sec;
		id->id.tv_usec=(suseconds_t)t.tv_usec;
		id->ver.tv_sec=(time_t)t.tv_sec;
		id->ver.tv_usec=(suseconds_t)t.tv_usec;
	}
	return 0;
}

int daemon_NodeTableWrite(){
	int i;
	int fd;
	int ret;
	char filename[MAX_FILE_PATH+1];
	T_NODE_HEADER nhd;

	sprintf(filename,"%s/.node.dat",G_TempDir);
	if ( (fd=open(filename,O_CREAT|O_RDWR|O_TRUNC,0666)) < 0 ){ return -1; }

	memcpy(&(nhd.file_type), FILE_TYPE_NODE, sizeof(nhd.file_type));	
	ret=write(fd, &nhd, sizeof(T_NODE_HEADER));
	if ( ret != sizeof(T_NODE_HEADER) ){ 
		close(fd);
		return -2;
	}

	for ( i=0; i<G_ConfTbl->max_node; i++){
		if ( G_NodeTbl[i].run_hostname[0] == NULL ){ continue; }
		ret=write(fd, &G_NodeTbl[i], sizeof(T_NODE_TABLE));
		if ( ret != sizeof(T_NODE_TABLE) ){
			close(fd);
			return -3;
		}
	}
	close(fd);
	return 0;
}

int daemon_NodeTableRead(){
	int i;
	int fd;
	int ret;
	u_long fsize;
	char filename[MAX_FILE_PATH+1];
	T_NODE_TABLE buff;
	T_NODE_HEADER nhd;
	
	sprintf(filename,"%s/.node.dat",G_TempDir);
	fsize=filGetFileSize(filename);
	if ( ((fsize - sizeof(T_NODE_HEADER)) % sizeof(T_NODE_TABLE)) != 0 ){ return 0; }

	if ( (fd=open(filename,O_RDONLY)) < 0 ){ return -1; }

	ret=read(fd,(char *)&nhd,sizeof(T_NODE_HEADER));
	if ( ret != sizeof(T_NODE_HEADER) ){
		close(fd);
		return -2;
	}

	i=0;
	while( (ret=read(fd,(char *)&buff,sizeof(T_NODE_TABLE))) > 0 ){
		if ( i > 0 ){
			memcpy((char*)&(G_NodeTbl[i]),(char *)&buff,sizeof(T_NODE_TABLE));
		}
		i++;
	}

	close(fd);
	return 0;
}

int daemon_NodeTableInit(){
	int i;
	int ck;
	char sfilename[MAX_FILE_PATH+1];
	FILE *fp;
	char buff[20480];
	u_long addr;
	T_NODE_TABLE wk;

	//ステイタス初期化
	for(i=0; i<=G_ConfTbl->max_node; i++){
		G_NodeTbl[i].sts=SYSTEM_STATUS_UNKNOWN;
		G_NodeTbl[i].master_flg=MASTER_FLG_NONE;
	}

	//server.lstでデータ設定
	sprintf(sfilename,"%s/conf/server.lst",G_BaseDir);
	if ( filisFile(sfilename) != 1 ){
		logPrint(DEBUG_INFO,"no file (%s)",sfilename);
		return 0; 
	}
	if ( (fp=fopen(sfilename,"r")) == NULL ){
		return -1;
	}
	while( fgets(buff,sizeof(buff),fp) != NULL ){
		utlCurNewLine(buff);
		if ( (addr=utlHostnameToAddress(buff)) == 0 ){ continue; }

		memset(&wk,0,sizeof(T_NODE_TABLE));
		strcpy(wk.run_hostname, buff);
		wk.svr_ip=addr;
		wk.clt_ip=addr;
		wk.sts=SYSTEM_STATUS_UNKNOWN;
		wk.master_flg=MASTER_FLG_NONE;
		utlGetSHA1(wk.run_hostname, wk.node_id);
		time(&wk.alv);

		//検索
		ck=-1;
		for(i=0; i<=G_ConfTbl->max_node; i++){
			if ( G_NodeTbl[i].run_hostname[0] == NULL ){ break; }
			if ( strcmp(G_NodeTbl[i].run_hostname, wk.run_hostname) == 0 ){ ck=i; break; }
			if ( G_NodeTbl[i].svr_ip == wk.svr_ip ){ ck=i; break; }
			if ( G_NodeTbl[i].clt_ip == wk.clt_ip ){ ck=i; break; }
		}
		if ( ck >= 0 ){ continue; }
		memcpy(&(G_NodeTbl[i]), &wk, sizeof(T_NODE_TABLE));
		logPrint(DEBUG_INFO,"G_NodeTbl[%d] %s %s",i,G_NodeTbl[i].run_hostname, utlDisplyIP((u_char*)&G_NodeTbl[i].svr_ip));
	}
	fclose(fp);

	return 0;
}

int daemon_exec(int no, int typ, char *name, char *para){
	int cld_pid;
	time_t tm;
	char pname[64];
	char command[MAX_FILE_PATH + 1];

	if ( no >= G_ConfTbl->max_process ){
		logPrint(DEBUG_ERROR,"process over (%d/%d)",no,G_ConfTbl->max_process);
		return -1;
	}
	G_ProcNo=no;
	sprintf(pname,"XICLUSTER_%03d_%s",no,name);
	sprintf(command,"%s/bin/xicluster_daemon",G_BaseDir);

	//子プロセス起動
	if ( (cld_pid=fork()) == 0 ){
		char *newargv[] = { pname, para, NULL };	
		execv(command, newargv);
		logPrint(DEBUG_WARNING,"child execute error");
		exit(0);
	}

	time(&tm);
	strcpy(G_ProcTbl[no].name,pname);
	strcpy(G_ProcTbl[no].para,para);
	G_ProcTbl[no].ptype=typ;
	G_ProcTbl[no].pid=cld_pid;
	cacheSetProcStatus(no, PROCESS_STATUS_INIT);
	G_ProcTbl[no].req=0;
	G_ProcTbl[no].fork_try++;
	G_ProcTbl[no].stime=tm;
	G_ProcTbl[no].etime=0;
	G_ProcTbl[no].alv=tm;
	G_ProcTbl[no].cache_idx=(-1);
	logPrint(DEBUG_INFO,"fork %s (pno=%d pid=%d pname=%s)",command,no,cld_pid,pname);

	usleep(G_ConfTbl->slow_start);
	return 0;
}

int daemon_CheckPerm(T_CLIENT_REQUEST req, T_META_INFO fi){
	int ck=-1;
	int wk;

//printf("diskrdCheckPerm() req=%d (%d:%d) (%d:%d,%X)\n",req.req, req.uid, req.gid, fi.st_uid,fi.st_gid,fi.st_mode);
	if ( req.req == REQUEST_CLIENT_FILE ){
		if ( req.uid == fi.st_uid and (fi.st_mode&0x400) != 0 ){ ck=0; }
		if ( req.gid == fi.st_gid and (fi.st_mode&0x040) != 0 ){ ck=0; }
		if ( (fi.st_mode&0x004) != 0 ){ ck=0; }
		return ck;
	}
	if ( req.req == REQUEST_CLIENT_LS ){
		if ( req.uid == fi.st_uid and (fi.st_mode&0x400) != 0 ){ ck=0; }
		if ( req.gid == fi.st_gid and (fi.st_mode&0x040) != 0 ){ ck=0; }
		if ( (fi.st_mode&0x004) != 0 ){ ck=0; }
		return ck;
	}
	if ( req.req == REQUEST_CLIENT_STAT ){
		if ( req.uid == fi.st_uid and (fi.st_mode&0x400) != 0 ){ ck=0; }
		if ( req.gid == fi.st_gid and (fi.st_mode&0x040) != 0 ){ ck=0; }
		if ( (fi.st_mode&0x004) != 0 ){ ck=0; }
		return ck;
	}
	if ( req.req == REQUEST_CLIENT_MKDIR ){
		if ( req.uid == fi.st_uid and (fi.st_mode&0x200) != 0 ){ ck=0; }
		if ( req.gid == fi.st_gid and (fi.st_mode&0x020) != 0 ){ ck=0; }
		if ( (fi.st_mode&0x002) != 0 ){ ck=0; }
		return ck;
	}
	if ( req.req == REQUEST_CLIENT_RM ){
		if ( req.uid == fi.st_uid and (fi.st_mode&0x200) != 0 ){ ck=0; }
		if ( req.gid == fi.st_gid and (fi.st_mode&0x020) != 0 ){ ck=0; }
		if ( (fi.st_mode&0x002) != 0 ){ ck=0; }
		return ck;
	}
	if ( req.req == REQUEST_CLIENT_PUT ){
		if ( req.uid == fi.st_uid and (fi.st_mode&0x200) != 0 ){ ck=0; }
		if ( req.gid == fi.st_gid and (fi.st_mode&0x020) != 0 ){ ck=0; }
		if ( (fi.st_mode&0x002) != 0 ){ ck=0; }
		return ck;
	}
	if ( req.req == REQUEST_CLIENT_RMDIR ){
		if ( req.uid == fi.st_uid and (fi.st_mode&0x200) != 0 ){ ck=0; }
		if ( req.gid == fi.st_gid and (fi.st_mode&0x020) != 0 ){ ck=0; }
		if ( (fi.st_mode&0x002) != 0 ){ ck=0; }
		return ck;
	}
	if ( req.req == REQUEST_CLIENT_CHMOD ){
		if ( req.uid == fi.st_uid ){ ck=0; }
		return ck;
	}
	if ( req.req == REQUEST_CLIENT_CHOWN ){
		if ( req.uid == G_NodeTbl[0].run_userid ){ ck=0; }
		return ck;
	}
	if ( req.req == REQUEST_CLIENT_CHGRP ){
		if ( req.uid == G_NodeTbl[0].run_userid ){ ck=0; }
		return ck;
	}
	return -1;
}

