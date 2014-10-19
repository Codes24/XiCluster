[XiCluster API reference Manual](API.md)  

## xi_rename()
  
**名前**  
  xi_rename - ファイル名を変更する。  
  
**書式**  
  #include "xi_client.h"  
  
  in xi_rename(const char *oldpath, const char *newpath);  
   
**説明**  
  xi_rename() はファイルの名前を変え、必要ならばディレクトリ間の移動を行なう。  
  
**返り値**  
  成功した場合はゼロが返される。エラーの場合は マイナス値 が返され、 xi_errno が適切に設定される。  
  
**注意**  
  
**エラー**  
  XI_ERRNO_PARA   パラメータエラー  
  
**制限**  
  

