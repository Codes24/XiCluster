## ファイル一覧

|ファイル名                                       |説明|
|-------------------------------------------------|---------------|
|/usr/local/xicluster/bin/xicluster_server        |XiClusterサーバ|
|/usr/local/xicluster/bin/xicluster_daemon        |XiClusterデーモン|
|/usr/local/xicluster/bin/xicluster_client        |XiClusterクライアント|
|/usr/local/xicluster/lib/libxicluster_common.so  |ライブラリ|
|/usr/local/xicluster/lib/libxicluster_server.so  |ライブラリ|
|/usr/local/xicluster/lib/libxicluster_client.so  |ライブラリ|
|/usr/local/xicluster/conf/xicluster.conf         |設定ファイル|
|/usr/local/xicluster/conf/server.lst             |サーバリスト|
|/usr/local/xicluster/src/*                       |ソース|
|/usr/local/xicluster/temp/*                      |一時ファイル|
|/usr/local/xicluster/log/YYYYMMDD.log            |ログファイル|
|/usr/local/xicluster/hlog/YYYYMMDD.log           |操作ログファイル|
|/usr/local/xicluster/sample/Makefile             |サンプル|
|/usr/local/xicluster/sample/sample.c             |サンプル|
|/etc/init.d/xicluster                            |起動・停止スクリプト|
|/usr/bin/xicluster_server                        |シンボリックリンク|
|/usr/bin/xicluster_daemon                        |シンボリックリンク|
|/usr/bin/xicluster_client                        |シンボリックリンク|
|/usr/lib/libxicluster_common.so                  |シンボリックリンク|
|/usr/lib/libxicluster_server.so                  |シンボリックリンク|
|/usr/lib/libxicluster_client.so                  |シンボリックリンク|

## データファイル  
データファイルは設定ファイルで複数ディレクトリを指定できます。  
デフォルトは/usr/local/xicluster/dataです。  
１つのファイルは*64Mbyte*単位に分割されて、複数のノードに分散配置されます。  
  
|ファイル名                                          |説明|
|----------------------------------------------------|---------------|
|${BASE}/DATA/xx/xx/xx/xxxxxxxx.xxxxxxxxxxxxxx/nn    |データブロック(nn=ブロック通番)|
|${BASE}/DATA/xx/xx/xx/xxxxxxxx.xxxxxxxxxxxxxx/nn.gz |圧縮データブロック(nn=ブロック通番)|
|${BASE}/IDX/.index                                  |インデックス・リスト|
|${BASE}/IDX/xx/xx/xx/xxxxxxxx.xxxxxxxxxxxxxx        |ブロック格納先情報|
|${BASE}/META/.index                                 |メタ・リスト|
|${BASE}/META/xx/xx/xx/xxxxxxxx.xxxxxxxxxxxxxx       |メタ・データ|
|${BASE}/LOG/YYYYMMDDMM/n-n                          |ジャーナル・ログ(n-nは起動通番と処理通番)|


