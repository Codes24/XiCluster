[XiCluster API reference Manual](API.md)  

## xi_stat()
  
**名前**  
  xi_stat - ファイルの状態を取得する。  
  
**書式**  
  #include "xi_client.h"  
  
  int xi_stat(const char *path, xi_stat *buf);
   
**説明**  
  pathで指定されたファイルの状態を取得して buf へ格納する。  
  
**返り値**  
  成功した場合はゼロが返される。エラーの場合は マイナス値 が返され、 xi_errno が適切に設定される。  
  
**注意**  
  
**エラー**  
  XI_ERRNO_PARA   パラメータエラー  
  
**制限**  
  

