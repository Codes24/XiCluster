[XiCluster API reference Manual](API.md)  

## xi_stat()
  
**名前**  
  xi_stat - ファイルの状態を取得する。  
  
**書式**  
  #include "xi_client.h"  
  
  int xi_stat(const char *path, xi_stat *buf);
   
**説明**  
  pathで指定されたファイルの状態を取得して buf へ格納する。  

xi_stat構造体は以下となります。  
```
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
```
  
**返り値**  
  成功した場合はゼロが返される。エラーの場合は マイナス値 が返され、 xi_errno が適切に設定される。  
  
**注意**  
  
**エラー**  
  XI_ERRNO_PARA   パラメータエラー  
  
**制限**  
  

