/*****************************************************************************
*  <システム>   分散処理フレームワーク
*  <名称>	デーモン制御
*  <目的>       
*  <機能>       
*  <開発環境>   UNIX
*  <特記事項>
*
*  VERSION    DATE        BY                CHANGE/COMMENT
*  -----------------------------------------------------------------------------
*  V0.00      2014/01/18  Takakusaki        新規作成
******************************************************************************/
#include "xi_server.h"

/*****************************************************************************
*  <関数名> print_help()
*  <機能>   ヘルプ表示
*  <説明>   
*  <引数>   
*  <リターン値> 
*		0:正常終了
*  <備考>
******************************************************************************/
int print_help(){
	printf("xicluster_server [parameter]\n\n");
	printf("  [parameter]\n");
	printf("  start       : xicluster server start\n");
	printf("  stop        : xicluster server stop\n");
	printf("  restart     : xicluster server restart\n");
	printf("  parameter   : parameter information\n");
	printf("  status      : cluster information\n");
	printf("  perf        : performance information\n");
	printf("  process     : process information\n");
	printf("  cache       : cache information\n");
	printf("  mem         : shared memory information\n");
	printf("  dump <file> : data file dump\n");
	printf("\n");
	return 0;
}

/*****************************************************************************
*  <関数名> main()
*  <機能>   メイン処理
*  <説明>   
*  <引数>   
*  <リターン値> 
*		0:正常終了
*       0以外:異常終了
*  <備考>
******************************************************************************/
main(int argc,char **argv){
	int i;
	int ret;
	int  l_RunType=0;
	char dumpfile[MAX_FILE_PATH+1];

	//パラメータ解析
	dumpfile[0]=NULL;
	for (i=1; i<argc; i++){
		//printf("argc=%d [%s]\n",argc,argv[i]);
		if ( argv[i][0] == '-' ){
			continue;
		}
		if ( l_RunType==REQUEST_SERVER_DUMP ){
			strcpy(dumpfile,argv[i]);
			continue;
		}
		if ( strcmp(argv[i],"start") == 0 ){
			l_RunType=REQUEST_SERVER_START;
			continue;
		}
		if ( strcmp(argv[i],"stop") == 0 ){
			l_RunType=REQUEST_SERVER_STOP;
			continue;
		}
		if ( strcmp(argv[i],"restart") == 0 ){
			l_RunType=REQUEST_SERVER_RESTART;
			continue;
		}
		if ( strcmp(argv[i],"dump") == 0 ){
			l_RunType=REQUEST_SERVER_DUMP;
			continue;
		}
		if ( strcmp(argv[i],"parameter") == 0 ){
			l_RunType=REQUEST_CLIENT_PARA;
			continue;
		}
		if ( strcmp(argv[i],"status") == 0 ){
			l_RunType=REQUEST_CLIENT_STATUS;
			continue;
		}
		if ( strcmp(argv[i],"status2") == 0 ){
			l_RunType=REQUEST_CLIENT_STATUS2;
			continue;
		}
		if ( strcmp(argv[i],"perf") == 0 ){
			l_RunType=REQUEST_CLIENT_PERF;
			continue;
		}
		if ( strcmp(argv[i],"process") == 0 ){
			l_RunType=REQUEST_CLIENT_PROCESS;
			continue;
		}
		if ( strcmp(argv[i],"mem") == 0 ){
			l_RunType=REQUEST_CLIENT_MEM;
			continue;
		}
		if ( strcmp(argv[i],"cache") == 0 ){
			l_RunType=REQUEST_CLIENT_CACHE;
			continue;
		}
		if ( strcmp(argv[i],"volume") == 0 ){
			l_RunType=REQUEST_CLIENT_VOLUME;
			continue;
		}
		if ( strcmp(argv[i],"memhex") == 0 ){
			l_RunType=REQUEST_SERVER_MEMHEX;
			continue;
		}
	}
	if ( l_RunType == 0 ){
		print_help();
		exit(1);
	}

	//ベースディレクトリ
	commGetBaseDir(G_BaseDir);
	sprintf(G_TempDir,"%s/temp",G_BaseDir);
	if ( filisDirectory(G_TempDir) != 1 ){
		printf("No Diretory(%s)\n",G_TempDir);
		exit(1);
	}
	sprintf(G_LogDir,"%s/log",G_BaseDir);
	if ( filisDirectory(G_LogDir) != 1 ){
		printf("No Diretory(%s)\n",G_LogDir);
		exit(1);
	}
	sprintf(G_ConfDir,"%s/conf",G_BaseDir);
	if ( filisDirectory(G_ConfDir) != 1 ){
		printf("No Diretory(%s)\n",G_ConfDir);
		exit(1);
	}
	//if ( l_RunType == REQUEST_SERVER_DUMP ){
	//	ret=filisFile(dumpfile);
	//	if ( ret != 1 ){
	//		printf("file open error (%s)\n",dumpfile);
	//		exit(1);
	//	}
	//}

	//IPCキー
	G_ShmKey=ftok(G_TempDir, 1);	
	G_SemKey=ftok(G_TempDir, 2);	
	G_MquKey=ftok(G_TempDir, 3);	

	//サーバ起動処理
	if ( l_RunType == REQUEST_SERVER_START ){
		ret=server_start(); 
		if ( ret != 0 ){ server_stop(); }
	}
	if ( l_RunType == REQUEST_SERVER_STOP ){ server_stop(); }
	if ( l_RunType == REQUEST_SERVER_RESTART ){
		server_stop();
		printf("\n"); 
		//sleep(3);
		ret=server_start(); 
		if ( ret != 0 ){ server_stop(); }
	}
	if ( l_RunType == REQUEST_SERVER_DUMP ){
		dispFileDump(dumpfile);
	}
	if ( l_RunType == REQUEST_CLIENT_PARA ){ server_parameter(); }
	if ( l_RunType == REQUEST_CLIENT_STATUS ){ server_status(); }
	if ( l_RunType == REQUEST_CLIENT_STATUS2 ){ server_status2(); }
	if ( l_RunType == REQUEST_CLIENT_PERF ){ server_perf(); }
	if ( l_RunType == REQUEST_CLIENT_PROCESS ){ server_process(); }
	if ( l_RunType == REQUEST_CLIENT_MEM ){ server_mem(); }
	if ( l_RunType == REQUEST_CLIENT_CACHE ){ server_cache(); }
	if ( l_RunType == REQUEST_CLIENT_VOLUME ){ server_volume(); }
	if ( l_RunType == REQUEST_SERVER_MEMHEX ){ server_memhex(); }

	exit(0);
}
