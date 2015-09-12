/*****************************************************************************
*  <システム>   分散処理フレームワーク
*  <名称>       共通インクルードファイル
*  <目的>
*  <機能>
*  <開発環境>   UNIX
*  <特記事項>
*
*  VERSION    DATE        BY                CHANGE/COMMENT
*  -----------------------------------------------------------------------------
*  V0.00      2014/01/18  Takakusaki        新規作成
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <resolv.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>
#include <pwd.h>
#include <grp.h>
#include <iconv.h>
#include <syslog.h>
#include <sys/ioctl.h>
#include <sys/statvfs.h>
#include <math.h>
#include <termios.h>
#include <dirent.h>
#include <sys/mman.h>

#include <sys/prctl.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/wait.h>

/* SOCKET */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <linux/if.h>
#include <sys/un.h>

/* IPC関連 */
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>

/* セキュリティー関連 sslライブラリ必要 */
#include <openssl/sha.h>

/* 動的リンク */
#include <dlfcn.h>

/* 圧縮 */
#include <zlib.h>


/***************************************************************
 マクロ
****************************************************************/
#define VALUP1024(a)  ((  (int)((a)/1024+1)  ) * 1024)
#define BYTE2MB(a)	( (float)(a)/1024/1024 )
#define KBYTE2MB(a)	( (float)(a)/1024 )

#define CRC_CCITT16         0x1021
#define BASE_DIR            "/usr/local/xicluster"    /* ベースディレクトリ */

#define MAX_FILE_NAME       255             /* ファイル名の最大長 */
#define MAX_FILE_PATH       2047            /* ファイルフルパスの最大長 */
#define IPC_SEM_NSEMS       25              /* セマフォ集合の要素数  */
#define PERF_CPU_AVG_KEEP	10
#define NODE_ID_LEN             20
#define VOLUME_ID_LEN           20

/* デバックレベル */
#define DEBUG_DEBUG             1               /* デバックメッセージ */
#define DEBUG_INFO              2               /* メッセージ */
#define DEBUG_NOTICE            3               /* 注意 */
#define DEBUG_WARNING           4               /* 注意 */
#define DEBUG_ERROR             5               /* 異常 */
#define DEBUG_CRIT              6               /* 異常 */
#define DEBUG_ALERT             7               /* 異常 */

/* ノード間通信方式 */
#define REQUEST_NODE_PROT_TCP   0               /* ノード間情報転送方式(TCP) */
#define REQUEST_NODE_PROT_UDP   1               /* ノード間情報転送方式(UDPブロードキャスト) */

/* リクエスト　*/
#define REQUEST_CLIENT_NONE     0               /* クライアント要求 */
#define REQUEST_CLIENT_PING     1               /* クライアント要求 */
#define REQUEST_CLIENT_PWD      2               /* クライアント要求 */
#define REQUEST_CLIENT_CD       3               /* クライアント要求 */
#define REQUEST_CLIENT_FILE     4               /* クライアント要求 */
#define REQUEST_CLIENT_HISTORY  5               /* クライアント要求 */
#define REQUEST_CLIENT_REQUEST  10              /* クライアント要求 */
#define REQUEST_CLIENT_PARA     11              /* クライアント要求 */
#define REQUEST_CLIENT_STATUS   12              /* クライアント要求 */
#define REQUEST_CLIENT_STATUS2  13              /* クライアント要求 */
#define REQUEST_CLIENT_PERF     14              /* クライアント要求 */
#define REQUEST_CLIENT_PROCESS  15              /* クライアント要求 */
#define REQUEST_CLIENT_MEM      16              /* クライアント要求 */
#define REQUEST_CLIENT_CACHE    17              /* クライアント要求 */
#define REQUEST_CLIENT_VOLUME   18              /* クライアント要求 */
#define REQUEST_CLIENT_MKDIR    21              /* クライアント要求 */
#define REQUEST_CLIENT_RMDIR    22              /* クライアント要求 */
#define REQUEST_CLIENT_LS       23              /* クライアント要求 */
#define REQUEST_CLIENT_STAT     24              /* クライアント要求 */
#define REQUEST_CLIENT_MV       25              /* クライアント要求 */
#define REQUEST_CLIENT_RM       26              /* クライアント要求 */
#define REQUEST_CLIENT_CHMOD    27              /* クライアント要求 */
#define REQUEST_CLIENT_CHOWN    28              /* クライアント要求 */
#define REQUEST_CLIENT_CHGRP    29              /* クライアント要求 */
#define REQUEST_CLIENT_GET      51              /* クライアント要求 */
#define REQUEST_CLIENT_PUT      52              /* クライアント要求 */
#define REQUEST_CLIENT_TAB      98              /* クライアント要求 */
#define REQUEST_CLIENT_EXIT     99              /* クライアント要求 */
#define REQUEST_SERVER_MKDIR    100             /* サーバ要求 */
#define REQUEST_SERVER_START    101             /* サーバ要求 */
#define REQUEST_SERVER_STOP     102             /* サーバ要求 */
#define REQUEST_SERVER_RESTART  103             /* サーバ要求 */
#define REQUEST_SERVER_DUMP     104             /* サーバ要求 */
#define REQUEST_SERVER_MEMHEX   105             /* サーバ要求 */
#define REQUEST_CACHE_META_CRE  201             /* マスター転送(METAデータ) */
#define REQUEST_CACHE_META_ADD  202             /* マスター転送(METAデータ) */
#define REQUEST_CACHE_IDX_CRE   203             /* マスター転送(INDEX作成) */
#define REQUEST_CACHE_IDX_ADD   204             /* マスター転送(INDEX追加) */
#define REQUEST_CACHE_DATA      205             /* キャッシュ転送(DATAデータ) */
#define REQUEST_TASK_META       301             /* データ取得要求(META/INDEX) */
#define REQUEST_TASK_DATA       302             /* データ取得要求(DATA) */
#define REQUEST_NODE_INFO       401             /* ノード情報 */
#define REQUEST_NODE_STOP       402             /* ノード情報 */
#define REQUEST_NODE_DELETE     409             /* ノード情報 */
#define REQUEST_HELP            999             /* HELP表示 */


/* クライアント応答　*/
#define REQUEST_CLIENT_OK       0               /* クライアント応答(OK) */
#define REQUEST_CLIENT_BUSY     11              /* クライアント応答(処理中) */
#define REQUEST_CLIENT_NOMASTER 21              /* クライアント応答(NO MASTER) */
#define REQUEST_CLIENT_NG       22              /* クライアント応答(NG) */


/***************************************************************
 グローバル変数宣言
***************************************************************/
/* FILE-ID */
typedef struct{
    struct timeval id;                  /* ファイルID */
    struct timeval ver;                 /* バージョン */
}T_FILE_ID;

/* DIRヘッダ-ID */
typedef struct{
    T_FILE_ID  up_dir_id;               /* 上位ディレクトリID */
    T_FILE_ID  dir_id;                  /* ディレクトリID */
}T_DIR_ID;

/* SCN定義 */
typedef struct{
    u_long  dt;                         /* YYYYMMDD */
    u_long  seq1;						/* 通番(マスタ起動時インクリメント) */
    u_long  seq2;                       /* 通番(更新単位インクリメント) */
}T_SCN;

/* グローバルパラメータ領域 */
typedef struct{
	/* 基本情報 */
	char	version[32];			/* バージョン */
	int		max_node;				/* 最大ノード数 */

	/* ノード間 */
	int 	data_trans_interval;	/* ノードデータ転送間隔 */
	int 	dsync_trans_interval;	/* データ同期チェック間隔 */
	int 	dsync_trans_wt_elap;	/* データblock書込み後にデータ転送可能になるまでの時間 */
	int 	disk_mente_interval;	/* DISKメンテナンス間隔 */
	int 	nodetbl_write_interval;	/* ノード情報出力間隔 */
	int		node_hang_timeout;		/* 無応答監視の異常検出時間(s) */
	int 	node_removal_time;		/* ノード排除までの時間(s) */
	int		meta_sync_max_rec;		/* META同期の１回あたりの最大レコード数 */
	int		meta_check_interval;	/* METAチェック間隔 */

	/* HDFS関連 */
	int		block_dup_cnt;			/* ブロックレプリケーション数 */
	int 	cache_files;			/* ファイル情報のキャッシュ数 */
	int		cache_blocks;			/* データキャッシュのブロック数 */
	int		max_data_dir;			/* 最大データディレクトリ */
	int		def_perm_fil;			/* デフォルトパーミッション */
	int		def_perm_dir;			/* デフォルトパーミッション */
	int		max_file_per_dir;		/* １ディレクトリ中の最大ファイル数 */
	int		max_blocks;				/* １ファイルの最大ブロック数 */
	int		jlog_max_rec;			/* JournalLogの最大レコード数 */
	int		compress_threshold;		/* 圧縮ファイルサイズ閾値 */
	int		compress_type;			/* 0=圧縮なし 1=gz 2=zip 3=deflate */

	/* ログ関連 */
	int		debug_level_file;		/* デバックレベル */
	int		debug_level_stdout;		/* デバックレベル */

	/* プロセス関連 */
	int		max_process;			/* 最大プロセス数 */
	int		task_process;			/* TASKプロセス数 */
	int		cache_process;			/* CACHEプロセス数 */
	int		msyn_process;			/* META同期プロセス数 */
	int		dsyn_process;			/* DATA同期プロセス数 */
	int		sleep_loop_sman;		/* １ループ毎の処理休眠時間(ms) */
	int		sleep_loop_nlsr;		/* １ループ毎の処理休眠時間(ms) */
	int		sleep_loop_clsr;		/* １ループ毎の処理休眠時間(ms) */
	int		sleep_loop_cach;		/* １ループ毎の処理休眠時間(ms) */
	int		sleep_loop_crcv;		/* １ループ毎の処理休眠時間(ms) */
	int		sleep_loop_csnd;		/* １ループ毎の処理休眠時間(ms) */
	int		sleep_loop_msyn;		/* １ループ毎の処理休眠時間(ms) */
	int		sleep_loop_dsyn;		/* １ループ毎の処理休眠時間(ms) */
	int		sleep_loop_disk;		/* １ループ毎の処理休眠時間(ms) */
	int		sleep_loop_dmnt;		/* １ループ毎の処理休眠時間(ms) */
	int		sleep_loop_task;		/* １ループ毎の処理休眠時間(ms) */
	int		slow_start;				/* fork()間の待ち時間(ms) */
	int		proc_ka;				/* プロセスフリーズ検出時間(s) */
	int		fork_max;				/* プロセス再起動チャレンジ回数 */
	int		fork_clear;				/* 起動リトライ数クリアまでの時間(s) */
	int		start_wait;				/* 子プロセス最大起動待ち時間(s) */
	int		stop_wait;				/* 子プロセス最大終了待ち時間(s) */
	int		rerun_wait;				/* 再起動までの待ち時間(s) */
	int		node_watch_interval;	/* ノード監視間隔(s) */

	/* TCP/IP関連 */
	char	network_if_svr[32];		/* ノード間通信用I/F */
	char	network_if_clt[32];		/* クライアント間通信I/F */
	int		network_prot_svr;		/* 0=TCP / 1=UDP */
	int		network_port_svr;		/* ポート番号(ノード情報転送) */
	int		network_port_cache;		/* ポート番号(キャッシュ転送) */
	int		network_port_clt;		/* ポート番号(クライアント間) */
	int		network_buff_size;		/* ネットワーク送信最大サイズ */
	int		con_retry;				/* コネクトリトライ回数	 */
	int		con_retrans;			/* コネクト障害時のリトライ間隔(msec)	 */
	int		con_nonblock;			/* コネクト非ブロックI/O */
	int		con_timeout;			/* コネクトタイムアウト */
	int		ping_timeout;			/* ノード情報転送時のコネクトタイムアウト */
	int		select_timeout;			/* selectタイムアウト */
	int		send_timeout;		 	/* 送信可能待ちタイムアウト */
	int		recv_timeout;		 	/* 受信待ちタイムアウト */
	int		req_timeout;		 	/* リクエスト受信待ちタイムアウト */
	int		node_recv_timeout;	 	/* 受信待ちタイムアウト(ノード間通信) */
	int		so_sndbuf;				/* 送信バッファサイズ */
	int		so_rcvbuf;				/* 受信バッファサイズ */
	int		con_sleep_loop;			/* 通信時の１ループ毎の処理休眠時間(ms) */
	int		con_compression;		/* 圧縮レベル(0～9) */

	/* 下位関数 */
	int		pipe_send_timeout;	 	/* PIPE送信可能待ちタイムアウト */
	int		pipe_recv_timeout;	 	/* PIPE受信待ちタイムアウト */
	int		mmap_blocks;	 		/* mmapページブロック数 */
	int		mmap_recv_timeout;	 	/* mmap待ちタイムアウト */
	int		enospc_retry;			/* ENOSPC時のリトライ回数 */
	int		enospc_retrans;			/* ENOSPC時のリトライ秒数 */
	int		lock_retry;				/* ロック取得リトライ回数 */
	int		lock_sleep;				/* ロック取得失敗時の待ち時間(ms) */
}T_CONFIG_TABLE;

/* ディスクパフォーマンス情報 */
typedef struct{
    struct timeval tm;              /* 更新時間 */
    u_long  rd_ios;                 /* 読込み回数 */
    u_long  wr_ios;                 /* 書込み回数 */
    u_long  run_ios;                /* 実行中 */
    u_long  rd_ticks;               /* 読込み時間 */
    u_long  wr_ticks;               /* 書込み時間 */
    u_long  tot_ticks;              /* 合計時間 */
}T_DISKSTAT;

/* ディスク管理領域 */
typedef struct{
    char dir[MAX_FILE_PATH+1];      /* ディレクトリ */
    char dev[MAX_FILE_PATH+1];      /* デバイス名 */
    int     sts;                    /* 状態 */
    T_SCN   vol_scn;                /* SCN番号 */
    u_long  f_bsize;                /* ブロックサイズ */
    u_long  f_blocks;               /* ブロック数 */
    u_long  f_use;                  /* 使用ブロック数 */
    u_long  f_free;                 /* 空きブロック数 */
    T_DISKSTAT stat_a;              /* 合計 */
    T_DISKSTAT stat_s;              /* 差分 */
}T_DISK_TABLE;

/* META情報 */
typedef struct{
    T_FILE_ID file_id;              /* ファイルID */
    u_long st_type;                 /* 種類 */
    u_long st_mode;                 /* アクセス権限 */
    u_long st_uid;                  /* ユーザID */
    u_long st_gid;                  /* グループID */
    u_long st_size;                 /* ファイルサイズ(byte) */
    u_long st_blksize;              /* ブロックサイズ */
    u_long st_blocks;               /* ブロック数 */
    u_int  dup_cnt;                 /* レプリケート数 */
    struct timeval st_atm;          /* 最終アクセス時刻 */
    struct timeval st_mtm;          /* 最終更新時刻 */
    struct timeval st_ctm;          /* 最終状態変更時刻 */
    struct timeval st_dtm;          /* 削除時刻 */
    char name[MAX_FILE_NAME+1];     /* ファイル名 */
}T_META_INFO;

/* [SGA]ノード管理テーブル */
typedef struct{
        u_char  node_id[NODE_ID_LEN];                   /* ノード識別子 */
        pid_t   run_userid;                             /* 実行ユーザID */
        gid_t   run_groupid;                            /* 実行グループID */
        char    run_hostname[256];                      /* 実行ホスト名 */
        long    svr_ip;                                 /* IPアドレス(ノード間通信用) */
        long    clt_ip;                                 /* IPアドレス(クライアント間通信用) */
        int     svr_mtu;                                /* MTUサイズ(ノード間通信用) */
        int     cache_mtu;                              /* MTUサイズ(キャッシュ転送用) */
        int     clt_mtu;                                /* MTUサイズ(クライアント間通信用) */
        int     sts;                                    /* ノード状態 */
        time_t  stime;                                  /* ノード開始時刻 */
        time_t  alv;                                    /* 稼働フラク゛ */
        T_SCN   node_scn;                               /* SCN */
        int     master_flg;                             /* マスタノードフラグ */
        int     task_busy;                              /* 処理中タスク数 */
        int     task_process;                           /* TASKタスク数 */
        int     cache_process;                          /* CACHEタスク数 */
        int     msyn_process;                           /* META同期プロセス数 */
        int     dsyn_process;                           /* DATA同期プロセス数 */
        int     cpu_cnt;                                /* CPU数 */
        float   cpu_user;                               /* CPU使用率 */
        float   cpu_nice;                               /* CPU使用率 */
        float   cpu_system;                             /* CPU使用率 */
        float   cpu_idle;                               /* CPU使用率 */
        float   cpu_iowait;                             /* CPU使用率 */
        int     cache_meta_use;				    		/* METAキャッシュ使用ブロック数 */
        int     cache_data_use;                         /* DATAキャッシュ使用ブロック数 */
        int     cache_meta_blocks;						/* METAキャッシュブロック数 */
        int     cache_data_blocks;						/* DATAキャッシュブロック数 */
        u_long  sga_size;                               /* SGAサイズ */
        u_long  mem_size;                               /* メモリサイズ */
        u_long  mem_free;                               /* メモリ空きサイズ */
        u_long  mem_buffer;                             /* メモリBUFFERサイズ */
        u_long  mem_cache;                              /* メモリCACHEサイズ */
        u_long  mem_active;                             /* メモリActiveサイズ */
        u_long  mem_inactive;                           /* メモリInActiveサイズ */
        u_long  disks;                                  /* ディスク数 */
        u_long  devices;                                /* デバイス数 */
        u_long  disk_size;                              /* ディスク合計サイズ */
        u_long  disk_use;                               /* ディスク使用サイズ */
        u_long  disk_free;                              /* ディスク空きサイズ */
        u_long  hdfs_meta_use;                          /* META領域使用サイズ */
        u_long  hdfs_data_use;                          /* DATA領域使用サイズ */
}T_NODE_TABLE;


/*----------------------------------------------------------------------------------*/
typedef struct{
    time_t  dt;             /* リクエスト送信時間 */
    uint16_t    crc;        /* CRC */
}T_TRANS_SEC;

/* [通信]クライアント要求 */
typedef struct{
    T_TRANS_SEC ck;     /* 不正電文チェック */
    int req;            /* リクエストID */
    int uid;            /* リクエストUserID */
    int gid;            /* リクエストGroupID */
    int compression;	/* 圧縮率 */
    union {
        struct {
            char para1[MAX_FILE_PATH+1];
            char para2[MAX_FILE_PATH+1];
            u_long size;
        }clt;
        struct {
            T_FILE_ID   dir_id;
            T_META_INFO inf;
            u_long block_no;
            u_long size;
        }meta;
        struct {
            T_FILE_ID   dir_id;
            T_FILE_ID   file_id;
        }row;
        struct {
            T_FILE_ID dir_id;
            T_FILE_ID file_id;
            u_long block_no;
            u_long size;
            u_char  node_id[NODE_ID_LEN];
        }block;
        struct {
            T_SCN scn;
        }scn;
        struct {
            T_FILE_ID file_id;
            u_long block_no;
        }data;
        struct {
            T_NODE_TABLE inf;
        }node; 
    };
}T_CLIENT_REQUEST;

/* [通信]クライアント応答 */
typedef struct{
    int     res;
    u_long  r_cnt;
    u_long  r_size;
    long    master_addr;
    int     compression;
    char    msg[1024];
}T_CLIENT_RESULT_M;

/* [通信]クライアント応答 */
typedef struct{
    int     res;
    u_long  r_cnt;
    u_long  r_size;
    long    master_addr;
    int compression;
}T_CLIENT_RESULT_S;

/* [通信]データヘッダ */
typedef struct{
    u_long  seq;
    u_long  size;
    u_long  total;
}T_CLIENT_DATA_HEADER;

/* 圧縮通信用 */
typedef struct{
	z_stream z;
	u_long	file_size;
	int 	buff_size;
	int 	stmout;
	int 	rtmout;
	char 	*ibuff;
	char 	*obuff;
	T_CLIENT_DATA_HEADER *ibuff_p;
	T_CLIENT_DATA_HEADER *obuff_p;
}T_ZTRANS_INFO;


/***************************************************************
 グローバル変数
***************************************************************/
extern T_DISK_TABLE *G_DataDir;
extern int G_Data_Disks;

/***************************************************************
 プロトタイプ宣言
***************************************************************/
/* ファイルアクセス */
char *filGetDirName(char *path);
char *filGetFileName(char *path);
char *filGetDeviceName(char *path);
u_long filGetFileSize(char *path);
int filisDirectory(char *path);
int filisFile(char *path);
int filisDevice(char *path);
int filCheckFileStat(char *path);
int filCreateDirectory(char *path);

/* ユーティリティー */
char *utlStrStr(char *moto, int pos, int len, char *out);
void utlStrCutRear(char *moto, int len);
void utlStrCut(char *moto, char *taisyo);
void chomp(char *s);
int utlName2Uid(char *nm);
int utlName2Gid(char *nm);
char *utlUid2Name(int id);
char *utlGid2Name(int id);
u_long utlCeil(u_long c1, u_long c2);
void utlCurNewLine(char *mbuf);
void utlStrChangeSmall(char *str);
void utlStrChangeBig(char *str);
char *utlStrExtracts(char *str, char *resbuf);
char *utlStrExtractsPre(char *str, char *resbuf);
int utlTargetWordCount(char *str, char *sep);
char *utlStringSep(char *str, char *sep, int gc, char *resbuf);
char *utlStringSep2(char *str, char *sep, int gc, char *resbuf);
char *utlStringSep3(char *str, char *sep, int gc, char *resbuf);
char *utlStringSepLast(char *str, char *sep, char *resbuf);
char *utlDisplyIP(u_char *ip, char *buff);
char *utlDisplyIP(u_char *ip);
int utlCheckIPAddress(char *ip);
u_long utlHostnameToAddress(char *ip);
u_long utlGetIPAddress(char *ip);
u_long utlStrIP2long(char *ip);
u_long utlMyIpAddr(char* device_name);
void utlGetSHA1(char *in, u_char *out);
char *utlSHA1_to_HEX(u_char *sha1);

/* 時間 */
void tmTimeToVal(char *in, char *out);
char *tmVal2YYYYMMDD(time_t tm);
char *tmVal2YYYYMMDDhhmm(time_t tm);
char *tmVal2YYYYMMDDhhmmss(time_t tm);
char *tmVal2YYYYMMDDhhmmssss(struct timeval tm);
int tmCompareMicroSec(struct timeval t1, struct timeval t2);
double tmTimevalElapsed(struct timeval t1, struct timeval t2);
double tmTransSpeedMB(u_long size, double dt);

/* IPC関連 */
int ipcMquCreat(key_t key, int *mqid);
int ipcMquOpen(key_t key, int *mqid);
int ipcMquSend(int mqid, char *msg, int  msize);
int ipcMquRecv(int mqid, char *msg, int msize, int mno);
int ipcMquInfo(int mqid, int *max, int *cnt, int *si);
int ipcMquClose(int mqid);
int ipcSemCreat(key_t key, int nums, int *semid);
int ipcSemOpen(key_t key, int *semid);
int ipcSemClose(int semid);
int ipcSemLock(int  semid, int smno, int retry, int retrans );
int ipcSemUnLock(int  semid, int smno, int retry, int retrans);
int ipcSemInf(int semid, int *val);
int ipcSemSet(int semid, int smno, int val);
int ipcShmCreat(key_t key, int ssize, int *shmid);
int ipcShmOpen(key_t key, int *shmid);
char *ipcShmAttch(int shmid);
int ipcShmInfo(int shmid, int *attch, int *cpid);
int ipcShmClose(int shmid, char  *addr);
int ipcShmFin(char  *addr);

/* セマフォ関連 */
int semCreat(key_t key,int nums);
int semOpen(key_t key);
int semClose();
int semLock(int smno, int retry, int retrans);
int semUnLock(int smno, int retry, int retrans);
int semInfo(int *val);
int semInfo(int *val);

/* 共有メモリ関連 */
int shmCreat(key_t key, int size);
int shmOpen(key_t key);
int shmDelete(key_t key);
int shmClose();
int shmFin();
int shmGetAttch();
int shmGetCpid();

/* メッセージキュー関連 */
int mquCreat(key_t key);
int mquOpen(key_t key );
int mquSend(char *msg, int siz);
int mquRecv(char *msg, int siz);
int mquRecv(char *msg, int siz, int mno);
int mquClose();
int mquGetInfo(int *max,int *cnt,int *si);

/* ストリーミング通信 */
int pipeCreate(int *fdes);
int pipeSend(int fd, char* buffer, int size, int timeout );
int pipeRecv(int fd, char* buffer, int size, int timeout );
int pipeClose(int *fd);

/* mmapライブラリ */
int mmapCreate(int blocks);
int mmapSend(char* buffer, int size);
int mmapRecv(char* buffer, int size, int timeout);
int mmapClose();

/* TCP関数 */
int tcpGetMTU(int fd, char *if_name);
int tcpSetBuffSize(int fd, int buffsize_snd, int buffsize_rcv);
int tcpGetBuffSize(int fd, int *buffsize_snd, int *buffsize_rcv);
int tcpListen(u_long addr,int port);
int tcpAccept(int fd);
int tcpSelect(int fd, int type, int tm);
int tcpConnect(u_long addr, int port, int timeout, int retry, int retrans );
int tcpRecv(int fd, char* buffer, int size, int timeout );
int tcpRecvFix(int fd, char* buffer, int size, int timeout );
int tcpSend(int fd, char* buffer, int size, int timeout );
int tcpClose(int fd);

/* UDP関連 */
int udpSelect(int fd, int type, int tm);
int udpBind(int port);
int udpBroadcast(int port, char *data, int size);
int udpRecv(int fd, char* buffer, int size, int timeout );
int udpClose(int fd);

/* UNIXドメインソケット */
int udListen(const char* path);
int udConnect(const char* path);
int udClose(int fd);
int udRecv(int fd, void* buff, size_t siz, int timeout);
int udSend(int fd, int snd_fd, void* buff, int siz, int timeout);

/* パフォーマンス */
int perfGetCPUavg(int *cpu_cnt, float *cpu_user, float *cpu_nice, float *cpu_system, float *cpu_idle, float *cpu_iowait);
int perfGetCPU(int *cpu_cnt, float *cpu_user, float *cpu_nice, float *cpu_system, float *cpu_idle, float *cpu_iowait);
int perfGetMEM(u_long *mem_size, u_long *mem_free, u_long *mem_buffer, u_long *mem_cache, u_long *mem_active, u_long *mem_inactive);
int perfGetDISK(char *dev, u_long *f_bsize, u_long *f_blocks, u_long *f_use, u_long *f_free);
int perfGetDISKSTAT(char *dev, T_DISKSTAT *stat_a, T_DISKSTAT *stat_s);

/* ダイナミックオブジェクト */
int dl_open(char *filename);
int dl_close();
int dl_map();
int dl_reduce();

char *commGetBaseDir(char *wk);
int commReqCheckSumSet(T_TRANS_SEC *ck);
int commReqCheckSumJudge(T_TRANS_SEC *ck);

/* パラメータチェック */
int paraDispPara(T_CONFIG_TABLE *cwk);
int paraCheckPara(T_CONFIG_TABLE *cwk);
int paraSetDefault(T_CONFIG_TABLE *cwk);
int paraModPara(T_CONFIG_TABLE *cwk, char *para, char *val);
int paraReadConfFile(char *path, T_CONFIG_TABLE *wk);

/* 圧縮データ通信 */
int ztrans_comp_init(T_ZTRANS_INFO *zpara, int compression);
int ztrans_decomp_init(T_ZTRANS_INFO *zpara);
int ztrans_data_send_client(T_ZTRANS_INFO *zpara, char *buff_data, int buff_size, int ofd);
int ztrans_data_send(T_ZTRANS_INFO *zpara, char *cache_data, int cache_size, int *cache_cnt, int ofd);
int ztrans_data_send(T_ZTRANS_INFO *zpara,int ifd, int ofd, int timeout);
int ztrans_data_send(T_ZTRANS_INFO *zpara,int ifd, int ofd);
int ztrans_data_send(T_ZTRANS_INFO *zpara,gzFile ifd, int ofd);
int ztrans_data_recv(T_ZTRANS_INFO *zpara,int ifd, int *out_size);
int ztrans_comp_fin(T_ZTRANS_INFO *zpara);
int ztrans_decomp_fin(T_ZTRANS_INFO *zpara);

