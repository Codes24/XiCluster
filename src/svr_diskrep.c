/*****************************************************************************
*  <システム>   分散処理フレームワーク
*  <名称>	ディスクレプリケーションAPI
*  <目的>		
*  <機能>	   
*  <開発環境>   UNIX
*  <特記事項>
*
*  VERSION	DATE		BY				CHANGE/COMMENT
*  -----------------------------------------------------------------------------
*  V0.00	  2014/05/16  Takakusaki		新規作成
******************************************************************************/
#include "xi_server.h"

int diskrepReplecateBlock(int pno, T_FILE_ID file_id, u_long block_no, u_long recv_total, int node_idx, int wflg){
	int ret;
	int iret;
	int i;
	int ofd;
	int ifd;
	int out_size=0;
	gzFile zp;
	int fret=0;
	long addr=0;
	char dirname[MAX_FILE_PATH+1];
	char filename1[MAX_FILE_PATH+1];
	char filename2[MAX_FILE_PATH+1];
	T_CLIENT_REQUEST req;
	T_CLIENT_RESULT_S res;
	T_ZTRANS_INFO zpara;
	T_FILE_ID dir_id;
	T_IDX_HEADER ihd;

	//DATAディレクトリ作成
	disknmGetDataDirName("", file_id, dirname);
	ret=filCreateDirectory(dirname);
	if ( ret < 0 ){
		logPrint(DEBUG_ERROR,"filCreateDirectory(%s) => %d",dirname,ret);
		ret=tcpClose(ifd);
		logPrint(DEBUG_DEBUG,"tcpClose(%d)=%d",ifd, ret);
		return -1;
	}
	if ( filisDirectory(dirname) != 1 ){
		logPrint(DEBUG_ERROR,"not found directory (%s)",dirname);
		ret=tcpClose(ifd);
		logPrint(DEBUG_DEBUG,"tcpClose(%d)=%d",ifd, ret);
		return -2;
	}

	//ファイル存在チェック
	sprintf(filename1,"%s/%d",dirname, block_no);
	if ( filisFile(filename1) == 1 ){
		logPrint(DEBUG_WARNING,"%s => file exist",filename1);
		return 0;
	}
	sprintf(filename1,"%s/%d.gz",dirname, block_no);
	if ( filisFile(filename1) == 1 ){
		logPrint(DEBUG_WARNING,"%s => file exist",filename1);
		return 0;
	}

	//DATAファイルチェック
	sprintf(filename1,"%s/%d.%d.tmp",dirname, block_no,getpid());
	if ( filisFile(filename1) == 1 ){
		logPrint(DEBUG_WARNING,"%s exist",filename1);
		ret=unlink(filename1);
		if ( ret != 0 ){
			logPrint(DEBUG_ERROR,"unlink(%s)=%d",filename1,ret);
			ret=tcpClose(ifd);
			logPrint(DEBUG_DEBUG,"tcpClose(%d)=%d",ifd, ret);
			return -3; 
		}
	}


	//ノード接続
	addr=G_NodeTbl[node_idx].svr_ip;
	ifd=transConnect(addr,G_ConfTbl->network_port_cache);
	if ( ifd < 0 ){
		logPrint(DEBUG_ERROR,"transConnect(%s,%d)=%d",utlDisplyIP((u_char*)&addr),G_ConfTbl->network_port_cache,ifd);
		return -10;
	}
	logPrint(DEBUG_INFO,"transConnect(%s,%d)=%d",utlDisplyIP((u_char*)&addr),G_ConfTbl->network_port_cache,ifd);

	//データブロック取得要求
	commReqCheckSumSet(&req.ck);
	req.req=REQUEST_TASK_DATA;
	req.uid=getuid();
	req.gid=getgid();
	req.compression=0;
	memcpy(&(req.data.file_id), &file_id, sizeof(T_FILE_ID));
	req.data.block_no=block_no;
	ret=transSendBuff(ifd, (char*)&req, sizeof(req), &res);
	if ( ret <= 0 ){
		logPrint(DEBUG_WARNING,"transSendBuff()=%d file_id=%s cnt=%lu size=%lu",ret, dispFileID(&file_id), res.r_cnt, res.r_size);
		ret=tcpClose(ifd);
		logPrint(DEBUG_DEBUG,"tcpClose(%d)=%d",ifd, ret);
		return 0;
	}
	//if ( res.r_size <= 0 ){
	//	logPrint(DEBUG_WARNING,"transSendBuff()=%d file_id=%s cnt=%lu size=%lu",ret, dispFileID(&file_id), res.r_cnt, res.r_size);
	//	ret=tcpClose(ifd);
	//	logPrint(DEBUG_DEBUG,"tcpClose(%d)=%d",ifd, ret);
	//	return 0;
	//}	
	logPrint(DEBUG_INFO,"transSendBuff()=%d file_id=%s cnt=%lu size=%lu",ret, dispFileID(&file_id), res.r_cnt, res.r_size);


	//ファイルオープン
	int comp_type=daemon_CompressType(res.r_size);
	if ( comp_type == 0 ){
		sprintf(filename2,"%s/%d",dirname, block_no);
		if ( filisFile(filename2) == 1 ){
			logPrint(DEBUG_WARNING,"file exist (%s)",filename2);
		}
		if ( (ofd=open(filename1,O_CREAT|O_RDWR|O_TRUNC,0622)) < 0 ){
			logPrint(DEBUG_ERROR,"open(%s)=%d",filename1,ofd);
			ret=tcpClose(ifd);
			logPrint(DEBUG_DEBUG,"tcpClose(%d)=%d",ifd, ret);
			return -20;
		}
	}else{
		sprintf(filename2,"%s/%d.gz",dirname, block_no);
		if ( filisFile(filename2) == 1 ){
			logPrint(DEBUG_WARNING,"file exist (%s)",filename2);
		}
		if ( (zp=gzopen(filename1,"wb9")) == NULL ){
			logPrint(DEBUG_ERROR,"gzopen(%s)",filename1);
			ret=tcpClose(ifd);
			logPrint(DEBUG_DEBUG,"tcpClose(%d)=%d",ifd, ret);
			return -20;
		}
	}


    //解凍初期化
    memset(&zpara,0,sizeof(T_ZTRANS_INFO));
    zpara.file_size=res.r_size;
    zpara.buff_size=G_ConfTbl->network_buff_size;
    zpara.stmout=G_ConfTbl->send_timeout;
    zpara.rtmout=G_ConfTbl->recv_timeout;
    ztrans_decomp_init(&zpara);

	//リモートノードよりブロックを受信してDATAファイルに書込む
	while( 1 ){

        //サーバから受信
        iret=ztrans_data_recv(&zpara, ifd, &out_size);
        if ( iret < 0 ){
            logPrint(DEBUG_ERROR,"ztrans_data_recv()=%d",iret);
			fret=(-13);
            break;
        }

		//書込み
		if ( out_size > 0 ){
			if ( comp_type == 0 ){
				ret=write(ofd, zpara.obuff + sizeof(T_CLIENT_DATA_HEADER), out_size);
				if ( ret != out_size ){
					logPrint(DEBUG_ERROR,"write(%d,*,%d)=%d",ofd, out_size, ret);
					fret=(-14);
					break;
				}
			}else{
				ret=gzwrite(zp, zpara.obuff + sizeof(T_CLIENT_DATA_HEADER), out_size);
				if ( ret != out_size ){
					logPrint(DEBUG_ERROR,"gzwrite(%d,*,%d)=%d",ofd, out_size, ret);
					fret=(-14);
					break;
				}
			}
		}

		if ( iret == 0 ){ break; }
	}

    ztrans_decomp_fin(&zpara);

	//ファイルクローズ
	if ( comp_type == 0 ){
		close(ofd);
	}else{
		gzclose(zp);
	}
	ret=tcpClose(ifd);
	logPrint(DEBUG_DEBUG,"tcpClose(%d)=%d",ifd, ret);

	//異常のあった場合はここでリターン
	if ( fret < 0 ){
		ret=unlink(filename1);
		return fret; 
	}

	//ファイル名変更
	ret=rename(filename1,filename2);
	if ( ret != 0 ){
		logPrint(DEBUG_ERROR,"rename(%s,%s)=%d",filename1,filename2,ret);
		return -30;	
	}
	logPrint(DEBUG_INFO,"T:W:[%s] %dbyte",filename2, recv_total);

	if ( wflg == 1 ){ return fret; }

	//INDEXヘッダよりdir_idを取得
	disknmGetIdxFileName("", file_id, filename1);
    if ( (ifd=open(filename1, O_RDONLY)) < 0 ){
            logPrint(DEBUG_ERROR,"open(%s)=%d",filename1,ifd);
            return -31;
    }
    ret=read(ifd, (char*)&ihd, sizeof(T_IDX_HEADER));
    if ( ret != sizeof(T_IDX_HEADER) ){ close(ifd); return -32; }
    close(ifd);
	memcpy(&dir_id, &(ihd.dir_id), sizeof(T_FILE_ID));


	//INDEX更新情報を送信
	ret=transCacheSendIndexAdd(pno, dir_id, file_id, block_no, recv_total);
	if ( ret < 0 ){
		logPrint(DEBUG_ERROR,"transCacheSendIndexAdd(%d,%s,%s,%d,%d)=%d",pno,
			dispFileID(&dir_id), dispFileID(&file_id),block_no,recv_total,ret);
		return -33;	
	}
	logPrint(DEBUG_INFO,"transCacheSendIndexAdd(%d,%s,%s,%d,%d)=%d",pno,
			dispFileID(&dir_id), dispFileID(&file_id),block_no,recv_total,ret);

	return 1;
}

/* ファイル単位のレプリケーション数を再算出 */
int diskrepRepNumReCal(T_FILE_ID file_id, u_long block_no){
	int ret;
	int dup_cnt;
	T_IDX_HEADER ihd;
	T_META_HEADER mhd;
	T_META_INFO  mdata;

	//ブロックのDUP数取得
	dup_cnt=diskrdGetDataBlockDup(file_id, block_no, &ihd);
	if ( dup_cnt < 0 ){ return -1; }
	if ( dup_cnt == 0 ){ return 0; }

	//META情報取得
	ret=diskrdGetFileInfo(ihd.dir_id, file_id, &mhd, &mdata);
	if ( ret < 0 ){ return -3; }
	if ( ret == 0 ){ return 0; }
	if ( mdata.dup_cnt >= dup_cnt ){ return 0; }

	//DUP数更新
	//logPrint(DEBUG_INFO,"%s dup_cnt %d -> %d",dispFileID(&file_id), mdata.dup_cnt, dup_cnt);
	mdata.dup_cnt = dup_cnt;
	ret=diskwtWriteMetaRecord(ihd.dir_id, mdata, 1);
	if ( ret < 0 ){ return -4; }

	return 1;
}
