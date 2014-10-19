[XiCluster API reference Manual](https://github.com/takakusaki/XiCluster/blob/master/doc/API.md)  

## xi_close()
  
**名前**  
  xi_close - ファイルのクローズを行う。
  
**書式**  
  #include "xi_client.h"  
  
  in xi_close(int fd);  
  
**説明**  
  
**返り値**  
  xi_close() は成功した場合は 0 を返す。エラーが発生した場合は マイナス値 を返して、 xi_errno を適切に設定する。  
  
**注意**  
  
**エラー**  
  XI_ERRNO_PARA   パラメータエラー  
  
**制限**  
  

