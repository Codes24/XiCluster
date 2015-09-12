/*****************************************************************************
*  <システム>   分散処理フレームワーク
*  <名称>	Client APIライブラリ
*  <目的>
*  <機能>
*  <開発環境>   UNIX
*  <特記事項>
*
*  VERSION  DATE		BY			  CHANGE/COMMENT
*  -----------------------------------------------------------------------------
*  V0.00	  2014/07/14  Takakusaki		新規作成
******************************************************************************/
#include "xi_common.h"
#include "xi_client.h"

int xi_errno;
char xi_message[2048];

static T_ZTRANS_INFO zpara;
static T_CONFIG_TABLE l_para;
static long l_addr_first;
static long l_addr_master;
static int  l_fd=(-1);
static int  l_init_flg=0;
static int  l_mode=(-1);

static int client_recv_fix(int l_fd, T_CONFIG_TABLE *l_para, char *buff, int size){
	int ret;
	int recv_size=0;
	T_CLIENT_RESULT_M res;

	if ( l_fd < 0 ){ return -1; }

	//受信
	recv_size=tcpRecvFix(l_fd,(char*)buff,size,l_para->recv_timeout);
	if ( recv_size < 0 ){ return -2; }

	//応答送信
	res.res=REQUEST_CLIENT_OK;
	res.r_cnt=0;
	res.r_size=0;
	res.master_addr=0;
	res.compression=0;
	ret=tcpSend(l_fd,(char*)&res,sizeof(T_CLIENT_RESULT_M),l_para->send_timeout);
	if ( ret < 0 ){ return -3; }

	return recv_size;
}

static int client_req_send(int l_fd, T_CONFIG_TABLE *l_para, int req_no, char *para1, char *para2, int compression, u_long fsize, T_CLIENT_RESULT_M *out_res){
	int ret;
	T_CLIENT_REQUEST req;

	memset((char*)out_res,0,sizeof(T_CLIENT_RESULT_M));

	if ( l_fd < 0 ){ return -1; }

	//リクエスト
	memset((char*)&req,0,sizeof(req));
	commReqCheckSumSet(&req.ck);
	req.req=req_no;
	req.uid=getuid();
	req.gid=getgid();
	req.compression=compression;
	req.clt.size=fsize;
	strcpy(req.clt.para1, para1); 
	strcpy(req.clt.para2, para2); 

	//リクエスト送信
	ret=tcpSend(l_fd,(char*)&req,sizeof(req),l_para->send_timeout);
	if ( ret < 0 ){ return -2; }

	//応答
	ret=tcpRecvFix(l_fd,(char*)out_res,sizeof(T_CLIENT_RESULT_M),l_para->recv_timeout);
	if ( ret <= 0 ){ return -3; }
	if ( out_res->res == REQUEST_CLIENT_OK ){ return 0; }
	return -4;
}

static int client_connect(T_CONFIG_TABLE *l_para, T_CLIENT_RESULT_M *out_res){
	int l_fd;
	int ret;

	memset((char*)out_res,0,sizeof(T_CLIENT_RESULT_M));

	//サーバ接続
	if ( (l_fd=tcpConnect(l_addr_first, l_para->network_port_clt, l_para->con_timeout, 1, 0)) < 0 ){ return -1; }

	//サーバ情報を取得
	if ( client_req_send(l_fd, l_para, REQUEST_CLIENT_PING,"","",0,0, out_res) != 0 ){ return -2; }
	tcpClose(l_fd);
	l_addr_master=out_res->master_addr;

	//マスターサーバへ接続
	if ( (l_fd=tcpConnect(l_addr_master, l_para->network_port_clt, l_para->con_timeout,1,0)) < 0 ){ return -3; }

	return l_fd;
}

static int client_req_recv(int l_fd, T_CONFIG_TABLE *l_para, int req_no, char *para1, char *para2, T_CLIENT_RESULT_M *out_res){
	int ret;
	int o_flg=0;
	char buff[l_para->network_buff_size + sizeof(T_CLIENT_DATA_HEADER) + 1024];

	memset((char*)out_res,0,sizeof(T_CLIENT_RESULT_M));

	if ( l_fd < 0 ){ return -1; }

	//リクエスト送信
	if ( (ret=client_req_send(l_fd, l_para, req_no, para1, para2, 0, 0, out_res)) < 0 ){ return -2; }

	//結果受信
	while(1){
		ret=tcpRecv(l_fd,buff,sizeof(buff),l_para->recv_timeout);
		if ( ret <= 0 ){ break; }
		write(1,buff,ret);
		o_flg=1;
	}

	return 0;
}

static int client_close(int l_fd){
	int ret;

	if ( l_fd < 0 ){ return -1; }

	ret=tcpClose(l_fd);
	l_fd=(-1);
	return 0;
}

static int xi_init(){
	char hostname[1024]="";
	struct hostent *hserv;
	char ipaddr[1024]="";

	xi_message[0]=NULL;
	if ( l_init_flg != 0 ){ return 0; }

	//パラメータ読込み
	paraSetDefault(&l_para);

	gethostname(hostname, sizeof(hostname) );
	hserv=gethostbyname(hostname);
	utlDisplyIP((u_char*)hserv->h_addr,ipaddr);

	l_addr_first=utlStrIP2long(ipaddr);
	l_init_flg=1;
	l_fd=(-1);
	l_mode=(-1);

	return 0;
}

static int xi_connect(){
	int ret;
	T_CLIENT_RESULT_M res;

	//既に接続済み
	if ( l_fd >= 0 ){
		xi_errno=XI_ERRNO_OPENED;
		strcpy(xi_message,"already opened");
		return -1; 
	}

	//サーバ接続
	l_fd=client_connect(&l_para, &res);
	if ( l_fd < 0 ){
		xi_errno=XI_ERRNO_OPEN;
		if ( res.msg[0] == NULL ){
			strcpy(xi_message, "connect error");
		}else{
			strcpy(xi_message, res.msg);
		}
		return -2; 
	}
	return 0;
}

static int xi_send(int req_no, char *para1, char *para2, int compression, u_long fsize){
	int ret;
	T_CLIENT_RESULT_M res;

	//未接続
	if ( l_fd < 0 ){
		xi_errno=XI_ERRNO_UNOPEN;
		strcpy(xi_message,"It is not connected");
		return -1;
	}

	//送信
	ret=client_req_send(l_fd, &l_para, req_no, para1, para2, compression, fsize, &res);
	strcpy(xi_message, res.msg);
	if ( ret < 0 ){
		xi_errno=XI_ERRNO_SEND;
		if ( xi_message[0] == NULL ){
			strcpy(xi_message, "send error");
		}
		return -2;
	}
	return 0;
}

static int xi_recv(int typ, char *para1, char *para2){
	int ret;
	T_CLIENT_RESULT_M res;

	//未接続
	if ( l_fd < 0 ){
		xi_errno=XI_ERRNO_UNOPEN;
		strcpy(xi_message,"It is not connected");
		return -1;
	}

	//受信
	if ( (ret=client_req_recv(l_fd, &l_para, typ, para1, para2, &res)) < 0 ){
		xi_errno=XI_ERRNO_RECV;
		if ( res.msg[0] == NULL ){
			strcpy(xi_message, "recv error");
		}else{
			strcpy(xi_message, res.msg);
		}
		return -2;
	}
	return 0;
}

static int xi_recv_fix(char *buff, int buff_size){
	int ret;
	T_CLIENT_RESULT_M res;

	//未接続
	if ( l_fd < 0 ){
		xi_errno=XI_ERRNO_UNOPEN;
		strcpy(xi_message,"It is not connected");
		return -1;
	}

	//受信
	ret=client_recv_fix(l_fd,&l_para,buff,buff_size);
	if ( ret != buff_size ){
		xi_errno=XI_ERRNO_RECV;
		if ( res.msg[0] == NULL ){
			strcpy(xi_message, "recv error");
		}else{
			strcpy(xi_message, res.msg);
		}
		return -2;
	}

	return 0;
}

static int xi_error_fin(){
	int ret;

	if ( l_fd >= 0 ){
		ret=client_close(l_fd);
		l_fd=(-1);
	}
	return 0;
}

static int xi_fin(){
	int ret;

	if ( l_fd < 0 ){
		xi_errno=XI_ERRNO_CLOSED;
		strcpy(xi_message,"already closed");
		return -1;
	}

	//クローズ
	ret=client_close(l_fd);
	l_fd=(-1);
	if ( ret < 0 ){
		xi_errno=XI_ERRNO_CLOSE;
		strcpy(xi_message,"close error");
		return -2;
	}
	return 0;
}

int xi_set(char *p1, char *p2){
	return paraModPara(&l_para,p1,p2);
}

int xi_set(int typ, int para){
	xi_init();
	if ( typ == SET_PARAMETER_PORT ){ l_para.network_port_clt=para; }
	return 0;
}

int xi_set(int typ, char *para){
	xi_init();
	if ( typ == SET_PARAMETER_IPADDR ){ l_addr_first=utlStrIP2long(para); }
	return 0;
}

int xi_open(char *hdfs_file, int mod){
	int ret;
	int req_no;
	int comp_level=l_para.con_compression;

	xi_init();

	comp_level = mod & 0xFFFF;
	mod &= 0xFFFF0000;

	//パラメータチェック
	if ( mod == XI_READ ){
		req_no=REQUEST_CLIENT_GET;
	}else if ( mod == XI_WRITE ){
		req_no=REQUEST_CLIENT_PUT;	
	}else{
		xi_errno=XI_ERRNO_PARA;
		strcpy(xi_message,"parameter Injustice");
		return -1;
	}
	l_mode=mod;

	if ( l_fd >= 0 ){
		xi_errno=XI_ERRNO_PHASE;
		strcpy(xi_message,"phase error");
		return -1;
	}

	//サーバ接続
	if ( (ret=xi_connect()) < 0 ){ return -3; }

	//リクエスト送信
	if ( (ret=xi_send(req_no, hdfs_file,"", comp_level, 0)) < 0 ){ xi_error_fin(); return -4; }

	//圧縮レベル設定
	memset(&zpara,0,sizeof(T_ZTRANS_INFO));
	zpara.file_size=0;
	zpara.buff_size=l_para.network_buff_size;
	zpara.stmout=l_para.send_timeout;
	zpara.rtmout=l_para.recv_timeout;
	if ( l_mode == XI_WRITE ){ ztrans_comp_init(&zpara, l_para.con_compression); }
	if ( l_mode == XI_READ ){ ztrans_decomp_init(&zpara); }

	return l_fd;
}

int xi_open(char *hdfs_file, int mod, ulong fsize){
	int ret;
	int comp_level=l_para.con_compression;
	int req_no;

	xi_init();

	comp_level = mod & 0xFFFF;
	mod &= 0xFFFF0000;

	//パラメータチェック
	if ( mod == XI_READ ){
		req_no=REQUEST_CLIENT_GET;
	}else if ( mod == XI_WRITE ){
		req_no=REQUEST_CLIENT_PUT;
	}else{
		xi_errno=XI_ERRNO_PARA;
		strcpy(xi_message,"parameter Injustice");
		return -1;
	}
	l_mode=mod;

	if ( l_fd >= 0 ){
		xi_errno=XI_ERRNO_PHASE;
		strcpy(xi_message,"phase error");
		return -1;
	}

	//サーバ接続
	if ( (ret=xi_connect()) < 0 ){ return -3; }

	//リクエスト送信
	if ( (ret=xi_send(req_no, hdfs_file,"", comp_level, fsize)) < 0 ){ xi_error_fin(); return -4; }

	//圧縮レベル設定
	memset(&zpara,0,sizeof(T_ZTRANS_INFO));
	zpara.file_size=fsize;
	zpara.buff_size=l_para.network_buff_size;
	zpara.stmout=l_para.send_timeout;
	zpara.rtmout=l_para.recv_timeout;
	if ( l_mode == XI_WRITE ){ ztrans_comp_init(&zpara, l_para.con_compression); }
	if ( l_mode == XI_READ ){ ztrans_decomp_init(&zpara); }

	return l_fd;
}

int xi_read(int ifd, char *ibuff, int isize){
	int ret;
	int i,j,n;
	int in_total=0;

	xi_init();

	//パラメータチェック
	if ( ifd < 0 ){
		xi_errno=XI_ERRNO_PARA;
		strcpy(xi_message,"parameter Injustice");
		return -1;
	}
	if ( isize < zpara.buff_size ){
		xi_errno=XI_ERRNO_BUFF_SMALL;
		strcpy(xi_message,"buffer is too small");
		return -1;
	}
	if ( l_fd < 0 ){
		xi_errno=XI_ERRNO_PHASE;
		strcpy(xi_message,"phase error");
		return -1;
	}

	while(1){

		//サーバから受信
		ret=ztrans_data_recv(&zpara, ifd, &n);
		if ( ret < 0 ){ return -2; }
		if ( ret == 0 && n ==0 ){ return in_total; }
		if ( n > 0 ){
			memcpy(ibuff + in_total, zpara.obuff + sizeof(T_CLIENT_DATA_HEADER), n);
			in_total += n;
			return in_total;
		}
	}

	xi_errno=XI_ERRNO_RECV_Z;
	strcpy(xi_message,"recv error");
	return -3;
}

int xi_write(int ofd, char *obuff, int osize){
	int ret;
	int ocnt;

	xi_init();

	//パラメータチェック
	if ( ofd < 0 || osize < 0 ){
		xi_errno=XI_ERRNO_PARA;
		strcpy(xi_message,"parameter Injustice");
		return -1;
	}
	if ( l_fd < 0 ){
		xi_errno=XI_ERRNO_PHASE;
		strcpy(xi_message,"phase error");
		return -1;
	}

	//圧縮転送
	ret=ztrans_data_send_client(&zpara, obuff, osize, ofd);
//printf("#W# ztrans_data_send_client()=%d in=%d,%d out=%d,%d\n",ret, 
//	zpara.ibuff_p->seq,zpara.ibuff_p->total, zpara.obuff_p->seq,zpara.obuff_p->total);

	if ( ret < 0 ){
		xi_errno=XI_ERRNO_SEND_Z;
		strcpy(xi_message,"compress send error");
		return -2;
	}
	return ret;
}

int xi_close(int ofd){
	int ret=0;

	xi_init();

	//パラメータチェック
	if ( ofd < 0 ){
		xi_errno=XI_ERRNO_PARA;
		strcpy(xi_message,"parameter Injustice");
		return -1;
	}
	if ( l_fd < 0 || l_mode < 0 ){
		xi_errno=XI_ERRNO_PHASE;
		strcpy(xi_message,"phase error");
		return -1; 
	}

	//書込みの場合
	if ( l_mode == XI_WRITE ){
		ret=ztrans_data_send_client(&zpara, NULL, 0, ofd);

//printf("#C# ztrans_data_send_client()=%d in=%d,%d out=%d,%d\n",ret, 
//	zpara.ibuff_p->seq,zpara.ibuff_p->total, zpara.obuff_p->seq,zpara.obuff_p->total);

		if ( ret < 0 ){
			xi_errno=XI_ERRNO_SEND_Z;
			strcpy(xi_message,"compress send error");
			xi_error_fin();
			return -2;
		}

		//ZLIB終了処理
		ztrans_comp_fin(&zpara);
	}

	//読込みの場合
	if ( l_mode == XI_READ ){
		ztrans_decomp_fin(&zpara);
	}
	l_mode=(-1);

	//通信切断
	if ( (ret=xi_fin()) < 0 ){ xi_error_fin(); return -4; }

	return 0;
}

int xi_info(int typ){
	int ret;

	xi_init();

	//パラメータチェック
	if ( typ < 0 ){
		xi_errno=XI_ERRNO_PARA;
		strcpy(xi_message,"parameter Injustice");
		return -1;
	}

	if ( (ret=xi_connect()) < 0 ){ xi_error_fin(); return -2; }
	if ( (ret=xi_recv(typ, "", "")) < 0 ){ xi_error_fin(); return -3; }
	if ( (ret=xi_fin()) < 0 ){ xi_error_fin(); return -4; }
	return 0;
}

int xi_info(int typ, char *para){
	int ret;
	T_CLIENT_RESULT_M res;

	xi_init();

	//パラメータチェック
	if ( typ < 0 || para[0]==NULL ){
		xi_errno=XI_ERRNO_PARA;
		strcpy(xi_message,"parameter Injustice");
		return -1;
	}

	if ( (ret=xi_connect()) < 0 ){ xi_error_fin(); return -2; }
	if ( (ret=xi_recv(typ, para, "")) < 0 ){ xi_error_fin(); return -3; }
	if ( (ret=xi_fin()) < 0 ){ xi_error_fin(); return -4; }
	return 0;
}

int xi_tab(char *path, char *out_path){
	int ret;

	xi_init();

	//パラメータチェック
	if ( path[0] == NULL || out_path[0]==NULL ){
		xi_errno=XI_ERRNO_PARA;
		strcpy(xi_message,"parameter Injustice");
		return -1;
	}

	if ( (ret=xi_connect()) < 0 ){ xi_error_fin(); return -2; }

	if ( (ret=xi_send(REQUEST_CLIENT_TAB, path,"", 0, 0)) < 0 ){ xi_error_fin(); return -3; }
	if ( xi_errno == REQUEST_CLIENT_OK ){
		if ( xi_message[0] != NULL ){
			strcpy((char*)out_path, xi_message);
		}
	}

	if ( (ret=xi_fin()) < 0 ){ xi_error_fin(); return -4; }

	return 0;
}

int xi_stat(char *path, struct xi_stat *buf){
	int ret;
	T_CLIENT_RESULT_M res;

	xi_init();

	//パラメータチェック
	if ( path[0] == NULL ){
		xi_errno=XI_ERRNO_PARA;
		strcpy(xi_message,"parameter Injustice");
		return -1;
	}

	if ( (ret=xi_connect()) < 0 ){ xi_error_fin(); return -2; }
	if ( (ret=xi_send(REQUEST_CLIENT_FILE, path,"", 0, 0)) < 0 ){ xi_error_fin(); return -3; }
	if ( (ret=xi_recv_fix((char*)buf,sizeof(struct xi_stat))) < 0 ){ xi_error_fin(); return -4; }
	if ( (ret=xi_fin()) < 0 ){ xi_error_fin(); return -5; }

	//デバック表示
	//printf("type	 : %d\n",buf->st_type);
	//printf("uid	  : %d\n",buf->st_uid);
	//printf("gid	  : %d\n",buf->st_gid);
	//printf("size	 : %d\n",buf->st_size);
	//printf("blksize  : %d\n",buf->st_blksize);
	//printf("blocks   : %d\n",buf->st_blocks);
	//printf("dup_cnt  : %d\n",buf->dup_cnt);

	return ret;
}

int xi_mkdir(char *path){
	int ret;

	xi_init();

	//パラメータチェック
	if ( path[0] == NULL ){
		xi_errno=XI_ERRNO_PARA;
		strcpy(xi_message,"parameter Injustice");
		return -1;
	}

	if ( (ret=xi_connect()) < 0 ){ xi_error_fin(); return -2; }
	if ( (ret=xi_send(REQUEST_CLIENT_MKDIR, path,"", 0, 0)) < 0 ){ xi_error_fin(); return -3; }
	if ( (ret=xi_fin()) < 0 ){ xi_error_fin(); return -4; }
	return ret;
}

int xi_rmdir(char *path){
	int ret;

	xi_init();

	//パラメータチェック
	if ( path[0] == NULL ){
		xi_errno=XI_ERRNO_PARA;
		strcpy(xi_message,"parameter Injustice");
		return -1;
	}

	if ( (ret=xi_connect()) < 0 ){ xi_error_fin(); return -2; }
	if ( (ret=xi_send(REQUEST_CLIENT_RMDIR, path,"", 0, 0)) < 0 ){ xi_error_fin(); return -3; }
	if ( (ret=xi_fin()) < 0 ){ xi_error_fin(); return -4; }
	return ret;
}

int xi_rename(char *path1, char *path2){
	int ret;

	xi_init();

	//パラメータチェック
	if ( path1[0] == NULL || path2[0] == NULL ){
		xi_errno=XI_ERRNO_PARA;
		strcpy(xi_message,"parameter Injustice");
		return -1;
	}

	if ( (ret=xi_connect()) < 0 ){ xi_error_fin(); return -2; }
	if ( (ret=xi_send(REQUEST_CLIENT_MV, path1,path2, 0, 0)) < 0 ){ xi_error_fin(); return -3; }
	if ( (ret=xi_fin()) < 0 ){ xi_error_fin(); return -4; }
	return ret;
}

int xi_unlink(char *path){
	int ret;

	xi_init();

	//パラメータチェック
	if ( path[0] == NULL ){
		xi_errno=XI_ERRNO_PARA;
		strcpy(xi_message,"parameter Injustice");
		return -1;
	}

	if ( (ret=xi_connect()) < 0 ){ xi_error_fin(); return -2; }
	if ( (ret=xi_send(REQUEST_CLIENT_RM, path,"", 0, 0)) < 0 ){ xi_error_fin(); return -3; }
	if ( (ret=xi_fin()) < 0 ){ xi_error_fin(); return -4; }
	return ret;
}

int xi_chmod(char *path, int mode){
	int ret;
	char buff[1024];

	xi_init();

	//パラメータチェック
	if ( mode < 0 || path[0] == NULL ){
		xi_errno=XI_ERRNO_PARA;
		strcpy(xi_message,"parameter Injustice");
		return -1;
	}

	sprintf(buff,"%d",mode);
	if ( (ret=xi_connect()) < 0 ){ xi_error_fin(); return -2; }
	if ( (ret=xi_send(REQUEST_CLIENT_CHMOD, buff,path, 0, 0)) < 0 ){ xi_error_fin(); return -3; }
	if ( (ret=xi_fin()) < 0 ){ xi_error_fin(); return -4; }
	return ret;
}

int xi_chown(char *path, int uid){
	int ret;
	char buff[1024];

	xi_init();

	//パラメータチェック
	if ( uid < 0 ){
		xi_errno=XI_ERRNO_PARA;
		strcpy(xi_message,"parameter Injustice");
		return -1;
	}

	sprintf(buff,"%d",uid);
	if ( (ret=xi_connect()) < 0 ){ xi_error_fin(); return -2; }
	if ( (ret=xi_send(REQUEST_CLIENT_CHOWN, buff,path, 0, 0)) < 0 ){ xi_error_fin(); return -3; }
	if ( (ret=xi_fin()) < 0 ){ xi_error_fin(); return -4; }
	return ret;
}

int xi_chgrp(char *path, int gid){
	int ret;
	char buff[1024];

	xi_init();

	//パラメータチェック
	if ( gid < 0 ){
		xi_errno=XI_ERRNO_PARA;
		strcpy(xi_message,"parameter Injustice");
		return -1;
	}

	sprintf(buff,"%d",gid);
	if ( (ret=xi_connect()) < 0 ){ xi_error_fin(); return -2; }
	if ( (ret=xi_send(REQUEST_CLIENT_CHGRP, buff,path, 0, 0)) < 0 ){ xi_error_fin(); return -3; }
	if ( (ret=xi_fin()) < 0 ){ xi_error_fin(); return -4; }
	return ret;
}

