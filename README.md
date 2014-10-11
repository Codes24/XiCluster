XiCluster
======================
XiClusterとは、C言語で開発された分散ファイルシステムと分散処理フレームワークで以下機能を提供しています。  

・ブロック単位のミラーリングとレプリケーション  
・ブロック単位のストライピング  
・ブロック単位のリバランシング  
・ブロック単位のフェイルオーバー  
・ディスクIOと分散処理の負荷分散  
・メタ情報とデータのキャッシュ  
・ジャーナルログを使ったリカバリ  
・マルチマスタ  
・データファイルや通信データの自動圧縮


動作環境
------
XiClusterは64bit Linux環境で動作します。  
32bit環境でも動作しますが、最大ファイル数や最大ファイルサイズに制限があります。

ご利用条件
------
XiClusterの使用もしくは再配布について、無料でご利用いただけます。  

XiCluster配布アーカイブに含まれる著作物に対する権利は、開発者が保持してお
り、GNU一般公衆利用許諾契約に基づいて配布しております。再配布・改変等は契
約の範囲内で自由に行うことが出来ます。詳しくは、添付のGNU一般公衆利用許諾
契約書をお読みください。

なお、XiClusterは一般的な利用において動作を確認しておりますが、ご利用の環
境や状況、設定もしくはプログラム上の不具合等により期待と異なる動作をする場
合が考えられます。XiClusterの利用に対する効果は無保証であり、あらゆる不利
益や損害等について、当方は一切の責任をいたしかねますので、ご了承いただきま
すようお願い申し上げます。

構成
------
XiClusterを起動すると複数のデーモンが起動し、ノード情報をブロードキャスト
する事で各ノードを自動認識します。共有メモリをキャッシュとしてTCP/IP通信
によりメタ情報とデータの同期を行います。ユーザはxicluster_clientコマンド
やXiClusterAPI(XiClusterクライアントライブラリ)を利用して分散ファイルに
アクセスします。

[プログラム]  
```  
xicluster_server：XiCluster制御プログラム（デーモンの起動、停止、状態確認）  
xicluster_daemon：XiClusterデーモン  
xicluster_client：XiClusterクライアント  
```  
[ライブラリ]  
```  
libxicluster_common.so：共通ライブラリ  
libxicluster_server.so：サーバ共通ライブラリ  
libxicluster_client.so：クライアントAPI  
```  
[通信ポート]  
```  
UDP/9010 : ノード間のサーバ情報転送  
TCP/9020 : ノード間のメタ・データ転送  
TCP/9030 : クライアント間通信  
```    

インストール方法
------
インストール方法は [インストール手順](https://github.com/takakusaki/XiCluster/blob/master/doc/INSTALL.md)を参照して下さい。

XiClusterサーバ
------
xicluster_serverコマンドを利用してデーモン起動、停止、キャッシュ情報情報
表示等が行えます。デーモンの起動・停止はxiclusterユーザで実行して下さい。

```  
$ xicluster_server start      ：XiClusterデーモン起動  
$ xicluster_server stop       ：XiClusterデーモン停止  
$ xicluster_server status     ：各ノード情報表示  
$ xicluster_server parameter  ：自ノードパラメータ表示  
$ xicluster_server process    ：自ノード実行中プロセス表示  
$ xicluster_server cache      ：自ノードキャッシュ情報表示  
$ xicluster_server mem        ：自ノード共有メモリ情報表示  
$ xicluster_server dump <file>：データファイルのダンプ  
```    
ログファイルは/usr/local/xicluster/log/YYYYMMDD.logに出力されます。  

XiClusterクライアントコマンド
------
xicluster_clientコマンドを利用してサーバの情報表示や各ノードに分散された
データのget/putが行えます。引数無しで実行するとプロンプトモードとなり、
引数有りで実行するとコマンド実行となります。
  
[プロンプトモード]  
```  
$ xicluster_client  
XICLUSTER> status  
XICLUSTER> ls  
XICLUSTER> pwd  
XICLUSTER> cd <ディレクトリ>  
XICLUSTER> mkdir <ディレクトリ>  
XICLUSTER> rmdir <ディレクトリ>  
XICLUSTER> chmod <パーミッション> <パス>  
XICLUSTER> chown <OSユーザ名> <パス>  
XICLUSTER> chgrp <OSグループ名> <パス>  
XICLUSTER> rm <パス>  
XICLUSTER> exit  
```    
[コマンド実行] 
```  
$ xicluster_client status  
$ xicluster_client ls <ディレクトリ>  
$ xicluster_client put <OSファイル名> <XIファイル名>  
$ xicluster_client get <XIファイル名> <OSファイル名>  
```  

XiClusterrAPIクライアントAPI
------
XiClusterクライアントAPIを利用して分散ファイルにアクセスする事ができます。  
C言語標準のファイルIOシステムコールに似た感覚で簡単にプログラミングが行えます。  
  
[使い方]  
"xi_client.h"をインクルードし libxicluster_common.so とlibxicluster_client.soライブラリをリンクする。  
  
[sample.c]  
```
#include "xi_client.h"  
main(int argc, char **argv){  
      int fd;  
      char buff[1024];  
      if ( (fd=xi_open("hogehoge.dat",XI_WRITE)) < 0 ){  
          exit(1);  
      }  
      memset(buff,1,sizeof(buff));  
      xi_write(fd,buff,sizeof(buff));  
      xi_close(fd);  
      exit(0);  
}  
```  
[コンパイル]  
```  
$ g++ -I/usr/local/xicluster/src -lssl -lz -lxicluster_common -lxicluster_client sample.c  
```  

APIリファレンス
------
```  
int xi_open(char *path, int mod);  
int xi_read(int fd, char *buff, int size);  
int xi_write(int fd, char *buff, int size);  
int xi_close(int fd);  
int xi_stat(char *path, struct xi_stat *buf);  
int xi_mkdir(char *path);  
int xi_rmdir(char *path);  
int xi_rename(char *path1, char *path2);  
int xi_unlink(char *path);  
int xi_chmod(int mode, char *path);  
int xi_chown(int uid, char *path);  
int xi_chgrp(int gid, char *path);  
```    

