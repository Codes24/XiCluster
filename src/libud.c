/*****************************************************************************
*  <システム>   分散処理フレームワーク
*  <名称>       UNIXドメインソケット通信API
*  <目的>       
*  <機能>       
*  <開発環境>   UNIX
*  <特記事項>
*
*  VERSION      DATE        BY             CHANGE/COMMENT
*  -----------------------------------------------------------------------------
*  V0.00        2014/01/18  Takakusaki     新規作成
******************************************************************************/
#include "xi_common.h"

/*****************************************************************************
*  <関数名>  udListen   
*  <機能>    UNIXドメインソケットを待ち受ける 
*  <説明>       
*  <引数>
*       path:I:ソケットのパス
*  <リターン値>
*       0:正常終了(ファイルディスクプリタ)
*       -1:異常終了
*       -2:異常終了
*  <備考>
******************************************************************************/
int udListen(const char* path)
{
	int ret;
	int gate_fd;
	struct sockaddr_un gate_addr;

	/* ソケット作成 */
	if( (gate_fd = socket(PF_UNIX, SOCK_DGRAM, 0)) < 0 ) {
		return -1;
	}
	memset(&gate_addr, 0, sizeof(gate_addr));
	gate_addr.sun_family = AF_UNIX;
	memcpy((char*)gate_addr.sun_path, (char*)path, sizeof(gate_addr.sun_path));
	if( (ret=bind(gate_fd, (struct sockaddr*)&gate_addr, sizeof(gate_addr))) < 0 ) {
		return -2;
	}
	return gate_fd;
}

/*****************************************************************************
*  <関数名>  udConnect
*  <機能>    UNIXドメインソケット接続
*  <説明>       
*  <引数>
*       path:I:ソケットのパス
*  <リターン値>
*       0:正常終了(ファイルディスクプリタ)
*       -1:異常終了
*  <備考>
******************************************************************************/
int udConnect(const char* path)
{
	int gate_fd;
	struct sockaddr_un gate_addr;

	/* ソケット作成 */
	if ( (gate_fd = socket(PF_UNIX, SOCK_DGRAM, 0)) < 0 ) {
		return -1;
	}
	memset(&gate_addr, 0, sizeof(gate_addr));
	gate_addr.sun_family = AF_UNIX;
	memcpy((char*)gate_addr.sun_path, (char*)path, sizeof(gate_addr.sun_path));
	if ( connect(gate_fd, (struct sockaddr*)&gate_addr, sizeof(gate_addr)) < 0 ) {
		return -1;
	}
	return gate_fd;
}

int udClose(int gate_fd){
	close(gate_fd);
}

/*****************************************************************************
*  <関数名>  udRecv
*  <機能>    UNIXドメインソケットからの受信
*  <説明>       
*  <引数>
*       gate_fd:I:UNIXドメインソケットソケットのパス
*		buff:I:バッファ
*		siz:I:バッファサイズ
*       timeout:I:タイムアウト
*  <リターン値>
*       0:正常終了(ファイディスクプリタ)
*       0未満:異常終了
*  <備考>
******************************************************************************/
int udRecv(int gate_fd, void* buff, size_t siz, int timeout ){
	int ret;
	struct msghdr msg;
	struct iovec iov;
	char cmsgbuf[CMSG_SPACE(sizeof(int))];

	if ( gate_fd < 0 ){ return -1; }

    /* 待ち */
    ret=tcpSelect(gate_fd, 0, timeout);
    if ( ret == (-1) ) return -2;
    if ( ret == 0 ) return -3;

	iov.iov_base = buff;
	iov.iov_len = siz;
	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = cmsgbuf;
	msg.msg_controllen = sizeof(cmsgbuf);
	msg.msg_flags = MSG_WAITALL;

	/* 受信 */
	if ( recvmsg(gate_fd, &msg, 0) < 0 ) { return -1; }

	struct cmsghdr *cmsg = (struct cmsghdr*)cmsgbuf;
	return *((int *)CMSG_DATA(cmsg));
}

/*****************************************************************************
*  <関数名>  udSend
*  <機能>    UNIXドメインソケットへの送信
*  <説明>       
*  <引数>
*       gate_fd:I:UNIXドメインソケットソケットのパス
*		snd_fd:I:送信ファイルディスクリプタ
*		buff:I:バッファ
*		siz:I:バッファサイズ
*       timeout:I:タイムアウト
*  <リターン値>
*       0:正常終了
*       0未満:異常終了
*  <備考>
******************************************************************************/
int udSend(int gate_fd, int snd_fd, void* buff, int siz, int timeout) {
	int ret;
	struct iovec iov;
	char cmsgbuf[CMSG_SPACE(sizeof(int))];

	if ( gate_fd < 0 ){ return -1; }

    /* 待ち */
    ret=tcpSelect(gate_fd, 1, timeout);
    if ( ret == (-1) ) return -2;
    if ( ret == 0 ) return -3;

	iov.iov_base = buff;
	iov.iov_len = siz;
	struct cmsghdr *cmsg = (struct cmsghdr*)cmsgbuf;
	cmsg->cmsg_len = CMSG_LEN(sizeof(int));
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	*((int *)CMSG_DATA(cmsg)) = snd_fd;

	struct msghdr msg;
	memset(&msg, 0, sizeof(msg));
	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = cmsgbuf;
	msg.msg_controllen = sizeof(cmsgbuf);
	msg.msg_flags = 0;

	if (sendmsg(gate_fd, &msg, 0) < 0) { return -1; }
	return 0;
}
