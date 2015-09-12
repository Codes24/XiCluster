/*****************************************************************************
*  <システム>   分散処理フレームワーク
*  <名称>       サーバ共通インクルードファイル
*  <目的>       
*  <機能>       
*  <開発環境>   UNIX
*  <特記事項>
*
*  VERSION    DATE        BY                CHANGE/COMMENT
*  -----------------------------------------------------------------------------
*  V0.00      2014/01/18  Takakusaki        新規作成
******************************************************************************/
#include "xi_common.h"
#include "xi_client.h"

/***************************************************************
 グローバルシンボル定義
****************************************************************/
#define BLOCK_SIZE				64*1024*1024	/* ブロックサイズ */

/* マスターフラグ */
#define MASTER_FLG_NONE			0
#define MASTER_FLG_MASTER		1
#define MASTER_FLG_SLAVE		2

/* ファイルタイプ */
#define FILE_TYPE_NODE			"NODE"			/* NODE情報ファイル */
#define FILE_TYPE_VOLUME		"VOLM"			/* VOLUME情報ファイル */
#define FILE_TYPE_META			"META"			/* META情報ファイル */
#define FILE_TYPE_INDX			"INDX"			/* INDX情報ファイル */
#define FILE_TYPE_JLOG 			"JLOG"			/* JournalLog情報ファイル */

/* Volume状態 */
#define VOLUME_STATUS_INIT 		0				/* 初期状態 */
#define VOLUME_STATUS_SYNC 		1				/* META同期中 */
#define VOLUME_STATUS_SERVICE	10				/* サービス中 */
#define VOLUME_STATUS_ERROR		-1				/* 障害中 */

/* METAキャッシュ状態 */
#define CACHE_META_STS_ERROR    -1				/* 異常終了 */
#define CACHE_META_STS_NONE		0				/* 未使用 */
#define CACHE_META_STS_MEM_S	1				/* メモリ出力中 */
#define CACHE_META_STS_MEM_E	2				/* メモリ出力完了 */
#define CACHE_META_STS_WRITE_S	3				/* ディスク書込み/キャッシュ転送中 */
#define CACHE_META_STS_WRITE_E	4				/* ディスク書込み/キャッシュ転送完了 */
#define CACHE_META_STS_SEND		5				/* META情報転送中 */
#define CACHE_META_STS_USE		10				/* 正常終了 */
#define CACHE_META_STS_CACHE	20				/* キャッシュ中 */

/* DATAキャッシュ状態 */
#define CACHE_DATA_STS_ERROR    -1				/* 異常終了 */
#define CACHE_DATA_STS_NONE		0				/* 未使用 */
#define CACHE_DATA_STS_MEM_S	1				/* メモリ出力中 */
#define CACHE_DATA_STS_MEM_E	2				/* メモリ出力完了 */
#define CACHE_DATA_STS_WRITE_S	3				/* ディスク書込み/キャッシュ転送中 */
#define CACHE_DATA_STS_WRITE_E	4				/* ディスク書込み/キャッシュ転送完了 */
#define CACHE_DATA_STS_SEND		5				/* INDEX情報転送中 */
#define CACHE_DATA_STS_USE		10				/* 正常終了 */
#define CACHE_DATA_STS_CACHE	20				/* キャッシュ中 */

/* システム状態 */
#define SYSTEM_STATUS_NONE		0				/* 初期状態 */
#define SYSTEM_STATUS_INIT		1				/* 起動中 */
#define SYSTEM_STATUS_SYNC		2				/* データ同期中 */
#define SYSTEM_STATUS_FIN		3				/* 停止中 */
#define SYSTEM_STATUS_NORES		4				/* 無応答 */
#define SYSTEM_STATUS_RUNNING	10				/* 実行中 */
#define SYSTEM_STATUS_UNKNOWN	99				/* 不明 */

/* プロセス状態 */
#define PROCESS_STATUS_NONE		0				/* 停止中 */
#define PROCESS_STATUS_INIT		1				/* 初期化中 */
#define PROCESS_STATUS_SYNC		2				/* データ同期中 */
#define PROCESS_STATUS_FIN 		3				/* 終了処理中 */
#define PROCESS_STATUS_IDLE		10				/* 処理中(IDLE) */
#define PROCESS_STATUS_BUSY		11				/* 処理中(BUSY) */
#define PROCESS_STATUS_WAIT		12				/* 処理中(WAIT) */

/* プロセス種別 */
#define PROCESS_TYPE_SMAN		1				/* プロセス種別（システム監視） */
#define PROCESS_TYPE_NLSR		2				/* プロセス種別（ノード間通信） */
#define PROCESS_TYPE_CLSR		3				/* プロセス種別（クライアントリスナー） */
#define PROCESS_TYPE_CACH		4				/* プロセス種別（キャッシュ管理） */
#define PROCESS_TYPE_CRCV		5				/* プロセス種別（キャッシュ受信） */
#define PROCESS_TYPE_CSND		6				/* プロセス種別（キャッシュ転送） */
#define PROCESS_TYPE_MSYN		7				/* プロセス種別（META同期） */
#define PROCESS_TYPE_DSYN		8				/* プロセス種別（DATA同期） */
#define PROCESS_TYPE_DISK		9				/* プロセス種別（DISK書込み） */
#define PROCESS_TYPE_DMNT		10				/* プロセス種別（DISKメンテ） */
#define PROCESS_TYPE_TASK		100				/* プロセス種別（処理タスク） */

/* ファイルタイプ */
#define FILE_TYPE_FILE			1				/* 通常ファイル */
#define FILE_TYPE_DIR			2				/* ディレクトリ */

/* JournalLog種類 */
#define JOURNAL_TYPE_META		1				/* META書込み */
#define JOURNAL_TYPE_INDEX_CRE	2				/* INDEX作成 */
#define JOURNAL_TYPE_INDEX_ADD	3				/* INDEX追加 */
#define JOURNAL_TYPE_SCN_UP		4				/* SCN番号更新 */

/* セマフォ配列番号 */
#define SEM_LOCK_IDXO001		1				/* セマフォ(METAキャッシュ) */
#define SEM_LOCK_IDXO002		2				/* セマフォ(INDEXキャッシュ) */
#define SEM_LOCK_IDXO003		3				/* セマフォ(新規FILE-ID取得) */
#define SEM_LOCK_IDXO004		4				/* セマフォ(ノード間通信接続先決定) */
#define SEM_LOCK_IDXO005		5				/* セマフォ */
#define SEM_LOCK_IDXO006		6				/* セマフォ */
#define SEM_LOCK_IDXO007		7				/* セマフォ */
#define SEM_LOCK_IDXO008		8				/* セマフォ */


/***************************************************************
 構造体定義
****************************************************************/
/* NODEファイルヘッダ */
typedef struct{
	char	file_type[4];				/* ファイルタイプ */
} T_NODE_HEADER;

/* Volumeヘッダ */
typedef struct{
	char	file_type[4];				/* ファイルタイプ */
	u_char	volume_id[VOLUME_ID_LEN];	/* Volume-ID */
	T_SCN	vol_scn;					/* SCN番号 */
	struct timeval	w_tm;				/* 最終書込み時間 */
} T_VOLUME_HEADER;

/* META情報ヘッダ */
typedef struct{
	char		file_type[4];			/* ファイルタイプ */
	T_DIR_ID	row;					/* ディレクトリID情報 */
} T_META_HEADER;

/* INDEXファイルヘッダ */
typedef struct{
	char		file_type[4];			/* ファイルタイプ */
	T_FILE_ID	dir_id;					/* ディレクトリID情報 */
	u_long		block_num;				/* ブロック数 */
} T_IDX_HEADER;

/* JournalLogファイルヘッダ */
typedef struct{
	char	file_type[4];				/* ファイルタイプ */
} T_JLOG_HEADER;

/* INDEX情報 */
typedef struct{
	u_char	node_id[NODE_ID_LEN];		/* ノード識別子 */
	u_long	block_no;					/* ブロック番号 */
	u_long	size;						/* 利用サイズ */
	struct timeval	w_tm;				/* 最終書込み時間 */
}T_IDX_INFO;

/* JournalLogレコード */
typedef struct{
	int			flg;					/* Journalタイプ */
	T_SCN		scn;					/* SCN番号 */
	T_FILE_ID	dir_id;					/* ディレクトリID */
	u_long		rec_no;					/* レコード番号 */
}T_JLOG_INFO;

/*----------------------------------------------------------------------------------*/

/* [SGA]共有メモリヘッダ */
typedef struct{
	uint		size_header;		/* ヘッダ領域 */
	uint		size_conf;			/* 定義情報サイズ */
	uint		size_node;			/* ノード管理情報サイズ */
	uint		size_proc;			/* プロセス管理情報サイズ */
	uint		size_disk;			/* ディスク管理領域サイズ */
	uint		size_meta;			/* METAキャッシュ管理領域サイズ */
	uint		size_index;			/* INDEXキャッシュサイズ */
	uint		size_data;			/* DATAキャッシュサイズ */
	uint		size_total;

	uint		offset_header;		/* ヘッダ領域開始位置 */
	uint		offset_conf;		/* 定義情報開始位置 */
	uint		offset_node;		/* ノード管理情報開始位置 */
	uint		offset_proc;		/* プロセス管理情報開始位置 */
	uint		offset_disk;		/* ディスク管理領域開始位置 */
	uint		offset_meta;		/* METAキャッシュ管理領域開始位置 */
	uint		offset_index;		/* INDEXキャッシュ開始位置 */
	uint		offset_data;		/* DATAキャッシュ開始位置 */

}T_SHM_HEADER;

/* [SGA]プロセス管理テーブル */
typedef struct{
	pid_t	 pid;					/* プロセスID */
	int		 sts;					/* プロセス状態 */
	int		 fork_try;				/* 起動回数 */
	int		 req;					/* 子プロセスへの要求 */
	int 	 ptype;					/* プロセス種別 */
	time_t	 stime;					/* プロセス開始時刻 */
	time_t	 etime;					/* プロセス終了時刻 */
	time_t	 alv;					/* プロセス稼働フラク゛ */
	char	 name[32];				/* プロセス名 */
	char	 para[1024];			/* 起動パラメータ */
	unsigned int    sem_lock;       /* セマフォ待ち回数 */
	int		cache_idx;				/* 参照ブロック */
}T_PROCESS_TABLE;

/* [SGA]METAキャッシュ管理テーブル */
typedef struct{
	int		req;					/* リクエスト種別 */
	int		sts;					/* FILEキャッシュ状態 */
	pid_t	pid;					/* 書込みプロセスID */
	int 	disk_flg;				/* ディスク書込み状態 */
	int 	node_flg;				/* ノード転送状態 */
	int		read_flg;				/* 参照中フラグ */
	u_long	disk_wcnt;				/* ブロック出力完了数 */
	u_long	node_wcnt;				/* ブロック出力完了数 */
	struct timeval	w_tm;			/* 最終書込み時間 */
	T_META_INFO inf;				/* ファイルMETA情報 */
	T_DIR_ID row;					/* ディレクトリID情報 */
}T_META_TABLE;

/* [SGA]INDEXキャッシュ管理テーブル */
typedef struct{
	u_long	file_idx;				/* ファイルテーブルの配列番号 */
	u_long	block_no;				/* ブロック番号 */
	u_long	size;					/* 利用サイズ */
	int		sts;					/* キャッシュ状態 */
	int		disk_flg;				/* ディスク書込みフラグ */
	int		node_flg;				/* ノード転送フラグ */
	int		read_flg;				/* 参照中フラグ */
	pid_t	pid;					/* 書込みプロセスID */
	struct timeval	w_tm;			/* 最終書込み時間 */
}T_INDEX_TABLE;

/* [SGA]DATAキャッシュ管理テーブル */
typedef struct{
	char	data[BLOCK_SIZE];
	char	dummy[64];
}T_DATA_TABLE;

/*----------------------------------------------------------------------------------*/
/* [通信]META同期要求 */
typedef struct{
	T_JLOG_INFO				jlog;		/* JournalLog */
	union {
		struct {
			T_DIR_ID 		row;		/* ディレクトリID情報 */
			T_META_INFO 	inf;		/* META情報 */
		}meta;
		struct {
			T_FILE_ID		dir_id;		/* INDEXレコード */
			u_long			block_num;	/* ブロック数 */
		}row;
		struct {
			T_IDX_INFO		inf;		/* INDEXレコード */
		}idx;
	};
}T_META_SYNC_RESULT;

/* [通信]ノード要求 */
typedef struct{
    int req;
    T_NODE_TABLE inf;
}T_NODE_DATA_REQUEST;

/***************************************************************
 グローバル変数宣言
***************************************************************/
extern char G_BaseDir[MAX_FILE_PATH+1];
extern char G_TempDir[MAX_FILE_PATH+1];
extern char G_LogDir[MAX_FILE_PATH+1];
extern char G_ConfDir[MAX_FILE_PATH+1];
extern char G_HLogDir[MAX_FILE_PATH+1];

extern int  G_ShmKey;
extern int  G_SemKey;
extern int  G_MquKey;
extern int  G_Data_Disks;
extern int  G_ProcNo;

/* 共有メモリ上アドレス */
extern char 			*G_shmaddr;
extern T_SHM_HEADER 	*G_ShmHeader;		/* SGA(ヘッダ領域) */
extern T_CONFIG_TABLE   *G_ConfTbl;	 		/* SGA(パラメータ領域) */
extern T_NODE_TABLE		*G_NodeTbl;	 		/* SGA(ノード管理領域) */
extern T_PROCESS_TABLE  *G_ProcTbl;	 		/* SGA(プロセス管理領域) */
extern T_DISK_TABLE		*G_DiskTbl;			/* SGA(ディスク管理領域) */
extern T_META_TABLE		*G_MetaTbl;			/* SGA(METAキャッシュ管理領域) */
extern T_INDEX_TABLE  	*G_IndexTbl;		/* SGA(INDEXキャッシュ管理領域) */
extern T_DATA_TABLE     *G_DataTbl;			/* SGA(DATAキャッシュ領域) */


/***************************************************************
 プロトタイプ宣言
***************************************************************/

/* ログ関連 */
int logPrintInit(int debug_flg, int file_flg, int stdout_flg);
int logPrint(int lvl, char *msgfmt, ...);
int logDebug(int lvl, char *msgfmt, ...);
int logSysLog(char *msgfmt, ...);

int hlogJobHistoryLogStr(T_CLIENT_REQUEST req);
int hlogJobHistoryLogEnd(T_CLIENT_REQUEST req, int rescode);

/* 通信共通関数 */
int transConnect(long addr, int port);
int transSendBuff(int fd, char *buff, int siz, T_CLIENT_RESULT_S *res);
int transSendClient(int fd, char *msgfmt, ...);
int transSendResultS(int fd, int res_flg);
int transSendResultS(int fd, int res_flg, u_long recv_cnt, u_long recv_size);
int transSendResultM(int fd, int res_flg);
int transSendResultM(int fd, int res_flg, u_long recv_cnt, u_long recv_size);
int transSendResultM(int fd, int res_flg, char *msgfmt, ...);
int transSendResultM(int fd, int res_flg, u_long recv_cnt, u_long recv_size, char *msgfmt, ...);
int transCacheSyncMeta(T_SCN scn, u_long *out_cnt, T_META_SYNC_RESULT *out_buff);
int transCacheSendMeta(int pno, int idx);
int transCacheSendIndexCre(int pno, T_FILE_ID dir_id, T_FILE_ID file_id);
int transCacheSendIndexAdd(int pno, T_FILE_ID dir_id, T_FILE_ID file_id, u_long block_no, u_long size);
int transCacheSendData(int pno, int idx);

/* 各種情報出力 */
char *dispSCN(T_SCN *scn);
char *dispSCN(T_SCN *scn, char *wk);
char *dispPerm2Mode(char c, char *out);
char *dispPerm2Name(u_long perm);
char *dispType2Name(u_long file_type);
char *dispNodeID2Name(u_char *node_id);
char *dispFileID(T_FILE_ID *p);
char *dispFileID(T_FILE_ID *p, char *buff);
int dispNodeInfo(int fd, T_NODE_TABLE *tbl);
int dispNodeInfoDetail(int fd, T_NODE_TABLE *tbl);
int dispProcessInfo(int fd, T_PROCESS_TABLE *tbl);
int dispMemInfo(int fd, T_INDEX_TABLE *tbl);
int dispCacheInfo(int fd, T_META_TABLE *tbl, T_INDEX_TABLE *idx);
int dispVolumeInfo(int fd, T_NODE_TABLE *ntbl, T_DISK_TABLE *dtbl);
int dispPerformance(int fd, T_NODE_TABLE *G_NodeTbl, T_DISK_TABLE *G_DiskTbl);
int dispFileList(int fd,  T_CLIENT_REQUEST req);
int dispFileStat(int fd,  T_CLIENT_REQUEST req);
int dispHeaderInfo(T_META_HEADER hd);
int dispMetaInfo(T_META_INFO in);
int dispFileDump(char *filename);
int dispMemDataDump(int idx);

/* キャッシュ */
int cacheSetAddress(T_SHM_HEADER *hwk);
int cacheGetFreeFileCache();
int cacheGetFreeDiskCache();
int cacheGetFreeTaskNo();
int cacheSetMasterFlag(int idx, int sts);
int cacheSetNodeStatus(int idx, int sts);
int cacheSetProcStatus(int pno, int sts);
int cacheSetFlagIndexStatus(int idx, int sts);
int cacheSetFlagIndexDisk(int idx, int sts);
int cacheSetFlagIndexNode(int idx, int sts);
int cacheSetFlagMetaStatus(int idx, int sts);
int cacheSetFlagMetaDisk(int idx, int sts);
int cacheSetFlagMetaNode(int idx, int sts);
int cacheSetVolumeStatus(int idx, int sts);
int cacheMetaCached(int idx);
int cacheMetaRelease(int idx);
int cacheDataCached(int idx);
int cacheDataRelease(int idx);
int cacheMetaWrite(T_META_HEADER hd, T_META_INFO fi, int req_no);
int cacheCheckBlockWrite(T_FILE_ID *file_id, u_long block_no);
int cacheCheckCachedBlock(T_FILE_ID *file_id, u_long block_no);
int cacheCheckCachedRelease(int idx);

/* デーモン処理 */
int daemon_CompressType(int out_size);
int daemon_DataFileType(char *filename);
int daemon_ActiveNodeNum();
int daemon_DuplicateNum();
int daemon_GetMasterIdx();
u_long daemon_GetMasterSvrIP();
u_long daemon_GetMasterCltIP();
int daemon_CheckMaster();
int daemon_GetSCN(T_SCN *scn, int flg);
int daemon_GetNewFileID(T_FILE_ID *id, int flg);
int daemon_NodeTableWrite();
int daemon_NodeTableRead();
int daemon_NodeTableInit();
int daemon_exec(int no, int typ, char *name,char *para);
int daemon_CheckPerm(T_CLIENT_REQUEST req, T_META_INFO fi);

/* server */
int server_start();
int server_stop();
int server_parameter();
int server_status();
int server_status2();
int server_perf();
int server_process();
int server_mem();
int server_cache();
int server_volume();
int server_memhex();

/* 各子プロセスの処理 */
int process_sman_init();
int process_nlsr_tcp_init();
int process_nlsr_udp_init();
int process_clsr_init();
int process_cach_init();
int process_crcv_init();
int process_csnd_init();
int process_msyn_init();
int process_dsyn_init();
int process_disk_init();
int process_dmnt_init();
int process_task_init();
int process_sman();
int process_nlsr_tcp();
int process_nlsr_udp();
int process_clsr();
int process_cach();
int process_crcv();
int process_csnd();
int process_msyn();
int process_dsyn();
int process_disk();
int process_dmnt();
int process_task();
int process_sman_fin();
int process_nlsr_tcp_fin();
int process_nlsr_udp_fin();
int process_clsr_fin();
int process_cach_fin();
int process_crcv_fin();
int process_csnd_fin();
int process_msyn_fin();
int process_dsyn_fin();
int process_disk_fin();
int process_dmnt_fin();
int process_task_fin();

/* ファイル名 */
int disknmChoiceServer(int flg);
int disknmGetMetaDirName(char *data_base_dir, T_FILE_ID id, char *filename);
int disknmGetMetaFileName(char *data_base_dir, T_FILE_ID id, char *filename);
int disknmGetIdxDirName(char *data_base_dir, T_FILE_ID id, char *filename);
int disknmGetIdxFileName(char *data_base_dir, T_FILE_ID id, char *filename);
int disknmGetDataDirName(char *data_base_dir, T_FILE_ID id, char *filename);
int disknmGetDataFileName(char *data_base_dir, T_FILE_ID id, int block_no, char *filename);
int disknmSearchDataFileName(char *data_base_dir, T_FILE_ID id, int block_no, char *filename);
int disknmGetLogDirName(char *data_base_dir, char *filename);
char *disknmFileName2FileID(char *filenm);
void disknmFileName2FileID(char *filenm, T_FILE_ID *file_id);
void disknmFileID2FileID(char *w, T_FILE_ID *file_id);

/* ディスク(read) */
int diskrdGetAllMetas(T_FILE_ID file_id, T_META_HEADER *hd, T_META_INFO *buff_arr);
int diskrdCheckDataFile(T_FILE_ID file_id, u_long block_no);
int diskrdGetVolumeSCN(int idx, char *base_dir, T_SCN *scn);
int diskrdGetDirInFiles(T_FILE_ID dir_id, int *file_cnt);
int diskrdGetDirInfo(T_FILE_ID dir_id, T_META_HEADER *hd);
int diskrdGetFileInfo(T_FILE_ID dir_id, T_FILE_ID fil_id, T_META_HEADER *hd, T_META_INFO *fi);
int diskrdGetDataBlockDup(T_FILE_ID file_id, u_long block_no, T_IDX_HEADER *ihd);
int diskrdGetDataBlockSize(T_FILE_ID file_id, u_long block_no);
int diskrdGetMetaInfo(T_FILE_ID dir_id, char *srch, T_META_HEADER *hd, T_META_INFO *fi);
int diskrdSearchMeta(char *file_path, T_META_HEADER *hd, T_META_INFO *fid);
int diskrdSearchDir(char *file_path, T_META_HEADER *hd, T_META_INFO *fi, char *out_path);
int diskrdSerchMetaFilePtn(T_FILE_ID dir_id, char *srch);

/* ディスク(write) */
int diskwtWriteMetaList(char *dir, char *filename);
int diskwtWriteMetaRecord(T_FILE_ID dir_id, T_META_INFO fi, int flg);
int diskwtWriteMetaCreateTop(T_FILE_ID up_dir_id, T_FILE_ID dir_id);
int diskwtWriteMetaCreate(T_FILE_ID dir_id, T_META_INFO in_fi);
int diskwtWriteDataCache(char *in_addr, T_INDEX_TABLE in_idx, char *out_base_dir, T_FILE_ID out_file_id);
int diskwtWriteIndexHeader(char *filename, T_FILE_ID row, u_long block_num);
int diskwtWriteIndexAppend(char *filename, T_FILE_ID file_id, T_IDX_INFO idx);
int diskwtWriteIndexList(char *dir, char *filename);
int diskwtWriteIndexInit(T_FILE_ID dir_id, T_FILE_ID file_id, u_long block_num);
int diskwtWriteIndexCache(T_FILE_ID dir_id, T_FILE_ID file_id, u_long block_no, u_long size, u_char *node_id);
int diskwtWriteScnCount();
int diskwtWriteVolumeSCN(int idx, char *base_dir, T_SCN *scn);
int diskwtWriteJournalLog(int flg, T_SCN *scn, char *out_dir, T_FILE_ID *dir_id, u_long rec_no);

/* ディスク同期 */
int disksyncReadMeta(T_CLIENT_REQUEST req, T_META_SYNC_RESULT *outbuff, u_long *r_cnt, u_long *r_size);
int disksyncWriteIndexHeader(T_META_SYNC_RESULT *out_buff);
int disksyncWriteIndexRecord(T_META_SYNC_RESULT *out_buff);
int disksyncWriteMeta(T_META_SYNC_RESULT *out_buff);

int diskrepReplecateBlock(int pno, T_FILE_ID file_id, u_long block_no, u_long size, int node_idx, int wflg);
int diskrepRepNumReCal(T_FILE_ID file_id, u_long block_no);
int disksyncWriteScnCount(T_META_SYNC_RESULT *out_buff);

/* ディスクアクセス要求 */
int diskreqMetaInit(T_FILE_ID up_dir_id, T_FILE_ID dir_id, char *out_path);
int diskreqMetaMkDir(T_CLIENT_REQUEST req, char *msg);
int diskreqMetaRmDir(T_CLIENT_REQUEST req, char *msg);
int diskreqMetaRmFile(T_CLIENT_REQUEST req, char *msg);
int diskreqMetaStat(T_CLIENT_REQUEST req, char *msg, struct xi_stat *stat);
int diskreqMetaChmod(T_CLIENT_REQUEST req, char *msg);
int diskreqMetaChown(T_CLIENT_REQUEST req, char *msg);
int diskreqMetaChgrp(T_CLIENT_REQUEST req, char *msg);
int diskreqMetaWriteCache(T_FILE_ID dir_id, T_META_INFO meta_inf);
int diskreqDataWriteCache(char *in_addr, T_INDEX_TABLE in_idx, char *out_base_dir, T_FILE_ID out_file_id);
int diskreqIdxWriteCache(T_INDEX_TABLE in_idx, T_FILE_ID out_file_id);

/* クライアント要求 */
int creqGetTabFilePtn(T_CLIENT_REQUEST req, char *buff);
int creqMkDir(int fd, T_CLIENT_REQUEST req);
int creqRmDir(int fd, T_CLIENT_REQUEST req);
int creqRmFile(int fd, T_CLIENT_REQUEST req);
int creqStat(int fd, T_CLIENT_REQUEST req);
int creqChmod(int fd, T_CLIENT_REQUEST req);
int creqChown(int fd, T_CLIENT_REQUEST req);
int creqChgrp(int fd, T_CLIENT_REQUEST req);
int creqFileGet(int fd, T_CLIENT_REQUEST req);
int creqFilePut(int fd, T_CLIENT_REQUEST req);
int creqRecvCacheMetaCre(int fd, T_CLIENT_REQUEST req);
int creqRecvCacheMetaAdd(int fd, T_CLIENT_REQUEST req);
int creqRecvCacheIndexCre(int fd, T_CLIENT_REQUEST req);
int creqRecvCacheIndexAdd(int fd, T_CLIENT_REQUEST req);
int creqRecvCacheData(int fd, T_CLIENT_REQUEST req);
int creqGetMeta(int fd, T_CLIENT_REQUEST req);
int creqGetData(int fd, T_CLIENT_REQUEST req);
int creqNodeInfo(int fd, T_CLIENT_REQUEST req);

