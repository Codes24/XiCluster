## パフォーマンス検証
hadoop(amazon EMR) vs XiCluster

## 検証環境の参考性能
ディスク書込み性能(ddコマンド)：52.8MB/sec(1回目)  53.4MB/sec(2回目)    
ネットワーク転送(scpコマンド)：54.8MB/sec(1回目)  61.7MB/sec(2回目)  

## セットアップ
(1)AWSにてマスター1台、スレーブ2台のEMRを作成  
(2)各ノード間でTCPのポート9010,9020,9030を許可  
(3)作成した3台に[XiClusterをインストール](INSTALL_emr.md)   
hadoopとXiClusterで同居した環境にて検証  

## 手順
(1)ファイル作成  
```
dd if=/dev/zero of=hoge.dat bs=1M count=1024  
```

(2)hadoopでファイルのget/putを実施(３回実行）
```
sync
echo 3 > /proc/sys/vm/drop_caches
time hadoop fs -put /mnt1/data/hoge.dat /hoge1.dat
time hadoop fs -put /mnt1/data/hoge.dat /hoge2.dat
time hadoop fs -put /mnt1/data/hoge.dat /hoge3.dat
time hadoop fs -get /hoge1.dat /mnt1/data/hoge1.dat
time hadoop fs -get /hoge2.dat /mnt1/data/hoge2.dat
time hadoop fs -get /hoge3.dat /mnt1/data/hoge3.dat
```

(3)XiClusterでファイルのget/putを実施(３回実行）
```
sync
echo 3 > /proc/sys/vm/drop_caches  
time xicluster_client put /mnt1/data/hoge.dat /hoge1.dat
time xicluster_client put /mnt1/data/hoge.dat /hoge2.dat
time xicluster_client put /mnt1/data/hoge.dat /hoge3.dat
time xicluster_client get /hoge1.dat /mnt1/data/hoge1.dat
time xicluster_client get /hoge2.dat /mnt1/data/hoge2.dat
time xicluster_client get /hoge3.dat /mnt1/data/hoge3.dat
```

## 検証結果
|処理|処理時間|user|sys|処理速度|
|----|--------|----|----|----|
|hadoop(put) 1回目|26.656s|13.233s|3.724s|38.4MB/sec|
|hadoop(put) 2回目|13.000s|12.417s|2.968s|78.7MB/sec|
|hadoop(put) 3回目|18.972s|12.601s|3.304s|53.9MB/sec|
|hadoop(get) 1回目|19.682s|6.756s|1.752s|52.0MB/sec|
|hadoop(get) 2回目|18.282s|6.740s|1.932s|56.0MB/sec|
|hadoop(get) 3回目|22.315s|6.964s|2.088s|45.8MB/sec|
|XiCluster(put) 1回目|27.266s|12.973s|1.484s|37.5MB/sec|
|XiCluster(put) 2回目|24.360s|12.585s|1.568s|42.0MB/sec|
|XiCluster(put) 3回目|24.617s|12.501s|1.480s|41.5MB/sec|
|XiCluster(get) 1回目|21.102s|5.108s|3.220s|48.5MB/sec|
|XiCluster(get) 2回目|20.010s|4.864s|3.304s|51.1MB/sec|
|XiCluster(get) 3回目|26.882s|4.796s|3.228s|38.0MB/sec|

## 考察

XiClusterのデータ転送はストリーミング圧縮が行われているので、スループットはhadoopに若干劣るが、
圧縮している分、データ転送量が少なくなるのでネットワーク帯域消費量が少ない。その結果、パラレル転送する場合はXiClusterが有利になる。  

