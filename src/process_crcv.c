/*****************************************************************************
*  <システム>   分散処理フレームワーク
*  <名称>	キャッシュ要求受付デーモン
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

static int  l_server_fd;
static int  l_client_fd;
static char l_sock_file[MAX_FILE_PATH+1];
static int  l_gate_fd;

int process_crcv_init(){
	int ret;
	int mtu_size;
	char ipbuf[128];
	int v1,v2;

	//ノード間接続受付
	l_server_fd=tcpListen(G_NodeTbl[0].svr_ip, G_ConfTbl->network_port_cache);
	if ( l_server_fd < 0 ){
		logPrint(DEBUG_ERROR,"client connector listen(%s:%d) fd=%d",
			utlDisplyIP((u_char*)&G_NodeTbl[0].svr_ip,ipbuf),
			G_ConfTbl->network_port_cache, l_server_fd);
		exit(1);
	}
	logPrint(DEBUG_INFO,"client connector listen(%s:%d) fd=%d",
			utlDisplyIP((u_char*)&G_NodeTbl[0].svr_ip,ipbuf),
			G_ConfTbl->network_port_cache, l_server_fd);

	//MTUサイズ取得
	mtu_size=tcpGetMTU(l_server_fd, G_ConfTbl->network_if_svr);
	if ( mtu_size < 0 ){
		logPrint(DEBUG_WARNING,"tcpGetMTU(%d,%s)=%d", l_server_fd, G_ConfTbl->network_if_svr, mtu_size);
		//exit(1);
	}else{
		G_NodeTbl[0].cache_mtu=mtu_size;
		logPrint(DEBUG_DEBUG,"tcpGetMTU(%d,%s)=%d", l_server_fd, G_ConfTbl->network_if_svr, mtu_size);
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
	logPrint(DEBUG_DEBUG,"tcpSetBuffSize(%d,%d,%d)=%d",
			l_server_fd, G_ConfTbl->so_sndbuf, G_ConfTbl->so_rcvbuf, ret);

	//送受信バッファ数
	ret=tcpGetBuffSize(l_server_fd, &v1, &v2);
	logPrint(DEBUG_DEBUG,"tcpGetBuffSize(%d,%d,%d)=%d",l_server_fd, v1, v2,ret);

	return 0;
}

int process_crcv(){
	int i,j;
	int ret;
	int free_no;
	T_CLIENT_REQUEST req;

	//受信受付
	if ( tcpSelect(l_server_fd,0,G_ConfTbl->select_timeout) != 1 ){ return 0; }
	l_client_fd=tcpAccept(l_server_fd);
	logPrint(DEBUG_DEBUG,"tcpAccept(%d) => %d",l_server_fd,l_client_fd);
	if ( l_client_fd == 0 ){ return 0; }

	//空きTASKプロセス検索
	free_no=cacheGetFreeTaskNo();
	if ( free_no < 0 ){
		logPrint(DEBUG_WARNING,"task is busy (no slot)");
		tcpClose(l_client_fd);
		return 0;
	}

	//TASKプロセスに割り振る
	sprintf(l_sock_file,"%s/ud%03d.sock",G_TempDir,free_no);
	l_gate_fd=udConnect(l_sock_file);
	ret=udSend(l_gate_fd, l_client_fd, &req, sizeof(req),G_ConfTbl->send_timeout);
	logPrint(DEBUG_DEBUG,"udSend(%d,%d,*,%d,%d) => %d",
			l_gate_fd, l_client_fd,sizeof(req),G_ConfTbl->send_timeout,ret);
	tcpClose(l_client_fd);

	return 0;
}

int process_crcv_fin(){
	int ret;
	ret=tcpClose(l_server_fd);
	logPrint(DEBUG_DEBUG,"tcpClose(%d) => %d",l_server_fd, ret);
	return 0;
}
