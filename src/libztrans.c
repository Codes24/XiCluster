/*****************************************************************************
*  <システム>   分散処理フレームワーク
*  <名称>	データ圧縮通信API
*  <目的>		
*  <機能>		
*  <開発環境>	UNIX
*  <特記事項>
*
*  VERSION	  DATE		BY			   CHANGE/COMMENT
*  -----------------------------------------------------------------------------
*  V0.00		2014/06/07  Takakusaki	   新規作成
******************************************************************************/
#include "xi_common.h"

/***************************************************************
 ローカル変数の定義
***************************************************************/
//static z_stream z;

int ztrans_comp_init(T_ZTRANS_INFO *zpara, int compression){

	if ( zpara->ibuff == NULL ){
		zpara->ibuff = (char*)malloc( sizeof(T_CLIENT_DATA_HEADER) + zpara->buff_size + 1024 );
		zpara->ibuff_p = (T_CLIENT_DATA_HEADER*)zpara->ibuff;
	}
	if ( zpara->obuff == NULL ){
		zpara->obuff = (char*)malloc( sizeof(T_CLIENT_DATA_HEADER) + zpara->buff_size + 1024 );
		zpara->obuff_p = (T_CLIENT_DATA_HEADER*)zpara->obuff;
	}

	zpara->ibuff_p->seq=0;
	zpara->ibuff_p->size=0;
	zpara->ibuff_p->total=0;
	zpara->obuff_p->seq=0;
	zpara->obuff_p->size=0;
	zpara->obuff_p->total=0;

	//初期化
	zpara->z.zalloc = Z_NULL;
	zpara->z.zfree = Z_NULL;
	zpara->z.opaque = Z_NULL;

	if (deflateInit(&zpara->z, compression) != Z_OK) { return -1; }

	zpara->z.avail_in = 0;
	zpara->z.next_out = (Bytef*)(zpara->obuff + sizeof(T_CLIENT_DATA_HEADER));
	zpara->z.avail_out = zpara->buff_size;

	return 0;
}

int ztrans_decomp_init(T_ZTRANS_INFO *zpara){
	if ( zpara->ibuff == NULL ){
		zpara->ibuff = (char*)malloc( sizeof(T_CLIENT_DATA_HEADER) + zpara->buff_size + 1024 );
		zpara->ibuff_p = (T_CLIENT_DATA_HEADER*)zpara->ibuff;
	}
	if ( zpara->obuff == NULL ){
		zpara->obuff = (char*)malloc( sizeof(T_CLIENT_DATA_HEADER) + zpara->buff_size + 1024 );
		zpara->obuff_p = (T_CLIENT_DATA_HEADER*)zpara->obuff;
	}

	zpara->ibuff_p->seq=0;
	zpara->ibuff_p->size=0;
	zpara->ibuff_p->total=0;
	zpara->obuff_p->seq=0;
	zpara->obuff_p->size=0;
	zpara->obuff_p->total=0;

	//初期化
	zpara->z.zalloc = Z_NULL;
	zpara->z.zfree = Z_NULL;
	zpara->z.opaque = Z_NULL;
	zpara->z.next_in = Z_NULL;
	zpara->z.avail_in = 0;

	if (inflateInit(&zpara->z) != Z_OK) { return -1; }

	zpara->z.next_out = (Bytef*)(zpara->obuff + sizeof(T_CLIENT_DATA_HEADER));
	zpara->z.avail_out = zpara->buff_size;

	return 0;
}

int ztrans_data_send_client_sub(T_ZTRANS_INFO *zpara, char *buff_data, int buff_size, int ofd){
	int dret=1;
	int fret=0;
	int ret=0;
	int flush;
	int osize;
	T_CLIENT_RESULT_S res;

	if ( buff_size == 0 ){
		flush = Z_FINISH;
	}else{
		flush = Z_NO_FLUSH;

		if (zpara->z.avail_in == 0) {
        	memcpy(zpara->ibuff + sizeof(T_CLIENT_DATA_HEADER), buff_data, buff_size);
			zpara->z.next_in = (Bytef*)(zpara->ibuff + sizeof(T_CLIENT_DATA_HEADER));
			zpara->z.avail_in = buff_size;
			zpara->ibuff_p->seq++;
			zpara->ibuff_p->size=buff_size;
			zpara->ibuff_p->total+=buff_size;
			fret=1;
		}
	}

	//圧縮
	ret = deflate(&zpara->z, flush);
	if ( ret == Z_OK ){
		dret=1;
	}else if ( ret == Z_STREAM_END){
		dret=0;
	}else{
		return -2;
	}

	//出力
	if ( dret==0 || zpara->z.avail_out == 0 ){

		osize = zpara->buff_size - zpara->z.avail_out;
		zpara->obuff_p->seq++;
		zpara->obuff_p->size=osize;
		zpara->obuff_p->total+=osize;

		ret=tcpSend(ofd, zpara->obuff, osize + sizeof(T_CLIENT_DATA_HEADER), zpara->stmout);
		if ( ret < 0 ){ return -3; }

		ret=tcpRecv(ofd, (char*)&res, sizeof(res), zpara->rtmout);
		if ( ret <= 0 ){ return -4; }
		if ( res.res != REQUEST_CLIENT_OK ){ return -5; }

		zpara->z.next_out = (Bytef*)(zpara->obuff + sizeof(T_CLIENT_DATA_HEADER));
		zpara->z.avail_out = zpara->buff_size;
	}

	return fret;
}

/* バッファのデータを送信 */
int ztrans_data_send_client(T_ZTRANS_INFO *zpara, char *buff_data, int buff_size, int ofd){
	int ret;
	int i,j;
	int osize;

//static u_long in_total=0;
//in_total+=buff_size;
//printf("ztrans_data_send_client() wsize=%d total=%d\n",buff_size, in_total);

	for(i=0; (zpara->buff_size * i) <= buff_size; i++){
		osize=buff_size - (zpara->buff_size * i);
		if ( osize > zpara->buff_size ){ osize=zpara->buff_size; }
		for(j=0; ; j++){
			if ( j > 10 ){ return -10; }
			ret=ztrans_data_send_client_sub(zpara, buff_data + (zpara->buff_size * i), osize, ofd);
//printf("  [%d,%d] ret=%d osize=%d total=%d\n",i,j,ret, osize,zpara->ibuff_p->total);
			if ( ret < 0 ){ return ret; }
			if ( ret > 0 ){ break; }
		}
	}

	return ret;
}

/* CACHEバッファのデータを送信 */
int ztrans_data_send(T_ZTRANS_INFO *zpara, char *cache_data, int cache_size,int *cache_cnt, int ofd){
	int fret=1;
	int ret=0;
	int iret=0;
	int flush;
	int osize;
	int cache_addr=0;
	T_CLIENT_RESULT_S res;

	//入力
	flush = Z_NO_FLUSH;
	if (zpara->z.avail_in == 0) {

        //読込みサイズ
        cache_addr = zpara->buff_size * (*cache_cnt);
        iret = cache_size - cache_addr;
		(*cache_cnt)++;
        if ( iret > zpara->buff_size ){ iret = zpara->buff_size; }
		if ( iret < 0 ){  return -1; }
        memcpy(zpara->ibuff + sizeof(T_CLIENT_DATA_HEADER), cache_data + cache_addr, iret);

		zpara->z.next_in = (Bytef*)(zpara->ibuff + sizeof(T_CLIENT_DATA_HEADER));
		zpara->z.avail_in = iret;
		zpara->ibuff_p->seq++;
		zpara->ibuff_p->size=iret;
		zpara->ibuff_p->total+=iret;

		if ( iret != zpara->buff_size ){ 
			if ( zpara->ibuff_p->total >= zpara->file_size){ flush=Z_FINISH; }
			fret=0; 
		}

	}

	//圧縮
	ret = deflate(&zpara->z, flush);
	if ( ret == Z_OK ){
		//fret=1;
	}else if ( ret == Z_STREAM_END){
		//fret=0;
	}else{
		return -2;
	}

	//出力
	if ( fret==0 || zpara->z.avail_out == 0 ){
		osize = zpara->buff_size - zpara->z.avail_out;
		zpara->obuff_p->seq++;
		zpara->obuff_p->size=osize;
		zpara->obuff_p->total+=osize;

		ret=tcpSend(ofd, zpara->obuff, osize + sizeof(T_CLIENT_DATA_HEADER), zpara->stmout);
		if ( ret < 0 ){ return -3; }

		ret=tcpRecv(ofd, (char*)&res, sizeof(res), zpara->rtmout);
		if ( ret <= 0 ){ return -4; }
		if ( res.res != REQUEST_CLIENT_OK ){ return -5; }

		zpara->z.next_out = (Bytef*)(zpara->obuff + sizeof(T_CLIENT_DATA_HEADER));
		zpara->z.avail_out = zpara->buff_size;
	}

	return fret;
}

/* 圧縮ファイルのデータを送信 */
int ztrans_data_send(T_ZTRANS_INFO *zpara, gzFile ifd, int ofd){
	int fret=1;
	int iret=0;
	int ret=0;
	int flush;
	int osize;
	T_CLIENT_RESULT_S res;

	//入力
	flush = Z_NO_FLUSH;
	if (zpara->z.avail_in == 0) {
		iret=gzread(ifd, zpara->ibuff + sizeof(T_CLIENT_DATA_HEADER), zpara->buff_size);
		if ( iret < 0 ){  return -1; }

		zpara->z.next_in = (Bytef*)(zpara->ibuff + sizeof(T_CLIENT_DATA_HEADER));
		zpara->z.avail_in = iret;
		zpara->ibuff_p->seq++;
		zpara->ibuff_p->size=iret;
		zpara->ibuff_p->total+=iret;

		if ( iret != zpara->buff_size ){ 
			if ( zpara->ibuff_p->total >= zpara->file_size){ flush=Z_FINISH; }
			fret=0; 
		}
	}

	//圧縮
	ret = deflate(&zpara->z, flush);
	if ( ret == Z_OK ){
		//fret=1;
	}else if ( ret == Z_STREAM_END){
		//fret=0;
	}else{
		return -2;
	}

	//出力
	if ( fret==0 || zpara->z.avail_out == 0 ){
		osize = zpara->buff_size - zpara->z.avail_out;
		zpara->obuff_p->seq++;
		zpara->obuff_p->size=osize;
		zpara->obuff_p->total+=osize;

		ret=tcpSend(ofd, zpara->obuff, osize + sizeof(T_CLIENT_DATA_HEADER), zpara->stmout);
		if ( ret < 0 ){ return -3; }

		ret=tcpRecv(ofd, (char*)&res, sizeof(res), zpara->rtmout);
		if ( ret <= 0 ){ return -4; }
		if ( res.res != REQUEST_CLIENT_OK ){ return -5; }

		zpara->z.next_out = (Bytef*)(zpara->obuff + sizeof(T_CLIENT_DATA_HEADER));
		zpara->z.avail_out = zpara->buff_size;
	}

	return fret;
}

/* 非圧縮ファイルのデータを送信 */
int ztrans_data_send(T_ZTRANS_INFO *zpara, int ifd, int ofd){
	int fret=1;
	int iret=0;
	int ret=0;
	int flush;
	int osize;
	T_CLIENT_RESULT_S res;

	//入力
	flush = Z_NO_FLUSH;
	if (zpara->z.avail_in == 0) {
		iret=read(ifd, zpara->ibuff + sizeof(T_CLIENT_DATA_HEADER), zpara->buff_size);
		if ( iret < 0 ){  return -1; }

		zpara->z.next_in = (Bytef*)(zpara->ibuff + sizeof(T_CLIENT_DATA_HEADER));
		zpara->z.avail_in = iret;
		zpara->ibuff_p->seq++;
		zpara->ibuff_p->size=iret;
		zpara->ibuff_p->total+=iret;

		if ( iret != zpara->buff_size ){ 
			if ( zpara->ibuff_p->total>=zpara->file_size){ flush=Z_FINISH; }
			fret=0; 
		}
	}

	//圧縮
	ret = deflate(&zpara->z, flush);
	if ( ret == Z_OK ){
		//fret=1;
	}else if ( ret == Z_STREAM_END){
		//fret=0;
	}else{
		return -2;
	}

	//出力
	if ( fret==0 || zpara->z.avail_out == 0 ){
		osize = zpara->buff_size - zpara->z.avail_out;
		zpara->obuff_p->seq++;
		zpara->obuff_p->size=osize;
		zpara->obuff_p->total+=osize;

		ret=tcpSend(ofd, zpara->obuff, osize + sizeof(T_CLIENT_DATA_HEADER), zpara->stmout);
		if ( ret < 0 ){ return -3; }

		ret=tcpRecv(ofd, (char*)&res, sizeof(res), zpara->rtmout);
		if ( ret <= 0 ){ return -4; }
		if ( res.res != REQUEST_CLIENT_OK ){ return -5; }

		zpara->z.next_out = (Bytef*)(zpara->obuff + sizeof(T_CLIENT_DATA_HEADER));
		zpara->z.avail_out = zpara->buff_size;
	}

	return fret;
}

/* 非圧縮ファイルのデータを送信(timeoutあり) */
int ztrans_data_send(T_ZTRANS_INFO *zpara, int ifd, int ofd, int timeout){
	int fret=1;
	int ret=0;
	int flush;
	int osize;
	T_CLIENT_RESULT_S res;

	//入力
	flush = Z_NO_FLUSH;
	if (zpara->z.avail_in == 0) {
		ret=pipeRecv(ifd, zpara->ibuff + sizeof(T_CLIENT_DATA_HEADER), zpara->buff_size, timeout);
		//ret=mmapRecv(zpara->ibuff + sizeof(T_CLIENT_DATA_HEADER), zpara->buff_size, timeout);
		if ( ret < 0 ){  return -1; }
		if ( ret == 0 ){ return 0; }

		zpara->z.next_in = (Bytef*)(zpara->ibuff + sizeof(T_CLIENT_DATA_HEADER));
		zpara->z.avail_in = ret;
		zpara->ibuff_p->seq++;
		zpara->ibuff_p->size=ret;
		zpara->ibuff_p->total+=ret;

		if ( ret != zpara->buff_size ){ 
			if ( zpara->ibuff_p->total>=zpara->file_size){ flush=Z_FINISH; }
			fret=0; 
		}
	}

	//圧縮
	ret = deflate(&zpara->z, flush);
	if ( ret == Z_OK ){
		//fret=1;
	}else if ( ret == Z_STREAM_END){
		//fret=0;
	}else{
		return -2;
	}

	//出力
	if ( fret==0 || zpara->z.avail_out == 0 ){
		osize = zpara->buff_size - zpara->z.avail_out;
		zpara->obuff_p->seq++;
		zpara->obuff_p->size=osize;
		zpara->obuff_p->total+=osize;

		ret=tcpSend(ofd, zpara->obuff, osize + sizeof(T_CLIENT_DATA_HEADER), zpara->stmout);
		if ( ret < 0 ){ return -3; }

		ret=tcpRecv(ofd, (char*)&res, sizeof(res), zpara->rtmout);
		if ( ret <= 0 ){ return -4; }
		if ( res.res != REQUEST_CLIENT_OK ){ return -5; }

		zpara->z.next_out = (Bytef*)(zpara->obuff + sizeof(T_CLIENT_DATA_HEADER));
		zpara->z.avail_out = zpara->buff_size;
	}

	return fret;
}

int ztrans_data_recv(T_ZTRANS_INFO *zpara, int ifd, int *in_size){
	int fret;
	int iret;
	int oret;
	int ret;
	T_CLIENT_RESULT_S res;

	*in_size=0;

	//入力
	if (zpara->z.avail_in == 0) {
		//ヘッダ受信
		ret=tcpRecvFix(ifd, (char*)zpara->ibuff, sizeof(T_CLIENT_DATA_HEADER),zpara->rtmout);
		if ( ret < 0 ){ return -1; }

		if ( ret == 0 ){
			zpara->z.next_in = (Bytef*)(zpara->ibuff + sizeof(T_CLIENT_DATA_HEADER));
			zpara->z.avail_in = 0;
		}else{
			//サーバから受信(データ部)
			iret=tcpRecvFix(ifd, zpara->ibuff + sizeof(T_CLIENT_DATA_HEADER), zpara->ibuff_p->size, zpara->rtmout);
   			if ( iret != zpara->ibuff_p->size ){ return -3; }

			//応答を返す
			res.res=REQUEST_CLIENT_OK;
			res.r_cnt=0;
			res.r_size=0;
			ret=tcpSend(ifd,(char *)&res,sizeof(res),zpara->stmout);
			if ( ret < 0 ){ return -4; }

			zpara->z.next_in = (Bytef*)(zpara->ibuff + sizeof(T_CLIENT_DATA_HEADER));
			zpara->z.avail_in = iret;
		}
	}

	//展開
	ret = inflate(&zpara->z, Z_NO_FLUSH);
	if ( ret == Z_OK ){
		fret=1;
	}else if ( ret == Z_STREAM_END){
		fret=0;
	}else{
		return -5;
	}

	//出力
	if ( fret==0 || zpara->z.avail_out == 0 ){
		*in_size=zpara->buff_size - zpara->z.avail_out;
		zpara->z.next_out = (Bytef*)(zpara->obuff + sizeof(T_CLIENT_DATA_HEADER));
		zpara->z.avail_out = zpara->buff_size;
		zpara->obuff_p->seq++;
		zpara->obuff_p->size= *in_size;
		zpara->obuff_p->total+= *in_size;
	}

	return fret;
}

int ztrans_comp_fin(T_ZTRANS_INFO *zpara){
	if (deflateEnd(&zpara->z) != Z_OK) { return -1; }
	return 0;
}

int ztrans_decomp_fin(T_ZTRANS_INFO *zpara){
	if (inflateEnd(&zpara->z) != Z_OK) { return -1; }
	return 0;
}

