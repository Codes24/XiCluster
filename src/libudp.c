/*****************************************************************************
*  <システム>   分散処理フレームワーク
*  <名称>       UDPソケット通信API
*  <目的>	
*  <機能>	
*  <開発環境>	UNIX
*  <特記事項>
*
*  VERSION      DATE	    BY		       CHANGE/COMMENT
*  -----------------------------------------------------------------------------
*  V0.00	2014/01/18  Takakusaki	   新規作成
******************************************************************************/
#include "xi_common.h"


/*****************************************************************************
*  <関数名>      udpSelect
*  <機能>	 同期Ｉ／Ｏ
*  <説明>	 指定されたディスクプリターが使用可能となるまでまつ
*  <引数>
*	       fd:I:ファイルディスクプリタ
*	       type:I:チェックタイプ(0=読込みチェック 1=書込みチェック 2=OOBチェック)
*	       tm:I:タイムアウト
*  <リターン値>
*	       0:タイムアウト
*	       -1:異常終了
*	       1:送受信可能
*  <備考>
******************************************************************************/
int udpSelect(int fd, int type, int tm) {
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
*  <関数名>     udpBind
*  <機能>       UDP通信初期化処理
*  <説明>       ソケット作成とBINDを行う
*  <引数>       
*		port:I:ポート番号
*  <リターン値> 
*		0:正常終了(ファイルディスクプリタ)
*	    -1:異常終了
*  <備考>
******************************************************************************/
int udpBind(int port) {
	int ret;
	int fd;
	int yes = 1;
	struct sockaddr_in addr;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if ( fd < 0 ){ return -1; }

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;

	ret=bind(fd, (struct sockaddr *)&addr, sizeof(addr));
	if ( ret < 0 ){ return -1; }

	return fd;
}

/*****************************************************************************
*  <関数名>     udpBroadcast
*  <機能>       ブロードキャストパケット送信
*  <説明>       各サーバにブロードキャストパケットを送信する
*  <引数>       
*		port:I:ポート番号
*		data:I:送信データ
*		size:I:送信サイズ
*  <リターン値> 
*		0:正常終了
*	    -1:異常終了
*  <備考>
******************************************************************************/
int udpBroadcast(int port, char *data, int size) {
	int ret;
	int fd;
	int yes = 1;
	struct sockaddr_in addr;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if ( fd < 0 ){ return -1; }

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr("255.255.255.255");
	ret=setsockopt(fd, SOL_SOCKET, SO_BROADCAST, (char *)&yes, sizeof(yes));
	if ( ret < 0 ){ close(fd); return -1; }

	ret=sendto(fd, data, size, 0, (struct sockaddr *)&addr, sizeof(addr));
	if ( ret < 0 ){ close(fd); return -1; }
	close(fd);

	return 0;
}

/*****************************************************************************
*  <関数名>      udpRecv
*  <機能>	 ソケット受信
*  <説明>	 ソケットからの受信を行う
*  <引数>	 fd:I:ファイルディスクプリター
*	       buffer:O:受信データ
*	       size:I:受信サイズ
*	       timeout:I:タイムアウト
*  <リターン値>
*	       0以上:正常終了(受信サイズ)
*	       0未満:異常終了
*  <備考>
******************************************************************************/
int udpRecv(int fd, char* buffer, int size, int timeout ){
	int ret;

	/* 待ち */
	ret=udpSelect(fd, 0, timeout);
	if ( ret == (-1) ) return -2;
	if ( ret == 0 ) return -3;

	/* 受信 */
	ret=read( fd, buffer, size);
	if ( ret < 0){
		//printf("read()=%d errno=%d\n",ret,errno);
		return -4;
	}
//printf("udpRecv(%d,*,%d,%d)=%d\n",fd,size,timeout,ret);
	return ret;
}


/*****************************************************************************
*  <関数名>	     udpClose
*  <機能>	     ソケットクローズ
*  <説明>	     ソケットのクローズを行う
*  <引数>
*		     fd:I:ファイルディスクプリター
*  <リターン値>
*		       0:正常終了
*		  -1:異常終了
*  <備考>
******************************************************************************/
int udpClose(int fd){
	int ret;

	if ( (ret=close(fd)) != 0 ){
		//printf("close(%d)=%d errno=%d\n",fd,ret,errno);
		return -1;
	}

	return 0;
}
