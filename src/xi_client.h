/*****************************************************************************
*  <システム>   分散処理フレームワーク
*  <名称>	クライアント用インクルードファイル
*  <目的>
*  <機能>
*  <開発環境>   UNIX
*  <特記事項>
*
*  VERSION  DATE        BY              CHANGE/COMMENT
*  -----------------------------------------------------------------------------
*  V0.00      2014/07/15  Takakusaki        新規作成
******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>

/* ファイルタイプ */
#define FILE_TYPE_FILE          	1               /* 通常ファイル */
#define FILE_TYPE_DIR           	2               /* ディレクトリ */

/* パラメータ設定 */
#define SET_PARAMETER_LOG_LEVEL		1
#define SET_PARAMETER_PORT			2
#define SET_PARAMETER_IPADDR		3

/* OPENパラメータ */
#define XI_COMP_LEVEL_0				0
#define XI_COMP_LEVEL_1				1
#define XI_COMP_LEVEL_2				2
#define XI_COMP_LEVEL_3				3
#define XI_COMP_LEVEL_4				4
#define XI_COMP_LEVEL_5				5
#define XI_COMP_LEVEL_6				6
#define XI_COMP_LEVEL_7				7
#define XI_COMP_LEVEL_8				8
#define XI_COMP_LEVEL_9				9
#define XI_READ						0x10000
#define XI_WRITE					0x20000

/* エラー番号 */
#define XI_ERRNO_PARA				1				/* パラメータ不正 */
#define XI_ERRNO_PHASE				2				/* フェーズエラー */
#define XI_ERRNO_OPENED				1001			/* オープン済み */
#define XI_ERRNO_OPEN_PIPE			1002			/* PIPE作成エラー */
#define XI_ERRNO_OPEN				1003			/* オープンエラー */
#define XI_ERRNO_UNOPEN				1009			/* 未接続 */
#define XI_ERRNO_SEND				2001			/* 送信エラー */
#define XI_ERRNO_SEND_Z				2002			/* 圧縮送信エラー */
#define XI_ERRNO_RECV				3001			/* 受信エラー */
#define XI_ERRNO_RECV_Z				3002			/* 圧縮受信エラー */
#define XI_ERRNO_PIPE				4001			/* PIPE IOエラー */
#define XI_ERRNO_PIPE_CREATE		4002			/* PIPE作成エラー */
#define XI_ERRNO_PIPE_READ			4003			/* PIPE読込みエラー */
#define XI_ERRNO_PIPE_WRITE			4004			/* PIPE書込みエラー */
#define XI_ERRNO_CLOSED				8001			/* クローズ済み */
#define XI_ERRNO_CLOSE				8002			/* クローズエラー */

//グローバル変数
extern int xi_errno;
extern char xi_message[];

//構造体
struct xi_stat {
unsigned long    st_type;       /* ファイル種類 */
unsigned long    st_mode;       /* パーミッション */
unsigned long    st_uid;        /* 所有者のユーザID */
unsigned long    st_gid;        /* 所有者のグループID */
unsigned long    st_size;       /* 全体のサイズ (byte単位) */
unsigned long    st_blksize;    /* ブロックサイズ */
unsigned long    st_blocks;     /* ブロック数 */
unsigned int     dup_cnt;       /* レプリケート数 */
struct timeval st_atm;   /* 最終アクセス時刻 */
struct timeval st_mtm;   /* 最終更新時刻 */
struct timeval st_ctm;   /* 最終状態変更時刻 */
};

//プロトタイプ宣言
int xi_set(int typ, int para);
int xi_set(int typ, char *para);
int xi_open(char *path, int mod);
int xi_read(int fd, char *buff, int size);
int xi_write(int fd, char *buff, int size);
int xi_close(int fd);
int xi_info(int typ);
int xi_info(int typ, char *para);
int xi_tab(char *path, char *out_path);
int xi_stat(char *path, struct xi_stat *buf);
int xi_mkdir(char *path);
int xi_rmdir(char *path);
int xi_rename(char *oldpath, char *newpath);
int xi_unlink(char *path);
int xi_chmod(char *path, int mod);
int xi_chown(char *path, int uid);
int xi_chgrp(char *path, int gid);

