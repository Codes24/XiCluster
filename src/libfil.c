/*****************************************************************************
*  <システム>   分散処理フレームワーク
*  <名称>	ファイルIO関連API 
*  <目的>	   
*  <機能>	   
*  <開発環境>   UNIX
*  <特記事項>
*
*  VERSION	  DATE			BY					  CHANGE/COMMENT
*  -----------------------------------------------------------------------------
*  V0.0.00	  2014/01/18	  Takakusaki			  新規作成
******************************************************************************/
#include "xi_common.h"

char *filGetDeviceName(char *path)
{
	FILE *fp;
	static char retbuff[MAX_FILE_PATH+1];
	char buff[MAX_FILE_PATH+1];
	char c1[MAX_FILE_PATH+1];
	char c2[MAX_FILE_PATH+1];
	char root_dev[MAX_FILE_PATH+1];

	root_dev[0]=(char)NULL;
	retbuff[0]=(char)NULL;
	if ( (fp=fopen("/proc/mounts","r")) == NULL ){
		return "";
	}
	while( fgets(buff,sizeof(buff),fp) != NULL ){
		sscanf(buff,"%s %s",c1,c2);
		if ( strcmp(c2,"/") == 0 ){ strcpy(root_dev,c1); continue; }
		if ( strcmp(c1,"/dev") == 0 ){ continue; }
		if ( strncmp(c1,"/dev",4) != 0 ){ continue; }
		if ( strncmp(path,c2,strlen(c2)) != 0 ){ continue; }
		strcpy(retbuff,c1);
		break;
	}
	fclose(fp);

	if ( retbuff[0] == NULL ){ strcpy(retbuff,root_dev); }

	return retbuff;
}

/*****************************************************************************
*  <関数名>		filGetDirName
*  <機能>		ディレクトリ名抽出
*  <説明>		ディレクトリ名を抽出する。
*  <引数>		
*			path:I:ファイル
*  <リターン値>
*			ディレクトリ名
*  <備考>
******************************************************************************/
char *filGetDirName(char *path)
{
	int i;
	static char buff[MAX_FILE_PATH+1];

	strcpy(buff,path);
	for ( i=strlen(buff) - 1; i>0; i--){
		if ( buff[i] == '/' ){
			buff[i]=(char)NULL;
			return buff;
		}
	}
	if ( i==0 and buff[0]=='/' ){
		buff[1]=(char)NULL;
		return buff; 
	}
	strcpy(buff,"/");
	return buff;
}

/*****************************************************************************
*  <関数名>		filGetFileName
*  <機能>		ファイル名抽出
*  <説明>		ファイル名を抽出する。
*  <引数>		
*			path:I:ファイル
*  <リターン値>
*			ファイル名
*  <備考>
******************************************************************************/
char *filGetFileName(char *path)
{
	int i;
	static char buff[MAX_FILE_PATH+1];

	strcpy(buff,path);
	for ( i=strlen(buff) - 1; i>=0; i--){
		if ( buff[i] == '/' ){
			strcpy(buff, path + i + 1);
			return buff;
		}
	}

	if ( strcmp(buff,".") == 0 ){ buff[0]=(char)NULL; }
	return buff;
}

/*****************************************************************************
*  <関数名>		filGetFileSize
*  <機能>		ファイルサイズ取得
*  <説明>		パラメータで指定したファイルのサイズを返す
*  <引数>		
*			path:I:ファイル
*  <リターン値>
*			0以上:ファイルサイズ
*			0:異常
*  <備考>
******************************************************************************/
u_long filGetFileSize(char *path)
{
	struct stat buf;
	if ( stat(path,&buf) != 0 ){ return 0; }
	return buf.st_size;
}

/*****************************************************************************
*  <関数名>		filisDirectory
*  <機能>		ディレクトリ存在チェック
*  <説明>		パラメータで指定したディレクトリが存在するかチェックする
*  <引数>		path:I:ディレクトリ
*  <リターン値>
*			   1:ディレクトリである
*			   0:ディレクトリ以外
*			   -1:異常
*  <備考>
******************************************************************************/
int filisDirectory(char *path)
{
	struct stat buf;
	if ( stat(path,&buf) != 0 ){ return -1; }
	if ( S_ISDIR(buf.st_mode) ){ return 1; }
	return 0;
}

/*****************************************************************************
*  <関数名>		filisFile
*  <機能>		ファイル存在チェック
*  <説明>		パラメータで指定したファイルが存在するかチェックする
*  <引数>		path:I:ファイル名
*  <リターン値>
*			   1:ファイルである
*			   0:ファイル以外
*			   -1:異常
*  <備考>
******************************************************************************/
int filisFile(char *path)
{
	struct stat buf;
	if ( stat(path,&buf) != 0 ){ return -1; }
	if ( S_ISREG(buf.st_mode) ){ return 1; }
	return 0;
}

/*****************************************************************************
*  <関数名>		filisDevice
*  <機能>		デバイスファイル存在チェック
*  <説明>		パラメータで指定したデバイスファイルが存在するかチェックする
*  <引数>		path:I:ファイル名
*  <リターン値>
*			   1:デバイスファイルである
*			   0:デバイスファイル以外
*			   -1:異常
*  <備考>
******************************************************************************/
int filisDevice(char *path)
{
	struct stat buf;
	if ( stat(path,&buf) != 0 ){ return -1; }
	if ( S_ISCHR(buf.st_mode) ){ return 1; }
	if ( S_ISBLK(buf.st_mode) ){ return 2; }
	if ( S_ISSOCK(buf.st_mode) ){ return 3; }
	return 0;
}

/*****************************************************************************
*  <関数名>		filCheckFileStat
*  <機能>		ファイル状態チェック
*  <説明>		パラメータで指定したファイルの状態をチェックする
*  <引数>		path:I:ファイル名
*  <リターン値>
*			   0以上:ファイル状態
*			   -1:異常
*  <備考>
******************************************************************************/
int filCheckFileStat(char *path)
{
	struct stat buf;
	if ( stat(path,&buf) != 0 ){ return -1; }
	return buf.st_mode;
}

/*****************************************************************************
*  <関数名>		filCreateDirectory
*  <機能>		ディレクトリ作成
*  <説明>		子ディレクトリを含めてディレクトリ作成
*  <引数>		path:I:ディレクトリ
*  <リターン値>
*			   1:既に存在する
*			   0:正常終了(ディレクトリ作成)
*			   -1:異常
*  <備考>
******************************************************************************/
static int filCreateDirectorySub(char *path) {
	int i;
	int ret;
	struct stat sbuf;
	char buff[MAX_FILE_PATH+1];

	//ディレクトリ作成
	strcpy(buff,"/");
	for (i=1; path[i]!=(char)NULL; i++){
		buff[i]=path[i];
		buff[i+1]=(char)NULL;
		if ( buff[i] != '/' ){ continue; }

		if ( stat(buff,&sbuf) == 0 ){
			if ( S_ISDIR(sbuf.st_mode) ){ continue; }
			return -1;
		}

		ret=mkdir(buff,0777);
		if ( ret != 0 ){ return -1; }
	}

	if ( stat(buff,&sbuf) == 0 ){
		if ( S_ISDIR(sbuf.st_mode) ){ return 0; }
		return -1;
	}
	ret=mkdir(buff,0777);
	if ( ret != 0 ){ return -1; }

	return 0;
}

int filCreateDirectory(char *path){
	int ret;
	struct stat sbuf;

	//ディレクトリチェック
	if ( stat(path,&sbuf) == 0 ){
		if ( S_ISDIR(sbuf.st_mode) ){ return 1; }
		return -1;
	}

	//ディレクトリ作成
	ret=filCreateDirectorySub(path);
	if ( ret != 0 ){ return ret; }

	//チェック
	if ( filisDirectory(path) != 1 ){ return -2; }
	return 0;
}

