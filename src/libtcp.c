/*****************************************************************************
*  <システム>   分散処理フレームワーク
*  <名称>	TCPソケット通信API
*  <目的>	
*  <機能>
*  <開発環境>	UNIX
*  <特記事項>
*
*  VERSION	  DATE		BY			   CHANGE/COMMENT
*  -----------------------------------------------------------------------------
*  V0.00		2014/01/18  Takakusaki	   新規作成
******************************************************************************/
#include "xi_common.h"

/*****************************************************************************
 プロトタイプ宣言
******************************************************************************/
static int tcpConnectSub(u_long addr, int port, int timeout );


/*****************************************************************************
*  <関数名>	 tcpGetMTU
*  <機能>	 MTU数の取得
*  <説明>	 
*  <引数>	   
*		fd:I:ファイルディスクプリタ
*		if_name:I:インターフェース名
*  <リターン値> 
*		0以上:MTU数
*		-1:異常終了
*  <備考>
******************************************************************************/
int tcpGetMTU(int fd, char *if_name) {
	struct ifreq ifr;

	strncpy(ifr.ifr_name, if_name, IFNAMSIZ-1);
	if (ioctl(fd, SIOCGIFMTU, &ifr) != 0) { return -1; }

	return ifr.ifr_mtu;
}

/*****************************************************************************
*  <関数名>	 tcpSetBuffSize
*  <機能>	 通信用バッファサイズ設定
*  <説明>	 通信用のバッファサイズを設定する
*  <引数>	   
*		fd:I:ファイルディスクプリタ
*		buffsize_snd:I:送信バッファサイズ(２倍の値が設定される)
*		buffsize_rcv:I:受信バッファサイズ(２倍の値が設定される)
*  <リターン値> 
*		0以上:正常
*		-1:異常終了
*		-2:異常終了
*  <備考>
******************************************************************************/
int tcpSetBuffSize(int fd, int buffsize_snd, int buffsize_rcv) {
	int ret;
	int val;

	//送信バッファサイズ変更
	if ( buffsize_snd > 0 ){
		val=buffsize_snd;
		ret = setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *)&val, sizeof(int));
//printf("setsockopt(%d,%d,%d,%d,%d) = %d\n",fd, SOL_SOCKET, SO_SNDBUF, buffsize_snd, sizeof(int),ret);
		if ( ret != 0 ){ return -1; }
	}

	//受信バッファサイズ変更
	if ( buffsize_rcv > 0 ){
		val=buffsize_rcv;
		ret = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *)&val, sizeof(int));
//printf("setsockopt(%d,%d,%d,%d,%d) = %d\n",fd, SOL_SOCKET, SO_RCVBUF, buffsize_rcv, sizeof(int),ret);
		if ( ret != 0 ){ return -2; }
	}

	return 0;
}

/*****************************************************************************
*  <関数名>	 tcpGetBuffSize
*  <機能>	 通信用バッファサイズ取得
*  <説明>	 通信用のバッファサイズを取得する
*  <引数>	   
*		fd:I:ファイルディスクプリタ
*		buffsize_snd:O:送信バッファサイズ
*		buffsize_rcv:O:受信バッファサイズ
*  <リターン値> 
*		0以上:正常
*		-1:異常終了
*		-2:異常終了
*  <備考>
******************************************************************************/
int tcpGetBuffSize(int fd, int *buffsize_snd, int *buffsize_rcv) {
	int ret;
	int len;

	len=sizeof(int);
	ret=getsockopt(fd, SOL_SOCKET, SO_SNDBUF, buffsize_snd, (socklen_t *)&len);
//printf("getsockopt(%d,%d,%d,%d,%d) = %d\n",fd, SOL_SOCKET, SO_SNDBUF, *buffsize_snd, len,ret);
	if ( ret != 0 ){ return -1; }

	ret=getsockopt(fd, SOL_SOCKET, SO_RCVBUF, buffsize_rcv, (socklen_t *)&len);
//printf("getsockopt(%d,%d,%d,%d,%d) = %d\n",fd, SOL_SOCKET, SO_RCVBUF, *buffsize_rcv, len,ret);
	if ( ret != 0 ){ return -1; }

	return 0;
}



/*****************************************************************************
*  <関数名>	 tcpListen
*  <機能>	   TCP/IP接続用キュー作成
*  <説明>	   TCP/IP接続用のキューを作成する
*  <引数>	   
*		addr:I:IPアドレス
*		port:I:ポート番号
*  <リターン値> 
*		0以上:正常終了(ファイルディスクプリタ)
*		-1:異常終了
*		-2:異常終了
*		-3:異常終了
*  <備考>
******************************************************************************/
int tcpListen(u_long addr,int port){
	int fd,ret,len,i,on;
	char ipbuf[128];
	struct sockaddr_in myaddr;

	utlDisplyIP( (u_char*)&addr, ipbuf);

	/* ソケットの生成 */
	fd=socket(AF_INET, SOCK_STREAM, 0);
	if ( fd < 0 ){
		//printf("socket()=%d errno=%d\n",fd,errno);
		return -1; 
	}

	/* TIME_WAIT対応として、即時ポートを再利用可能にする */
	on=1;
	ret = setsockopt( fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) );
	if ( ret < 0 ){
//printf("setsockopt()=%d errno=%d\n",ret,errno);
		return -2; 
	}

	/* ソケットに名前をつける */
	memset(&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family   	= AF_INET;
	myaddr.sin_port	 	= htons(port);
	myaddr.sin_addr.s_addr  = inet_addr(ipbuf);
	len = sizeof(myaddr);
	ret=bind(fd,(struct sockaddr *)&myaddr, len);
	if ( ret < 0 ){
//printf("bind()=%d errno=%d\n",ret,errno);
		close(fd);
		return -3; 
	}

	/* 接続キューを作成 */
	ret=listen(fd, 5);
	if ( ret < 0 ){
		//printf("listen()=%d errno=%d\n",ret,errno);
		close(fd);
		return -4; 
	}

	return fd;
}

/*****************************************************************************
*  <関数名>	 tcpAccept
*  <機能>	   TCP/IP接続受け入れ
*  <説明>	   TCP/IP接続を受け付ける
*  <引数>	   
*		fd:I:ファイルディスクプリタ
*  <リターン値> 
*		0以上:正常終了(ファイルディスクプリタ)
*		-1:異常終了
*  <備考>
******************************************************************************/
int tcpAccept(int fd) {
	int client_fd;
	struct sockaddr_in claddr;
	socklen_t len=sizeof(claddr);

	client_fd = accept(fd,(struct sockaddr *)&claddr, &len);
	if ( fd < 0 ){ return -1; }

	return client_fd;
}

/*****************************************************************************
*  <関数名>	 tcpSelect
*  <機能>	   同期Ｉ／Ｏ
*  <説明>	   指定されたディスクプリターが使用可能となるまでまつ
*  <引数>	   
*		fd:I:ファイルディスクプリタ
*		type:I:チェックタイプ(0=読込みチェック 1=書込みチェック 2=OOBチェック)
*		tm:I:タイムアウト
*  <リターン値> 
*		0:タイムアウト
*		-1:異常終了
*		1:送受信可能
*  <備考>
******************************************************************************/
int tcpSelect(int fd, int type, int tm) {
	int ret;
	fd_set cset;
	struct timeval timer;

	/* タイムアウト値設定 */
	if ( tm == 0 ){
		timer.tv_sec=0;
		timer.tv_usec=0;
	}else{
		timer.tv_sec = tm/1000000;
		timer.tv_usec = tm%1000000;
	}

	/* 検査するディスクリプターの設定 */
	FD_ZERO(&cset);
	FD_SET(fd, &cset);

	switch(type){
	case 0: /* 読み取り可能チェック */
		ret=select(fd+1, &cset, 0, 0, &timer); break;
	case 1: /* 書き込み可能チェック */
		ret=select(fd+1, 0, &cset, 0, &timer); break;
	case 2: /* 帯域外データOOB(Out Of Bounds)の受信チェック */
		ret=select(fd+1, 0, 0, &cset, &timer); break;
	default:
		return -1;
	}

	/* 異常 */
	if ( ret < 0 ){ return -1; }

	/* タイムアウト */
	if ( ret == 0 ){ return 0; }

	/* チェック */
	if ( FD_ISSET(fd, &cset) == 0){
		return 0;
	}

	return 1;
}

/*****************************************************************************
*  <関数名>	 tcpConnect
*  <機能>	   サーバ接続
*  <説明>	   サーバへの接続を行う
*  <引数>	   
*		addr:I:IPアドレス
*		port:I:ポート番号
*		timeout:I:接続待ちタイムアウト
*		retry:I:リトライ回数
*		retrans:I:リトライ間隔
*  <リターン値> 
*		0以上:正常終了(ファイルディスクプリタ)
*		-1:異常終了
*		-2:コネクトタイムアウト
*  <備考>	非ブロッキングconnectを行う
******************************************************************************/
int tcpConnect(u_long addr, int port, int timeout, int retry, int retrans ){
	int i,fd;

	for (i=0; i<retry; i++){
		fd=tcpConnectSub(addr, port, timeout);
		if ( fd >= 0 ){ return fd; }
		usleep( retrans );
	}
	return -1;
}

static int tcpConnectSub(u_long addr, int port, int timeout ){
	int i,flags,fd,ret,error,len;
	int *val;
	char ipbuf[128];
	struct sockaddr_in myaddr;
	struct timeval timer;
	struct linger lval;

	utlDisplyIP( (u_char*)&addr, ipbuf);

	/* ソケットの生成 */
	fd=socket(AF_INET, SOCK_STREAM, 0);
	if ( fd < 0 ){
		return -1;
	}

	/* 引数設定 */
	memset(&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family	= AF_INET;
	myaddr.sin_port	 = htons(port);
	myaddr.sin_addr.s_addr  = inet_addr(ipbuf);

	/* 非ブロッキングconnect */
	flags = fcntl(fd, F_GETFL, 0);
	if ( flags < 0 ){
		close(fd);
		return -1;
	}
	ret = fcntl(fd, F_SETFL, flags|O_NONBLOCK);
	if ( ret < 0 ){
		close(fd);
		return -1;
	}

	/* 接続 */
	ret=connect(fd, (struct sockaddr *)&myaddr, sizeof(myaddr));
	if ( ret == 0 ) return fd;
	if ( errno == EISCONN ) return fd;

	/* 非ブロッキングconnectでは即時制御が戻る->コネクションが確立されると書込可能となる->selectで待てる */
	errno=0;
	ret=tcpSelect(fd, 1, timeout);
	if ( ret <= 0  ){
		close(fd);
		if ( ret == 0 ) return -2;
		return -1;
	}

	/* 保留中のエラー */
	error=0;
	len=sizeof(error);
	ret=getsockopt(fd, SOL_SOCKET, SO_ERROR, (char*)&error, (socklen_t *)&len);
	if ( ret < 0 ){
		close(fd);
		return -1;
	}
	/* Berkeley版に対応する為にerror値もチェック */
	if ( error != 0 ){
		close(fd);
		return -1;
	}

	return fd;
}


/*****************************************************************************
*  <関数名>	 tcpRecv
*  <機能>	   ソケット受信
*  <説明>	   ソケットからの受信を行う
*  <引数>	   fd:I:ファイルディスクプリター
*		buffer:O:受信データ
*		size:I:受信サイズ
*		timeout:I:タイムアウト
*  <リターン値> 
*		0以上:正常終了(受信サイズ)
*		0未満:異常終了
*  <備考>
******************************************************************************/
int tcpRecv(int fd, char* buffer, int size, int timeout ){
	int ret;

	/* 待ち */
	ret=tcpSelect(fd, 0, timeout); 
	if ( ret == (-1) ) return -2;
	if ( ret == 0 ) return -3;
	
	/* 受信 */
	ret=read( fd, buffer, size);
	if ( ret < 0){
		//printf("read()=%d errno=%d\n",ret,errno);
		return -4;
	}
//printf("tcpRecv(%d,*,%d,%d)=%d\n",fd,size,timeout,ret);
	return ret;
}

/*****************************************************************************
*  <関数名>	 tcpRecvFix
*  <機能>	   ソケット受信
*  <説明>	   ソケットから指定バイト受信を行う
*  <引数>	   
*		fd:I:ファイルディスクプリター
*		buffer:O:受信データ
*		size:I:受信サイズ
*		timeout:I:タイムアウト
*  <リターン値> 
*		0以上:正常終了(受信サイズ)
*		0未満:異常終了
*  <備考>
******************************************************************************/
int tcpRecvFix(int fd, char* buffer, int size, int timeout ){
	int ret;
	u_long r_size=0;

	while(1){
		//待ち
		ret=tcpSelect(fd, 0, timeout); 
		if ( ret == (-1) ) return -2;
		if ( ret == 0 ) return -3;
	
		//受信
		ret=read( fd, buffer + r_size, size - r_size);
		if ( ret == 0 ){ return r_size; }
		if ( ret < 0){
			//printf("read()=%d errno=%d\n",ret,errno);
			return -4;
		}
		r_size += ret;
		if ( r_size >= size ){ break; }
	}
	return r_size;
}

/*****************************************************************************
*  <関数名>	 tcpSend
*  <機能>	   ソケット書き込み
*  <説明>	   ソケットへの書き込みを行う
*  <引数>	   fd:I:ファイルディスクプリター
*		buffer:I:送信データ
*		size:I:送信サイズ
*		timeout:I:タイムアウト
*  <リターン値> 
*			0:正常終了
*		   0以下:異常終了
*  <備考>
******************************************************************************/
int tcpSend(int fd, char* buffer, int size, int timeout ){
	int ret;

	/* 待ち */
	ret=tcpSelect(fd, 1, timeout);
	if ( ret == (-1) ) return -2;
	if ( ret == 0 ) return -3;

	/* 送信 */
	ret=write( fd, buffer, size);
	if ( ret != size ){
		//printf("write()=%d errno=%d\n",ret,errno);
		return -4;
	}

	return ret;
}

/*****************************************************************************
*  <関数名>		tcpClose
*  <機能>		ソケットクローズ
*  <説明>		ソケットのクローズを行う
*  <引数>	   
*			fd:I:ファイルディスクプリター
*  <リターン値> 
*			0:正常終了
*		   -1:異常終了
*  <備考>
******************************************************************************/
int tcpClose(int fd){
	int ret;

	if ( (ret=close(fd)) != 0 ){
		//printf("close(%d)=%d errno=%d\n",fd,ret,errno);
		return -1;
	} 

	return 0;
}

