/*****************************************************************************
*  <システム>   分散処理フレームワーク
*  <名称>	情報表示API
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

typedef struct{
char name[2048];
u_char node_id[NODE_ID_LEN];
} T_SERVER_LIST;

char *dispCacheIndex(int idx){
    static char wk[32];
	if ( idx < 0 ){
		strcpy(wk,"---");
	}else if ( idx >= 999 ){
		strcpy(wk,"999");
	}else{
		sprintf(wk,"%03d",idx);
	}
	return wk;
}

char *dispSCN(T_SCN *scn){
    static char wk[32];
	sprintf(wk,"%08d:%05d:%05d",scn->dt,scn->seq1,scn->seq2);
	return wk;
}

char *dispSCN(T_SCN *scn, char *wk){
	sprintf(wk,"%08d:%05d:%05d",scn->dt,scn->seq1,scn->seq2);
	return wk;
}

char *dispPerm2Mode(char c, char *out){
    strcpy(out,"---");
    if ( c == '7' ){ strcpy(out,"rwx");  }
    if ( c == '6' ){ strcpy(out,"rw-");  }
    if ( c == '5' ){ strcpy(out,"r-x");  }
    if ( c == '4' ){ strcpy(out,"r--");  }
    if ( c == '3' ){ strcpy(out,"-wx");  }
    if ( c == '2' ){ strcpy(out,"-w-");  }
    if ( c == '1' ){ strcpy(out,"--x");  }
    return out;
}

char *dispPerm2Name(u_long perm){
    static char wk[16];
    char wk0[4];
    char wk1[4];
    char wk2[4];

    sprintf(wk,"%X",perm);
    dispPerm2Mode(wk[0],wk0);
    dispPerm2Mode(wk[1],wk1);
    dispPerm2Mode(wk[2],wk2);
    sprintf(wk,"%s%s%s",wk0,wk1,wk2);
    return wk;
}

char *dispDupCount(u_long type, u_int cnt){
    static char wk[4];
    if ( type == FILE_TYPE_DIR ){ return "-"; }
	if ( cnt > 9 ){ cnt='X'; }
	sprintf(wk,"%d",cnt);
	return wk;
}

char *dispType2Name(u_long file_type){
    if ( file_type == FILE_TYPE_FILE ){ return "-"; }
    if ( file_type == FILE_TYPE_DIR ){ return "d"; }
    return "u";
}

char *dispType2NameDetail(u_long file_type){
    if ( file_type == FILE_TYPE_FILE ){ return "regular file"; }
    if ( file_type == FILE_TYPE_DIR ){ return "directory"; }
    return "u";
}

char dispMasterFlg(int d){
	if ( d == MASTER_FLG_MASTER ){ return 'M'; }
	if ( d == MASTER_FLG_SLAVE ){ return 'S'; }
	return ' ';
}

char *dispFileID(T_FILE_ID *p){
	static char buff[32];
	sprintf(buff,"%08X%06X.%08X%06X",
		p->id.tv_sec,
		p->id.tv_usec,
		p->ver.tv_sec,
		p->ver.tv_usec
		);
	return buff;
}

char *dispFileID(T_FILE_ID *p, char *buff){
	sprintf(buff,"%08X%06X.%08X%06X",
		p->id.tv_sec,
		p->id.tv_usec,
		p->ver.tv_sec,
		p->ver.tv_usec
		);
	return buff;
}

int dispMemInfo(int fd, T_INDEX_TABLE *tbl){
	int i;
	int free_blocks=0;

	//空きブロック
	for ( i=0; i<G_ConfTbl->cache_blocks; i++){
		if ( tbl[i].sts == CACHE_DATA_STS_NONE ){ free_blocks++; }
	}

	transSendClient(fd,"[Shared Memory]\n");
	transSendClient(fd,"memory header Area       = %dbyte\n",G_ShmHeader->size_header);
	transSendClient(fd,"system configration Area = %dbyte\n",G_ShmHeader->size_conf);
	transSendClient(fd,"cluster information Area = %dbyte\n",G_ShmHeader->size_node);
	transSendClient(fd,"process information Area = %dbyte\n",G_ShmHeader->size_proc);
	transSendClient(fd,"Disk Management Area     = %dbyte\n",G_ShmHeader->size_disk);
	transSendClient(fd,"Meta Cache Area          = %dbyte\n",G_ShmHeader->size_meta);
	transSendClient(fd,"Index Cache Area         = %dbyte\n",G_ShmHeader->size_index);
	transSendClient(fd,"Data Cache Area          = %dbyte\n",G_ShmHeader->size_data);
	transSendClient(fd,"TOTAL                    = %dbyte\n",G_ShmHeader->size_total);

	transSendClient(fd,"\n[Data Cache information]\n");
	transSendClient(fd,"Memory Block Size   = %dbyte\n",BLOCK_SIZE);
	transSendClient(fd,"Memory Total Blocks = %d\n",G_ConfTbl->cache_blocks);
	transSendClient(fd,"Memory Free Blocks  = %d\n\n",free_blocks);

	return 0;
}

int dispCacheInfo(int fd, T_META_TABLE *tbl, T_INDEX_TABLE *idx){
	int i,j;
	char w[256];

	//METAキャッシュ
	transSendClient(fd,"[META CACHE]\n");
	transSendClient(fd,"No   sts         pid      dcnt ncnt write time          FileID\n");
	transSendClient(fd,"---- ----------- -------- ---- ---- ------------------- -----------------------------\n");
	for ( i=0; i<G_ConfTbl->cache_files; i++){
		if ( tbl[i].sts == 0 ){ continue; }
		sprintf(w,"%02d:%02d:%02d:%02d",tbl[i].sts, tbl[i].disk_flg, tbl[i].node_flg, tbl[i].read_flg);
		transSendClient(fd,"%04d %-11s %8d %4d %4d %s %s\n",
			i, 
			w,
			tbl[i].pid, 
			tbl[i].disk_wcnt,
			tbl[i].node_wcnt,
			tmVal2YYYYMMDDhhmmss(tbl[i].w_tm.tv_sec),
			dispFileID(&(tbl[i].inf.file_id))
			);
	}
	transSendClient(fd,"\n");

	//DATAキャッシュ
	transSendClient(fd,"[DATA CACHE]\n");
	transSendClient(fd,"No   sts         pid      size     write time          BlockID\n");
	transSendClient(fd,"---- ----------- -------- -------- ------------------- --------------------------------\n");
	for ( i=0; i<G_ConfTbl->cache_blocks; i++){

		j=idx[i].file_idx;
		sprintf(w,"%02d:%02d:%02d:%02d",idx[i].sts, idx[i].disk_flg, idx[i].node_flg, idx[i].read_flg);

		if ( idx[i].sts == CACHE_META_STS_NONE ){
			transSendClient(fd,"%04d %-11s %8d %8d %s\n",
			i, 
			w,
			idx[i].pid, 
			idx[i].size, 
			tmVal2YYYYMMDDhhmmss(idx[i].w_tm.tv_sec)
			);
		}else{
			transSendClient(fd,"%04d %-8s %8d %8d %s %s:%d\n",
			i, 
			w,
			idx[i].pid, 
			idx[i].size, 
			tmVal2YYYYMMDDhhmmss(idx[i].w_tm.tv_sec),
			dispFileID(&(G_MetaTbl[j].inf.file_id)),
			idx[i].block_no
			);
		}
	}

	return 0;
}

int dispVolumeInfo(int fd, T_NODE_TABLE *ntbl, T_DISK_TABLE *dtbl){
	int i;
	double tm_sa,tm_ticks,disk_busy;

	transSendClient(fd,"DISK disks=%d devices=%d total=%dMB use=%dMB(%.0f%c) free=%dMB\n\n",
			ntbl[0].disks,
			ntbl[0].devices,
			ntbl[0].disk_size,
			ntbl[0].disk_use,
			(float)ntbl[0].disk_use / ntbl[0].disk_size * 100, '%',
			ntbl[0].disk_free
	);

	transSendClient(fd,"FilePath                  DeviceName           STS SCN                  Size(MB)  Used(MB)  Free(MB)  Use%c Busy%c\n",'%','%');
	transSendClient(fd,"------------------------- -------------------- --- -------------------- --------- --------- --------- ---- ------\n");
	for(i=0; i<ntbl[0].disks; i++) {

		tm_sa = (dtbl[i].stat_a.tm.tv_sec - dtbl[i].stat_s.tm.tv_sec) +
					(double)(dtbl[i].stat_a.tm.tv_usec - dtbl[i].stat_s.tm.tv_usec) / 1000 / 1000;
		tm_ticks = (dtbl[i].stat_s.rd_ticks + dtbl[i].stat_s.wr_ticks) / 1000;
		disk_busy = tm_ticks / tm_sa * 100;
		if ( disk_busy > 100 ){ disk_busy=100; }

		transSendClient(fd,"%-25s %-20s %03d %-14s %9.0f %9.0f %9.0f %3.0f%c %5.1f%c\n",
			dtbl[i].dir, 
			dtbl[i].dev, 
			dtbl[i].sts, 
			dispSCN( &(dtbl[i].vol_scn) ),
			(float)dtbl[i].f_bsize * dtbl[i].f_blocks / 1024 / 1024,
			(float)dtbl[i].f_bsize * dtbl[i].f_use / 1024 / 1024,
			(float)dtbl[i].f_bsize * dtbl[i].f_free / 1024 / 1024,
			(float)dtbl[i].f_use / dtbl[i].f_blocks * 100,
			'%',
			disk_busy,
			'%'
		);
	}
	return 0;
}

int dispNodeInfo(int fd, T_NODE_TABLE *tbl){
	int i;
	int ka_keika;
	time_t tm;
	char ipbuf[128];
	char buff1[128];
	char buff2[128];
	char buff3[128];

	time(&tm);

	//クラスタ情報
	transSendClient(fd,"[cluster information]\n");
	transSendClient(fd,"No  F nodename           ipaddress       sts start_tm   SCN                  KA   Task    Meta    Data\n");
	transSendClient(fd,"--- - ------------------ --------------- --- ---------- -------------------- ---- ------- ------- -------\n");
	for ( i=0; i<G_ConfTbl->max_node; i++){
		ka_keika= tm - tbl[i].alv;
		if ( ka_keika > 9999){ ka_keika=9999; }

		if ( tbl[i].run_hostname[0] == (char)NULL ){ continue; }

		sprintf(buff1,"%d/%d", tbl[i].task_busy, tbl[i].task_process);
		sprintf(buff2,"%d/%d", tbl[i].cache_meta_use, tbl[i].cache_meta_blocks);
		sprintf(buff3,"%d/%d", tbl[i].cache_data_use, tbl[i].cache_data_blocks);

		transSendClient(fd,"%03d %c %-18.18s %-15s %03d %-10s %-14s %-4d %-7s %-7s %-7s\n",
			i,
			dispMasterFlg(tbl[i].master_flg),
			tbl[i].run_hostname,
			utlDisplyIP((u_char*)&tbl[i].svr_ip,ipbuf),
			tbl[i].sts,
			tmVal2YYYYMMDD(tbl[i].stime),
			dispSCN( &(tbl[i].node_scn) ),
			ka_keika,
			buff1,
			buff2,
			buff3
			);
	}
	transSendClient(fd,"\n");

	return 0;
}

int dispNodeInfoDetail(int fd, T_NODE_TABLE *tbl){
	int i;
	int ka_keika;
	time_t tm;
	char ipbuf[128];

	time(&tm);

	//クラスタ情報
	transSendClient(fd,"[cluster information]\n");
	for ( i=0; i<G_ConfTbl->max_node; i++){

		if ( tbl[i].run_hostname[0] == (char)NULL ){ continue; }
		ka_keika= tm - tbl[i].alv;
		if ( ka_keika > 9999){ ka_keika=9999; }

		transSendClient(fd,"[%d] %s(%s)\n",i, utlSHA1_to_HEX(tbl[i].node_id), tbl[i].run_hostname);

		transSendClient(fd,"userid       : %d\n",tbl[i].run_userid);
		transSendClient(fd,"groupid      : %d\n",tbl[i].run_groupid);
		transSendClient(fd,"master       : %d\n",dispMasterFlg(tbl[i].master_flg));
		transSendClient(fd,"server ip    : %s\n",utlDisplyIP((u_char*)&tbl[i].svr_ip,ipbuf));
		transSendClient(fd,"client ip    : %s\n",utlDisplyIP((u_char*)&tbl[i].clt_ip,ipbuf));
		transSendClient(fd,"status       : %d\n",tbl[i].sts);
		transSendClient(fd,"start time   : %s\n",tmVal2YYYYMMDDhhmmss(tbl[i].stime));
		transSendClient(fd,"SCN          : %s\n",dispSCN( &(tbl[i].node_scn)) );
		transSendClient(fd,"task process : %d\n",tbl[i].task_process);
		transSendClient(fd,"cpu cnt      : %d\n",tbl[i].cpu_cnt);
		transSendClient(fd,"mem size     : %d\n",tbl[i].mem_size);
		transSendClient(fd,"sga size     : %d\n",tbl[i].sga_size);
		transSendClient(fd,"disk#        : %d\n",tbl[i].disks);

		transSendClient(fd,"\n");
	}

	return 0;
}

int dispProcessInfo(int fd, T_PROCESS_TABLE *tbl){
	int i;
	time_t tm;

	time(&tm);

	//常駐プロセス
	transSendClient(fd,"[process information]\n");
	transSendClient(fd,"No  pid   sts start time          REQ IRD fork KA   process name\n");
	transSendClient(fd,"--- ----- --- ------------------- --- --- ---- ---- -------------------------\n");
	for ( i=0; i<G_ConfTbl->max_process; i++){
		if ( tbl[i].name[0] == (char)NULL ){ continue; }
		transSendClient(fd,"%03d %-5d %03d %-19s %03d %s %4d %4d %s %s\n",
			i,
			tbl[i].pid,
			tbl[i].sts,
			tmVal2YYYYMMDDhhmmss(tbl[i].stime),
			tbl[i].req,
			dispCacheIndex(tbl[i].cache_idx),
			tbl[i].fork_try,
			tm - tbl[i].alv,
			tbl[i].name,
			tbl[i].para
			);
	}

	return 0;
}

int dispPerformance(int fd, T_NODE_TABLE *ntbl, T_DISK_TABLE *dtbl){
	int i;
	double tm_sa,tm_ticks,disk_busy;

	transSendClient(fd,"[performance information]\n\n");
	transSendClient(fd,"[CPU]\n");
	transSendClient(fd,"cpus=%d user=%.1f%c nice=%.1f%c sys=%.1f%c idle=%.1f%c wait=%.1f%c\n\n",
			ntbl[0].cpu_cnt,
			ntbl[0].cpu_user,'%',
			ntbl[0].cpu_nice,'%',
			ntbl[0].cpu_system,'%',
			ntbl[0].cpu_idle,'%',
			ntbl[0].cpu_iowait, '%'
	);

	transSendClient(fd,"[Memory]\n");
	transSendClient(fd,"total=%.0fMB free=%.0fMB buff=%.0fMB cache=%.0fMB\n",	
			KBYTE2MB(ntbl[0].mem_size),
			KBYTE2MB(ntbl[0].mem_free),
			KBYTE2MB(ntbl[0].mem_buffer),
			KBYTE2MB(ntbl[0].mem_cache)
	);

	transSendClient(fd,"\n[Disk]\n");
	dispVolumeInfo(fd, ntbl, dtbl);

	return 0;
}

int dispFileListSort(const void *p1 , const void *p2 ){
	int ret;
	T_META_INFO *a=(T_META_INFO*)p1;
	T_META_INFO *b=(T_META_INFO*)p2;
	ret=strcmp(a->name, b->name);
	return ret;
}

int dispFileList(int fd, T_CLIENT_REQUEST req){
	int ret,ifd,ck;
	T_META_HEADER hd;
	T_META_INFO fid;
	T_META_INFO buff_arr[G_ConfTbl->max_file_per_dir + 1];
	u_long i,j;
	double t_size=0;
	u_long t_cnt=0;
	int m_cnt=0;
	char uname[64];
	char gname[64];
	
	//ディレクトリ情報検索
	if ( req.clt.para1[0] == (char)NULL ){ strcpy(req.clt.para1,"/"); }
	ret=diskrdSearchMeta(req.clt.para1, &hd, &fid);
	//logPrint(DEBUG_INFO,"diskrdSearchMeta(%s,*,*)=%d",req.clt.para1,ret);
	if ( ret <= 0 ){
		transSendClient(fd,"no such file or directory (%s)\n",req.clt.para1);
		return 0; 
	}

	//パーミッションチェック
	ret=daemon_CheckPerm(req, fid);
	//logPrint(DEBUG_INFO,"daemon_CheckPerm()=%d",ret);
	if ( ret < 0 ){
		transSendClient(fd,"permission error (%s)\n",req.clt.para1);
		return -1;
	}

	//META情報読込み
	m_cnt=diskrdGetAllMetas(fid.file_id, &hd, buff_arr);
	if ( m_cnt < 0 ){
		transSendClient(fd,"file open error (%s)\n",req.clt.para1);
		return -2;
	}

	//ソート
	qsort( (void*)buff_arr , m_cnt , sizeof( T_META_INFO ) , dispFileListSort );

	//合計サイズ
	for(i=0; i<m_cnt; i++){
		if ( buff_arr[i].st_dtm.tv_sec != 0 ){ continue; }
		t_size = t_size + (buff_arr[i].st_size / 1024 /1024);
		t_cnt++;
	}

	//表示
	transSendClient(fd,"Total cnt=%ld size=%.1fMByte\n",t_cnt, t_size);
	for(i=0; i<m_cnt; i++){
		if ( buff_arr[i].st_dtm.tv_sec != 0 ){ continue; }
		strcpy(uname, utlUid2Name(buff_arr[i].st_uid) );
		strcpy(gname, utlGid2Name(buff_arr[i].st_gid) );
		transSendClient(fd,"%s%s %s %-7s %-7s %10d %s %s\n",
			dispType2Name(buff_arr[i].st_type),
			dispPerm2Name(buff_arr[i].st_mode),
			dispDupCount(buff_arr[i].st_type,buff_arr[i].dup_cnt),
			uname,
			gname,
			buff_arr[i].st_size,
			tmVal2YYYYMMDDhhmm(buff_arr[i].st_mtm.tv_sec),
			buff_arr[i].name
		);
	}

	return 0;
}

int dispFileStat(int fd, T_CLIENT_REQUEST req){
	int ret,ifd,ck,i;
	T_META_HEADER hd;
	T_META_INFO fid;
	T_META_INFO mdata;
	T_IDX_HEADER ihd;
	T_IDX_INFO idata;
	char filename[MAX_FILE_PATH+1];
	char filename_meta[MAX_FILE_PATH+1];
	char filename_idx[MAX_FILE_PATH+1];
	char uname[64];
	char gname[64];
	
	//ディレクトリ情報検索
	if ( req.clt.para1[0] == (char)NULL ){ strcpy(req.clt.para1,"/"); }
	ret=diskrdSearchMeta(req.clt.para1, &hd, &fid);
	//logPrint(DEBUG_INFO,"diskrdSearchMeta(%s,*,*)=%d",req.clt.para1,ret);
	if ( ret <= 0 ){
		transSendClient(fd,"no such file or directory (%s)\n",req.clt.para1);
		return 0; 
	}

	//パーミッションチェック
	ret=daemon_CheckPerm(req, fid);
	//logPrint(DEBUG_INFO,"daemon_CheckPerm()=%d",ret);
	if ( ret < 0 ){
		transSendClient(fd,"permission error (%s)\n",req.clt.para1);
		return -1;
	}

	//ファイル読込み
	disknmGetMetaFileName("", hd.row.dir_id, filename_meta);
	//logPrint(DEBUG_INFO,"open(%s,%d)",filename_meta,O_RDONLY);
	if ( (ifd=open(filename_meta,O_RDONLY)) < 0 ){
		transSendClient(fd,"file open error (%s)\n",req.clt.para1);
		return -2; 
	}
	ret=read(ifd, (char*)&hd,sizeof(T_META_HEADER));
	if ( ret != sizeof(T_META_HEADER) ){
		transSendClient(fd,"file read error (%s)\n",req.clt.para1);
		close(ifd);
		return -3; 
	}

	//詳細情報表示対象ファイル
	strcpy(filename,filGetFileName(req.clt.para1));
	if ( filename[0] == NULL ){ strcpy(filename,"."); }
	//logPrint(DEBUG_INFO,"search=[%s]",filename);

	//読込み
	memset((char*)&mdata, 0, sizeof(T_META_INFO) );
	while ( 1 ){
		ret=read(ifd, (char*)&fid, sizeof(T_META_INFO) );
		if ( ret < sizeof(T_META_INFO) ){ break; }

		if ( strcmp(filename,fid.name) != 0 ){ continue; }

		//重複チェック
		if ( mdata.file_id.id.tv_sec != 0 ){
			ck=tmCompareMicroSec(fid.file_id.id, mdata.file_id.id);
			if ( ck != 0 ){ continue; }
			ck=tmCompareMicroSec(fid.file_id.ver, mdata.file_id.ver);
			if ( ck >= 0 ){ continue; }
		}

		memcpy((char*)&mdata, (char*)&fid, sizeof(T_META_INFO) );
	}
	close(ifd);

	if ( mdata.file_id.id.tv_sec == 0 ){
		transSendClient(fd,"not found file (%s)\n",req.clt.para1);
		return -6; 
	}

	//表示
	strcpy(uname, utlUid2Name(mdata.st_uid) );
	strcpy(gname, utlGid2Name(mdata.st_gid) );
	transSendClient(fd,"[%s]\n",filename_meta);
	transSendClient(fd,"     name:%s\n",filename);
	transSendClient(fd,"up_dir_id:%s\n",dispFileID(&(hd.row.up_dir_id)) );
	transSendClient(fd,"   dir_id:%s\n",dispFileID(&(hd.row.dir_id)) );
	transSendClient(fd,"  file_id:%s\n",dispFileID(&(fid.file_id)) );
	transSendClient(fd,"     Size:%ld    Blocks:%ld    IO Block:%ld    %s\n",
			mdata.st_size, mdata.st_blocks, mdata.st_blksize, dispType2NameDetail(mdata.st_type));
	transSendClient(fd,"   Access:(%04X/%s)  Uid:(%d/%s) Gid:(%d/%s)\n",
			mdata.st_mode, dispPerm2Name(mdata.st_mode),
			mdata.st_uid, uname,
			mdata.st_gid, gname);
	transSendClient(fd,"   Access:%s\n", tmVal2YYYYMMDDhhmmssss(mdata.st_atm));
	transSendClient(fd,"   Modify:%s\n", tmVal2YYYYMMDDhhmmssss(mdata.st_mtm));
	transSendClient(fd,"   Change:%s\n", tmVal2YYYYMMDDhhmmssss(mdata.st_ctm));
	transSendClient(fd,"\n");

	//ブロック情報
	disknmGetIdxFileName("", fid.file_id, filename_idx);
	//logPrint(DEBUG_INFO,"open(%s,%d)",filename_idx,O_RDONLY);
	if ( (ifd=open(filename_idx,O_RDONLY)) < 0 ){
		return -11; 
	}
	ret=read(ifd, (char*)&ihd,sizeof(T_IDX_HEADER));
	if ( ret != sizeof(T_IDX_HEADER) ){
		close(ifd);
		return -3; 
	}

	transSendClient(fd,"[%s]\n",filename_idx);
	for(i=1; ; i++){
		ret=read(ifd, (char*)&idata, sizeof(T_IDX_INFO) );
		if ( ret < sizeof(T_IDX_INFO) ){ break; }
		transSendClient(fd,"[%03d] node_id:%-40s block_no:%d wdt:%s size:%d\n",
			i, utlSHA1_to_HEX(idata.node_id), idata.block_no, tmVal2YYYYMMDDhhmmssss(idata.w_tm), idata.size);
	}
	close(ifd);

	return 0;

}

int dispHeaderInfo(T_META_HEADER hd){
	printf("[directory header]\n");
	printf("up_dir_id  : %s\n",dispFileID( &(hd.row.up_dir_id) ) );
	printf("dir_id     : %s\n",dispFileID( &(hd.row.dir_id) ) );
	//printf("total size : %d\n",hd.size);
	//printf("total cnt  : %d\n",hd.cnt);
	return 0;
}

int dispMetaInfo(T_META_INFO in){
	printf("file_id    : %s\n",dispFileID(&(in.file_id)) );
	printf("st_type    : %d\n",in.st_type );
	printf("st_mode    : %s\n",dispPerm2Name(in.st_mode) );
	printf("st_uid     : %d\n",in.st_uid );
	printf("st_gid     : %d\n",in.st_gid );
	printf("st_size    : %ld\n",in.st_size );
	printf("st_blksize : %ld\n",in.st_blksize );
	printf("st_blocks  : %ld\n",in.st_blocks );
	printf("st_dup_cnt : %ld\n",in.dup_cnt );
	printf("st_atm     : %s\n",tmVal2YYYYMMDDhhmmssss(in.st_atm) );
	printf("st_mtm     : %s\n",tmVal2YYYYMMDDhhmmssss(in.st_mtm) );
	printf("st_ctm     : %s\n",tmVal2YYYYMMDDhhmmssss(in.st_ctm) );
	printf("name       : %s\n",in.name );
	return 0;
}

int dispFileDump(char *filename){
	int ret;
	int fd;
	int cnt;
	char file_type[4];
	T_META_HEADER mhd;
	T_META_INFO   mtbl;
	T_IDX_HEADER  ihd;
	T_IDX_INFO    idx;
	T_JLOG_HEADER jhd;
	T_JLOG_INFO   jlog;
	T_VOLUME_HEADER vhd;
	T_NODE_HEADER nhd;
	T_NODE_TABLE ntbl;
	T_FILE_ID file_id;
	char filename2[MAX_FILE_PATH+1];
	char dirname2[MAX_FILE_PATH+1];
	FILE *fp;
	char buff[20480];
	u_char node_id[1024];
	T_SERVER_LIST server_list[100];

	//サーバリスト
	cnt=0;
	memset(server_list,0,sizeof(T_SERVER_LIST));
	sprintf(filename2,"%s/conf/server.lst",BASE_DIR);
	if ( (fp=fopen(filename2,"r")) != NULL ){
		while( fgets(buff,sizeof(buff),fp) != NULL ){
			chomp(buff);
			if ( buff[0] == NULL ){ continue; }

			strcpy(server_list[cnt].name,buff);
			utlGetSHA1(buff, server_list[cnt].node_id);
			//printf("%s %s\n",buff,utlSHA1_to_HEX(node_id));
			cnt++;
		}
		fclose(fp);
	}

	//printf("FILE:%s\n\n",filename);
	if ( (fd=open(filename,O_RDONLY)) < 0 ){
		disknmFileID2FileID(filename,&file_id);
		sprintf(dirname2,"%s/data",BASE_DIR);
		disknmGetIdxDirName(dirname2,file_id,dirname2);
		sprintf(filename2,"%s/%.8s.%.14s",dirname2, filename, filename+15);
		//printf("%s\n",filename2);
		if ( (fd=open(filename2,O_RDONLY)) < 0 ){
			return -1; 
		}
	}

	//ファイルタイプ
	ret=read(fd, (char*)file_type,sizeof(file_type));
	if ( ret != sizeof(file_type) ){
		close(fd); return -2;
	}

	ret=lseek(fd, 0, SEEK_SET);
	if ( ret < 0 ){ close(fd); return -3; }

	//タイプ別表示
	if ( memcmp(file_type,FILE_TYPE_META,sizeof(file_type)) == 0 ){
		//ヘッダ
		ret=read(fd, (char*)&mhd,sizeof(T_META_HEADER));
		if ( ret != sizeof(T_META_HEADER) ){
			close(fd); return -2;
		}
		dispHeaderInfo(mhd);
		printf("\n");

		//ファイル情報
		for(cnt=1; ; cnt++){
			ret=read(fd, (char*)&(mtbl), sizeof(T_META_INFO) );
			if ( ret < sizeof(T_META_INFO) ){ break; }
			printf("[Rec.%d]\n",cnt);
			dispMetaInfo(mtbl);
			printf("\n");
		}
	}else if ( memcmp(file_type,FILE_TYPE_INDX,sizeof(file_type)) == 0 ){

		//ヘッダ
		ret=read(fd, (char*)&ihd,sizeof(T_IDX_HEADER));
		if ( ret != sizeof(T_IDX_HEADER) ){
			close(fd); return -2;
		}

		disknmFileName2FileID(filename,&file_id);
		printf("dir_id   :%s\n", dispFileID(&ihd.dir_id) );
		printf("file_id  :%s\n", dispFileID(&file_id) );
		printf("block_num:%d\n", ihd.block_num);
		printf("\n");

		printf("No  node_id                                  host       blk# size     wtm\n");
		printf("--- ---------------------------------------- ---------- ---- -------- --------------------------\n");
		for(cnt=1; ; cnt++){
			ret=read(fd, (char*)&(idx), sizeof(T_IDX_INFO) );
			if ( ret < sizeof(T_IDX_INFO) ){ break; }

			buff[0]=NULL;
			for(int i=0; i<100 && server_list[i].name[0]!=NULL; i++){
				if ( memcmp(idx.node_id,server_list[i].node_id,NODE_ID_LEN) == 0 ){
					strcpy(buff, server_list[i].name);
					break;
				}
			}
			printf("%03d %-40s %-10s %4d %8d %s\n",
				cnt, utlSHA1_to_HEX(idx.node_id), buff, idx.block_no, idx.size, tmVal2YYYYMMDDhhmmssss(idx.w_tm));
		}
	}else if ( memcmp(file_type,FILE_TYPE_JLOG,sizeof(file_type)) == 0 ){

		//ヘッダ
		ret=read(fd, (char*)&jhd,sizeof(T_JLOG_HEADER));
		if ( ret != sizeof(T_JLOG_HEADER) ){
			close(fd); return -2;
		}

		printf("No  F scn                  dir_id:rec_no\n");
		printf("--- - -------------------- ---------------------------------\n");
		for(cnt=1; ; cnt++){
			ret=read(fd, (char*)&(jlog), sizeof(T_JLOG_INFO) );
			if ( ret < sizeof(T_JLOG_INFO) ){ break; }
			printf("%03d %d %s %s:%d\n",
				cnt, jlog.flg, dispSCN( &(jlog.scn)), dispFileID(&(jlog.dir_id)), jlog.rec_no );
		}
	}else if ( memcmp(file_type,FILE_TYPE_VOLUME,sizeof(file_type)) == 0 ){

		//ヘッダ
		ret=read(fd, (char*)&vhd,sizeof(T_VOLUME_HEADER));
		if ( ret != sizeof(T_VOLUME_HEADER) ){
			close(fd); return -2;
		}

		printf("Volume-ID : %s\n", utlSHA1_to_HEX(vhd.volume_id));
		printf("SCN       : %s\n", dispSCN(&(vhd.vol_scn)));
		printf("write dt  : %s\n", tmVal2YYYYMMDDhhmmssss(vhd.w_tm));

	}else if ( memcmp(file_type,FILE_TYPE_NODE,sizeof(file_type)) == 0 ){

		ret=read(fd, (char*)&nhd,sizeof(T_NODE_HEADER));
		if ( ret != sizeof(T_NODE_HEADER) ){
			close(fd); return -2;
		}

		cnt=0;
		while( (ret=read(fd,(char *)&ntbl,sizeof(T_NODE_TABLE))) > 0 ){
			cnt++;
			printf("[%d]\n",cnt);
			printf("maste_flg    : %c\n",dispMasterFlg(ntbl.master_flg));
			printf("hostname     : %s\n",ntbl.run_hostname);
			printf("ip address   : %s\n",utlDisplyIP((u_char*)&ntbl.svr_ip));
			printf("status       : %d\n",ntbl.sts);
			printf("scn          : %s\n",dispSCN( &(ntbl.node_scn) ));
			printf("start dt     : %s\n",tmVal2YYYYMMDDhhmmss(ntbl.stime));
			printf("keep alive   : %s\n",tmVal2YYYYMMDDhhmmss(ntbl.alv));
			printf("task process : %d\n",ntbl.task_process);
			printf("busy task    : %d\n",ntbl.task_busy);
			printf("\n");
		}

	}else{
		printf("unknown file type\n");
	}

	close(fd);
	return 0;
}

int dispMemDataDump(int idx){
	int line;
	int i;
	u_char *addr, *c;

	printf("[%d]\n",idx);
	addr=(u_char*)G_DataTbl[idx].data;
	for ( line=0; line<5; line++){
		for ( i=0; i<32; i++){
			c = addr + line * 32 + i;
			printf("%02X ",*c);
		}
		printf("\n");
	}
	printf("\n");

	addr=(u_char*)G_DataTbl[idx].data - (32*5);
	for ( line=0; line<5; line++){
		for ( i=0; i<32; i++){
			c = addr + line * 32 + i;
			printf("%02X ",*c);
		}
		printf("\n");
	}
	printf("\n");

	return 0;
}
