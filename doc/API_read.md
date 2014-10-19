[XiCluster API reference Manual](API.md)  

## xi_read()
  
**名前**  
  xi_read - ファイル・ディスクリプターから読み込む  
  
**書式**  
  #include "xi_client.h"  
  
  int xi_read(int fd, char *buf, int count);  
   
**説明**  
  xi_read() はファイル・ディスクリプター (file descriptor) fd から最大 count バイトを buf で始まるバッファーへ読み込む。  
  
**返り値**  
  成功した場合は読み込んだバイト数が返される。エラーの場合は マイナス値 が返され、 xi_errno が適切に設定される。  
  
**注意**  
  
**エラー**  
  XI_ERRNO_PARA   パラメータエラー  
  
**制限**  
  

