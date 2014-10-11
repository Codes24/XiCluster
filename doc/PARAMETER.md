##パラメータファイル
/usr/local/xicluster/conf/xicluster.conf  

##稼動サーバのパラメータ確認
xicluster_serverコマンドで現在のパラメータ値を確認できます。
```
xicluster_server parameter
```

##パラメータ一覧

|パラメータ             |デフォルト値|説明|
|-----------------------|------------|---|
|version                |固定        |バージョン|  
|max_node               |255         |最大ノード数|  
|version                |固定        |バージョン|  
|max_node               |255         |最大ノード数|  
|data_trans_interval    |5           |ノード情報転送間隔(sec)|  
|dsync_trans_interval   |5           |データ同期間隔(sec)|  
|disk_mente_interval    |10          |データメンテナンス間隔(sec)|  
|dsync_trans_wt_elap    |10          |１回のデータ同期最大処理時間(sec)|  
|nodetbl_write_interval |5           |ノード情報ディスク書込み間隔(sec)|  
|node_hang_timeout      |60          |ノード無応答検知時間(sec)|  
|node_removal_time      |12096000    |ノード排除時間(sec)|  
|meta_sync_max_rec      |100         |メタ同期時の最大レコード数|  
|meta_check_interval    |5           |メタ同期チェック間隔|  
|block_dup_cnt          |3           |レプリケーション数|  
|cache_files            |1000        |メタキャッシュのレコード数|  
|cache_blocks           |5           |データキャッシュのレコード数|  
|max_data_dir           |10          |データ出力先の最大数|  
|def_perm_fil           |0x644       |ファイル新規作成時のパーミッション|  
|def_perm_dir           |0x755       |ディレクトリ新規作成時のパーミッション|  
|max_file_per_dir       |5000        |ディレクトリ格納できる最大ファイル数|  
|max_blocks             |100000      |１ファイルの最大ブロック数| 
|jlog_max_rec           |10000       |ジャーナルログの最大レコード数|  
|compress_threshold     |1024        |ファイルがこのサイズ以上の時に自動圧縮(byte)|  
|compress_type          |1           |0=非圧縮 1=圧縮|  
|debug_level_file       |2           |ログファイルのデバックレベル|  
|debug_level_stdout     |7           |標準出力のメッセージ出力レベル|  
|max_process            |100         |最大プロセス数(デーモンプロセス数＋task_process以上にする)|  
|task_process           |10          |処理プロセス数|  
|cache_process          |2           |キャッシュ転送プロセス数|  
|msyn_process           |1           |メタ同期プロセス数|  
|dsyn_process           |1           |データ同期プロセス数|  
|sleep_loop_sman        |5000        |スリープ間隔(ms)|  
|sleep_loop_nlsr        |0           |スリープ間隔(ms)|  
|sleep_loop_clsr        |0           |スリープ間隔(ms)|  
|sleep_loop_cach        |5000        |スリープ間隔(ms)|  
|sleep_loop_crcv        |5000        |スリープ間隔(ms)|  
|sleep_loop_csnd        |5000        |スリープ間隔(ms)|  
|sleep_loop_msyn        |5000        |スリープ間隔(ms)|  
|sleep_loop_dsyn        |5000        |スリープ間隔(ms)|  
|sleep_loop_disk        |5000        |スリープ間隔(ms)|  
|sleep_loop_dmnt        |5000        |スリープ間隔(ms)|  
|sleep_loop_task        |5000        |スリープ間隔(ms)|  
|slow_start             |5000        |スリープ間隔(ms)|  
|proc_ka                |60          |プロセスハング検知時間(sec)|  
|fork_max               |10          |再起動最大回数|  
|fork_clear             |3600        |再起動数のクリアー時間(sec)|  
|start_wait             |10          |デーモン起動待ちタイムアウト(sec)|  
|stop_wait              |10          |デーモン停止時の強制終了までの待ち時間(sec)|  
|rerun_wait             |300         |再起動可能間隔(sec)|  
|network_if_svr         |eth0        |サーバ間通信で利用するインターフェース|  
|network_if_clt         |eth0        |クライアント間通信で利用するインターフェース|  
|network_port_svr       |9010        |サーバ間通信で利用するポート番号|  
|network_port_clt       |9020        |クライアント間通信で利用するポート番号|  
|network_port_cache     |9030        |キャッシュ転送で利用するポート番号|  
|network_buff_size      |15000       |転送最大サイズ(byte)|  
|con_retry              |3           |接続リトライ回数|  
|con_retrans            |5000000     |接続リトライ時の待ち時間(ms)|  
|con_timeout            |5000000     |接続待ちタイムアウト(ms)|  
|select_timeout         |5000000     |接続受付待ちタイムアウト(ms)|  
|send_timeout           |30000000    |送信バッファ空き待ちタイムアウト(ms)|  
|recv_timeout           |60000000    |応答待ちタイムアウト(ms)|  
|req_timeout            |100000      |処理タスク受付待ちタイムアウト(ms)|  
|node_recv_timeout      |1000000     |ノード情報受信待ちタイムアウト(ms)|  
|so_sndbuf              |65536       |SO_SNDBUFサイズ|  
|so_rcvbuf              |131072      |SO_RCVBUFサイズ|  
|con_sleep_loop         |10          |未使用|  
|con_compression        |6           |通信データのストリーミング圧縮レベル(0～9)|  
|pipe_send_timeout      |0           |パイプ送信待ちタイムアウト(ms)|  
|pipe_recv_timeout      |0           |パイプ受信待ちタイムアウト(ms)|  
|enospc_retry           |50          |セマフォロック確保リトライ回数|  
|enospc_retrans         |5000        |セマフォロックのリトライ間隔(ms)|  
|lock_retry             |100         |キャッシュロック確保リトライ回数|  
|lock_sleep             |1000000     |キャッシュロックのリトライ間隔(ms)|  



