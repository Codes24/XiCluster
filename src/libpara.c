/*****************************************************************************
*  <システム>   分散処理フレームワーク
*  <名称>	パラメータファイルアクセスAPI
*  <目的>
*  <機能>
*  <開発環境>   UNIX
*  <特記事項>
*
*  VERSION	DATE		  BY					CHANGE/COMMENT
*  -----------------------------------------------------------------------------
*  V0.0.00	2014/01/18	  Takakusaki			  新規作成
******************************************************************************/
#include "xi_common.h"

static T_CONFIG_TABLE l_cwk;

static struct{
	int  type;		/* 1=int 2=hex 3=char */
	char *name;		/* parameter name */
	void *addr;		/* 構造体アドレス */
	char *def;		/* デフォルト値 */
	char *min;		/* 最小値 */
	char *max;		/* 最大値 */
}l_parameter[]={
{3,  "version",					l_cwk.version,						"Ver0.00", 	"",		""},
{1,  "max_node",				&(l_cwk.max_node),					"255",		"1",	"9999"},

/* ノード間通信 */
{1,  "data_trans_interval",		&(l_cwk.data_trans_interval),		"5",		"1",	"999999999"},
{1,  "dsync_trans_interval",	&(l_cwk.dsync_trans_interval),		"5",		"1",	"999999999"},
{1,  "disk_mente_interval",		&(l_cwk.disk_mente_interval),		"10",		"1",	"999999999"},
{1,  "dsync_trans_wt_elap",		&(l_cwk.dsync_trans_wt_elap),		"10",		"1",	"999999999"},
{1,  "nodetbl_write_interval",	&(l_cwk.nodetbl_write_interval),	"5",		"1",	"999999999"},
{1,  "node_hang_timeout",		&(l_cwk.node_hang_timeout),			"60",		"1",	"999999999"},
{1,  "node_removal_time",		&(l_cwk.node_removal_time),			"12096000",	"1",	"999999999"},
{1,  "meta_sync_max_rec",		&(l_cwk.meta_sync_max_rec),			"100",		"1",	"999999999"},
{1,  "meta_check_interval",		&(l_cwk.meta_check_interval),		"5",		"1",	"999999999"},

/* HDFS関連 */
{1,  "block_dup_cnt",			&(l_cwk.block_dup_cnt),				"3",		"0",	""},
{1,  "cache_files",				&(l_cwk.cache_files),				"1000",		"10",	""},
{1,  "cache_blocks",			&(l_cwk.cache_blocks),				"5",		"0",	""},
{1,  "max_data_dir",			&(l_cwk.max_data_dir),				"10",		"0",	"100"},
{2,  "def_perm_fil",			&(l_cwk.def_perm_fil),				"644",		"",		""},
{2,  "def_perm_dir",			&(l_cwk.def_perm_dir),				"755",		"",		""},
{1,  "max_file_per_dir",		&(l_cwk.max_file_per_dir),			"5000",		"0",	""},
{1,  "max_blocks",				&(l_cwk.max_blocks),				"100000",	"0",	""},
{1,  "jlog_max_rec",			&(l_cwk.jlog_max_rec),				"10000",	"1",	""},
{1,  "compress_threshold",		&(l_cwk.compress_threshold),		"1024",		"1",	""},
{1,  "compress_type",			&(l_cwk.compress_type),				"1",		"-1",	"3"},

/*ログ関連 */
{1,  "debug_level_file",		&(l_cwk.debug_level_file),			"2",		"-1",   "10"},
{1,  "debug_level_stdout",		&(l_cwk.debug_level_stdout),		"7",		"-1",   "10"},

/* プロセス関連 */
{1,  "max_process",				&(l_cwk.max_process),				"100", 		"10",	"9999"},
{1,  "task_process",			&(l_cwk.task_process),				"10", 		"1",	"9999"},
{1,  "cache_process",			&(l_cwk.cache_process),				"2", 		"1",	"999"},
{1,  "msyn_process",			&(l_cwk.msyn_process),				"1", 		"0",	"999"},
{1,  "dsyn_process",			&(l_cwk.dsyn_process),				"1", 		"0",	"999"},
{1,  "sleep_loop_sman",			&(l_cwk.sleep_loop_sman),			"1000000", 	"-1",	"999999999"},
{1,  "sleep_loop_nlsr",			&(l_cwk.sleep_loop_nlsr),			"0",	 	"-1",	"999999999"},
{1,  "sleep_loop_clsr",			&(l_cwk.sleep_loop_clsr),			"0",	 	"-1",	"999999999"},
{1,  "sleep_loop_cach",			&(l_cwk.sleep_loop_cach),			"5000", 	"-1",	"999999999"},
{1,  "sleep_loop_crcv",			&(l_cwk.sleep_loop_crcv),			"5000", 	"-1",	"999999999"},
{1,  "sleep_loop_csnd",			&(l_cwk.sleep_loop_csnd),			"5000", 	"-1",	"999999999"},
{1,  "sleep_loop_msyn",			&(l_cwk.sleep_loop_msyn),			"5000", 	"-1",	"999999999"},
{1,  "sleep_loop_dsyn",			&(l_cwk.sleep_loop_dsyn),			"5000", 	"-1",	"999999999"},
{1,  "sleep_loop_disk",			&(l_cwk.sleep_loop_disk),			"5000", 	"-1",	"999999999"},
{1,  "sleep_loop_dmnt",			&(l_cwk.sleep_loop_dmnt),			"5000", 	"-1",	"999999999"},
{1,  "sleep_loop_task",			&(l_cwk.sleep_loop_task),			"5000", 	"-1",	"999999999"},
{1,  "slow_start",				&(l_cwk.slow_start),				"5000", 	"0",	"999999999"},
{1,  "proc_ka",					&(l_cwk.proc_ka),					"90", 		"5",	"999999999"},
{1,  "fork_max",				&(l_cwk.fork_max),					"10", 		"0",	"999999999"},
{1,  "fork_clear",				&(l_cwk.fork_clear),				"3600", 	"0",	"999999999"},
{1,  "start_wait",				&(l_cwk.start_wait),				"10", 		"0",	"999999999"},
{1,  "stop_wait",				&(l_cwk.stop_wait),					"10", 		"0",	"999999999"},
{1,  "rerun_wait",				&(l_cwk.rerun_wait),				"300", 		"0",	"999999999"},
{1,  "node_watch_interval",		&(l_cwk.node_watch_interval),		"1", 		"0",	"999999999"},

/* 通信関連 */
{3,  "network_if_svr",			l_cwk.network_if_svr,				"eth0", 	"",		""},
{3,  "network_if_clt",			l_cwk.network_if_clt,				"eth0", 	"",		""},
{1,  "network_prot_svr",		&(l_cwk.network_prot_svr),			"TCP", 		"",		""},
{1,  "network_port_svr",		&(l_cwk.network_port_svr),			"9010", 	"",		""},
{1,  "network_port_clt",		&(l_cwk.network_port_clt),			"9020", 	"",		""},
{1,  "network_port_cache",		&(l_cwk.network_port_cache),		"9030", 	"",		""},
{1,  "network_buff_size",		&(l_cwk.network_buff_size),			"15000", 	"",		""},
{1,  "con_retry",				&(l_cwk.con_retry),					"3", 		"1",	"999999999"},
{1,  "con_retrans",				&(l_cwk.con_retrans),				"5000000", 	"-1",	"999999999"},
{1,  "con_timeout",				&(l_cwk.con_timeout),				"5000000", 	"-1",	"999999999"},
{1,  "ping_timeout",			&(l_cwk.ping_timeout),				"1000000", 	"-1",	"999999999"},
{1,  "select_timeout",			&(l_cwk.select_timeout),			"5000000", 	"-1",	"999999999"},
{1,  "send_timeout",			&(l_cwk.send_timeout),				"30000000",	"-1",	"999999999"},
{1,  "recv_timeout",			&(l_cwk.recv_timeout),				"60000000",	"-1",	"999999999"},
{1,  "req_timeout",				&(l_cwk.req_timeout),				"100000",	"-1",	"999999999"},
{1,  "node_recv_timeout",		&(l_cwk.node_recv_timeout),			"1000000", 	"-1",	"999999999"},
{1,  "so_sndbuf",				&(l_cwk.so_sndbuf),					"65536", 	"-1",	"999999999"},	/* 16384 */
{1,  "so_rcvbuf",				&(l_cwk.so_rcvbuf),					"131072", 	"-1",	"999999999"},	/* 87380 */
{1,  "con_sleep_loop",			&(l_cwk.con_sleep_loop),			"10",		"-1",	"999999999"},
{1,  "con_compression",			&(l_cwk.con_compression),			"6",		"-1",	"10"},

/* 下位関数 */	
{1,  "pipe_send_timeout",		&(l_cwk.pipe_send_timeout),			"1000000",		"-1",	"999999999"},
{1,  "pipe_recv_timeout",		&(l_cwk.pipe_recv_timeout),			"1000000",		"-1",	"999999999"},
{1,  "mmap_blocks",				&(l_cwk.mmap_blocks),				"1",			"0",	"999999999"},
{1,  "mmap_recv_timeout",		&(l_cwk.mmap_recv_timeout),			"60000000",		"-1",	"999999999"},
{1,  "enospc_retry",			&(l_cwk.enospc_retry),				"50",		"-1",   ""},
{1,  "enospc_retrans",			&(l_cwk.enospc_retrans),			"5000",		"-1",   ""},
{1,  "lock_retry",				&(l_cwk.lock_retry),				"60",		"0",    ""},
{1,  "lock_sleep",				&(l_cwk.lock_sleep),				"1000000",	"0",    ""},
{0,  "",		 				NULL,								"",	 		"", 	""}
};

/*****************************************************************************
*  <関数名> paraDispConf
*  <機能>   パラメータ情報表示
*  <説明>   
*  <引数>
*       cwk:I:パラメータ
*  <リターン値>
*       0:正常
*  <備考>
******************************************************************************/
int paraDispPara(T_CONFIG_TABLE *cwk)
{
	int i;
	int *val;

	memcpy((char*)&l_cwk, (char*)cwk, sizeof(T_CONFIG_TABLE) );

	for ( i=0; l_parameter[i].type!=0; i++){
		if ( l_parameter[i].type == 1 ){
			val=(int*)l_parameter[i].addr;
			printf("%-20s = %d\n", l_parameter[i].name, *val);
		}
		if ( l_parameter[i].type == 2 ){
			val=(int*)l_parameter[i].addr;
			printf("%-20s = 0x%X\n", l_parameter[i].name, *val);
		}
		if ( l_parameter[i].type == 3 ){
			printf("%-20s = %s\n", l_parameter[i].name, l_parameter[i].addr);
		}
	}

	return 0;
}

/*****************************************************************************
*  <関数名> paraCheckPara
*  <機能>   パラメータチェック
*  <説明>   
*  <引数>
*       cwk:I:パラメータ
*  <リターン値>
*       0:チェックOK
*       -1:チェックNG
*  <備考>
******************************************************************************/
int paraCheckPara(T_CONFIG_TABLE *cwk)
{
	int i;
	int *val;

	memcpy((char*)&l_cwk, (char*)cwk, sizeof(T_CONFIG_TABLE) );

	for ( i=0; l_parameter[i].type!=0; i++){
		if ( l_parameter[i].type == 3 ) continue;
		val=(int*)l_parameter[i].addr;
		if ( l_parameter[i].min[0]!=(char)NULL ){
			if ( *val <= atoi(l_parameter[i].min) ){
				fprintf(stderr,"Parameter Error(%s=%d)\n",l_parameter[i].name,*val);
				return -1;
			}
		}
		if ( l_parameter[i].max[0]!=(char)NULL ){
			if ( *val >= atoi(l_parameter[i].max) ){
				fprintf(stderr,"Parameter Error(%s=%d)\n",l_parameter[i].name,*val);
				return -1;
			}
		}
	}

	return 0;
}

/*****************************************************************************
*  <関数名> paraSetDefault
*  <機能>   デフォルト値設定
*  <説明>   
*  <引数>
*       cwk:I:パラメータ
*  <リターン値>
*       0:正常
*  <備考>
******************************************************************************/
int paraSetDefault(T_CONFIG_TABLE *cwk){
	int i;
	int *val;
	int wk;

	memset((char*)&l_cwk, 0, sizeof(T_CONFIG_TABLE) );

	for ( i=0; l_parameter[i].type!=0; i++){
		if ( l_parameter[i].type == 1 ){
			val=(int*)l_parameter[i].addr;
			*val = atoi(l_parameter[i].def);
		}
		if ( l_parameter[i].type == 2 ){
			val=(int*)l_parameter[i].addr;
			sscanf(l_parameter[i].def,"%x",&wk);
			*val = wk;
		}
		if ( l_parameter[i].type == 3 ){
			if ( l_parameter[i].def[0] == (char)NULL ) continue;
			strcpy((char*)l_parameter[i].addr, (char*)l_parameter[i].def);
		}
	}

	memcpy((char*)cwk, (char*)&l_cwk, sizeof(T_CONFIG_TABLE) );
	return 0;
}

int paraModPara(T_CONFIG_TABLE *cwk, char *para, char *val){
	int i;
	int idx=-1;
	int *addr;
	int wk;

	if ( para[0] == NULL ){ return 0; }
	if ( val[0] == NULL ){ return 0; }

	memcpy((char*)&l_cwk, cwk, sizeof(T_CONFIG_TABLE) );

	//パラメータ検索とチェック
	for ( i=0; l_parameter[i].type!=0; i++){
		if ( strcmp(l_parameter[i].name,para) != 0 ){ continue; }
		if ( l_parameter[i].type == 1 ){
			if ( atoi(val) < atoi(l_parameter[i].min) ){ return -2; }
			if ( atoi(val) > atoi(l_parameter[i].max) ){ return -2; }
		}
		idx=i;
		break;
	}
	if ( idx < 0 ){ return -1; }

	//値設定
	if ( l_parameter[idx].type == 1 ){
		addr=(int*)l_parameter[idx].addr;
		*addr = atoi(val);
	}
	if ( l_parameter[idx].type == 2 ){
		addr=(int*)l_parameter[idx].addr;
		sscanf(val,"%x",&wk);
		*addr = wk;
	}
	if ( l_parameter[idx].type == 3 ){
		strcpy((char*)l_parameter[idx].addr, (char*)val);
	}

	memcpy((char*)cwk, (char*)&l_cwk, sizeof(T_CONFIG_TABLE) );
	return 0;
}


/*****************************************************************************
*  <関数名>		paraReadConfFile
*  <機能>		設定ファイル読込み
*  <説明>		設定ファイル読込みを読み込んで変数に設定する。
*  <引数>		
*		path:I:ファイル名
*		wk:O:パラメータ
*  <リターン値>
*		0:正常
*		-1:異常
*  <備考>
******************************************************************************/
int paraReadConfFile(char *path, T_CONFIG_TABLE *wk)
{
	FILE *fp;
	char buff[10240];
	char p1[10240];
	char p2[10240];
	char p3[10240];

	G_DataDir=(T_DISK_TABLE*)malloc( sizeof(T_DISK_TABLE) );

	/* 設定読込み */
	if ( (fp=fopen(path,"r")) == NULL ){
		return 1;
	}
	while( fgets(buff,sizeof(buff),fp) != NULL ){
		if ( buff[0] == '#' ){ continue; }
		utlCurNewLine(buff);
		utlStringSep(buff," =\t",1,p1);
		utlStringSep(buff," =\t",2,p2);
		if ( p1[0] == (char)NULL ){ continue; }
		if ( p2[0] == (char)NULL ){ continue; }
		//printf("%s = %s\n",p1,p2);

		if ( strcmp(p1,"version") == 0 ){ strcpy(wk->version,p2); }
		if ( strcmp(p1,"max_node") == 0 ){ wk->max_node=atoi(p2); }

		/* ノード間通信 */
		if ( strcmp(p1,"data_trans_interval") == 0 ){ wk->data_trans_interval=atoi(p2); }
		if ( strcmp(p1,"dsync_trans_interval") == 0 ){ wk->dsync_trans_interval=atoi(p2); }
		if ( strcmp(p1,"disk_mente_interval") == 0 ){ wk->disk_mente_interval=atoi(p2); }
		if ( strcmp(p1,"dsync_trans_wt_elap") == 0 ){ wk->dsync_trans_wt_elap=atoi(p2); }
		if ( strcmp(p1,"nodetbl_write_interval") == 0 ){ wk->nodetbl_write_interval=atoi(p2); }
		if ( strcmp(p1,"node_hang_timeout") == 0 ){ wk->node_hang_timeout=atoi(p2); }
		if ( strcmp(p1,"node_removal_time") == 0 ){ wk->node_removal_time=atoi(p2); }
		if ( strcmp(p1,"meta_sync_max_rec") == 0 ){ wk->meta_sync_max_rec=atoi(p2); }
		if ( strcmp(p1,"meta_check_interval") == 0 ){ wk->meta_check_interval=atoi(p2); }

		/* HDFS関連 */
		if ( strcmp(p1,"block_dup_cnt") == 0 ){ wk->block_dup_cnt=atoi(p2); }
		if ( strcmp(p1,"cache_files") == 0 ){ wk->cache_files=atoi(p2); }
		if ( strcmp(p1,"cache_blocks") == 0 ){ wk->cache_blocks=atoi(p2); }
		if ( strcmp(p1,"max_data_dir") == 0 ){ wk->max_data_dir=atoi(p2); }
		if ( strcmp(p1,"def_perm_fil") == 0 ){ wk->def_perm_fil=atoi(p2); }
		if ( strcmp(p1,"def_perm_dir") == 0 ){ wk->def_perm_dir=atoi(p2); }
		if ( strcmp(p1,"max_file_per_dir") == 0 ){ wk->max_file_per_dir=atoi(p2); }
		if ( strcmp(p1,"max_blocks") == 0 ){ wk->max_blocks=atoi(p2); }
		if ( strcmp(p1,"jlog_max_rec") == 0 ){ wk->jlog_max_rec=atoi(p2); }
		if ( strcmp(p1,"compress_threshold") == 0 ){ wk->compress_threshold=atoi(p2); }
		if ( strcmp(p1,"compress_type") == 0 ){ wk->compress_type=atoi(p2); }

		/* ログ */
		if ( strcmp(p1,"debug_level_file") == 0 ){ wk->debug_level_file=atoi(p2); }
		if ( strcmp(p1,"debug_level_stdout") == 0 ){ wk->debug_level_stdout=atoi(p2); }

		/* プロセス関連 */
		if ( strcmp(p1,"max_process") == 0 ){ wk->max_process=atoi(p2); }
		if ( strcmp(p1,"cache_process") == 0 ){ wk->cache_process=atoi(p2); }
		if ( strcmp(p1,"msyn_process") == 0 ){ wk->msyn_process=atoi(p2); }
		if ( strcmp(p1,"dsyn_process") == 0 ){ wk->dsyn_process=atoi(p2); }
		if ( strcmp(p1,"task_process") == 0 ){ wk->task_process=atoi(p2); }
		if ( strcmp(p1,"sleep_loop_sman") == 0 ){ wk->sleep_loop_sman=atoi(p2); }
		if ( strcmp(p1,"sleep_loop_nlsr") == 0 ){ wk->sleep_loop_nlsr=atoi(p2); }
		if ( strcmp(p1,"sleep_loop_clsr") == 0 ){ wk->sleep_loop_clsr=atoi(p2); }
		if ( strcmp(p1,"sleep_loop_cach") == 0 ){ wk->sleep_loop_cach=atoi(p2); }
		if ( strcmp(p1,"sleep_loop_crcv") == 0 ){ wk->sleep_loop_crcv=atoi(p2); }
		if ( strcmp(p1,"sleep_loop_csnd") == 0 ){ wk->sleep_loop_csnd=atoi(p2); }
		if ( strcmp(p1,"sleep_loop_msyn") == 0 ){ wk->sleep_loop_msyn=atoi(p2); }
		if ( strcmp(p1,"sleep_loop_dsyn") == 0 ){ wk->sleep_loop_dsyn=atoi(p2); }
		if ( strcmp(p1,"sleep_loop_disk") == 0 ){ wk->sleep_loop_disk=atoi(p2); }
		if ( strcmp(p1,"sleep_loop_dmnt") == 0 ){ wk->sleep_loop_dmnt=atoi(p2); }
		if ( strcmp(p1,"sleep_loop_task") == 0 ){ wk->sleep_loop_task=atoi(p2); }
		if ( strcmp(p1,"slow_start") == 0 ){ wk->slow_start=atoi(p2); }
		if ( strcmp(p1,"proc_ka") == 0 ){ wk->proc_ka=atoi(p2); }
		if ( strcmp(p1,"fork_max") == 0 ){ wk->fork_max=atoi(p2); }
		if ( strcmp(p1,"fork_clear") == 0 ){ wk->fork_clear=atoi(p2); }
		if ( strcmp(p1,"start_wait") == 0 ){ wk->start_wait=atoi(p2); }
		if ( strcmp(p1,"stop_wait") == 0 ){ wk->stop_wait=atoi(p2); }
		if ( strcmp(p1,"rerun_wait") == 0 ){ wk->rerun_wait=atoi(p2); }

		/* 通信関連 */
		if ( strcmp(p1,"network_if_svr") == 0 ){ strcpy(wk->network_if_svr,p2); }
		if ( strcmp(p1,"network_if_clt") == 0 ){ strcpy(wk->network_if_clt,p2); }
		if ( strcmp(p1,"network_prot_svr") == 0 ){ 
			strcpy(p3,p2);
			utlStrChangeSmall(p3);
			if ( strcmp(p3,"tcp") == 0 ){
				wk->network_prot_svr=REQUEST_NODE_PROT_TCP; 
			}else{ 
				wk->network_prot_svr=REQUEST_NODE_PROT_UDP; 
			}
		}
		if ( strcmp(p1,"network_port_svr") == 0 ){ wk->network_port_svr=atoi(p2); }
		if ( strcmp(p1,"network_port_cache") == 0 ){ wk->network_port_cache=atoi(p2); }
		if ( strcmp(p1,"network_port_clt") == 0 ){ wk->network_port_clt=atoi(p2); }
		if ( strcmp(p1,"network_buff_size") == 0 ){ wk->network_buff_size=atoi(p2); }
		if ( strcmp(p1,"con_retry") == 0 ){ wk->con_retry=atoi(p2); }
		if ( strcmp(p1,"con_retrans") == 0 ){ wk->con_retrans=atoi(p2); }
		if ( strcmp(p1,"con_nonblock") == 0 ){ wk->con_nonblock=atoi(p2); }
		if ( strcmp(p1,"con_timeout") == 0 ){ wk->con_timeout=atoi(p2); }
		if ( strcmp(p1,"select_timeout") == 0 ){ wk->select_timeout=atoi(p2); }
		if ( strcmp(p1,"send_timeout") == 0 ){ wk->send_timeout=atoi(p2); }
		if ( strcmp(p1,"recv_timeout") == 0 ){ wk->recv_timeout=atoi(p2); }
		if ( strcmp(p1,"req_timeout") == 0 ){ wk->req_timeout=atoi(p2); }
		if ( strcmp(p1,"node_recv_timeout") == 0 ){ wk->node_recv_timeout=atoi(p2); }
		if ( strcmp(p1,"so_sndbuf") == 0 ){ wk->so_sndbuf=atoi(p2); }
		if ( strcmp(p1,"so_rcvbuf") == 0 ){ wk->so_rcvbuf=atoi(p2); }
		if ( strcmp(p1,"con_sleep_loop") == 0 ){ wk->con_sleep_loop=atoi(p2); }
		if ( strcmp(p1,"con_compression") == 0 ){ wk->con_compression=atoi(p2); }

		/* 下位関数 */
		if ( strcmp(p1,"pipe_send_timeout") == 0 ){ wk->pipe_send_timeout=atoi(p2); }
		if ( strcmp(p1,"pipe_recv_timeout") == 0 ){ wk->pipe_recv_timeout=atoi(p2); }
		if ( strcmp(p1,"mmap_blocks") == 0 ){ wk->mmap_blocks=atoi(p2); }
		if ( strcmp(p1,"mmap_recv_timeout") == 0 ){ wk->mmap_recv_timeout=atoi(p2); }
		if ( strcmp(p1,"enospc_retry") == 0 ){ wk->enospc_retry=atoi(p2); }
		if ( strcmp(p1,"enospc_retrans") == 0 ){ wk->enospc_retrans=atoi(p2); }
		if ( strcmp(p1,"lock_retry") == 0 ){ wk->lock_retry=atoi(p2); }
		if ( strcmp(p1,"lock_sleep") == 0 ){ wk->lock_sleep=atoi(p2); }

		//ディスク
		if ( strcmp(p1,"data_dir") == 0 ){
			G_Data_Disks++;
			G_DataDir=(T_DISK_TABLE*)realloc(G_DataDir, sizeof(T_DISK_TABLE) * G_Data_Disks );
			strcpy(G_DataDir[ G_Data_Disks - 1 ].dir, p2);
		}
	}
	fclose(fp);

	return 0;
}

