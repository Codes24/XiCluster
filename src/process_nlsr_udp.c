/*****************************************************************************
*  <システム>   分散処理フレームワーク
*  <名称>	ノード情報転送デーモン(UDP版)
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

//自ノード情報を全クラスターに転送
static int ServerInfPushUDP(int req){
	int ofd;
	int ret;
	T_NODE_DATA_REQUEST nodereq;

	//転送情報
	time( &(G_NodeTbl[0].alv) );
	nodereq.req=req;
	memcpy((char*)&(nodereq.inf), (char*)&(G_NodeTbl[0]), sizeof(T_NODE_TABLE) );

	//UDPブロードキャスト
	ret=udpBroadcast(G_ConfTbl->network_port_svr, (char*)&nodereq, sizeof(T_NODE_DATA_REQUEST) );
	if ( ret != 0 ){ logPrint(DEBUG_ERROR,"udpBroadcast() => %d",ret); }

	return 0;
}

int process_nlsr_udp_init(){
	int ret;

	////////////////////////////////////////////
	//UDP通信
	////////////////////////////////////////////
	l_server_fd=udpBind(G_ConfTbl->network_port_svr);
	if ( l_server_fd < 0 ){
		logPrint(DEBUG_ERROR,"node connector listen(%s:%d) fd=%d",
				utlDisplyIP((u_char*)&G_NodeTbl[0].svr_ip), G_ConfTbl->network_port_svr, l_server_fd);
		exit(1);
	}
	logPrint(DEBUG_INFO,"node connector listen (%s:%d) fd=%d",
		utlDisplyIP((u_char*)&G_NodeTbl[0].svr_ip), G_ConfTbl->network_port_svr, l_server_fd);

	return 0;
}

int process_nlsr_udp(){
	int i,j;
	int ret;
	int r_ret;
	int cno;
	T_NODE_DATA_REQUEST nodereq;
	static time_t last_broadcast=0;
	static time_t last_nodetbl_write=0;
	time_t tm;

	time(&tm);

	//////////////////////////////////////////////////////////////////
	//自ノード状態を他サーバに伝達
	//////////////////////////////////////////////////////////////////
	if ( tm > (last_broadcast + G_ConfTbl->data_trans_interval) ){
		ret=ServerInfPushUDP(REQUEST_NODE_INFO);
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

	//////////////////////////////////////////////////////////////////
	//他サーバからのデータ受信
	//////////////////////////////////////////////////////////////////
	ret=0;
	r_ret=udpRecv(l_server_fd,(char *)&nodereq,sizeof(T_NODE_DATA_REQUEST),G_ConfTbl->node_recv_timeout);
	//logPrint(DEBUG_DEBUG,"udpRecv()=%d req=%d",r_ret, nodereq.req);

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

int process_nlsr_udp_fin(){
	int ret;
	T_NODE_DATA_REQUEST nodereq;

	//停止通知
	cacheSetNodeStatus(0, SYSTEM_STATUS_FIN);
	ret=ServerInfPushUDP(REQUEST_NODE_STOP);

	//
	ret=udpClose(l_server_fd);
	logPrint(DEBUG_DEBUG,"udpClose(%d) => %d",l_server_fd, ret);
	return 0;
}
