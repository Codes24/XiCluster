[XiCluster API reference Manual](API.md)  

## xi_write()
  
**名前**  
  xi_write - ファイル・ディスクリプターから読み込む  
  
**書式**  
  #include "xi_client.h"  
  
  int xi_write(int fd, char *buf, int count);  
   
**説明**  
  xi_write() は buf で示されるバッファから最大 count バイトまでをファイル・ディスクリプタ fd によって参照されるファイルへと書き込む。   
  
**返り値**  
  成功した場合は書き込んだバイト数が返される。エラーの場合は マイナス値 が返され、 xi_errno が適切に設定される。  
  
**注意**  
  
**エラー**  
  XI_ERRNO_PARA   パラメータエラー  
  
**制限**  
  

