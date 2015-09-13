# XiCluster

## 概要
XiClusterとは、C言語で開発された分散キャッシュを実装した分散処理フレームワークです。
以下機能を提供しています。  

・ブロック単位の分散キャッシュ
・ブロック単位の分散ファイルシステム
・ブロック単位のミラーリングとレプリケーション  
・ブロック単位のストライピング  
・ブロック単位のリバランシング  
・ブロック単位のフェイルオーバー  
・ディスクIOと分散処理の負荷分散  
・メタキャッシュとデータブロックキャッシュによる高速化  
・ジャーナルログを使ったリカバリ  
・マルチマスタ構成  
・データブロックの自動圧縮  
・通信データの圧縮、暗号化  
・専用線やインターネット回線を介した遠隔地レプリケーション  

## 動作環境
XiClusterは64bit Linux環境で動作します。  
32bit環境は最大ファイル数や最大ファイルサイズに制限があります。  

## ご利用条件
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

## 構成
XiClusterサーバは共有メモリと複数のデーモンから構成されます。
デーモン共通情報やキャッシュは共有モメモリに確保され、
各ノード間はTCP/IP通信によりメタ情報とデータブロックの同期を行います。
ユーザはxicluster_clientコマンドやXiClusterAPI(XiClusterクライアントライブラリ)
を利用して分散キャッシュ・ファイルシステムにアクセスします。

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
ファイルの一覧は[ファイル一覧](doc/FILES.md)を参照して下さい。  

## 通信ポート 
デフォルト通信ポートは以下となります。設定ファイルxicluster.confにて設定変更できます。
```  
9010 : ノード間のノード情報転送（キャッシュ転送と同一ポートの利用も可能）  
9020 : ノード間のメタ・データ転送   
9030 : クライアント間通信  
```    
詳しくは[パラメータ一覧](doc/PARAMETER.md)や[デーモン一覧](doc/PROCESS.md)を参照して下さい。 

## インストール方法
インストール方法は [インストール手順](doc/INSTALL.md)を参照して下さい。

## 起動／停止  
```  
$ su -
# service xicluster start   :XiCluster起動
# service xicluster stop    :XiCluster停止
```  

## XiClusterサーバ
xicluster_serverコマンドを利用してデーモン状態やキャッシュ情報の確認が行えます。
xicluster_serverコマンドは直接共有メモリの情報をダンプ表示するので、デーモン障害時でも状態が確認できます。
コマンド実行例は [コマンド実行例](doc/EXEMPLE.md)を参照して下さい。  

```  
$ xicluster_server status     ：各ノード情報表示  
$ xicluster_server parameter  ：自ノードパラメータ表示  
$ xicluster_server process    ：自ノード実行中プロセス表示  
$ xicluster_server cache      ：自ノードキャッシュ情報表示  
$ xicluster_server mem        ：自ノード共有メモリ情報表示  
$ xicluster_server dump <file>：データファイルのダンプ  
```    
ログファイルは/usr/local/xicluster/log/YYYYMMDD.logに出力されます。  

## XiClusterクライアントコマンド
xicluster_clientコマンドを利用してサーバの情報表示や各ノードに分散されたデータのget/putが行えます。
xicluster_clientはサーバ側のデーモンと通信を行い情報表示を行っています。
引数無しで実行するとプロンプトモードとなり、引数有りで実行するとコマンド実行モードとなります。  
コマンド実行例は [コマンド実行例](doc/EXEMPLE.md)を参照して下さい。  
  
[プロンプトモード]  
```  
$ xicluster_client  
XICLUSTER> status  
XICLUSTER> parameter
XICLUSTER> process
XICLUSTER> cache
XICLUSTER> mem
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
$ xicluster_client status  
$ xicluster_client parameter
$ xicluster_client process
$ xicluster_client cache
$ xicluster_client mem
$ xicluster_client ls <ディレクトリ>  
$ xicluster_client put <OSファイル名> <XIファイル名>  
$ xicluster_client get <XIファイル名> <OSファイル名>  
```  

## XiClusterrAPIクライアントAPI
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

## APIリファレンス
APIリファレンスマニュアルは[APIリファレンス](doc/API.md)を参照して下さい。  
```  
int xi_open(char *path, int mod);
int xi_read(int fd, char *buff, int size);  
int xi_write(int fd, char *buff, int size);  
int xi_close(int fd);  
int xi_stat(char *path, struct xi_stat *buf);  
int xi_mkdir(char *path);  
int xi_rmdir(char *path);  
int xi_rename(char *oldpath, char *newpath);  
int xi_unlink(char *path);  
int xi_chmod(char *path, int mode);  
int xi_chown(char *path, int uid);  
int xi_chgrp(char *path, int gid);  
```    

## パフォーマンス
XiClusterの性能については[パフォーマンス比較](doc/PEFORMANCE.md)を参照して下さい。  

## NFSマウント  


