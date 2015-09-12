/*****************************************************************************
*  <システム>   分散処理フレームワーク
*  <名称>	クライアント・コマンドライン
*  <目的>	
*  <機能>	   
*  <開発環境>   UNIX
*  <特記事項>
*
*  VERSION	DATE		BY				CHANGE/COMMENT
*  -----------------------------------------------------------------------------
*  V0.00	  2014/01/18  Takakusaki		新規作成
******************************************************************************/
#include "xi_common.h"
#include "xi_client.h"

//定数
#define CMD_HISTORY_MAX		30
#define CMD_BUFF			2560
#define MAX_FILE_PATH       2047

//マクロ
#define BUFF_CLEAR client_printf(0,"XICLUSTER>"); in_cnt=0; in_buff[0]=NULL;
#define LINE_CLEAR client_printf(0,"\r\033[2K");
#define BACK_SPACE client_printf(0,"\033[1D");
#define LEFT_CLEAR client_printf(0,"\033[0K");
#define CURSOR_MOVE(n) client_printf(0,"\033[%dD",(n));

static int  l_debug=0;
static int  l_shinchoku=1;
static int  l_CtrlC=0;
static int  l_mode=0;
static char l_cdir[CMD_BUFF];
static char l_ldir[CMD_BUFF];
static unsigned char sv_buff[CMD_HISTORY_MAX][CMD_BUFF];
static int    sv_cnt=-1;

static char *logLevel2Name(int lvl){
    if ( lvl == DEBUG_DEBUG ){ return "DEB"; }
    if ( lvl == DEBUG_INFO ){ return "INF"; }
    if ( lvl == DEBUG_NOTICE ){ return "WAR"; }
    if ( lvl == DEBUG_WARNING ){ return "WAR"; }
    if ( lvl == DEBUG_ERROR ){ return "ERR"; }
    if ( lvl == DEBUG_CRIT ){ return "ERR"; }
    if ( lvl == DEBUG_ALERT ){ return "ERR"; }
    return "";
}


int client_printf(int flg, char *msgfmt, ...)
{
    char mbuf[10240];
    va_list argptr;

	if ( flg == 0 && l_shinchoku == 0 ){ return 0; }

    va_start(argptr,msgfmt);
    vsprintf(mbuf,msgfmt,argptr);
    va_end(argptr);

    write(1,mbuf,strlen(mbuf));
    return 0;
}

int client_debug(int lvl, char *msgfmt, ...)
{
    char mbuf[10240];
    struct tm *dt;
    time_t nowtime;
    va_list argptr;

    if ( l_debug == 0 and lvl <= DEBUG_INFO ){ return 0; }

    /* 可変引数文字列の編集  */
    va_start(argptr,msgfmt);
    vsprintf(mbuf,msgfmt,argptr);
    va_end(argptr);

	LINE_CLEAR
    printf("\r%s:%s\n",logLevel2Name(lvl),mbuf);

    return 0;
}

int client_get(int req_no, char *in_file, char *out_file){
	int ofd;
	int ifd;
	int ret;
	int oret=0;
	int wret;
	int iret;
	char buff[20480];
	unsigned long fsize=0;
	unsigned long ftotal=0;
	struct timeval str_dt;
	struct timeval end_dt;
	double elap_dt;
	double elap_spd;
	struct xi_stat fstat;

	client_debug(DEBUG_INFO,"client_get(%d,%s,%s)",req_no,in_file,out_file);
	gettimeofday(&str_dt,NULL);

	//出力先チェック
	if ( filisFile(out_file) == 1 ){
		client_debug(DEBUG_ERROR,"file exist (%s)",out_file);
		return -1;
	}

	//ファイル情報取得
	ret=xi_stat(in_file, &fstat);
	if ( ret < 0 ){
		//client_debug(DEBUG_ERROR,"file stat get error (%s)",in_file);
		client_debug(DEBUG_ERROR,xi_message);
		return -2;
	}
	fsize=fstat.st_size;

	//出力ファイルオープン
	if ( (ofd=open(out_file,O_CREAT|O_RDWR|O_TRUNC,0666)) < 0 ){
		client_debug(DEBUG_ERROR,"write open error (%s)",out_file);
		return -4;
	}

	//HDFS側初期化
	if ( (ifd=xi_open(in_file, XI_READ)) < 0 ){
		//client_debug(DEBUG_ERROR,"write open error (%s)=%d",in_file,ifd);
		client_debug(DEBUG_ERROR,xi_message);
		close(ofd);
		return -3;
	}

	//結果受信
    l_CtrlC=0;
	while(1){

		//読込み
		iret=xi_read(ifd, buff, sizeof(buff));
		if ( iret == 0 ){ break; }
		if ( iret < 0 ){
			client_debug(DEBUG_ERROR,xi_message);
			oret=-11;
			break;
		}

		//書込み
		wret=write(ofd, buff, iret);
		if ( iret != wret ){
			client_debug(DEBUG_ERROR,"write error (%s)",out_file);
			oret=-6;
			break;
		}

		//進捗
		ftotal+=iret;
		gettimeofday(&end_dt,NULL);
		elap_dt=tmTimevalElapsed(str_dt, end_dt);
		elap_spd=tmTransSpeedMB(ftotal, elap_dt);
		client_printf(0,"\r%d/%d(%.0f%) %.2fsec %.2fMB/sec",ftotal,fsize, (double)ftotal/fsize*100, elap_dt, elap_spd);
		

		//停止
		if ( l_CtrlC != 0 ){
			LINE_CLEAR
			client_printf(1,"Ctrl+C\n");
			xi_close(ifd);
			close(ofd);
			return -1;
		}

		if ( iret == 0 ){ break; }
	}

	xi_close(ifd);
	close(ofd);

	//終了処理
	gettimeofday(&end_dt,NULL);
	elap_dt=tmTimevalElapsed(str_dt, end_dt);
	elap_spd=tmTransSpeedMB(ftotal, elap_dt);
	client_debug(DEBUG_INFO,"size=%d elap=%.6fsec %.6fMB/sec",ftotal,elap_dt,elap_spd);
	if ( oret == 0 ){
		LINE_CLEAR
		client_printf(1,"Success  %dbyte %.2fsec %.2fMB/sec\n",ftotal, elap_dt, elap_spd);
	}else{
		unlink(out_file);
		LINE_CLEAR
		client_printf(1,"Error\n");
	}
	return oret;
}

int client_put(int req_no, char *in_file, char *out_file){
	int ret;
	int iret;
	int oret=0;
	int ifd;
	int ofd;
	unsigned long fsize=0;
	unsigned long ftotal=0;
	struct timeval str_dt;
	struct timeval end_dt;
	double elap_dt;
	double elap_spd;
	char buff[20480];

	client_debug(DEBUG_INFO,"client_put(%d,%s,%s)",req_no,in_file,out_file);
	gettimeofday(&str_dt,NULL);

	//入力ファイル
	if ( filisFile(in_file) != 1 ){
		client_debug(DEBUG_ERROR,"not found file (%s)",in_file);
		return -1;
	}
	fsize=filGetFileSize(in_file);
	if ( (ifd=open(in_file,O_RDONLY)) < 0 ){
		client_debug(DEBUG_ERROR,"read open error (%s)",in_file);
		return -2;
	}

	//HDFS側初期化
	if ( (ofd=xi_open(out_file, XI_WRITE|XI_COMP_LEVEL_6)) < 0 ){
		//client_debug(DEBUG_ERROR,"write open error (%s)=%d",out_file,ofd);
		client_debug(DEBUG_ERROR,xi_message);
		return -3;
	}

	//ファイル転送
    l_CtrlC=0;
	while( 1 ){

		//データ送信
		iret=read(ifd, buff, sizeof(buff));
		if ( iret == 0 ){ break; }
		if ( iret < 0 ){
			oret=-10;
			break;
		}

		ret=xi_write(ofd, buff, iret);
		if ( ret < 0 ){
			client_debug(DEBUG_ERROR,xi_message);
			oret=-11;
			break;
		}

		//進捗
		ftotal+=iret;
		gettimeofday(&end_dt,NULL);
		elap_dt=tmTimevalElapsed(str_dt, end_dt);
		elap_spd=tmTransSpeedMB(ftotal, elap_dt);
		client_printf(0,"\r%d/%d(%.0f%) %.2fsec %.2fMB/sec",ftotal,fsize, (double)ftotal/fsize*100, elap_dt, elap_spd);

		//停止
		if ( l_CtrlC != 0 ){
			LINE_CLEAR
			client_printf(1,"Ctrl+C\n");
			xi_close(ofd);
			close(ifd);
			return -12;
		}
	}

	xi_close(ofd);
	close(ifd);

	if ( oret != 0 ){
		LINE_CLEAR
		client_printf(1,"Error\n");
		return oret;
	}

	//終了処理
	gettimeofday(&end_dt,NULL);
	elap_dt=tmTimevalElapsed(str_dt, end_dt);
	elap_spd=tmTransSpeedMB(ftotal, elap_dt);
	client_debug(DEBUG_INFO,"size=%d elap=%.6fsec %.6fMB/sec",ftotal,elap_dt,elap_spd);
	if ( oret == 0 ){
		LINE_CLEAR
		client_printf(1,"Success  %dbyte %.2fsec %.2fMB/sec\n",ftotal, elap_dt, elap_spd);
	}

	return oret;
}

/*****************************************************************************
*  <関数名> print_help()
*  <機能>   ヘルプ表示
*  <説明>
*  <引数>
*  <リターン値>
*	   0:正常終了
*  <備考>
******************************************************************************/
int print_help(){
	printf("xicluster_client [option] [parameter]\n\n");
	printf("  [parameter]\n");
	printf("  status          : cluster information\n");
	printf("  perf            : server performance\n");
	printf("  process         : process information\n");
	printf("  mem             : shared memory information\n");
	printf("  cache           : cache information\n");
	printf("  volume          : volume information\n");
	printf("  ls              : file information\n");
	printf("  get             : file get\n");
	printf("  put             : file put\n");
	printf("\n");
	printf("  [option]\n");
	printf("  -h <ip address> : connect host\n");
	printf("  -p <portno>     : connect port no\n");
	printf("  -d              : debug mode\n");
	printf("  -n              : no display\n");
	printf("\n");
	return 0;
}

int command_help(){

	printf("\n");
	printf("$xicluster_client\n");
	printf("XICLUSTER> [command] [parameter...]\n\n");
	printf("  [command]\n");
	printf("  status                    : cluster information\n");
	printf("  perf                      : server performance\n");
	printf("  process                   : process information\n");
	printf("  mem                       : shared memory information\n");
	printf("  cache                     : cache information\n");
	printf("\n");

	printf("  cd <directory name>       : change current directory\n");
	printf("  pwd                       : display current directory\n");
	printf("  ls                        : file information\n");
	printf("  mkdir <directory name>    : create directory\n");
	printf("  rmdir <directory name>    : delete directory\n");
	printf("  rm <file name>            : delete file\n");
	printf("  chmod <mode> <file name>  : change file permission\n");
	printf("  chown <owner> <file name> : change file owner\n");
	printf("  chgrp <group> <file name> : change file group\n");
	printf("  get <file name>           : file get\n");
	printf("  put <file name>           : file put\n");
	printf("  exit                      : command line exit\n");
	printf("  [Ctrl]+[d]                : command line exit\n");
	printf("\n");
	return 0;
}

void xicluster_intsig(int signo)
{
    l_CtrlC=signo;
	signal(signo, xicluster_intsig);
}

char *ConvertDirectory(char *cdir, char *para, char *para_add){
	int i;
	int cnt;
	char wk[CMD_BUFF];
	static char w_dir[CMD_BUFF];

	//ディレクトリ名置換
	if ( strcmp(para,".") == 0 or strcmp(para,"./") == 0 ){
		strcpy(wk,filGetFileName(para_add));
		sprintf(w_dir,"%s/%s",cdir,wk);

	}else if ( strcmp(para,"..") == 0 or strcmp(para,"../") == 0 ){
		strcpy(wk,filGetFileName(para_add));
		sprintf(w_dir,"%s/%s",filGetDirName(cdir),wk);

	}else if ( strcmp(para,"/") == 0 ){
		strcpy(wk,filGetFileName(para_add));
		sprintf(w_dir,"/%s",wk);

	}else if ( para[0]=='.' && para[1]=='/' ){
		sprintf(w_dir,"%s/%s",cdir,para + 2);

	}else if ( para[0]=='.' && para[1]=='.' && para[2]=='/' ){
		sprintf(w_dir,"%s/%s",filGetDirName(cdir), para + 3);

	}else if ( para[0] == '/' ){
		strcpy(w_dir,para);

	}else if ( para[0] == (char)NULL ){
		strcpy(w_dir,cdir);

	}else{
		sprintf(w_dir,"%s/%s",cdir,para);
	}

	//スラッシュ終わりなら
	if ( para_add[0]!=(char)NULL && w_dir[strlen(w_dir) - 1]=='/' ){
		strcpy(wk,filGetFileName(para_add));
		strcat(w_dir,wk);
	}

	//不要除去(//を除去)
	strcpy(wk,w_dir);
	for(i=0,cnt=0; i<strlen(wk); i++){
		if ( wk[i]=='/' && wk[i+1]=='/' ){
			continue;
		}
		w_dir[cnt]=wk[i];
		cnt++;
		w_dir[cnt]=(char)NULL;
	}
	int len=strlen(w_dir);
	if ( len>1 && w_dir[len -1 ] == '/' ){ w_dir[ len -1 ]=(char)NULL; }

	client_debug(DEBUG_INFO,"ConvertDirectory(%s,%s,%s) -> %s",cdir,para,para_add, w_dir);

	return w_dir;
}

int check_para(char *cmd, int l_mode, char *in_para1, char *in_para2){

	if ( l_mode == 0 ){ return 0; }

	if ( l_mode == REQUEST_CLIENT_EXIT){ return -1; }

	//パラメータチェック
	if ( l_mode == REQUEST_CLIENT_GET and (in_para1[0]==(char)NULL or in_para2[0]==(char)NULL) ){
		printf("%sget <local file> <remote file>\n",cmd);
		return 1;
	}
	if ( l_mode == REQUEST_CLIENT_PUT and (in_para1[0]==(char)NULL or in_para2[0]==(char)NULL) ){
		printf("%sput <remote file> <local file>\n",cmd);
		return 1;
	}
	if ( l_mode == REQUEST_CLIENT_MKDIR and in_para1[0]==(char)NULL ){
		printf("%smkdir <directory path>\n",cmd);
		return 1;
	}
	if ( l_mode == REQUEST_CLIENT_RMDIR and in_para1[0]==(char)NULL ){
		printf("xicluster_client rmdir <directory path>\n",cmd);
		return 1;
	}
	if ( l_mode == REQUEST_CLIENT_MV and (in_para1[0]==(char)NULL or in_para2[0]==(char)NULL) ){
		printf("%smv <file path> <file path>\n",cmd);
		return 1;
	}
	if ( l_mode == REQUEST_CLIENT_RM and in_para1[0]==(char)NULL ){
		printf("%srm <file path>\n",cmd);
		return 1;
	}
	if ( l_mode == REQUEST_CLIENT_CHMOD and (in_para1[0]==(char)NULL or in_para2[0]==(char)NULL) ){
		printf("%schmod <mode> <file path>\n",cmd);
		return 1;
	}
	if ( l_mode == REQUEST_CLIENT_CHOWN and (in_para1[0]==(char)NULL or in_para2[0]==(char)NULL) ){
		printf("%schown <user name> <file path>\n",cmd);
		return 1;
	}
	if ( l_mode == REQUEST_CLIENT_CHGRP and (in_para1[0]==(char)NULL or in_para2[0]==(char)NULL) ){
		printf("%schgrp <group name> <file path>\n",cmd);
		return 1;
	}

	return 0;
}

void StringDump(unsigned char *buff){
	int i;
	for(i=0; i<strlen((char*)buff); i++){
		printf("%02X ",buff[i]);
	}
	printf("\n");

}

void StringCutCtrlCode(unsigned char *buff){
	int i,j;
	unsigned char wk[CMD_BUFF];

	//StringDump(buff);

	//変換
	wk[0]=NULL;
	for(i=0,j=0; i<strlen((char*)buff); i++){
		if ( buff[i] == 0x0a || buff[i+1] == 0x0d){
			break;
		}
		if ( buff[i] == 0x1b && buff[i+1] == 0x5B && buff[i+2]==0x44 ){
			i+=2;
			continue;
		}
		if ( buff[i] == 0x1b && buff[i+1] == 0x5B && buff[i+2]==0x43 ){
			i+=2;
			continue;
		}
		wk[j++]=buff[i];
		wk[j]=NULL;
	}
	strcpy((char*)buff,(char*)wk);

	//StringDump(buff);
}

int GetLmode(char *para){
	if ( strcmp(para,"help") == 0 ){ return REQUEST_HELP; }
	if ( strcmp(para,"exit") == 0 ){ return REQUEST_CLIENT_EXIT; }
	if ( strcmp(para,"quit") == 0 ){ return REQUEST_CLIENT_EXIT; }
	if ( strcmp(para,"pwd") == 0 ){ return REQUEST_CLIENT_PWD; }
	if ( strcmp(para,"cd") == 0 ){ return REQUEST_CLIENT_CD; }
	if ( strcmp(para,"history") == 0 ){ return REQUEST_CLIENT_HISTORY; }

	if ( strcmp(para,"cluster") == 0 ){ return REQUEST_CLIENT_STATUS; }
	if ( strcmp(para,"status") == 0 ){ return REQUEST_CLIENT_STATUS; }
	if ( strcmp(para,"status2") == 0 ){ return REQUEST_CLIENT_STATUS2; }
	if ( strcmp(para,"perf") == 0 ){  return REQUEST_CLIENT_PERF; }
	if ( strcmp(para,"df") == 0 ){ return REQUEST_CLIENT_PERF; }
	if ( strcmp(para,"vmstat") == 0 ){  return REQUEST_CLIENT_PERF; }
	if ( strcmp(para,"ps") == 0 ){ return REQUEST_CLIENT_PROCESS; }
	if ( strcmp(para,"process") == 0 ){ return REQUEST_CLIENT_PROCESS; }
	if ( strcmp(para,"mem") == 0 ){ return REQUEST_CLIENT_MEM; }
	if ( strcmp(para,"cache") == 0 ){ return REQUEST_CLIENT_CACHE; }
	if ( strcmp(para,"vol") == 0 ){ return REQUEST_CLIENT_VOLUME; }
	if ( strcmp(para,"volume") == 0 ){ return REQUEST_CLIENT_VOLUME; }
	if ( strcmp(para,"disk") == 0 ){ return REQUEST_CLIENT_VOLUME; }

	if ( strcmp(para,"mkdir") == 0 ){ return REQUEST_CLIENT_MKDIR; }
	if ( strcmp(para,"rmdir") == 0 ){ return REQUEST_CLIENT_RMDIR; }
	if ( strcmp(para,"ls") == 0 ){ return REQUEST_CLIENT_LS; }
	if ( strcmp(para,"dir") == 0 ){ return REQUEST_CLIENT_LS; }
	if ( strcmp(para,"stat") == 0 ){ return REQUEST_CLIENT_STAT; }
	if ( strcmp(para,"mv") == 0 ){ return REQUEST_CLIENT_MV; }
	if ( strcmp(para,"rm") == 0 ){ return REQUEST_CLIENT_RM; }
	if ( strcmp(para,"chmod") == 0 ){ return REQUEST_CLIENT_CHMOD; }
	if ( strcmp(para,"chown") == 0 ){ return REQUEST_CLIENT_CHOWN; }
	if ( strcmp(para,"chgrp") == 0 ){ return REQUEST_CLIENT_CHGRP; }
	if ( strcmp(para,"get") == 0 ){ return REQUEST_CLIENT_GET; }
	if ( strcmp(para,"put") == 0 ){ return REQUEST_CLIENT_PUT; }
	return 0;
}

int command_exec(int l_mode, char *in_para1, char *in_para2){
	int i;
	int ret;

	client_debug(DEBUG_INFO,"command_exec(%d,%s,%s)",l_mode,in_para1,in_para2);

	if ( l_mode == REQUEST_HELP ){ 
		command_help();
		return 0;
	}

	//サーバ接続不要コマンド系
	if ( l_mode == REQUEST_CLIENT_PWD ){ 
		printf("%s\n",l_cdir);
		return 0;
	}
	if ( l_mode == REQUEST_CLIENT_CD && in_para1[0] == (char)NULL ){
		printf("%s\n",l_cdir);
		return 0;
	}
	if ( l_mode == REQUEST_CLIENT_HISTORY ){ 
		for(i=0; i<CMD_HISTORY_MAX; i++){
			if ( sv_buff[i][0]==NULL){ continue; }
			printf("[%d] %s\n",i,sv_buff[i]);
		}
		return 0;
	}

	//ファイルチェック
	if ( l_mode == REQUEST_CLIENT_GET ){
		strcpy(in_para1,ConvertDirectory(l_cdir, in_para1,""));
		strcpy(in_para2,ConvertDirectory(l_ldir, in_para2, in_para1));
		if ( filisFile(in_para2) == 1 ){
			client_debug(DEBUG_ERROR,"file exist (%s)",in_para2);
			return 0;
		}
	}

	//処理分岐
	if ( l_mode == REQUEST_CLIENT_CD ){ 
		struct xi_stat fstat;
		strcpy(in_para1, ConvertDirectory(l_cdir, in_para1,"") );
		ret=xi_stat(in_para1, &fstat);
		if ( ret == 0 ){
			if ( fstat.st_type == FILE_TYPE_DIR ){
				strcpy(l_cdir, ConvertDirectory(l_cdir, in_para1,"")); 
			}else{
				printf("not directory\n");
			}
		}
	}
	if ( l_mode == REQUEST_CLIENT_STATUS ){ 
		ret=xi_info(l_mode); 
		if ( ret < 0 ){ client_debug(DEBUG_ERROR, xi_message); }
	}
	if ( l_mode == REQUEST_CLIENT_STATUS2 ){ 
		ret=xi_info(l_mode); 
		if ( ret < 0 ){ client_debug(DEBUG_ERROR, xi_message); }
	}
	if ( l_mode == REQUEST_CLIENT_PERF ){
		ret=xi_info(l_mode); 
		if ( ret < 0 ){ client_debug(DEBUG_ERROR, xi_message); }
	}
	if ( l_mode == REQUEST_CLIENT_PROCESS ){
		ret=xi_info(l_mode); 
		if ( ret < 0 ){ client_debug(DEBUG_ERROR, xi_message); }
	}
	if ( l_mode == REQUEST_CLIENT_MEM ){
		ret=xi_info(l_mode); 
		if ( ret < 0 ){ client_debug(DEBUG_ERROR, xi_message); }
	}
	if ( l_mode == REQUEST_CLIENT_CACHE ){
		ret=xi_info(l_mode); 
		if ( ret < 0 ){ client_debug(DEBUG_ERROR, xi_message); }
	}
	if ( l_mode == REQUEST_CLIENT_VOLUME ){
		ret=xi_info(l_mode); 
		if ( ret < 0 ){ client_debug(DEBUG_ERROR, xi_message); }
	}
	if ( l_mode == REQUEST_CLIENT_LS ){
		strcpy(in_para1, ConvertDirectory(l_cdir,in_para1,"") );
		ret=xi_info(l_mode,in_para1); 
		if ( ret < 0 ){ client_debug(DEBUG_ERROR, xi_message); }
	}
	if ( l_mode == REQUEST_CLIENT_STAT ){
		strcpy(in_para1, ConvertDirectory(l_cdir,in_para1,"") );
		ret=xi_info(l_mode,in_para1); 
		if ( ret < 0 ){ client_debug(DEBUG_ERROR, xi_message); }
	}
	if ( l_mode == REQUEST_CLIENT_MKDIR ){
		strcpy(in_para1, ConvertDirectory(l_cdir,in_para1,"") );
		ret=xi_mkdir(in_para1);
		if ( ret < 0 ){ client_debug(DEBUG_ERROR, xi_message); }
	}
	if ( l_mode == REQUEST_CLIENT_RMDIR ){
		strcpy(in_para1, ConvertDirectory(l_cdir,in_para1,"") );
		ret=xi_rmdir(in_para1);
		if ( ret < 0 ){ client_debug(DEBUG_ERROR, xi_message); }
	}
	if ( l_mode == REQUEST_CLIENT_MV ){
		strcpy(in_para1, ConvertDirectory(l_cdir,in_para1,"") );
		strcpy(in_para2, ConvertDirectory(l_cdir,in_para2,in_para1) );
		ret=xi_rename(in_para1,in_para2);
		if ( ret < 0 ){ client_debug(DEBUG_ERROR, xi_message); }
	}
	if ( l_mode == REQUEST_CLIENT_RM ){
		strcpy(in_para1, ConvertDirectory(l_cdir,in_para1,"") );
		ret=xi_unlink(in_para1);
		if ( ret < 0 ){ client_debug(DEBUG_ERROR, xi_message); }
	}
	if ( l_mode == REQUEST_CLIENT_CHMOD ){
		strcpy(in_para2, ConvertDirectory(l_cdir,in_para2,"") );
		ret=xi_chmod( in_para2, atoi(in_para1));
		if ( ret < 0 ){ client_debug(DEBUG_ERROR, xi_message); }
	}
	if ( l_mode == REQUEST_CLIENT_CHOWN ){
		ret=utlName2Uid(in_para1);
		if ( ret < 0 ){
			client_debug(DEBUG_ERROR,"unknown user name");
		}else{
			strcpy(in_para2, ConvertDirectory(l_cdir,in_para2,"") );
			ret=xi_chown(in_para2,ret);
			if ( ret < 0 ){ client_debug(DEBUG_ERROR, xi_message); }
		}
	}
	if ( l_mode == REQUEST_CLIENT_CHGRP ){
		ret=utlName2Gid(in_para1);
		if ( ret < 0 ){
			client_debug(DEBUG_ERROR,"unknown group name");
		}else{
			strcpy(in_para2, ConvertDirectory(l_cdir,in_para2,"") );
			ret=xi_chgrp(in_para2,ret);
			if ( ret < 0 ){ client_debug(DEBUG_ERROR, xi_message); }
		}
	}
	if ( l_mode == REQUEST_CLIENT_GET ){
		strcpy(in_para1,ConvertDirectory(l_cdir, in_para1,""));
		strcpy(in_para2,ConvertDirectory(l_ldir, in_para2, in_para1));
		client_get(l_mode, in_para1, in_para2 ); 
	}
	if ( l_mode == REQUEST_CLIENT_PUT ){
		strcpy(in_para2,ConvertDirectory(l_cdir, in_para2, in_para1));
		client_put(l_mode,in_para1,in_para2); 
	}

	return 0;
}

void save_command_history(unsigned char *buff){
	int i;
	sv_cnt=-1;
	for (i=CMD_HISTORY_MAX-1; i>0; i--){
		strcpy((char*)sv_buff[i], (char*)sv_buff[i-1]);
	}
	strcpy((char*)sv_buff[0], (char*)buff);
}

int tab_convert(unsigned char *wk){
	int ret;
	char *wk_last_p;
	char para1[CMD_BUFF]="";
	char para2[CMD_BUFF]="";

	if ( wk[0] == NULL ){ return 0; }

	//最終文字
	wk_last_p=utlStringSepLast((char*)wk," ",para1);
	if ( para1[0] == NULL ){ return 0; }
	strcpy(para2, ConvertDirectory(l_cdir,para1,"") );

	//該当ファイル検索
	ret=xi_tab(para2, wk_last_p);

	return 1;
}

int command_mode(){
	int  i;
	int  ret;
	char cmd[CMD_BUFF];
	char in_para1[CMD_BUFF]="";
	char in_para2[CMD_BUFF]="";
	struct termios term_attr;
	struct termios SavedTermAttr;
	unsigned char wk1[CMD_BUFF];
	unsigned char wk2[CMD_BUFF];
	unsigned char in_buff[CMD_BUFF];
	int  in_cnt=0;

	//初期化
	signal(SIGINT, xicluster_intsig);
	signal(SIGQUIT, xicluster_intsig);
	signal(SIGTERM, xicluster_intsig);
	l_CtrlC=0;
	in_buff[0]=(char)NULL;
	for (i=0; i<CMD_HISTORY_MAX; i++){ sv_buff[i][0]=(char)NULL; }

	//ターミナル設定
	if( tcgetattr(0, &term_attr) < 0 ){ return -1; }
	SavedTermAttr = term_attr;
	term_attr.c_lflag &= ~(ICANON|ECHO);
	term_attr.c_cc[VMIN] = 1;
	term_attr.c_cc[VTIME] = 0;
	if( tcsetattr(0, TCSANOW, &term_attr) < 0 ){ return -1; }

	//入力ループ
	i=0;
	client_printf(1,"XICLUSTER>");
	while(1){
		ret=read(0,wk1,1);

		if ( ret <= 0 ){ 
			printf("\n");
			break; 
		}

		//エスケープシーケンスの場合
		if ( wk1[0]==0x1b ){
			ret=read(0,wk1+1,2);
			if ( ret <= 0 ){ 
				printf("\n");
				break; 
			}
		}

		//上
		if ( wk1[0]==0x1b && wk1[1]==0x5b && wk1[2] == 0x41 ){
			//コマンドセーブ
			if ( sv_cnt == -1 && in_cnt > 0 ){
				save_command_history(in_buff);
			}

			LINE_CLEAR
			BUFF_CLEAR

			//history
			sv_cnt++;
			if ( sv_cnt > (CMD_HISTORY_MAX - 1) ){ sv_cnt=CMD_HISTORY_MAX - 1; }
			strcpy((char*)in_buff, (char*)sv_buff[sv_cnt]);
			client_printf(1,(char*)in_buff);
			in_cnt=strlen((char*)in_buff);
			continue; 
		}

		//下
		if ( wk1[0]==0x1b && wk1[1]==0x5b && wk1[2] == 0x42 ){
			LINE_CLEAR
			BUFF_CLEAR

			//history
			sv_cnt--;
			if ( sv_cnt < 0 ){ sv_cnt=0; }
			strcpy((char*)in_buff, (char*)sv_buff[sv_cnt]);
			client_printf(1,(char*)in_buff);
			in_cnt=strlen((char*)in_buff);
			continue; 
		}

		//右
		if ( wk1[0]==0x1b && wk1[1]==0x5b && wk1[2] == 0x43 ){
			if ( in_cnt >= strlen((char*)in_buff)){ continue; }
			in_cnt++;
			client_printf(1,"%c%c%c",wk1[0],wk1[1],wk1[2]);
			continue; 
		}

		//左
		if ( wk1[0]==0x1b && wk1[1]==0x5b && wk1[2] == 0x44 ){
			if ( in_cnt <= 0){ continue; }
			in_cnt--;
			client_printf(1,"%c%c%c",wk1[0],wk1[1],wk1[2]);
			continue; 
		}

		//Ctrl+D
		if ( wk1[0] == 0x4 ){ break; }

		//TAB
		if ( wk1[0] == 0x9 ){

			tab_convert(in_buff);
			in_cnt=strlen((char*)in_buff);

			LINE_CLEAR
			client_printf(1,"XICLUSTER>");
			client_printf(1,"%s",in_buff);
			continue; 
		}

		//back space
		if ( wk1[0] == 0x8 ){
			if ( in_cnt <= 0 ){ continue; }
			if ( in_cnt == strlen((char*)in_buff) ){
				in_cnt--;
				in_buff[in_cnt]=NULL;
				BACK_SPACE
				LEFT_CLEAR
			}else{
				BACK_SPACE
				strcpy((char*)wk2, (char*)in_buff + in_cnt);
				write(1,wk2,strlen((char*)wk2));
				LEFT_CLEAR
				CURSOR_MOVE(strlen((char*)wk2));
				in_cnt--;
				strcpy((char*)in_buff + in_cnt,(char*)wk2);
			}
			continue; 
		}

		//改行
		if ( wk1[0] == 0xa or wk1[0]==0xd ){

			client_printf(1,"\n");

			//制御コード除外
			StringCutCtrlCode(in_buff);

			//パラメータ分割
			utlStringSep((char*)in_buff," ",1,cmd);
			utlStringSep((char*)in_buff," ",2,in_para1);
			utlStringSep((char*)in_buff," ",3,in_para2);
			if ( cmd[0] == (char)NULL ){ 
				BUFF_CLEAR
				continue; 
			}

			//コマンドのセーブ
			save_command_history(in_buff);

			//処理判定
			l_mode=GetLmode(cmd);
			if ( l_mode == 0 ){
				client_printf(1,"command not found (%s)\n",cmd); 
				BUFF_CLEAR
				continue; 
			}
			if ( l_mode == REQUEST_CLIENT_EXIT ){ break; }
			if ( l_mode == REQUEST_CLIENT_LS ){ strcpy(in_para1,l_cdir); }

			//パラメータチェック
			ret=check_para("", l_mode, in_para1, in_para2);
			if ( ret != 0 ){
				BUFF_CLEAR
				continue; 
			}

			//実行
			command_exec(l_mode, in_para1, in_para2);
			BUFF_CLEAR
			continue; 
		}
		//printf("[%d][%02X] ret=%d errno=%d\n",in_cnt,in_buff[in_cnt],ret,errno);

		//コマンド文字数オーバ
		if ( in_cnt > 2048 ){
			client_printf(1,"\n");
			BUFF_CLEAR
			continue; 
		}

		//通常出力
		if ( in_cnt == strlen((char*)in_buff) ){
			if ( wk1[0]<0x20 || wk1[0]>0x7f ){ continue; }
			in_buff[in_cnt++]=wk1[0];
			in_buff[in_cnt]=NULL;
			write(1,wk1,1);
		}else{
			wk2[0]=wk1[0];
			strcpy((char*)wk2+1, (char*)in_buff + in_cnt);
			write(1,(char*)wk2,strlen((char*)wk2));
			CURSOR_MOVE(strlen((char*)wk2)-1);
			strcpy((char*)in_buff + in_cnt, (char*)wk2);
			in_cnt++;
		}

	}

	//ターミナル設定を戻す
	if( tcsetattr(0, TCSANOW, &SavedTermAttr) < 0 ){ return -1; }

	return 0;
}


/*****************************************************************************
*  <関数名> main()
*  <機能>   メイン処理
*  <説明>
*  <引数>
*  <リターン値>
*	   0:正常終了
*	   0以外:異常終了
*  <備考>
******************************************************************************/
main(int argc, char **argv){
	int ret;
	int flg=0;
	int i;
	int wk;
	struct hostent *hserv;
	char hostname[1024]="";
	char in_para1[CMD_BUFF]="";
	char in_para2[CMD_BUFF]="";

	//デフォルト
	strcpy(l_cdir,"/");
	getcwd(l_ldir, sizeof(l_ldir) );

	//パラメータ解析
	for (i=1; i<argc; i++){
		if ( strlen(argv[i]) >= MAX_FILE_PATH ){
			printf("parameter to long\n");
			exit(1);
		}
		if ( argv[i][0] == '-' ){
			if ( strcmp(argv[i],"-help") == 0 ){ print_help(); exit(1); }
			if ( strcmp(argv[i],"-p") == 0 ){ flg=1; }
			if ( strcmp(argv[i],"-h") == 0 ){ flg=2; }
			if ( strcmp(argv[i],"-d") == 0 ){ xi_set(SET_PARAMETER_LOG_LEVEL,1); l_debug=1; }
			if ( strcmp(argv[i],"-n") == 0 ){ xi_set(SET_PARAMETER_LOG_LEVEL,0); l_debug=0; l_shinchoku=0; }
			if ( strcmp(argv[i],"-para") == 0 ){ flg=3; }
			continue;
		}
		if ( flg != 0 ){
			if ( flg == 1 ){ xi_set(SET_PARAMETER_PORT,atoi(argv[i])); }
			if ( flg == 2 ){ xi_set(SET_PARAMETER_IPADDR,argv[i]); }
			if ( flg == 3 ){
				char p1[CMD_BUFF];
				char p2[CMD_BUFF];
				utlStringSep(argv[i],"=",1,p1);
				utlStringSep(argv[i],"=",2,p2);
				if ( xi_set(p1, p2) != 0 ){ printf("parameter error\n"); exit(1); }
			}
			flg=0;
			continue;
		}

		ret=GetLmode(argv[i]);
		if ( ret != 0 ){ l_mode=ret; continue; }
		if ( in_para1[0] == (char)NULL ){ strcpy(in_para1,argv[i]); continue; }
		if ( in_para2[0] == (char)NULL ){ strcpy(in_para2,argv[i]); continue; }
	}

	//パラメータチェック
	ret=check_para("xicluster_client", l_mode, in_para1, in_para2);
	if ( ret != 0 ){ exit(1); }


	//実行
	if ( l_mode == 0 ){
		command_mode();
	}else{
		ret=command_exec(l_mode, in_para1, in_para2);
		if ( ret != 0 ){ exit(1); }
	}

	exit(0);
}
