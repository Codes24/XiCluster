/*****************************************************************************
*  <システム>   分散処理フレームワーク
*  <名称>	mmapプロセス間通信API
*  <目的>	
*  <機能>		
*  <開発環境>	UNIX
*  <特記事項>
*
*  VERSION	  DATE		BY			   CHANGE/COMMENT
*  -----------------------------------------------------------------------------
*  V0.00		2014/10/25  Takakusaki	   新規作成
******************************************************************************/
#include "xi_common.h"

/*****************************************************************************
 プロトタイプ宣言
******************************************************************************/
typedef struct{
    int flg;
    int pgsize;
    int size;
    int w_size;
	int close;
} T_MMAP_IF;

static T_MMAP_IF *l_data=NULL;
static char *l_buff=NULL;

/*****************************************************************************
*  <関数名>	 mmapCreate
*  <機能>	 mmapの作成
*  <説明>	 mmapを作成する
*  <引数>	 blocks=ページブロック数  
*  <リターン値> 
*		0以上:正常終了
*		-1:異常終了
*  <備考>
******************************************************************************/
int mmapCreate(int blocks){
	int pgsize;

	if ( l_data != NULL ){ return -1; }

	//ページサイズ
	pgsize=getpagesize() * blocks;

	//mmap作成
	l_data = (T_MMAP_IF*)mmap(NULL, pgsize, PROT_READ | PROT_WRITE, MAP_ANON | MAP_SHARED, -1, 0);

	//領域初期化
	l_data->w_size=0;
	l_data->pgsize=pgsize;
	l_data->size=pgsize - sizeof(T_MMAP_IF);
	l_data->flg=0;
	l_data->close=0;
	l_buff = (char*)l_data + sizeof(T_MMAP_IF);

	return 0;
}

/*****************************************************************************
*  <関数名>	mmapSend
*  <機能>	mmap書き込み
*  <説明>	mmapへの書き込みを行う
*  <引数>	
*		buffer:I:送信データ
*		size:I:送信サイズ
*  <リターン値> 
*		0:正常終了
*		0以下:異常終了
*  <備考>
******************************************************************************/
int mmapSend(char* buffer, int size){
	int i;

	if ( l_data == NULL ){ return -1; }

	//メモリロック
	while(l_data->flg!=0);
	l_data->flg=1;

	//バッファチェック
	if ( (l_data->w_size + size) > l_data->size ){
		l_data->flg=0;
		return -2; 
	}

	//mmap領域への書込み
	memcpy(l_buff + l_data->w_size, buffer,  size);
	l_data->w_size+=size;
	l_data->flg=0;

	return size;
}

/*****************************************************************************
*  <関数名>	mmapRecv
*  <機能>	mmap受信
*  <説明>	mmapからの受信を行う
*  <引数>	
*		buffer:O:受信データ
*		size:I:受信サイズ
*  <リターン値> 
*		0以上:正常終了(受信サイズ)
*		0未満:異常終了
*  <備考>
******************************************************************************/
int mmapRecvSub(char* buffer, int size){
	int i;
	int ret;

	//メモリロック
	while(l_data->flg!=0);
	l_data->flg=2;

	//受信チェック
	if ( l_data->w_size > size ){
		memcpy(buffer, l_buff, size);
		l_data->w_size=l_data->w_size - size;
		memcpy(l_buff, l_buff+size, l_data->w_size);
		l_data->flg=0;
		return size;
	}

	//mmap領域から読込み
	memcpy(buffer, l_buff, l_data->w_size);
	ret=l_data->w_size;
	l_data->w_size=0;
	l_data->flg=0;

	return ret;
}

int mmapRecv(char* buffer, int size, int tmout){
	int i;
	int ret;
	int try_cnt = tmout / 100000;

	if ( l_data == NULL ){ return -1; }

	for(i=0; i<try_cnt; i++){
		ret=mmapRecvSub(buffer, size);
		if ( ret > 0 ){ return ret; }
		if ( l_data->close != 0 ){ return 0; }
		usleep(100000);
	}
	return -3;
}


/*****************************************************************************
*  <関数名>	mmapClose
*  <機能>	mmap開放
*  <説明>	mmapの開放を行う。
*  <引数>	   
*  <リターン値> 
*		0:正常終了
*		-1:異常終了
*  <備考>
******************************************************************************/
int mmapClose(){
	if ( l_data == NULL ){ return -1; }
	l_data->close++;
	munmap(l_data, l_data->pgsize);
	l_data=NULL;
	return 0;
}

