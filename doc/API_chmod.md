[XiCluster API reference Manual](API.md)  

## xi_chmod()
  
**名前**  
  xi_chmod - ファイルのモードを変更する。  
  
**書式**  
  #include "xi_client.h"  
  
  in xi_chmod(const char *path, int mode);  
   
**説明**  
  path で与えられたファイルで参照されるファイルのモードを変更する。  
  
**返り値**  
  成功した場合はゼロが返される。エラーの場合は マイナス値 が返され、 xi_errno が適切に設定される。  
  
**注意**  
  
**エラー**  
  XI_ERRNO_PARA   パラメータエラー  
  
**制限**  
  

