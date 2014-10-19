[XiCluster API reference Manual](API.md)  

## xi_unlink()
  
**名前**  
  xi_unlink - ファイルを削除する。  
  
**書式**  
  #include "xi_client.h"  
  
  in xi_unlink(const char *pathname);  
   
**説明**  
  unlink()は分散ファイルシステム上のファイルを削除する。  
  
**返り値**  
  成功した場合はゼロが返される。エラーの場合は マイナス値 が返され、 xi_errno が適切に設定される。  
  
**注意**  
  
**エラー**  
  XI_ERRNO_PARA   パラメータエラー  
  
**制限**  
  

