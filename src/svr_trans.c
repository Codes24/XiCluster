/*****************************************************************************
*  <システム>   分散処理フレームワーク
*  <名称>	データ転送API
*  <目的>
*  <機能>
*  <開発環境>   UNIX
*  <特記事項>
*
*  VERSION  DATE		BY			  CHANGE/COMMENT
*  -----------------------------------------------------------------------------
*  V0.00	  2014/02/17  Takakusaki		新規作成
******************************************************************************/
#include "xi_server.h"

static long transConnectServerChoice(){
	int i;
	int j;
	int ret;
	long addr=0;
	int sv_connect_cnt=99999999;
	int connect_cnt=0;

	if ( semLock(SEM_LOCK_IDXO004,  G_ConfTbl->enospc_retry, G_ConfTbl->enospc_retrans) < 0 ){ return 0; }

	for(i=1; i<=G_ConfTbl->max_node; i++){
		if ( G_NodeTbl[i].run_hostname[0] == (char)NULL ){ continue; }
		if ( G_NodeTbl[i].sts != SYSTEM_STATUS_RUNNING ){ continue; }

		//接続数
		connect_cnt=0;
		for ( j=0; j<G_ConfTbl->max_process; j++){
			if ( G_ProcTbl[i].pid == 0 ){ continue; }
			if ( G_ProcTbl[j].ptype != PROCESS_TYPE_CSND ){ continue; }
			if ( G_NodeTbl[i].svr_ip == utlStrIP2long(G_ProcTbl[j].para) ){
				connect_cnt++;
			}
		}
		if ( connect_cnt > sv_connect_cnt ){ continue; }
		sv_connect_cnt=connect_cnt;
		addr=G_NodeTbl[i].svr_ip;
	}

	if ( semUnLock(SEM_LOCK_IDXO004,G_ConfTbl->enospc_retry, G_ConfTbl->enospc_retrans) < 0 ){ return 0; }
	
	return addr;
}

int transConnect(long addr, int port){
	int ofd;

	//接続
	ofd=tcpConnect(addr,port,G_ConfTbl->con_timeout,1,0);
	if ( ofd < 0 ){
		logPrint(DEBUG_ERROR,"tcpConnect(%s,%d/TCP)=%d",utlDisplyIP((u_char*)&addr),port,ofd);
		return -1;
	}
	logPrint(DEBUG_DEBUG,"tcpConnect(%s,%d/TCP)=%d",utlDisplyIP((u_char*)&addr),port,ofd);

	return ofd;
}

int transSendBuff(int fd, char *buff, int siz, T_CLIENT_RESULT_S *res){
	int ret;

	res->r_cnt=0;
	res->r_size=0;

	//送信
	ret=tcpSend(fd,(char*)buff, siz, G_ConfTbl->send_timeout);
	if ( ret < 0 ){
		logPrint(DEBUG_ERROR,"tcpSend(%d,*,%d,%d)=%d",fd, siz, G_ConfTbl->send_timeout, ret);
		return -1;
	}
	logPrint(DEBUG_DEBUG,"tcpSend(%d,*,%d,%d)=%d",fd, siz, G_ConfTbl->send_timeout, ret);

	//応答受信
	ret=tcpRecvFix(fd,(char*)res, sizeof(T_CLIENT_RESULT_S), G_ConfTbl->recv_timeout);

	if ( ret != sizeof(T_CLIENT_RESULT_S) ){
		logPrint(DEBUG_ERROR,"tcpRecvFix(%d,*,%d,%d)=%d",fd, sizeof(T_CLIENT_RESULT_S), G_ConfTbl->recv_timeout, ret);
		return -2;
	}
	if ( res->res == REQUEST_CLIENT_OK ){
		logPrint(DEBUG_DEBUG,"tcpRecvFix(%d,*,%d,%d)=%d",fd, sizeof(T_CLIENT_RESULT_S), G_ConfTbl->recv_timeout, ret);
		return 1; 
	}
	if ( res->res == REQUEST_CLIENT_BUSY ){
		logPrint(DEBUG_WARNING,"tcpRecvFix(%d,*,%d,%d)=%d (res=%d)",	
			fd, sizeof(T_CLIENT_RESULT_S), G_ConfTbl->recv_timeout, ret, res->res);
		return 0;
	}

	logPrint(DEBUG_ERROR,"tcpRecvFix(%d,*,%d,%d)=%d (res=%d)",fd, sizeof(T_CLIENT_RESULT_S), G_ConfTbl->recv_timeout, ret, res->res);
	return -3;
}

int transSendClient(int fd, char *msgfmt, ...) {
	int ret;
	char mbuf[10240];
	va_list argptr;

	/* 可変引数文字列の編集  */
	va_start(argptr,msgfmt);
	vsprintf(mbuf,msgfmt,argptr);
	va_end(argptr);

	ret=tcpSend(fd, mbuf, strlen(mbuf), G_ConfTbl->send_timeout);
	logPrint(DEBUG_DEBUG,"tcpSend(%d,*,%d,%d)=%d",fd, strlen(mbuf), G_ConfTbl->send_timeout, ret);
	return ret;
}

int transSendResultS(int fd, int res_flg){
	int ret;
	T_CLIENT_RESULT_S res;

	res.res=res_flg;
	res.r_cnt=0;
	res.r_size=0;
	res.master_addr=daemon_GetMasterCltIP();

	ret=tcpSend(fd,(char *)&res,sizeof(res),G_ConfTbl->send_timeout);
	if ( ret < 0 ){
		logPrint(DEBUG_ERROR,"tcpSend(%d,*,%d,%d)=%d",fd, sizeof(res), G_ConfTbl->send_timeout, ret);
	}
	logPrint(DEBUG_DEBUG,"tcpSend(%d,*,%d,%d)=%d",fd, sizeof(res), G_ConfTbl->send_timeout, ret);
	return ret;
}

int transSendResultS(int fd, int res_flg, u_long recv_cnt, u_long recv_size){
	int ret;
	T_CLIENT_RESULT_S res;

	res.res=res_flg;
	res.r_cnt=recv_cnt;
	res.r_size=recv_size;
	res.master_addr=daemon_GetMasterCltIP();

	ret=tcpSend(fd,(char *)&res,sizeof(res),G_ConfTbl->send_timeout);
	if ( ret < 0 ){
		logPrint(DEBUG_ERROR,"tcpSend(%d,*,%d,%d)=%d",fd, sizeof(res), G_ConfTbl->send_timeout, ret);
	}
	logPrint(DEBUG_DEBUG,"tcpSend(%d,*,%d,%d)=%d",fd, sizeof(res), G_ConfTbl->send_timeout, ret);
	return ret;
}

int transSendResultM(int fd, int res_flg){
	int ret;
	T_CLIENT_RESULT_M res;

	res.res=res_flg;
	res.r_cnt=0;
	res.r_size=0;
	res.msg[0]=(char)NULL;
	res.master_addr=daemon_GetMasterCltIP();

	ret=tcpSend(fd,(char *)&res,sizeof(res),G_ConfTbl->send_timeout);
	logPrint(DEBUG_DEBUG,"tcpSend(%d,*,%d,%d)=%d",fd, sizeof(res), G_ConfTbl->send_timeout, ret);
	return ret;
}

int transSendResultM(int fd, int res_flg, u_long recv_cnt, u_long recv_size){
	int ret;
	T_CLIENT_RESULT_M res;

	res.res=res_flg;
	res.r_cnt=recv_cnt;
	res.r_size=recv_size;
	res.msg[0]=(char)NULL;
	res.master_addr=daemon_GetMasterCltIP();

	ret=tcpSend(fd,(char *)&res,sizeof(res),G_ConfTbl->send_timeout);
	logPrint(DEBUG_DEBUG,"tcpSend(%d,*,%d,%d)=%d",fd, sizeof(res), G_ConfTbl->send_timeout, ret);
	return ret;
}

int transSendResultM(int fd, int res_flg, char *msgfmt, ...){
	int ret;
	char mbuf[10240];
	va_list argptr;
	T_CLIENT_RESULT_M res;

	/* 可変引数文字列の編集  */
	va_start(argptr,msgfmt);
	vsprintf(mbuf,msgfmt,argptr);
	va_end(argptr);

	res.res=res_flg;
	res.r_cnt=0;
	res.r_size=0;
	strcpy(res.msg, mbuf);
	res.master_addr=daemon_GetMasterCltIP();

	ret=tcpSend(fd,(char *)&res,sizeof(res),G_ConfTbl->send_timeout);
	logPrint(DEBUG_DEBUG,"tcpSend(%d,*,%d,%d)=%d",fd, sizeof(res), G_ConfTbl->send_timeout, ret);
	return ret;
}

int transSendResultM(int fd, int res_flg, u_long recv_cnt, u_long recv_size, char *msgfmt, ...){
	int ret;
	char mbuf[10240];
	va_list argptr;
	T_CLIENT_RESULT_M res;

	/* 可変引数文字列の編集  */
	va_start(argptr,msgfmt);
	vsprintf(mbuf,msgfmt,argptr);
	va_end(argptr);

	res.res=res_flg;
	res.r_cnt=recv_cnt;
	res.r_size=recv_size;
	strcpy(res.msg, mbuf);
	res.master_addr=daemon_GetMasterCltIP();

	ret=tcpSend(fd,(char *)&res,sizeof(res),G_ConfTbl->send_timeout);
	logPrint(DEBUG_DEBUG,"tcpSend(%d,*,%d,%d)=%d",fd, sizeof(res), G_ConfTbl->send_timeout, ret);
	return ret;
}

int transCacheSyncMeta(T_SCN scn, u_long *out_cnt, T_META_SYNC_RESULT *out_buff){
	int ofd;
	int ret;
	long addr;
	T_CLIENT_REQUEST req;
	T_CLIENT_RESULT_S res;

	*out_cnt=0;

	//MASTERノード検索
	addr=daemon_GetMasterSvrIP();
	if ( addr == 0 ){ return -1; }

	//MASTERノード接続
	ofd=transConnect(addr,G_ConfTbl->network_port_cache);
	if ( ofd < 0 ){
		logPrint(DEBUG_ERROR,"transConnect(%s,%d)=%d",utlDisplyIP((u_char*)&addr),G_ConfTbl->network_port_cache,ofd);
		return -2; 
	}
	logPrint(DEBUG_DEBUG,"transConnect(%s,%d)=%d",utlDisplyIP((u_char*)&addr),G_ConfTbl->network_port_cache,ofd);

	//META情報転送要求
	commReqCheckSumSet(&req.ck);
	req.req=REQUEST_TASK_META;
	req.uid=getuid();
	req.gid=getgid();
	req.compression=0;
	memcpy(&(req.scn.scn), &scn, sizeof(T_SCN));
	ret=transSendBuff(ofd, (char*)&req, sizeof(req), &res);
	if ( ret <= 0 ){
		logPrint(DEBUG_ERROR,"transSendBuff(%d,*,%d,*)=%d",ofd,sizeof(req),ret);
		ret=tcpClose(ofd);
		logPrint(DEBUG_DEBUG,"tcpClose(%d)=%d",ofd, ret);
		return -3; 
	}

	//METAデータ受信
	ret=tcpRecvFix(ofd, (char*)out_buff, sizeof(T_META_SYNC_RESULT) * res.r_size , G_ConfTbl->recv_timeout);
	if ( ret != (sizeof(T_META_SYNC_RESULT) * res.r_size) ){
		logPrint(DEBUG_ERROR,"tcpRecvFix(%d,*,%d,%d)=%d",ofd, sizeof(T_META_SYNC_RESULT) * res.r_size,  G_ConfTbl->recv_timeout, ret);
		ret=tcpClose(ofd);
		logPrint(DEBUG_DEBUG,"tcpClose(%d)=%d",ofd, ret);
		return -4;
	}
	*out_cnt = res.r_size;
	logPrint(DEBUG_DEBUG,"tcpRecvFix(%d,*,%d,%d)=%d",ofd, sizeof(T_META_SYNC_RESULT) * res.r_size, G_ConfTbl->recv_timeout, ret);

	//METAデータ受信の応答
	ret=transSendResultS(ofd, REQUEST_CLIENT_OK);
	if ( ret < 0 ){
		*out_cnt = 0;
		logPrint(DEBUG_ERROR,"transSendResultS(%d,%d)=%d",ofd, REQUEST_CLIENT_OK, ret);
		ret=tcpClose(ofd);
		logPrint(DEBUG_DEBUG,"tcpClose(%d)=%d",ofd, ret);
		return -5;
	}
	logPrint(DEBUG_DEBUG,"transSendResultS(%d,%d)=%d",ofd, REQUEST_CLIENT_OK, ret);

	ret=tcpClose(ofd);
	logPrint(DEBUG_DEBUG,"tcpClose(%d)=%d",ofd, ret);
	return 1;
}

int transCacheSendMeta(int pno, int idx){
	int ofd;
	int ret;
	long addr;
	T_CLIENT_REQUEST req;
	T_CLIENT_RESULT_S res;

	//MASTERノード検索
	addr=daemon_GetMasterSvrIP();
	if ( addr == 0 ){ return -1; }

	//MASTERノード接続
	ofd=transConnect(addr,G_ConfTbl->network_port_cache);
	if ( ofd < 0 ){
		logPrint(DEBUG_ERROR,"transConnect(%s,%d)=%d",utlDisplyIP((u_char*)&addr),G_ConfTbl->network_port_cache,ofd);
		return -2; 
	}
	sprintf(G_ProcTbl[pno].para,"%s",utlDisplyIP((u_char*)&addr));
	logPrint(DEBUG_DEBUG,"transConnect(%s,%d)=%d",utlDisplyIP((u_char*)&addr),G_ConfTbl->network_port_cache,ofd);

	//METAキャッシュ転送
	commReqCheckSumSet(&req.ck);
	if ( G_MetaTbl[idx].req == REQUEST_CLIENT_MKDIR ){
		req.req=REQUEST_CACHE_META_CRE;
	}else{
		req.req=REQUEST_CACHE_META_ADD;
	}
	req.uid=getuid();
	req.gid=getgid();
	req.compression=0;
	memcpy(&(req.meta.inf), &(G_MetaTbl[idx].inf), sizeof(T_META_INFO));
	memcpy(&(req.meta.dir_id), &(G_MetaTbl[idx].row.dir_id), sizeof(T_FILE_ID));
	ret=transSendBuff(ofd, (char*)&req, sizeof(req), &res);
	if ( ret <= 0 ){
		logPrint(DEBUG_ERROR,"transSendBuff(%d,*,%d,*)=%d",ofd,sizeof(req),ret);
		G_ProcTbl[pno].para[0]=(char)NULL;
		ret=tcpClose(ofd);
		logPrint(DEBUG_DEBUG,"tcpClose(%d)=%d",ofd, ret);
		return -3; 
	}

	G_ProcTbl[pno].para[0]=(char)NULL;
	ret=tcpClose(ofd);
	logPrint(DEBUG_DEBUG,"tcpClose(%d)=%d",ofd, ret);
	return 0;
}

int transCacheSendIndexCre(int pno, T_FILE_ID dir_id, T_FILE_ID file_id){
	int ofd;
	int ret;
	long addr;
	T_CLIENT_REQUEST req;
	T_CLIENT_RESULT_S res;

	//MASTERノード検索
	addr=daemon_GetMasterSvrIP();
	if ( addr == 0 ){ return -1; }

	//MASTERノード接続
	ofd=transConnect(addr,G_ConfTbl->network_port_cache);
	if ( ofd < 0 ){
		logPrint(DEBUG_ERROR,"transConnect(%s,%d)=%d",utlDisplyIP((u_char*)&addr),G_ConfTbl->network_port_cache,ofd);
		return -2; 
	}
	sprintf(G_ProcTbl[pno].para,"%s",utlDisplyIP((u_char*)&addr));
	logPrint(DEBUG_DEBUG,"transConnect(%s,%d)=%d",utlDisplyIP((u_char*)&addr),G_ConfTbl->network_port_cache,ofd);

	//INDEXキャッシュ転送
	commReqCheckSumSet(&req.ck);
	req.req=REQUEST_CACHE_IDX_CRE;
	req.uid=getuid();
	req.gid=getgid();
	req.compression=0;
	memcpy(&(req.row.dir_id), &file_id, sizeof(T_FILE_ID));
	memcpy(&(req.row.file_id), &file_id, sizeof(T_FILE_ID));
	ret=transSendBuff(ofd, (char*)&req, sizeof(req), &res);
	if ( ret <= 0 ){
		logPrint(DEBUG_ERROR,"transSendBuff(%d,*,%d,*)=%d",ofd,sizeof(req),ret);
		G_ProcTbl[pno].para[0]=(char)NULL;
		ret=tcpClose(ofd);
		logPrint(DEBUG_DEBUG,"tcpClose(%d)=%d",ofd, ret);
		return -3; 
	}

	G_ProcTbl[pno].para[0]=(char)NULL;
	ret=tcpClose(ofd);
	logPrint(DEBUG_DEBUG,"tcpClose(%d)=%d",ofd, ret);
	return 0;
}

int transCacheSendIndexAdd(int pno, T_FILE_ID dir_id, T_FILE_ID file_id, u_long block_no, u_long size){
	int ofd;
	int ret;
	long addr;
	T_CLIENT_REQUEST req;
	T_CLIENT_RESULT_S res;

	//MASTERノード検索
	addr=daemon_GetMasterSvrIP();
	if ( addr == 0 ){ return -1; }

	//MASTERノード接続
	ofd=transConnect(addr,G_ConfTbl->network_port_cache);
	if ( ofd < 0 ){
		logPrint(DEBUG_ERROR,"transConnect(%s,%d)=%d",utlDisplyIP((u_char*)&addr),G_ConfTbl->network_port_cache,ofd);
		return -2; 
	}
	sprintf(G_ProcTbl[pno].para,"%s",utlDisplyIP((u_char*)&addr));
	logPrint(DEBUG_DEBUG,"transConnect(%s,%d)=%d",utlDisplyIP((u_char*)&addr),G_ConfTbl->network_port_cache,ofd);

	//INDEXキャッシュ転送
	commReqCheckSumSet(&req.ck);
	req.req=REQUEST_CACHE_IDX_ADD;
	req.uid=getuid();
	req.gid=getgid();
	req.compression=0;
	memcpy(&(req.block.dir_id), &dir_id, sizeof(T_FILE_ID));
	memcpy(&(req.block.file_id), &file_id, sizeof(T_FILE_ID));
	req.block.block_no=block_no;
	req.block.size=size;
	memcpy(req.block.node_id, G_NodeTbl[0].node_id, NODE_ID_LEN);
	ret=transSendBuff(ofd, (char*)&req, sizeof(req), &res);
	if ( ret <= 0 ){
		logPrint(DEBUG_ERROR,"transSendBuff(%d,*,%d,*)=%d",ofd,sizeof(req),ret);
		G_ProcTbl[pno].para[0]=(char)NULL;
		ret=tcpClose(ofd);
		logPrint(DEBUG_DEBUG,"tcpClose(%d)=%d",ofd, ret);
		return -3; 
	}
	logPrint(DEBUG_DEBUG,"transSendBuff(%d,*,%d,*)=%d",ofd,sizeof(req),ret);

	G_ProcTbl[pno].para[0]=(char)NULL;
	ret=tcpClose(ofd);
	logPrint(DEBUG_DEBUG,"tcpClose(%d)=%d",ofd, ret);
	return 0;
}

int transCacheSendData(int pno, int idx){
	int ofd;
	int ret;
	int oret;
	long addr;
	u_long file_idx;
	int cache_cnt=0;
	int rflg=0;
	T_CLIENT_REQUEST req;
	T_CLIENT_RESULT_S res;
	T_ZTRANS_INFO zpara;

	//接続先選択
	addr=transConnectServerChoice();
	if ( addr == 0 ){ return 0; }

	//他ノード接続
	ofd=transConnect(addr,G_ConfTbl->network_port_cache);
	if ( ofd < 0 ){
		logPrint(DEBUG_ERROR,"transConnect(%s,%d)=%d",utlDisplyIP((u_char*)&addr),G_ConfTbl->network_port_cache,ofd);
		return -1; 
	}
	sprintf(G_ProcTbl[pno].para,"%s",utlDisplyIP((u_char*)&addr));
	logPrint(DEBUG_DEBUG,"transConnect(%s,%d)=%d",utlDisplyIP((u_char*)&addr),G_ConfTbl->network_port_cache,ofd);

	//DATAキャッシュ転送
	file_idx=G_IndexTbl[idx].file_idx;
	commReqCheckSumSet(&req.ck);
	req.req=REQUEST_CACHE_DATA;
	req.uid=G_NodeTbl[0].run_userid;
	req.gid=G_NodeTbl[0].run_groupid;
	memcpy(&(req.meta.inf), &(G_MetaTbl[file_idx].inf), sizeof(T_META_INFO));
	memcpy(&(req.meta.dir_id), &(G_MetaTbl[file_idx].row.dir_id), sizeof(T_FILE_ID));
	req.meta.block_no=G_IndexTbl[idx].block_no;
	req.meta.size=G_IndexTbl[idx].size;
	ret=transSendBuff(ofd, (char*)&req, sizeof(T_CLIENT_REQUEST), &res);
	if ( ret <= 0 ){
		logPrint(DEBUG_ERROR,"transSendBuff(%d,*,%d,*)=%d",ofd,sizeof(T_CLIENT_REQUEST),ret);
		G_ProcTbl[pno].para[0]=(char)NULL;
		ret=tcpClose(ofd); 
		logPrint(DEBUG_DEBUG,"tcpClose(%d)=%d",ofd, ret);
		return -2; 
	}

    //圧縮初期化
    memset(&zpara,0,sizeof(T_ZTRANS_INFO));
    zpara.file_size=req.meta.size;
    zpara.buff_size=G_ConfTbl->network_buff_size;
    zpara.stmout=G_ConfTbl->send_timeout;
    zpara.rtmout=G_ConfTbl->recv_timeout;
    ztrans_comp_init(&zpara, G_ConfTbl->con_compression);

	//転送
    rflg=1;
    while(1){
        oret=ztrans_data_send(&zpara, G_DataTbl[idx].data, req.meta.size, &cache_cnt, ofd);
        if ( oret == 0 ){ break; }
        if ( oret < 0 ){
            logPrint(DEBUG_ERROR,"ztrans_data_send()=%d",oret);
            rflg=-2;
            break;
        }
    }

    ztrans_comp_fin(&zpara);

	G_ProcTbl[pno].para[0]=(char)NULL;
	ret=tcpClose(ofd);
	logPrint(DEBUG_DEBUG,"tcpClose(%d)=%d",ofd, ret);
	return rflg;
}
