## パフォーマンス検証
hadoop(amazon EMR) vs XiCluster

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
|処理|処理時間|user|sys|
|----|--------|----|----|
|hadoop(put) 1回目|26.656s|13.233s|3.724s|
|hadoop(put) 2回目|13.000s|12.417s|2.968s|
|hadoop(put) 3回目|18.972s|12.601s|3.304s|
|hadoop(get) 1回目|19.682s|6.756s|1.752s|
|hadoop(get) 2回目|18.282s|6.740s|1.932s|
|hadoop(get) 3回目|22.315s|6.964s|2.088s|
|XiCluster(put) 1回目|27.266s|12.973s|1.484s|
|XiCluster(put) 2回目|24.360s|12.585s|1.568s|
|XiCluster(put) 3回目|24.617s|12.501s|1.480s|
|XiCluster(get) 1回目|21.102s|5.108s|3.220s|
|XiCluster(get) 2回目|20.010s|4.864s|3.304s|
|XiCluster(get) 3回目|26.882s|4.796s|3.228s|

## 考察
XiClusterのデータ転送はストリーミング圧縮が行われているので、処理時間でhadoopに負けている。 
データ転送量が少なくなるのでsys使用量が少なく、ネットワーク帯域の使用量はXiClusterが少ないので、パラレル転送する場合はXiClusterが有利になる。  
ディスクIO、ネットワーク転送（カーネルバッファ）がほぼ性能限界まで利用されているので、この検証では、どちらも性能に大差なし。  

