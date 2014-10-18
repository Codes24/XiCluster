# XiCluster API reference Manual 

## xi_open()
  
**名前**  
  xi_open - ファイルのオープン、作成を行う。  
  
**書式**  
  #include "xi_client.h"  
  
  in xi_open(const char *pathname, int flags);  
  in xi_open(const char *pathname, int flags, int mode);  
  
**説明**  
  ファイルの pathname を与えると、xi_open() はファイル・ディスクリプタを返す。
  ファイル・ディスクリプタは、この後に続く関数 (xi_read(), xi_write()など)
で使用される小さな非負の整数である。  
  この関数が成功した場合に返されるファイル・ディスクリプタは そのプロセスがその時点でオープンしていないファイル・ディスクリプタのうち最小の数字のものとなる。  
  パラメータ flags には XI_READ, XI_WRITE のどれかを指定しなければならない。
これらはそれぞれ読み込み専用、書き込み専用にファイルをオープンすることを要求するものである。
  さらに、 flags には圧縮フラグ (compress flag) を「ビット単位の OR (bitwise-or) 」で指定することができる。 
  
  XI_COMP_LEVEL_0 ：圧縮なし  
  XI_COMP_LEVEL_1 ：圧縮レベル１  
  XI_COMP_LEVEL_2 ：圧縮レベル２  
  XI_COMP_LEVEL_3 ：圧縮レベル３  
  XI_COMP_LEVEL_4 ：圧縮レベル４  
  XI_COMP_LEVEL_5 ：圧縮レベル５  
  XI_COMP_LEVEL_6 ：圧縮レベル６（デフォルト）  
  XI_COMP_LEVEL_7 ：圧縮レベル７  
  XI_COMP_LEVEL_8 ：圧縮レベル８  
  XI_COMP_LEVEL_9 ：圧縮レベル９  
  
**返り値**  
  open()は新しいファイル・ディスクプリタを返す。エラーが発生した場合はマイナス値を返す
  （その場合はxi_errnoが適切に設定される）。  
  
  
**注意**  
  
  
**エラー**  
  XI_ERRNO_PARA   パラメータエラー  
  XI_ERRNO_OPENED ファイルオープン中である。   
  XI_ERRNO_OPEN   ファイルのオープンに失敗した  
  XI_ERRNO_SEND_Z サーバへのデータ転送でエラーが発生した。  
  XI_ERRNO_RECV_Z サーバからの応答でエラーが発生した。  
  
  
**制限**  
  今のバージョンでは複数のファイルをオープンする事は許可されていない。  
  

