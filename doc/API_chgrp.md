[XiCluster API reference Manual](API.md)  

## xi_chgrp()
  
**名前**  
  xi_chgrp - ファイルのグループを変更する。  
  
**書式**  
  #include "xi_client.h"  
  
  int xi_chgrp(const char *path, int gid);
   
**説明**  
  path で与えられたファイルで参照されるファイルのグループを変更する。  
  
**返り値**  
  成功した場合はゼロが返される。エラーの場合は マイナス値 が返され、 xi_errno が適切に設定される。  
  
**注意**  
  
**エラー**  
  XI_ERRNO_PARA   パラメータエラー  
  
**制限**  
  

