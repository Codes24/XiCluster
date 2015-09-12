/*****************************************************************************
*  <システム>   分散処理フレームワーク
*  <名称>	ディスクアクセス要求処理API
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

static int  l_data_cache_free_no=-1;
static int  l_data_cache_write_cnt=0;
static int  l_data_cache_write_size=0;
static int  l_file_cache_free_no=-1;

static int creqCheckMETA(int fd, T_CLIENT_REQUEST req, T_META_INFO *out_f, T_DIR_ID *out_d){
	int ret;
	u_long out_oya_id;
	char filename[MAX_FILE_PATH+1];
	char dirname[MAX_FILE_PATH+1];
	T_META_HEADER hdd;
	T_META_HEADER hdf;
	T_META_INFO fid;
	T_META_INFO fif;

	memset((char*)out_d,0,sizeof(T_DIR_ID));

	//ディレクトリ情報検索
	strcpy(dirname, filGetDirName(out_f->name) );
	strcpy(filename, filGetFileName(out_f->name) );
	ret=diskrdSearchMeta(dirname, &hdd, &fid);
	if ( ret == 0 ){
		logPrint(DEBUG_WARNING,"diskSearchMETA(%s,*,*)=%d",dirname, ret);
		transSendResultM(fd, REQUEST_CLIENT_NG,"directory not found (%s)",req.clt.para1);
		return -1;
	}
	if ( ret < 0 ){
		logPrint(DEBUG_WARNING,"diskSearchMETA(%s,*,*)=%d",dirname, ret);
		transSendResultM(fd, REQUEST_CLIENT_NG,"meta search error (%s)",req.clt.para1);
		return -2;
	}
	if ( fid.st_type == FILE_TYPE_FILE ){
		logPrint(DEBUG_WARNING,"diskSearchMETA(%s,*,*)=%d",dirname, ret);
		transSendResultM(fd, REQUEST_CLIENT_NG, "not directory (%s)",req.clt.para1);
		return -3;
	}
	logPrint(DEBUG_DEBUG,"diskSearchMETA(%s,*,*)=%d",dirname, ret);

	//ファイル検索
	ret=diskrdSearchMeta(out_f->name, &hdf, &fif);
	//logPrint(DEBUG_DEBUG,"diskrdSearchMeta(%s,*,*)=%d",out_f->name, ret);
	if ( ret == 1 ){
		if ( fif.st_type != FILE_TYPE_FILE ){
			transSendResultM(fd, REQUEST_CLIENT_NG, "not file (%s)",req.clt.para1);
			return -4;
		}
	}

	//パーミッションチェック
	if ( ret == 1 ){
		ret=daemon_CheckPerm(req, fif);
	}else{
		ret=daemon_CheckPerm(req, fid);
	}
	logPrint(DEBUG_DEBUG,"daemon_CheckPerm()=%d",ret);
	if ( ret < 0 ){
		transSendResultM(fd, REQUEST_CLIENT_NG,"permission error (%s)",req.clt.para1);
		return -5;
	}

	//情報を返す
	strcpy(out_f->name, filename);
	memcpy((char*)&(out_d->up_dir_id), (char*)&(hdd.row.up_dir_id), sizeof(T_FILE_ID));
	memcpy((char*)&(out_d->dir_id), (char*)&(fid.file_id), sizeof(T_FILE_ID));
	memcpy((char*)&(out_f->file_id), (char*)&(fif.file_id), sizeof(T_FILE_ID));

	return 1;
}

static int creqWriteDataCache(char *buff, int size){
	int i;
	int ret;
	char *addr;
	int s=0;
	int w_size=0;

	if ( size == 0 ){ return 0; }

	//メモリ書込み(BlockOver処理)
	if ( (l_data_cache_write_size + size) >= BLOCK_SIZE ){
		w_size=BLOCK_SIZE - l_data_cache_write_size;
		addr=G_DataTbl[l_data_cache_free_no].data + l_data_cache_write_size;
		memcpy(addr, buff, w_size);

//printf("BO cache[%d]=(%d,%d) w_size=%d\n",l_data_cache_free_no, l_data_cache_write_cnt, l_data_cache_write_size, w_size);

		G_IndexTbl[l_data_cache_free_no].size = BLOCK_SIZE;
		cacheSetFlagIndexStatus(l_data_cache_free_no, CACHE_DATA_STS_MEM_E);
		l_data_cache_free_no=(-1);
		l_data_cache_write_size=0;
		size -= w_size;
	}
	if ( size <= 0 ){ return 1; }

	//空きキャッシュ取得
	if ( l_data_cache_free_no == (-1) ){
		l_data_cache_free_no=cacheGetFreeDiskCache();
		l_data_cache_write_size=0;
		logPrint(DEBUG_DEBUG,"cacheGetFreeDiskCache()=%d",l_data_cache_free_no);
	
		if ( l_data_cache_free_no < 0 ){
			logPrint(DEBUG_WARNING,"cacheGetFreeDiskCache()=%d (NO_FREE_DATA_CACHE)");
			return -2;
		}
		l_data_cache_write_cnt++;
	}

	//メモリ書込み
	addr=G_DataTbl[l_data_cache_free_no].data + l_data_cache_write_size;
	memcpy(addr, buff + w_size, size);

	l_data_cache_write_size += size;
	G_IndexTbl[l_data_cache_free_no].file_idx = l_file_cache_free_no;
	G_IndexTbl[l_data_cache_free_no].block_no = l_data_cache_write_cnt;
	G_IndexTbl[l_data_cache_free_no].size = l_data_cache_write_size;

	return 1;
}

static int creqReadLocalCache(T_ZTRANS_INFO *zpara, int ofd, T_FILE_ID file_id, u_long block_no){
	int i;
	int ret;
	int oret;
	int rflg=0;
	int cache_idx=0;
	int cache_cnt=0;

	//該当ブロックがキャッシュされているかチェック
	cache_idx=cacheCheckCachedBlock(&file_id, block_no);
	logPrint(DEBUG_DEBUG,"cacheCheckCachedBlock()=%d",cache_idx);
	if ( cache_idx < 0 ){ return 0; }

	rflg=1;
	while(1){

		time(&G_ProcTbl[G_ProcNo].alv);

		oret=ztrans_data_send(zpara, G_DataTbl[cache_idx].data, G_IndexTbl[cache_idx].size, &cache_cnt, ofd);
		if ( oret == 0 ){ break; }
		if ( oret < 0 ){
			logPrint(DEBUG_ERROR,"ztrans_data_send()=%d",oret);
			rflg=-2;
			break;
		}
	}

	//logPrint(DEBUG_INFO,"#total=%d in=%d,%d out=%d,%d",zpara->file_size, 
	//		zpara->ibuff_p->seq, zpara->ibuff_p->total, zpara->obuff_p->seq, zpara->obuff_p->total);
	cacheCheckCachedRelease(cache_idx);

	return rflg;
}

/* ローカルディスクよりデータを取得しクライアントに返す */
static int creqReadLocalDisk(T_ZTRANS_INFO *zpara,int ofd, T_FILE_ID file_id, u_long block_no){
	int i;
	int ret;
	int oret;
	int file_type;
	int ifd;
	gzFile zp;
	char filename[MAX_FILE_PATH+1];
	T_CLIENT_RESULT_S res;
	int rflg=0;

	//Volume選択
	for(i=0; i<G_NodeTbl[0].disks; i++){
		if ( G_DiskTbl[i].sts != VOLUME_STATUS_SERVICE){ continue; }
		ret=disknmSearchDataFileName(G_DiskTbl[i].dir, file_id, block_no, filename);
		if ( ret != 0 ){ break; }
	}
	if ( filename[0] == NULL ){
		logPrint(DEBUG_WARNING,"file not found (local disk)");
		return -1;
	}

	//ファイル読込み
	file_type=daemon_DataFileType(filename);
	logPrint(DEBUG_DEBUG,"RD:[%s]",filename);
	if ( file_type == 0 ){
		if ( (ifd=open(filename,O_RDONLY)) < 0 ){
			logPrint(DEBUG_WARNING,"file open error (%s)",filename);
			return -1;
		}
	}else if ( file_type == 1 ){
		if ( (zp = gzopen(filename, "rb")) == NULL ){
			logPrint(DEBUG_WARNING,"file open error (%s)",filename);
			return -1;
		}
	}else{
		logPrint(DEBUG_ERROR,"file type error (%s)",filename);
		return -1;
	}

	//読込みループ
	while ( 1 ){

		time(&G_ProcTbl[G_ProcNo].alv);

		if ( file_type == 0 ){
			oret=ztrans_data_send(zpara, ifd, ofd);
		}
		if ( file_type == 1 ){
			oret=ztrans_data_send(zpara, zp, ofd);
		}

		if ( oret == 0 ){ break; }
		if ( oret < 0 ){
			logPrint(DEBUG_ERROR,"ztrans_data_send()=%d",oret);
			rflg=-2;
			break;
		}
	}

	//logPrint(DEBUG_INFO,"#total=%d in=%d,%d out=%d,%d",zpara->file_size, 
	//		zpara->ibuff_p->seq, zpara->ibuff_p->total, zpara->obuff_p->seq, zpara->obuff_p->total);

	//クローズ処理
	if ( file_type == 0 ){ close(ifd); }
	if ( file_type == 1 ){ gzclose(zp); }

	return rflg;
}

static int creqReadLocalNode(T_ZTRANS_INFO *zpara, int fd, T_FILE_ID file_id, u_long block_no){
	int ret;

	//キャッシュから取得
	ret=creqReadLocalCache(zpara, fd, file_id, block_no);
	if ( ret == 1 ){
		logPrint(DEBUG_DEBUG,"creqReadLocalCache(%d,%s,%d)=%d",fd,dispFileID(&file_id),block_no,ret);
		return 0; 
	}

	//ローカルディスクから取得
	ret=creqReadLocalDisk(zpara, fd, file_id, block_no);
	logPrint(DEBUG_DEBUG,"creqReadLocalDisk(%d,%s,%d)=%d",fd,dispFileID(&file_id),block_no,ret);
	return ret;
}

static int creqReadRemoteDisk(T_ZTRANS_INFO *zpara, int ofd, T_FILE_ID file_id, u_long block_no, u_char *node_id){
	int i;
	int ret;
	int iret;
	int oret;
	int fret=0;
	int out_size;
	int ifd;
	int pfd[2];
	long addr=0;
	T_CLIENT_REQUEST req;
	T_CLIENT_RESULT_S res;
	T_ZTRANS_INFO zpara_in;

	//接続先選択
	for(i=1; i<=G_ConfTbl->max_node; i++){
		if ( G_NodeTbl[i].sts != SYSTEM_STATUS_RUNNING ){ continue; }
		if ( memcmp(node_id, G_NodeTbl[i].node_id, NODE_ID_LEN) == 0){
			addr=G_NodeTbl[i].svr_ip;
			break;
		}
	}
	if ( addr == 0 ){
		logPrint(DEBUG_WARNING,"no data block");
		return -1;
	}
	logPrint(DEBUG_DEBUG,"connect %s:%d",utlDisplyIP((u_char*)&addr),G_ConfTbl->network_port_cache);


	//接続
	ifd=transConnect(addr,G_ConfTbl->network_port_cache);
	if ( ifd < 0 ){
		logPrint(DEBUG_WARNING,"transConnect(%s,%d)=%d",utlDisplyIP((u_char*)&addr),G_ConfTbl->network_port_cache,ifd);
		return -2;
	}
	logPrint(DEBUG_DEBUG,"transConnect(%s,%d)=%d",utlDisplyIP((u_char*)&addr),G_ConfTbl->network_port_cache,ifd);

	//要求
	commReqCheckSumSet(&req.ck);
	req.req=REQUEST_TASK_DATA;
	req.uid=getuid();
	req.gid=getgid();
	req.compression=0;
	memcpy(&(req.data.file_id), &file_id, sizeof(T_FILE_ID));
	req.data.block_no=block_no;
	ret=transSendBuff(ifd, (char*)&req, sizeof(req), &res);
	if ( ret <= 0 ){
		logPrint(DEBUG_WARNING,"transSendBuff()=%d cnt=%d size=%d",ret,res.r_cnt, res.r_size);
		ret=tcpClose(ifd);
		logPrint(DEBUG_DEBUG,"tcpClose(%d)=%d",ifd, ret);
		return -3;
	}
	//logPrint(DEBUG_DEBUG,"transSendBuff()=%d cnt=%d size=%d",ret,res.r_cnt, res.r_size);

	//パイプ
	ret=pipeCreate(pfd);
	//ret=mmapCreate(G_ConfTbl->mmap_blocks);
	if ( ret < 0 ){
		logPrint(DEBUG_WARNING,"pipeCreate()=%d",ret);
		ret=tcpClose(ifd);
		logPrint(DEBUG_DEBUG,"tcpClose(%d)=%d",ifd, ret);
		return -4;
	}

	//解凍
	memset(&zpara_in,0,sizeof(T_ZTRANS_INFO));
	zpara_in.file_size=res.r_size;
	zpara_in.buff_size=G_ConfTbl->network_buff_size;
	zpara_in.stmout=G_ConfTbl->send_timeout;
	zpara_in.rtmout=G_ConfTbl->recv_timeout;
	ztrans_decomp_init(&zpara_in);


	//リモートノードよりブロックを受信してクライアントに転送
	iret=1;
	oret=1;
	while( 1 ){

		time(&G_ProcTbl[G_ProcNo].alv);

		//リモートデータ受信
		out_size=0;
		if ( iret > 0 ){
			iret=ztrans_data_recv(&zpara_in, ifd, &out_size);
			if ( iret < 0 ){
				logPrint(DEBUG_WARNING,"ztrans_data_recv(*,%d,%d)=%d", ifd,out_size,iret);
				fret=-12;
				break;
			}
			if ( out_size > 0){
				ret=pipeSend(pfd[1], zpara_in.obuff + sizeof(T_CLIENT_DATA_HEADER), out_size, G_ConfTbl->pipe_send_timeout);
				//ret=mmapSend(zpara_in.obuff + sizeof(T_CLIENT_DATA_HEADER), out_size);
				if ( ret != out_size ){
					fret=-13;
					break;
				}
			}
		}

		//クライアントに結果送信
		oret=ztrans_data_send(zpara, pfd[0], ofd, G_ConfTbl->mmap_recv_timeout);
		//logPrint(DEBUG_INFO,"ztrans_data_send()=%d",oret);
		if ( oret < 0 ){
   			logPrint(DEBUG_WARNING,"ztrans_data_send(%d,%d)=%d", pfd[0], ofd, oret);
			fret=-14;
			break;
		}

		//logPrint(DEBUG_INFO,"size=%d in=(%d,%d,%d,%d) out=(%d,%d,%d,%d) %d,%d",
		//	res.r_size,
		//	zpara_in.ibuff_p->seq, zpara_in.ibuff_p->total,
		//	zpara_in.obuff_p->seq, zpara_in.obuff_p->total,
		//	zpara->ibuff_p->seq, zpara->ibuff_p->total,
		//	zpara->obuff_p->seq, zpara->obuff_p->total,
		//	iret,oret);

		if ( iret <= 0 and oret<=0 ){ break; }
	}

	//logPrint(DEBUG_INFO,"#pipe in=%d,%d out=%d,%d",
	//		zpara_in.ibuff_p->seq, zpara_in.ibuff_p->total, zpara_in.obuff_p->seq, zpara_in.obuff_p->total);
	//logPrint(DEBUG_INFO,"#total=%d in=%d,%d out=%d,%d",zpara->file_size, 
	//		zpara->ibuff_p->seq, zpara->ibuff_p->total, zpara->obuff_p->seq, zpara->obuff_p->total);
	ztrans_decomp_fin(&zpara_in);
	pipeClose(pfd);
	//mmapClose();

	ret=tcpClose(ifd);
	logPrint(DEBUG_DEBUG,"tcpClose(%d)=%d",ifd, ret);
	return fret;
}

int creqGetTabFilePtn(T_CLIENT_REQUEST req, char *outbuff){
	int ret;
	T_META_HEADER hd;
	T_META_INFO fi;
	T_META_INFO fif;
	char out_path[MAX_FILE_PATH+1];

	outbuff[0]=NULL;
	//printf("[%s][%s][%d]\n",req.clt.para1, req.clt.para2, req.clt.size);

	//ディレクトリチェック
	if ( req.clt.para1[0] != '/' ){ return -1; }

	//ディレクトリ検索
	ret=diskrdSearchDir(req.clt.para1, &hd, &fi, out_path);
	if ( ret != 0 ){ return -2; }

	//ファイル情報
	ret=diskrdSearchMeta(req.clt.para1, &hd, &fif);
	if ( ret < 0 ){ return -3; }

	//printf("hd=%s file_id=%s [%s]\n",
	//	dispFileID(&(hd.row.dir_id)), dispFileID(&(fif.file_id)), out_path );

	//ファイルパターン一致検索
	ret=diskrdSerchMetaFilePtn(hd.row.dir_id, out_path);
	if ( ret <= 0 ){ return -4; }

	strcpy(outbuff,out_path);

	return 0;
}

int creqRecvCacheMetaCre(int fd, T_CLIENT_REQUEST req){
	int ret;
	char buff1[128];
	char buff2[128];
	char buff3[128];

	//logPrint(DEBUG_DEBUG,"creqRecvCacheMetaCre(%s %s)",
	//	dispFileID( &(req.meta.dir_id), buff2 ), dispFileID( &(req.meta.inf.file_id), buff3 ));

	//ディスク出力
	ret=diskwtWriteMetaCreate(req.meta.dir_id, req.meta.inf);
	if ( ret < 0 ){
		logPrint(DEBUG_WARNING,"diskwtWriteMetaCreate()=%d",ret);
		ret=transSendResultS(fd, REQUEST_CLIENT_NG);
		logPrint(DEBUG_DEBUG,"transSendResultS(%d,%d)=%d",fd, REQUEST_CLIENT_NG, ret);
		return -1;
	}
	logPrint(DEBUG_DEBUG,"diskwtWriteMetaCreate(%s,*)=%d",dispFileID(&(req.meta.dir_id),buff2), ret);

	//正常応答
	ret=transSendResultS(fd, REQUEST_CLIENT_OK);
	if ( ret < 0 ){
		logPrint(DEBUG_WARNING,"transSendResultS(%d,%d)=%d",fd, REQUEST_CLIENT_OK, ret);
		return -2;
	}
	logPrint(DEBUG_DEBUG,"transSendResultS(%d,%d)=%d",fd, REQUEST_CLIENT_OK, ret);

	return 0;
}

int creqRecvCacheMetaAdd(int fd, T_CLIENT_REQUEST req){
	int ret;
	char buff1[128];
	char buff2[128];
	char buff3[128];

	logPrint(DEBUG_DEBUG,"creqRecvCacheMetaAdd(%s %s)",
		dispFileID( &(req.meta.dir_id), buff2 ), dispFileID( &(req.meta.inf.file_id), buff3 ));

	//ディスク出力
	ret=diskwtWriteMetaRecord(req.meta.dir_id, req.meta.inf, 1);
	if ( ret < 0 ){
		logPrint(DEBUG_WARNING,"diskwtWriteMetaRecord(%s,*,1)=%d",dispFileID( &(req.meta.dir_id), buff2 ), ret);
		ret=transSendResultS(fd, REQUEST_CLIENT_NG);
		logPrint(DEBUG_DEBUG,"transSendResultS(%d,%d)=%d",fd, REQUEST_CLIENT_NG, ret);
		return -1;
	}
	logPrint(DEBUG_DEBUG,"diskwtWriteMetaRecord(%s,*,1)=%d",dispFileID( &(req.meta.dir_id), buff2 ), ret);

	//正常応答
	ret=transSendResultS(fd, REQUEST_CLIENT_OK);
	if ( ret < 0 ){
		logPrint(DEBUG_WARNING,"transSendResultS(%d,%d)=%d",fd, REQUEST_CLIENT_OK, ret);
		return -2;
	}
	logPrint(DEBUG_DEBUG,"transSendResultS(%d,%d)=%d",fd, REQUEST_CLIENT_OK, ret);

	return 0;
}

int creqRecvCacheIndexAdd(int fd, T_CLIENT_REQUEST req){
	int ret;

	//INDEXレコード追加
	ret=diskwtWriteIndexCache(req.block.dir_id, req.block.file_id, req.block.block_no, req.block.size, req.block.node_id);
	if ( ret < 0 ){
		logPrint(DEBUG_WARNING,"diskwtWriteIndexCache()=%d",ret);
		ret=transSendResultS(fd, REQUEST_CLIENT_NG);
		logPrint(DEBUG_DEBUG,"transSendResultS(%d,%d)=%d",fd, REQUEST_CLIENT_NG, ret);
		return -1;
	}
	logPrint(DEBUG_DEBUG,"diskwtWriteIndexCache()=%d",ret);

	//正常応答
	ret=transSendResultS(fd, REQUEST_CLIENT_OK);
	if ( ret < 0 ){
		logPrint(DEBUG_WARNING,"transSendResultS(%d,%d)=%d",fd, REQUEST_CLIENT_OK, ret);
		return -1;
	}
	logPrint(DEBUG_DEBUG,"transSendResultS(%d,%d)=%d",fd, REQUEST_CLIENT_OK, ret);

	return 0;
}

int creqRecvCacheData(int fd, T_CLIENT_REQUEST req){
	int ret;
	int iret=0;
	int out_size;
	int out_offset=0;
	char *addr;
	T_CLIENT_DATA_HEADER hd;
	T_ZTRANS_INFO zpara;

	//logPrint(DEBUG_DEBUG,"creqRecvCacheData(file_id=%s block_no=%d size=%d)",
	//	dispFileID( &(req.block.file_id) ), req.block.block_no, req.block.size);

	//空きMETAキャッシュ検索
	l_file_cache_free_no=cacheGetFreeFileCache();
	if ( l_file_cache_free_no < 0 ){
		logPrint(DEBUG_WARNING,"cacheGetFreeFileCache()=%d",l_file_cache_free_no);
		return -1;
	}
	logPrint(DEBUG_DEBUG,"cacheGetFreeFileCache()=%d",l_file_cache_free_no);

	//METAキャッシュ設定
	memcpy( &(G_MetaTbl[l_file_cache_free_no].inf), &(req.meta.inf), sizeof(T_META_INFO));
	memcpy( &(G_MetaTbl[l_file_cache_free_no].row.dir_id), &(req.meta.dir_id), sizeof(T_FILE_ID));
	G_MetaTbl[l_file_cache_free_no].node_wcnt=1;
	cacheSetFlagMetaDisk(l_file_cache_free_no, CACHE_META_STS_USE);
	cacheSetFlagMetaNode(l_file_cache_free_no, CACHE_META_STS_USE);


	//空きDATAキャッシュ検索
	l_data_cache_free_no=cacheGetFreeDiskCache();
	if ( l_data_cache_free_no < 0 ){
		logPrint(DEBUG_WARNING,"cacheGetFreeDiskCache()=%d",l_data_cache_free_no);
		cacheSetFlagMetaStatus(l_file_cache_free_no, CACHE_META_STS_ERROR);
		return -2;
	}
	logPrint(DEBUG_DEBUG,"cacheGetFreeDiskCache()=%d",l_data_cache_free_no);

	//INDEX設定
	G_IndexTbl[l_data_cache_free_no].file_idx=l_file_cache_free_no;
	G_IndexTbl[l_data_cache_free_no].block_no=req.meta.block_no;
	G_IndexTbl[l_data_cache_free_no].size=req.meta.size;
	cacheSetFlagIndexDisk(l_data_cache_free_no, CACHE_DATA_STS_NONE);
	cacheSetFlagIndexNode(l_data_cache_free_no, CACHE_DATA_STS_USE);

	/*
	logPrint(DEBUG_DEBUG,"dir_id=%s file_id=%s block_no=%d size=%d",
			dispFileID( &(G_MetaTbl[l_file_cache_free_no].row.dir_id) ),
			dispFileID( &(G_MetaTbl[l_file_cache_free_no].inf.file_id) ),
			G_IndexTbl[l_data_cache_free_no].block_no,
			G_IndexTbl[l_data_cache_free_no].size);
	*/

	//正常応答
	ret=transSendResultS(fd, REQUEST_CLIENT_OK);
	if ( ret < 0 ){
		logPrint(DEBUG_WARNING,"transSendResultS(%d,%d)=%d",fd, REQUEST_CLIENT_OK, ret);
		cacheSetFlagMetaDisk(l_file_cache_free_no, CACHE_META_STS_ERROR);
		cacheSetFlagMetaStatus(l_file_cache_free_no, CACHE_META_STS_ERROR);
		return -4;
	}
	logPrint(DEBUG_DEBUG,"transSendResultS(%d,%d)=%d",fd, REQUEST_CLIENT_OK, ret);

	//解凍
	memset(&zpara,0,sizeof(T_ZTRANS_INFO));
	zpara.file_size=req.clt.size;
	zpara.buff_size=G_ConfTbl->network_buff_size;
	zpara.stmout=G_ConfTbl->send_timeout;
	zpara.rtmout=G_ConfTbl->recv_timeout;
	ztrans_decomp_init(&zpara);

	//DATA部受信
	while (1) {

		time(&G_ProcTbl[G_ProcNo].alv);

		//クライアントから受信
		iret=ztrans_data_recv(&zpara, fd, &out_size);
		if ( iret < 0 ){
			logPrint(DEBUG_WARNING,"ztrans_data_recv(*,%d,%d)=%d", fd,out_size,iret);
			cacheSetFlagIndexStatus(l_data_cache_free_no, CACHE_DATA_STS_ERROR);
			cacheSetFlagMetaStatus(l_file_cache_free_no, CACHE_META_STS_ERROR);
			return -7;
		}
		if ( out_size >  0 ){
			out_offset=zpara.obuff_p->total - out_size;
			if ( out_offset < 0 ){ out_offset=0; }
			memcpy(G_DataTbl[l_data_cache_free_no].data + out_offset, zpara.obuff + sizeof(T_CLIENT_DATA_HEADER), out_size);
		}
		//logPrint(DEBUG_INFO,"ztrans_data_recv(*,%d,%d)=%d",ifd,out_size,iret);
		if ( iret == 0 ){ break; }
	}

	//logPrint(DEBUG_INFO,"#total=%d in=%d,%d out=%d,%d",zpara.file_size, 
	//		zpara.ibuff_p->seq, zpara.ibuff_p->total, zpara.obuff_p->seq, zpara.obuff_p->total);
	ztrans_decomp_fin(&zpara);

	//処理完了
	cacheSetFlagIndexStatus(l_data_cache_free_no, CACHE_DATA_STS_MEM_E);
	return 0;
}


int creqMkDir(int fd, T_CLIENT_REQUEST req){
	int ret;
	char msg[1024];

	ret=diskreqMetaMkDir(req, msg);
	if ( ret == 0 ){
		logPrint(DEBUG_INFO,"diskreqMetaMkDir()=%d",ret);
		transSendResultM(fd, REQUEST_CLIENT_OK, msg);
		return 0;
	}else{
		logPrint(DEBUG_WARNING,"diskreqMetaMkDir()=%d",ret);
		transSendResultM(fd, REQUEST_CLIENT_NG, msg);
		return ret;
	}
}

int creqRmDir(int fd, T_CLIENT_REQUEST req){
	int ret;
	char msg[1024];

	ret=diskreqMetaRmDir(req, msg);
	if ( ret == 0 ){
		logPrint(DEBUG_DEBUG,"diskreqMetaRmDir()=%d",ret);
		transSendResultM(fd, REQUEST_CLIENT_OK, msg);
		return 0;
	}else{
		logPrint(DEBUG_WARNING,"diskreqMetaRmDir()=%d",ret);
		transSendResultM(fd, REQUEST_CLIENT_NG, msg);
		return ret;
	}
	return 0;
}

int creqRmFile(int fd, T_CLIENT_REQUEST req){
	int ret;
	char msg[1024];

	ret=diskreqMetaRmFile(req, msg);
	if ( ret == 0 ){
		logPrint(DEBUG_DEBUG,"diskreqMetaRmFile()=%d",ret);
		transSendResultM(fd, REQUEST_CLIENT_OK, msg);
		return 0;
	}else{
		logPrint(DEBUG_WARNING,"diskreqMetaRmFile()=%d",ret);
		transSendResultM(fd, REQUEST_CLIENT_NG, msg);
		return ret;
	}
	return 0;
}

int creqStat(int fd, T_CLIENT_REQUEST req){
	int ret;
	char msg[1024];
	struct xi_stat stat;

	//ファイル情報取得
	ret=diskreqMetaStat(req, msg, &stat);
//printf("diskreqMetaStat()=%d\n",ret);
//printf("req      : %d (%d:%d)\n",req.req, req.uid, req.gid);
//printf("type     : %d\n",stat.st_type);
//printf("uid      : %d\n",stat.st_uid);
//printf("gid      : %d\n",stat.st_gid);
//printf("size     : %d\n",stat.st_size);
//printf("blksize  : %d\n",stat.st_blksize);
//printf("blocks   : %d\n",stat.st_blocks);
//printf("dup_cnt  : %d\n",stat.dup_cnt);

	if ( ret == 0 ){
		logPrint(DEBUG_DEBUG,"diskreqMetaStat()=%d",ret);
		transSendResultM(fd, REQUEST_CLIENT_OK, 1, sizeof(struct xi_stat), msg );
	}else{
		logPrint(DEBUG_WARNING,"diskreqMetaStat()=%d",ret);
		transSendResultM(fd, REQUEST_CLIENT_NG, 0, 0, msg);
		return ret;
	}


	//ファイル情報を転送
	T_CLIENT_RESULT_S res;
	ret=transSendBuff(fd, (char*)&stat, sizeof(struct xi_stat), &res );
	if ( ret <= 0 ){
		logPrint(DEBUG_WARNING,"transSendBuff()=%d",ret);
	}

	return ret;
}

int creqChmod(int fd, T_CLIENT_REQUEST req){
	int ret;
	char msg[1024];

	ret=diskreqMetaChmod(req, msg);
	if ( ret == 0 ){
		logPrint(DEBUG_DEBUG,"diskreqMetaChmod()=%d",ret);
		transSendResultM(fd, REQUEST_CLIENT_OK, msg);
		return 0;
	}else{
		logPrint(DEBUG_WARNING,"diskreqMetaChmod()=%d",ret);
		transSendResultM(fd, REQUEST_CLIENT_NG, msg);
		return ret;
	}
	return 0;
}

int creqChown(int fd, T_CLIENT_REQUEST req){
	int ret;
	char msg[1024];

	ret=diskreqMetaChown(req, msg);
	if ( ret == 0 ){
		logPrint(DEBUG_DEBUG,"diskreqMetaChown()=%d",ret);
		transSendResultM(fd, REQUEST_CLIENT_OK, msg);
		return 0;
	}else{
		logPrint(DEBUG_WARNING,"diskreqMetaChown()=%d",ret);
		transSendResultM(fd, REQUEST_CLIENT_NG, msg);
		return ret;
	}
	return 0;
}

int creqChgrp(int fd, T_CLIENT_REQUEST req){
	int ret;
	char msg[1024];

	ret=diskreqMetaChgrp(req, msg);
	if ( ret == 0 ){
		logPrint(DEBUG_DEBUG,"diskreqMetaChgrp()=%d",ret);
		transSendResultM(fd, REQUEST_CLIENT_OK, msg);
		return 0;
	}else{
		logPrint(DEBUG_WARNING,"diskreqMetaChgrp()=%d",ret);
		transSendResultM(fd, REQUEST_CLIENT_NG, msg);
		return ret;
	}
	return 0;
}

int creqFilePut(int fd, T_CLIENT_REQUEST req){
	int i;
	int ret;
	int iret=0;
	int oret=0;
	int out_size;
	T_ZTRANS_INFO zpara;
	char filename1[MAX_FILE_PATH+1];

	//ファイル管理領域
	l_file_cache_free_no=cacheGetFreeFileCache();
	//logPrint(DEBUG_DEBUG,"cacheGetFreeFileCache()=%d",l_file_cache_free_no);
	if ( l_file_cache_free_no < 0 ){
		ret=transSendResultM(fd, REQUEST_CLIENT_BUSY);
		logPrint(DEBUG_WARNING,"no free meta cache");
		return -1;
	}

	//MEA情報をメモリに設定
	G_MetaTbl[l_file_cache_free_no].inf.st_type=FILE_TYPE_FILE;
	G_MetaTbl[l_file_cache_free_no].inf.st_mode=G_ConfTbl->def_perm_fil;
	G_MetaTbl[l_file_cache_free_no].inf.st_uid=req.uid;
	G_MetaTbl[l_file_cache_free_no].inf.st_gid=req.gid;
	G_MetaTbl[l_file_cache_free_no].inf.st_size=req.clt.size;
	G_MetaTbl[l_file_cache_free_no].inf.st_blksize=BLOCK_SIZE;
	G_MetaTbl[l_file_cache_free_no].inf.st_blocks=utlCeil(req.clt.size, BLOCK_SIZE);
	G_MetaTbl[l_file_cache_free_no].inf.dup_cnt=0;
	gettimeofday(&(G_MetaTbl[l_file_cache_free_no].inf.st_atm), NULL);
	gettimeofday(&(G_MetaTbl[l_file_cache_free_no].inf.st_mtm), NULL);
	gettimeofday(&(G_MetaTbl[l_file_cache_free_no].inf.st_ctm), NULL);
	G_MetaTbl[l_file_cache_free_no].inf.st_dtm.tv_sec=0;
	G_MetaTbl[l_file_cache_free_no].inf.st_dtm.tv_usec=0;
	strcpy(G_MetaTbl[l_file_cache_free_no].inf.name, req.clt.para1);

	//META情報チェック
	ret=creqCheckMETA(fd, req, &(G_MetaTbl[l_file_cache_free_no].inf), &(G_MetaTbl[l_file_cache_free_no].row));
	if ( ret != 1 ){
		logPrint(DEBUG_WARNING,"creqCheckMETA()=%d",ret);
		cacheSetFlagMetaDisk(l_file_cache_free_no, CACHE_META_STS_ERROR);
		return -2;
	}
	logPrint(DEBUG_DEBUG,"creqCheckMETA()=%d",ret);

	//キャッシュ書込み中状態にセット
	cacheSetFlagMetaDisk(l_file_cache_free_no, CACHE_META_STS_WRITE_S);

	//ファイルID
	ret=daemon_GetNewFileID(&(G_MetaTbl[l_file_cache_free_no].inf.file_id),1);

	//正常応答
	ret=transSendResultM(fd, REQUEST_CLIENT_OK);
	if ( ret < 0 ){
		logPrint(DEBUG_WARNING,"transSendResultM(%d,%d)=%d",fd, REQUEST_CLIENT_OK, ret);
		cacheSetFlagMetaDisk(l_file_cache_free_no, CACHE_META_STS_ERROR);
		return -3;
	}
	logPrint(DEBUG_DEBUG,"transSendResultM(%d,%d)=%d",fd, REQUEST_CLIENT_OK, ret);

	//INDEXヘッダ書込み
	ret=diskwtWriteIndexInit(
		G_MetaTbl[l_file_cache_free_no].row.dir_id, 
		G_MetaTbl[l_file_cache_free_no].inf.file_id,
		G_MetaTbl[l_file_cache_free_no].inf.st_blocks);
	if ( ret < 0 ){
		logPrint(DEBUG_WARNING,"diskwtWriteIndexInit()=%d",ret);
		cacheSetFlagMetaDisk(l_file_cache_free_no, CACHE_META_STS_ERROR);
		return -4;
	}
	logPrint(DEBUG_DEBUG,"diskwtWriteIndexInit()=%d",ret);

	//解凍
	memset(&zpara,0,sizeof(T_ZTRANS_INFO));
	zpara.file_size=req.clt.size;
	zpara.buff_size=G_ConfTbl->network_buff_size;
	zpara.stmout=G_ConfTbl->send_timeout;
	zpara.rtmout=G_ConfTbl->recv_timeout;
	ztrans_decomp_init(&zpara);

	//データ取得ループ
	l_data_cache_write_cnt=0;
	while(1){

		time(&G_ProcTbl[G_ProcNo].alv);

		//クライアントから受信
		iret=ztrans_data_recv(&zpara, fd, &out_size);
		if ( iret < 0 ){
			logPrint(DEBUG_WARNING,"ztrans_data_recv(*,%d,%d)=%d", fd,out_size,iret);
			oret=-11;
			break;
		}
		//logPrint(DEBUG_INFO,"ztrans_data_recv(*,%d,%d)=%d",fd,out_size,iret);

//printf("#P# ztrans_data_recv(*,%d,%d)=%d in=%d,%d out=%d,%d\n",fd,out_size,iret,
//			zpara.ibuff_p->seq, zpara.ibuff_p->total, zpara.obuff_p->seq, zpara.obuff_p->total);

		//DATAキャッシュ書込み
		ret=creqWriteDataCache(zpara.obuff + sizeof(T_CLIENT_DATA_HEADER), out_size);
		if ( ret < 0 ){
			logPrint(DEBUG_WARNING,"creqWriteDataCache(*,%d)=%d",out_size, ret);
			oret=-14;
			break;
		}

		if ( iret == 0 ){ break; }
	}

	ztrans_decomp_fin(&zpara);

	//サイズをセット
	//logPrint(DEBUG_INFO,"#total=%d in=%d,%d out=%d,%d",zpara.file_size, 
	//		zpara.ibuff_p->seq, zpara.ibuff_p->total, zpara.obuff_p->seq, zpara.obuff_p->total);
	G_MetaTbl[l_file_cache_free_no].inf.st_size=zpara.obuff_p->total;

//printf("#P# ztrans_data_recv(*,%d,%d)=%d in=%d,%d out=%d,%d\n",fd,out_size,iret,
//			zpara.ibuff_p->seq, zpara.ibuff_p->total, zpara.obuff_p->seq, zpara.obuff_p->total);


	/* 処理完了へフラグ更新 */
	if ( oret == 0 ){
		cacheSetFlagMetaStatus(l_file_cache_free_no, CACHE_META_STS_MEM_E);
	}else{
		logPrint(DEBUG_DEBUG,"put cancel (oret=%d)",oret);
		cacheSetFlagMetaStatus(l_file_cache_free_no, CACHE_META_STS_ERROR);
	}
	if ( l_data_cache_free_no >= 0 ){
		if ( oret == 0 ){
			cacheSetFlagIndexStatus(l_data_cache_free_no, CACHE_DATA_STS_MEM_E);
		}else{
			cacheSetFlagIndexStatus(l_data_cache_free_no, CACHE_DATA_STS_ERROR);
		}
	}
	l_data_cache_free_no=(-1);
	l_data_cache_write_cnt=0;
	l_data_cache_write_size=0;
	return oret;
}

int creqFileGet(int fd, T_CLIENT_REQUEST req){
	int i;
	int block_no=0;
	int ret;
	int ifd;
	T_META_HEADER hd;
	T_META_INFO fid;
	T_IDX_HEADER ihd;
	T_IDX_INFO c_idx;
	T_IDX_INFO buff_idx[G_ConfTbl->max_blocks + 1];
	T_ZTRANS_INFO zpara;
	char filename[MAX_FILE_PATH+1];

	//ファイル検索
	ret=diskrdSearchMeta(req.clt.para1, &hd, &fid);
	if ( ret != 1 ){
		logPrint(DEBUG_WARNING,"diskSearchMETA(%s,*,*)=%d",req.clt.para1,ret);
		ret=transSendResultM(fd, REQUEST_CLIENT_BUSY,"no file (%s)",req.clt.para1);
		return -1;
	}
	//logPrint(DEBUG_DEBUG,"diskSearchMETA(%s,*,*)=%d",req.clt.para1,ret);

	//INDEXファイル名
	disknmGetIdxFileName("", fid.file_id, filename);
	if ( filisFile(filename) != 1 ){
		ret=transSendResultM(fd, REQUEST_CLIENT_BUSY,"no file (%s)",req.clt.para1);
		logPrint(DEBUG_WARNING,"no file (%s)",filename);
		return -2;
	}
	logPrint(DEBUG_DEBUG,"S:[%s] %s",filename,dispFileID(&(fid.file_id)));

	//正常応答
	ret=transSendResultM(fd, REQUEST_CLIENT_OK, 1, fid.st_size);
	logPrint(DEBUG_DEBUG,"transSendResultM(%d,%d,%d,%d)=%d",fd, REQUEST_CLIENT_OK, 1, fid.st_size, ret);

	//データブロック格納場所取得
	memset(&buff_idx, 0, sizeof(T_IDX_INFO) * G_ConfTbl->max_blocks );
	if ( (ifd=open(filename,O_RDONLY)) < 0 ){
		ret=transSendResultM(fd, REQUEST_CLIENT_NG,"no file (%s)",req.clt.para1);
		logPrint(DEBUG_WARNING,"no file (%s)",req.clt.para1);
		return -3;
	}
	ret=read(ifd, (char*)&ihd, sizeof(T_IDX_HEADER));
	if ( ret < sizeof(T_IDX_HEADER) ){
		ret=transSendResultM(fd, REQUEST_CLIENT_NG,"bat file (%s)",req.clt.para1);
		logPrint(DEBUG_WARNING,"bat file (%s)",req.clt.para1);
		return -4;
	}
	while ( (ret=read(ifd, (char*)&c_idx, sizeof(T_IDX_INFO))) > 0 ){
		if ( buff_idx[c_idx.block_no].size == 0 ){
			memcpy( &(buff_idx[c_idx.block_no]), &c_idx, sizeof(T_IDX_INFO) );
			continue;
		}
		if ( memcmp(c_idx.node_id, G_NodeTbl[0].node_id, NODE_ID_LEN) != 0 ){ continue; }
		memcpy( &(buff_idx[c_idx.block_no]), &c_idx, sizeof(T_IDX_INFO) );
		block_no++;
	}
	close(ifd);

	//圧縮初期化
	memset(&zpara,0,sizeof(T_ZTRANS_INFO));
	zpara.file_size=fid.st_size;
	zpara.buff_size=G_ConfTbl->network_buff_size;
	zpara.stmout=G_ConfTbl->send_timeout;
	zpara.rtmout=G_ConfTbl->recv_timeout;
	ztrans_comp_init(&zpara, G_ConfTbl->con_compression);

	//ファイル転送
	for (i=1; i<=G_ConfTbl->max_blocks; i++){
		if ( buff_idx[i].size == 0 ){ break; }

		logPrint(DEBUG_DEBUG,"block_no=%d node=%s size=%d",
			buff_idx[i].block_no, utlSHA1_to_HEX(buff_idx[i].node_id), buff_idx[i].size);

		if ( memcmp( buff_idx[i].node_id, G_NodeTbl[0].node_id, NODE_ID_LEN ) == 0 ){
			//ローカルノードから取得
			ret=creqReadLocalNode(&zpara, fd, fid.file_id, i);
			if ( ret != 0 ){
				logPrint(DEBUG_WARNING,"creqReadLocalNode(%d,%s,%d)=%d",fd, dispFileID(&(fid.file_id)), i, ret);
				return ret;
			}
			//logPrint(DEBUG_INFO,"creqReadLocalNode(%d,%s,%d)=%d",fd, dispFileID(&(fid.file_id)), i, ret);
		}else{
			//リモートノードから取得
			ret=creqReadRemoteDisk(&zpara, fd, fid.file_id, i, buff_idx[i].node_id);
			if ( ret != 0 ){
				logPrint(DEBUG_WARNING,"creqReadRemoteDisk(%d,%s,%d,*)=%d",fd, dispFileID(&(fid.file_id)), i, ret);
				return ret;
			}
			logPrint(DEBUG_INFO,"creqReadRemoteDisk(%d,%s,%d,*)=%d",fd, dispFileID(&(fid.file_id)), i, ret);
		}
	}

	ztrans_comp_fin(&zpara);
	return 0;
}

int creqGetData(int fd, T_CLIENT_REQUEST req){
	int ret;
	u_long r_cnt=req.data.block_no;
	u_long r_size=0;
	T_ZTRANS_INFO zpara;

	//ブロックサイズ取得
	ret=diskrdGetDataBlockSize(req.data.file_id, req.data.block_no);
	if ( ret < 0 ){
		logPrint(DEBUG_WARNING,"diskrdGetDataBlockSize(%s,%d)=%d",dispFileID(&(req.data.file_id)), req.data.block_no, ret);
		ret=transSendResultS(fd, REQUEST_CLIENT_BUSY, 0, 0);
		return -1;
	}
	r_size=(u_long)ret;

	//データファイルチェック
	ret=diskrdCheckDataFile(req.data.file_id, req.data.block_no);
	if ( ret < 0 ){
		logPrint(DEBUG_WARNING,"diskrdCheckDataFile(%s,%d)=%d",dispFileID(&(req.data.file_id)), req.data.block_no, ret);
		ret=transSendResultS(fd, REQUEST_CLIENT_BUSY, 0, 0);
		return -2;
	}

	//正常応答
	ret=transSendResultS(fd, REQUEST_CLIENT_OK, r_cnt, r_size);
	if ( ret < 0 ){
		logPrint(DEBUG_WARNING,"transSendResultS(%d,%d,%d,%d)=%d",fd, REQUEST_CLIENT_OK, r_cnt, r_size, ret);
		return -3;
	}
	logPrint(DEBUG_DEBUG,"transSendResultS(%d,%d,%d,%d)=%d",fd, REQUEST_CLIENT_OK, r_cnt, r_size, ret);

	//圧縮初期化
	memset(&zpara,0,sizeof(T_ZTRANS_INFO));
	zpara.file_size=r_size;
	zpara.buff_size=G_ConfTbl->network_buff_size;
	zpara.stmout=G_ConfTbl->send_timeout;
	zpara.rtmout=G_ConfTbl->recv_timeout;
	ztrans_comp_init(&zpara, G_ConfTbl->con_compression);

	//ローカルノードから読み込みクライアントに返す
	ret=creqReadLocalNode(&zpara, fd, req.data.file_id, req.data.block_no);
	if ( ret != 0 ){
		logPrint(DEBUG_WARNING,"creqReadLocalNode(%d,%s,%d)=%d",fd, dispFileID(&(req.data.file_id)), req.data.block_no, ret);
		ztrans_comp_fin(&zpara);
		return -4;
	}
	//logPrint(DEBUG_INFO,"creqReadLocalNode(%d,%s,%d)=%d",fd, dispFileID(&(req.data.file_id)), req.data.block_no, ret);

	ztrans_comp_fin(&zpara);
	return 0;
}

int creqGetMeta(int fd, T_CLIENT_REQUEST req){
	int ret;
	u_long r_cnt=0;
	u_long r_size=0;
	T_META_SYNC_RESULT outbuff[G_ConfTbl->meta_sync_max_rec];
	T_CLIENT_RESULT_S res;

	//META読込み
	ret=disksyncReadMeta(req, outbuff, &r_cnt, &r_size);
	if ( ret < 0 ){
		logPrint(DEBUG_WARNING,"disksyncReadMeta()=%d",ret);
		ret=transSendResultS(fd, REQUEST_CLIENT_NG, r_cnt, r_size);
		return -1;
	}

	//正常応答
	ret=transSendResultS(fd, REQUEST_CLIENT_OK, r_cnt, r_size);
	if ( ret < 0 ){
		logPrint(DEBUG_WARNING,"transSendResultS(%d,%d,%d,%d)=%d",fd, REQUEST_CLIENT_OK, r_cnt, r_size, ret);
		return -2;
	}
	logPrint(DEBUG_DEBUG,"transSendResultS(%d,%d,%d,%d)=%d",fd, REQUEST_CLIENT_OK, r_cnt, r_size, ret);

	if ( r_size <= 0 ){ return 0; }

	//META転送
	ret=transSendBuff(fd, (char*)outbuff, sizeof(T_META_SYNC_RESULT) * r_size, &res);
	if ( ret <= 0 ){
		logPrint(DEBUG_WARNING,"transSendBuff(%d,*,%d,*)=%d",fd, sizeof(T_META_SYNC_RESULT) * r_size, ret);
		return -3;
	}
	//logPrint(DEBUG_DEBUG,"transSendBuff(%d,*,%d,*)=%d",fd, sizeof(T_META_SYNC_RESULT) * r_size, ret);

	return 0;
}

int creqNodeInfo(int fd, T_CLIENT_REQUEST req){
	int i;
	int cno=0;
	int ret;

	//メモリ更新
	if ( req.req == REQUEST_NODE_INFO || req.req == REQUEST_NODE_STOP ){
		cno=-1;
		for (i=0; i<G_ConfTbl->max_node; i++){
			if ( G_NodeTbl[i].svr_ip == req.node.inf.svr_ip ){ cno=i; break; }
			if ( G_NodeTbl[i].svr_ip == 0 ){ cno=i; break; }
		}
//printf("creqNodeInfo() REQ=%d IDX=%d STS=%d FLG=%d\n",req.req, cno, req.node.inf.sts, req.node.inf.master_flg);
		if ( cno > 0 ){
			cacheSetNodeStatus(cno, req.node.inf.sts);
			cacheSetMasterFlag(cno, req.node.inf.master_flg);
			memcpy((char*)&(G_NodeTbl[cno]), (char*)&(req.node.inf), sizeof(T_NODE_TABLE));
		}
	}

	//正常応答
	ret=transSendResultS(fd, REQUEST_CLIENT_OK);
	if ( ret < 0 ){
		logPrint(DEBUG_WARNING,"transSendResultS(%d,%d)=%d",fd, REQUEST_CLIENT_OK, ret);
		return -2;
	}
	logPrint(DEBUG_DEBUG,"transSendResultS(%d,%d)=%d",fd, REQUEST_CLIENT_OK, ret);

	return 0;
}
