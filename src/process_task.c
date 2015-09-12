/*****************************************************************************
*  <システム>   分散処理フレームワーク
*  <名称>	処理タスクデーモン
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

static char l_sock_file[MAX_FILE_PATH+1];
static int  l_gate_fd=(-1);
static int  l_client_fd=(-1);

static int process_task_close(){
    if ( l_client_fd != (-1) ){
        close(l_client_fd);
        l_client_fd=(-1);
    }
    G_ProcTbl[G_ProcNo].req=0;
	cacheSetProcStatus(G_ProcNo, PROCESS_STATUS_IDLE);
    return 0;
}

int process_task_init(){
	int ret;

	//UNIXドメインソケット
	sprintf(l_sock_file,"%s/ud%03d.sock",G_TempDir,G_ProcNo);	

	if ( (ret=filisDevice(l_sock_file)) > 0 ){
		ret=unlink(l_sock_file);
		logPrint(DEBUG_INFO,"unlink(%s)=%d",l_sock_file,ret);
	}
	l_gate_fd = udListen(l_sock_file);
	if ( l_gate_fd < 0 ){
		logPrint(DEBUG_ERROR,"udListen(%s)=%d",l_sock_file,l_gate_fd);
		exit(1); 
	}

	return 0;
}

int process_task(){
	int ret;
	//T_MESSAGE_QUEUE_FMT mbuff;
	T_CLIENT_REQUEST req;
	char buff[MAX_FILE_PATH + 1];
	char buff1[256];
	char buff2[256];

	//UNIXドメインソケット通信
	l_client_fd = udRecv(l_gate_fd, &req, sizeof(req),G_ConfTbl->req_timeout);
	if ( l_client_fd < 0 ){
		//logPrint(DEBUG_ERROR,"udRecv(%d,*,%d,%d)=%d",l_gate_fd, sizeof(req), G_ConfTbl->recv_timeout,l_client_fd);
		process_task_close();
		return 0; 
	}
	logPrint(DEBUG_DEBUG,"udRecv(%d,*,%d,%d)=%d",l_gate_fd, sizeof(req), G_ConfTbl->recv_timeout,l_client_fd);

	cacheSetProcStatus(G_ProcNo, PROCESS_STATUS_BUSY);

	//処理タスク要求受信
	ret=tcpRecvFix(l_client_fd,(char *)&req,sizeof(req),G_ConfTbl->recv_timeout);
	if ( ret != sizeof(req) ){
		logPrint(DEBUG_ERROR,"tcpRecvFix(%d,*,%d,%d)=%d",l_client_fd,sizeof(req),G_ConfTbl->recv_timeout,ret);
		process_task_close();
		return -1; 
	}
	logPrint(DEBUG_DEBUG,"tcpRecvFix(%d,*,%d,%d)=%d",l_client_fd,sizeof(req),G_ConfTbl->recv_timeout,ret);

	//電文チェック
	ret=commReqCheckSumJudge(&(req.ck));
	if ( ret < 0 ){
		logPrint(DEBUG_ERROR,"commReqCheckSumJudge()=%d",ret);
		process_task_close();
		return -2; 
	}

	///////////////////////////
	//ノード情報転送
	///////////////////////////
	if ( req.req == REQUEST_NODE_INFO || req.req == REQUEST_NODE_STOP ){
		ret=creqNodeInfo(l_client_fd, req);
		if ( ret < 0 ){ logPrint(DEBUG_ERROR,"creqNodeInfo()=%d",ret); }
	}


	///////////////////////////
	//リクエスト要求不許可状態チェック
	///////////////////////////
	//未サービス中
	if ( G_ProcTbl[0].sts != SYSTEM_STATUS_RUNNING ){
		ret=transSendResultM(l_client_fd, REQUEST_CLIENT_NG,"not running");
		process_task_close();
		return 0;
	}
	//MASTERサーバなし
	if ( daemon_GetMasterIdx() < 0 ){
		ret=transSendResultM(l_client_fd, REQUEST_CLIENT_NOMASTER,"not master node");
		process_task_close();
		return 0;
	}


	///////////////////////////
	//クライアントリクエスト
	///////////////////////////
	if ( req.req == REQUEST_CLIENT_PING ){
		ret=transSendResultM(l_client_fd, REQUEST_CLIENT_OK,"");
	}
	if ( req.req == REQUEST_CLIENT_TAB ){
		ret=creqGetTabFilePtn(req, buff); 
		ret=transSendResultM(l_client_fd, REQUEST_CLIENT_OK,buff);
	}
	if ( req.req == REQUEST_CLIENT_STATUS ){ 
		ret=transSendResultM(l_client_fd, REQUEST_CLIENT_OK);
		ret=dispNodeInfo(l_client_fd,G_NodeTbl); 
		if ( ret < 0 ){	
			logPrint(DEBUG_WARNING,"dispNodeInfo()=%d",ret); 
		}else{
			logPrint(DEBUG_INFO,"dispNodeInfo()=%d",ret); 
		}
	}
	if ( req.req == REQUEST_CLIENT_STATUS2 ){ 
		ret=transSendResultM(l_client_fd, REQUEST_CLIENT_OK);
		ret=dispNodeInfoDetail(l_client_fd,G_NodeTbl); 
		if ( ret < 0 ){	
			logPrint(DEBUG_WARNING,"dispNodeInfoDetail()=%d",ret); 
		}else{
			logPrint(DEBUG_INFO,"dispNodeInfoDetail()=%d",ret); 
		}
	}
	if ( req.req == REQUEST_CLIENT_PERF ){ 
		ret=transSendResultM(l_client_fd, REQUEST_CLIENT_OK);
		ret=dispPerformance(l_client_fd,G_NodeTbl,G_DiskTbl); 
		if ( ret < 0 ){	
			logPrint(DEBUG_WARNING,"dispPerformance()=%d",ret); 
		}else{
			logPrint(DEBUG_INFO,"dispPerformance()=%d",ret); 
		}
	}
	if ( req.req == REQUEST_CLIENT_PROCESS ){ 
		ret=transSendResultM(l_client_fd, REQUEST_CLIENT_OK);
		ret=dispProcessInfo(l_client_fd,G_ProcTbl); 
		if ( ret < 0 ){	
			logPrint(DEBUG_WARNING,"dispProcessInfo()=%d",ret); 
		}else{
			logPrint(DEBUG_INFO,"dispProcessInfo()=%d",ret); 
		}
	}
	if ( req.req == REQUEST_CLIENT_MEM ){ 	
		ret=transSendResultM(l_client_fd, REQUEST_CLIENT_OK);
		ret=dispMemInfo(l_client_fd,G_IndexTbl); 
		if ( ret < 0 ){	
			logPrint(DEBUG_WARNING,"dispMemInfo()=%d",ret); 
		}else{
			logPrint(DEBUG_INFO,"dispMemInfo()=%d",ret); 
		}
	}
	if ( req.req == REQUEST_CLIENT_CACHE ){ 	
		ret=transSendResultM(l_client_fd, REQUEST_CLIENT_OK);
		ret=dispCacheInfo(l_client_fd, G_MetaTbl, G_IndexTbl); 
		if ( ret < 0 ){	
			logPrint(DEBUG_WARNING,"dispCacheInfo()=%d",ret); 
		}else{
			logPrint(DEBUG_INFO,"dispCacheInfo()=%d",ret); 
		}
	}
	if ( req.req == REQUEST_CLIENT_VOLUME ){ 	
		ret=transSendResultM(l_client_fd, REQUEST_CLIENT_OK);
		ret=dispVolumeInfo(l_client_fd, G_NodeTbl, G_DiskTbl);
		if ( ret < 0 ){	
			logPrint(DEBUG_WARNING,"dispVolumeInfo()=%d",ret); 
		}else{
			logPrint(DEBUG_INFO,"dispVolumeInfo()=%d",ret); 
		}
	}
	if ( req.req == REQUEST_CLIENT_LS ){
		ret=transSendResultM(l_client_fd, REQUEST_CLIENT_OK);
		ret=dispFileList(l_client_fd, req); 
		if ( ret < 0 ){	
			logPrint(DEBUG_WARNING,"dispFileList(%s)=%d",req.clt.para1, ret); 
		}else{
			logPrint(DEBUG_INFO,"dispFileList(%s)=%d",req.clt.para1, ret); 
		}
	}
	if ( req.req == REQUEST_CLIENT_STAT ){
		ret=transSendResultM(l_client_fd, REQUEST_CLIENT_OK);
		ret=dispFileStat(l_client_fd, req); 
		if ( ret < 0 ){	
			logPrint(DEBUG_WARNING,"dispFileStat(%s)=%d",req.clt.para1, ret); 
		}else{
			logPrint(DEBUG_INFO,"dispFileStat(%s)=%d",req.clt.para1, ret); 
		}
	}
	if ( req.req == REQUEST_CLIENT_FILE ){
		ret=creqStat(l_client_fd, req); 
		if ( ret < 0 ){	
			logPrint(DEBUG_WARNING,"creqStat(%s)=%d",req.clt.para1, ret); 
		}else{
			logPrint(DEBUG_INFO,"creqStat(%s)=%d",req.clt.para1, ret); 
		}
	}
	if ( req.req == REQUEST_CLIENT_MKDIR ){ 
		hlogJobHistoryLogStr(req);
		ret=creqMkDir(l_client_fd, req); 
		hlogJobHistoryLogEnd(req,ret);
		if ( ret < 0 ){	
			logPrint(DEBUG_WARNING,"creqMkDir(%s)=%d",req.clt.para1, ret); 
		}else{
			logPrint(DEBUG_INFO,"creqMkDir(%s)=%d",req.clt.para1, ret); 
		}
	}
	if ( req.req == REQUEST_CLIENT_RMDIR ){
		hlogJobHistoryLogStr(req);
		ret=creqRmDir(l_client_fd, req); 
		hlogJobHistoryLogEnd(req,ret);
		if ( ret < 0 ){	
			logPrint(DEBUG_WARNING,"creqRmDir(%s)=%d",req.clt.para1, ret); 
		}else{
			logPrint(DEBUG_INFO,"creqRmDir(%s)=%d",req.clt.para1, ret); 
		}
	}
	if ( req.req == REQUEST_CLIENT_RM ){
		hlogJobHistoryLogStr(req);
		ret=creqRmFile(l_client_fd, req); 
		hlogJobHistoryLogEnd(req,ret);
		if ( ret < 0 ){	
			logPrint(DEBUG_WARNING,"creqRmFile(%s)=%d",req.clt.para1, ret); 
		}else{
			logPrint(DEBUG_INFO,"creqRmFile(%s)=%d",req.clt.para1, ret); 
		}
	}
	if ( req.req == REQUEST_CLIENT_CHMOD ){
		hlogJobHistoryLogStr(req);
		ret=creqChmod(l_client_fd, req); 
		hlogJobHistoryLogEnd(req,ret);
		if ( ret < 0 ){	
			logPrint(DEBUG_WARNING,"creqChmod(%s,%s)=%d",req.clt.para1, req.clt.para2, ret); 
		}else{
			logPrint(DEBUG_INFO,"creqChmod(%s,%s)=%d",req.clt.para1, req.clt.para2, ret); 
		}
	}
	if ( req.req == REQUEST_CLIENT_CHOWN ){
		hlogJobHistoryLogStr(req);
		ret=creqChown(l_client_fd, req); 
		hlogJobHistoryLogEnd(req,ret);
		if ( ret < 0 ){	
			logPrint(DEBUG_WARNING,"creqChown(%s,%s)=%d",req.clt.para1, req.clt.para2, ret); 
		}else{
			logPrint(DEBUG_INFO,"creqChown(%s,%s)=%d",req.clt.para1, req.clt.para2, ret); 
		}
	}
	if ( req.req == REQUEST_CLIENT_CHGRP ){
		hlogJobHistoryLogStr(req);
		ret=creqChgrp(l_client_fd, req); 
		hlogJobHistoryLogEnd(req,ret);
		if ( ret < 0 ){	
			logPrint(DEBUG_WARNING,"creqChgrp(%s,%s)=%d",req.clt.para1, req.clt.para2, ret); 
		}else{
			logPrint(DEBUG_INFO,"creqChgrp(%s,%s)=%d",req.clt.para1, req.clt.para2, ret); 
		}
	}
	if ( req.req == REQUEST_CLIENT_GET ){
		hlogJobHistoryLogStr(req);
		ret=creqFileGet(l_client_fd, req);
		hlogJobHistoryLogEnd(req,ret);
		if ( ret < 0 ){	
			logPrint(DEBUG_WARNING,"creqFileGet(%s)=%d",req.clt.para1, ret); 
		}else{
			logPrint(DEBUG_INFO,"creqFileGet(%s)=%d",req.clt.para1, ret); 
		}
	}
	if ( req.req == REQUEST_CLIENT_PUT ){
		hlogJobHistoryLogStr(req);
		ret=creqFilePut(l_client_fd, req);
		hlogJobHistoryLogEnd(req,ret);
		if ( ret < 0 ){	
			logPrint(DEBUG_WARNING,"creqFilePut(%s)=%d",req.clt.para1, ret); 
		}else{
			logPrint(DEBUG_INFO,"creqFilePut(%s)=%d",req.clt.para1, ret); 
		}
	}

	///////////////////////////
	//非マスターサーバからのキャッシュ転送リクエスト
	///////////////////////////
	if ( req.req == REQUEST_CACHE_META_CRE ){
		ret=creqRecvCacheMetaCre(l_client_fd, req);
		if ( ret < 0 ){
			logPrint(DEBUG_ERROR,"creqRecvCacheMetaCre(%s,%s)=%d",
				dispFileID( &(req.meta.dir_id), buff1 ),
				dispFileID( &(req.meta.inf.file_id), buff2 ),ret); 
		}else{
			logPrint(DEBUG_DEBUG,"creqRecvCacheMetaCre(%s,%s)=%d",
				dispFileID( &(req.meta.dir_id), buff1 ),
				dispFileID( &(req.meta.inf.file_id), buff2 ),ret); 
		}
	}

	if ( req.req == REQUEST_CACHE_META_ADD ){
		ret=creqRecvCacheMetaAdd(l_client_fd, req);
		if ( ret < 0 ){
			logPrint(DEBUG_ERROR,"creqRecvCacheMetaAdd(%s,%s)=%d",
				dispFileID( &(req.meta.dir_id), buff1 ),
				dispFileID( &(req.meta.inf.file_id), buff2 ),ret); 
		}else{
			logPrint(DEBUG_DEBUG,"creqRecvCacheMetaAdd(%s,%s)=%d",
				dispFileID( &(req.meta.dir_id), buff1 ),
				dispFileID( &(req.meta.inf.file_id), buff2 ),ret); 
		}
	}

	if ( req.req == REQUEST_CACHE_IDX_ADD ){
		ret=creqRecvCacheIndexAdd(l_client_fd, req);
		if ( ret < 0 ){
			logPrint(DEBUG_ERROR,"creqRecvCacheIndexAdd(%s,%s,%d,%d)=%d",
				dispFileID( &(req.block.dir_id) ), 
				dispFileID( &(req.block.file_id) ), 
				req.block.block_no, req.block.size, ret); 
		}else{
			logPrint(DEBUG_DEBUG,"creqRecvCacheIndexAdd(%s,%s,%d,%d)=%d",
				dispFileID( &(req.block.dir_id) ), 
				dispFileID( &(req.block.file_id) ), 
				req.block.block_no, req.block.size, ret); 
		}
	}

	if ( req.req == REQUEST_CACHE_DATA ){
		ret=creqRecvCacheData(l_client_fd, req);
		if ( ret < 0 ){
			logPrint(DEBUG_ERROR,"creqRecvCacheData(%s,%d,%d)=%d",
				dispFileID( &(req.block.file_id) ), req.block.block_no, req.block.size, ret); 
		}else{
			logPrint(DEBUG_DEBUG,"creqRecvCacheData(%s,%d,%d)=%d",
				dispFileID( &(req.block.file_id) ), req.block.block_no, req.block.size, ret); 
		}
	}

	///////////////////////////
	//他ノードからのデータ取得リクエスト
	///////////////////////////
	if ( req.req == REQUEST_TASK_META ){
		ret=creqGetMeta(l_client_fd, req);
		if ( ret < 0 ){	logPrint(DEBUG_ERROR,"creqGetMeta()=%d",ret); }
	}

	if ( req.req == REQUEST_TASK_DATA ){
		ret=creqGetData(l_client_fd, req);
		if ( ret < 0 ){ logPrint(DEBUG_ERROR,"creqGetData()=%d",ret); }
	}

	//後始末
	process_task_close();

	return 0;
}

int process_task_fin(){
	int ret;

	if ( l_gate_fd != (-1) ){
		ret=udClose(l_gate_fd); 
		logPrint(DEBUG_DEBUG,"udClose(%d)=%d",l_gate_fd,ret);
	}
	if ( l_sock_file[0] != (char)NULL ){
		ret=unlink(l_sock_file); 
		logPrint(DEBUG_INFO,"unlink(%s)=%d",l_sock_file,ret);
	}

	return 0;
}
