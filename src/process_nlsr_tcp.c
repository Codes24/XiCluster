/*****************************************************************************
*  <システム>   分散処理フレームワーク
*  <名称>	ノード情報転送デーモン(TCP版)
*  <目的>	自ノード情報の転送とノード情報の受信を行う
*  <機能>	   
*  <開発環境>   UNIX
*  <特記事項>
*
*  VERSION	DATE		BY				CHANGE/COMMENT
*  -----------------------------------------------------------------------------
*  V0.00	  2014/01/18  Takakusaki		新規作成
******************************************************************************/
#include "xi_server.h"

static int l_server_fd;

static u_long *server_list;
static int   server_cnt=0;

//自ノード情報を全クラスターに転送(CRCVプロセスに転送)
static int ServerInfPushTCPshare(int reqid){
	int ofd;
	int ret;
	T_CLIENT_REQUEST req;
	T_CLIENT_RESULT_S res;
	char ipbuf[128];

	//リクエスト
	time( &(G_NodeTbl[0].alv) );
	commReqCheckSumSet(&req.ck);
	req.req=reqid;
	req.uid=G_NodeTbl[0].run_userid;
	req.gid=G_NodeTbl[0].run_groupid;
	memcpy((char*)&(req.node.inf), (char*)&(G_NodeTbl[0]), sizeof(T_NODE_TABLE) );

	//各ノードにTCP転送
	for(int i=0; i<server_cnt; i++){
		if ( memcmp((char*)&(server_list[i]), (char*)&(G_NodeTbl[0].svr_ip), sizeof(long)) == 0 ){ continue; }
		
		ofd=tcpConnect(server_list[i],G_ConfTbl->network_port_svr,G_ConfTbl->ping_timeout,1,0);
		if ( ofd < 0 ){ continue; }
		ret=transSendBuff(ofd, (char*)&req, sizeof(T_CLIENT_REQUEST), &res);
		tcpClose(ofd);

		//logPrint(DEBUG_INFO,"[%d] send node request IP=%s REQ=%d RET=%d",i,utlDisplyIP((u_char*)&(server_list[i]),ipbuf),reqid,ret);
	}


	return 0;
}


//自ノード情報を全クラスターに転送(NLSRプロセスに転送)
static int ServerInfPushTCPown(int reqid){
	int ofd;
	int ret;
	T_NODE_DATA_REQUEST nodereq;

	//転送情報
	time( &(G_NodeTbl[0].alv) );
	nodereq.req=reqid;
	memcpy((char*)&(nodereq.inf), (char*)&(G_NodeTbl[0]), sizeof(T_NODE_TABLE) );

	//各ノードにTCP転送
	for(int i=0; i<server_cnt; i++){
		if ( memcmp((char*)&(server_list[i]), (char*)&(G_NodeTbl[0].svr_ip), sizeof(long)) == 0 ){ continue; }

		ofd=tcpConnect(server_list[i],G_ConfTbl->network_port_svr,G_ConfTbl->ping_timeout,1,0);
		if ( ofd < 0 ){ continue; }
		ret=tcpSend(ofd, (char*)&(nodereq), sizeof(T_NODE_DATA_REQUEST), G_ConfTbl->send_timeout);
		tcpClose(ofd);
	}

	return 0;
}

static int ServerInfPushTCP(int reqid){
	if ( G_ConfTbl->network_port_svr == G_ConfTbl->network_port_cache ){
		return ServerInfPushTCPshare(reqid);
	}else{
		return ServerInfPushTCPown(reqid);
	}
}


int process_nlsr_tcp_init(){
	int ret;
	int mtu_size;
	int v1,v2;
	char ipbuf[128];
	char sfilename[MAX_FILE_PATH+1];
	char buff[20480];
	FILE *fp;
	u_long addr;

	////////////////////////////////////////////
	//サーバリスト読込み
	////////////////////////////////////////////
	server_list=(u_long*)malloc( sizeof(long) * G_ConfTbl->max_node );
	sprintf(sfilename,"%s/conf/server.lst",G_BaseDir);
	if ( filisFile(sfilename) == 1 ){
		logPrint(DEBUG_INFO,"%s",sfilename);
	}else{
		logPrint(DEBUG_ERROR,"not found server list(%s)",sfilename);
	}
	if ( (fp=fopen(sfilename,"r")) == NULL ){
		logPrint(DEBUG_ERROR,"file open error(%s)",sfilename);
	}else{
		while( fgets(buff,sizeof(buff),fp) != NULL ){
			if ( (addr=utlHostnameToAddress(buff)) == 0 ){ continue; }
			for(int i=0; i<server_cnt; i++){
				if ( server_list[i] == addr ){ addr=0; break; }
			}
			if ( addr == 0 ){ continue; }
			server_list[server_cnt]=addr;
			server_cnt++;
		}
		fclose(fp);
	}

	//ポート番号が同じ場合はキャッシュプロセスがノード要求を処理
	if ( G_ConfTbl->network_port_svr == G_ConfTbl->network_port_cache ){ return 0; }


	////////////////////////////////////////////
	//ノード間接続受付
	////////////////////////////////////////////
	l_server_fd=tcpListen(G_NodeTbl[0].svr_ip, G_ConfTbl->network_port_svr);
	if ( l_server_fd < 0 ){
		logPrint(DEBUG_ERROR,"client connector listen(%s:%d) fd=%d",
			utlDisplyIP((u_char*)&G_NodeTbl[0].svr_ip,ipbuf), G_ConfTbl->network_port_svr, l_server_fd);
		exit(1);
	}
	logPrint(DEBUG_INFO,"client connector listen(%s:%d) fd=%d",
			utlDisplyIP((u_char*)&G_NodeTbl[0].svr_ip,ipbuf), G_ConfTbl->network_port_svr, l_server_fd);

	//MTUサイズ取得
	mtu_size=tcpGetMTU(l_server_fd, G_ConfTbl->network_if_svr);
	if ( mtu_size < 0 ){
		logPrint(DEBUG_WARNING,"tcpGetMTU(%d,%s)=%d",l_server_fd, G_ConfTbl->network_if_svr, mtu_size);
		//exit(1);
	}else{
		G_NodeTbl[0].svr_mtu=mtu_size;
		logPrint(DEBUG_DEBUG,"tcpGetMTU(%d,%s)=%d",l_server_fd, G_ConfTbl->network_if_svr, mtu_size);
	}

	//送受信バッファ数
	ret=tcpGetBuffSize(l_server_fd, &v1, &v2);
	logPrint(DEBUG_DEBUG,"tcpGetBuffSize(%d,%d,%d)=%d",l_server_fd, v1, v2,ret);

	//送受信バッファ設定
	ret=tcpSetBuffSize(l_server_fd, G_ConfTbl->so_sndbuf, G_ConfTbl->so_rcvbuf);
	if ( ret < 0 ){
		logPrint(DEBUG_ERROR,"tcpSetBuffSize(%d,%d,%d)=%d",
				l_server_fd, G_ConfTbl->so_sndbuf, G_ConfTbl->so_rcvbuf, ret);
		exit(1);
	}
	logPrint(DEBUG_DEBUG,"tcpSetBuffSize(%d,%d,%d)=%d", l_server_fd, G_ConfTbl->so_sndbuf, G_ConfTbl->so_rcvbuf, ret);

	//送受信バッファ数
	ret=tcpGetBuffSize(l_server_fd, &v1, &v2);
	logPrint(DEBUG_DEBUG,"tcpGetBuffSize(%d,%d,%d)=%d",l_server_fd, v1, v2,ret);

	return 0;
}

int process_nlsr_tcp(){
	int i,j;
	int ret;
	int r_ret;
	int cno;
	T_NODE_DATA_REQUEST nodereq;
	static time_t last_broadcast=0;
	static time_t last_nodetbl_write=0;
	time_t tm;
	int l_client_fd;

	time(&tm);

	//////////////////////////////////////////////////////////////////
	//自ノード状態を他サーバに伝達
	//////////////////////////////////////////////////////////////////
	if ( tm > (last_broadcast + G_ConfTbl->data_trans_interval) ){
		ret=ServerInfPushTCP(REQUEST_NODE_INFO);
		time(&last_broadcast);
	}

	//////////////////////////////////////////////////////////////////
	//ノード情報をファイル出力
	//////////////////////////////////////////////////////////////////
	if ( tm > (last_nodetbl_write + G_ConfTbl->nodetbl_write_interval) ){
		ret=daemon_NodeTableWrite();
		if ( ret != 0 ){
			logPrint(DEBUG_ERROR,"NodeTableWrite() => %d",ret);
		}
		time(&last_nodetbl_write);
	}


	//ポート番号が同じ場合はキャッシュプロセスがノード要求を処理
	if ( G_ConfTbl->network_port_svr == G_ConfTbl->network_port_cache ){ return 0; }


	//////////////////////////////////////////////////////////////////
	//他サーバからのデータ受信
	//////////////////////////////////////////////////////////////////
	r_ret=0;
	if ( tcpSelect(l_server_fd,0,G_ConfTbl->select_timeout) == 1 ){
		l_client_fd=tcpAccept(l_server_fd);
		//logPrint(DEBUG_DEBUG,"tcpAccept(%d) => %d",l_server_fd,l_client_fd);
		if ( l_client_fd > 0 ){
			r_ret=tcpRecvFix(l_client_fd,(char *)&nodereq,sizeof(T_NODE_DATA_REQUEST),G_ConfTbl->ping_timeout);
			//logPrint(DEBUG_DEBUG,"tcpRecv()=%d req=%d",r_ret, nodereq.req);
			ret=tcpClose(l_client_fd);
		}
	}

	//////////////////////////////////////////////////////////////////
	//受信データをメモリに反映
	//////////////////////////////////////////////////////////////////
	if ( r_ret == sizeof(T_NODE_DATA_REQUEST) ){
		//ノード情報更新
		if ( nodereq.req == REQUEST_NODE_INFO || nodereq.req == REQUEST_NODE_STOP ){
			cno=-1;
			for (i=0; i<G_ConfTbl->max_node; i++){
				if ( G_NodeTbl[i].svr_ip == nodereq.inf.svr_ip ){ cno=i; break; }
				if ( G_NodeTbl[i].svr_ip == 0 ){ cno=i; break; }
			}
			if ( cno > 0 ){
				cacheSetNodeStatus(cno, nodereq.inf.sts);
				cacheSetMasterFlag(cno, nodereq.inf.master_flg);
				memcpy((char*)&(G_NodeTbl[cno]), (char*)&(nodereq.inf), sizeof(T_NODE_TABLE));
			}
		}
	}

	return 0;
}

int process_nlsr_tcp_fin(){
	int ret;
	T_NODE_DATA_REQUEST nodereq;

	//停止通知
	cacheSetNodeStatus(0, SYSTEM_STATUS_FIN);
	ret=ServerInfPushTCP(REQUEST_NODE_STOP);

	//ポート番号が同じ場合はキャッシュプロセスがノード要求を処理
	if ( G_ConfTbl->network_port_svr == G_ConfTbl->network_port_cache ){ return 0; }

	//ポートクローズ
	ret=tcpClose(l_server_fd);
	logPrint(DEBUG_DEBUG,"tcpClose(%d) => %d",l_server_fd, ret);
	return 0;
}
