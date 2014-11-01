## パフォーマンス検証
hadoop(amazon EMR) vs XiCluster
hadoopとXiClusterで同居した環境にて検証  

## 検証環境の参考性能
ディスク書込み性能(ddコマンド)：65.4MB/sec    
ネットワーク転送(scpコマンド)：55.6MB/sec  

## セットアップ
(1)AWSにてマスター1台、スレーブ2台のEMRを作成  
(2)各ノード間でTCPのポート9020,9030を許可  
(3)作成した3台にXiClusterをインストール  
※[XiClusterをインストール手順](INSTALL_emr.md)を参照

## 手順
(0)基本性能確認
```
time dd if=/dev/zero of=/mnt/hoge.dat bs=10M count=1024
time scp -i XXXXX.pem /mnt/hoge.dat ec2-user@xxx.xxx.xxx.xxx:/mnt/data
```

(1)ファイル作成  
```
dd if=/dev/zero of=/mnt/data/hoge1024.dat bs=1M count=1024
dd if=/dev/zero of=/mnt/data/hoge64.dat bs=1M count=64
```

(2)hadoopで1GByteファイルをput/get(２回実行）
```
sudo su -
sync
echo 3 > /proc/sys/vm/drop_caches
su - hadoop
time hadoop fs -put /mnt/data/hoge1024.dat /hoge1.dat
time hadoop fs -put /mnt/data/hoge1024.dat /hoge2.dat
time hadoop fs -get /hoge1.dat /mnt/data/hoge1.dat
time hadoop fs -get /hoge2.dat /mnt/data/hoge2.dat
```

(3)XiClusterで1GByteファイルをput/get(２回実行）
```
sync
echo 3 > /proc/sys/vm/drop_caches  
time xicluster_client -n put /mnt/data/hoge1024.dat /hoge1.dat
time xicluster_client -n put /mnt/data/hoge1024.dat /hoge2.dat
time xicluster_client -n get /hoge1.dat /mnt/data/hoge1.dat
time xicluster_client -n get /hoge2.dat /mnt/data/hoge2.dat
```

(4)XiClusterで1GByteファイルを非圧縮put/get(２回実行）
```
sync
echo 3 > /proc/sys/vm/drop_caches  
time xicluster_client -para con_compression=0 -n put /mnt/data/hoge1024.dat /hoge1.dat
time xicluster_client -para con_compression=0 -n put /mnt/data/hoge1024.dat /hoge2.dat
time xicluster_client -para con_compression=0 -n get /hoge1.dat /mnt/data/hoge1.dat
time xicluster_client -para con_compression=0 -n get /hoge2.dat /mnt/data/hoge2.dat
```

(5)hadoopで64MByteの1000ファイルをput/get
```
for (( i=0; i<100; i++ ))
do
time hadoop fs -put /mnt/data/hoge64.dat /hoge${i}.dat
done

for (( i=0; i<100; i++ ))
do
time hadoop fs -get /hoge${i}.dat /mnt/data/hoge${i}.dat 
done
```

(6)XiClusterで64MByteの100ファイルをput/get
```
for (( i=0; i<100; i++ ))
do
time xicluster_client -n put /mnt/data/hoge64.dat /hoge${i}.dat
done

for (( i=0; i<100; i++ ))
do
time xicluster_client -n get /hoge${i}.dat  /mnt/data/hoge${i}.dat 
done

```

## 検証結果

(1)1GByteのファイルput/get処理  
|処理|処理時間|user|sys|処理速度|
|----|--------|----|----|----|
|hadoop(put) 1回目|17.167s|12.941s|4.356s|59.6MB/sec|
|hadoop(put) 2回目|16.353s|12.665s|4.052s|62.6MB/sec|
|hadoop(get) 1回目|15.050s|8.709s|4.972s|68.0MB/sec|
|hadoop(get) 2回目|17.508s|7.892s|5.148s|58.0MB/sec|
|XiCluster(put) 1回目|26.21s|12.461s|0.748s|39.0MB/sec|
|XiCluster(put) 2回目|23.622s|11.909s|0.508s|43.3MB/sec|
|XiCluster(get) 1回目|26.091s|4.232s|1.176s|39.2MB/sec|
|XiCluster(get) 2回目|23.101s|4.256s|1.164s|44.3MB/sec|
|XiCluster非圧縮(put) 1回目|24.229s|12.857s|0.908s|42.2MB/sec|
|XiCluster非圧縮(put) 2回目|22.164s|12.553s|0.58s|46.2MB/sec|
|XiCluster(非圧縮get) 1回目|18.955s|4.444s|1.5s|54.0MB/sec|
|XiCluster非圧縮(get) 2回目|18.669s|4.396s|1.568s|54.8MB/sec|


(2)64MBの100ファイルのput/get処理時間  
|処理|処理時間|user|sys|処理速度|
|----|--------|----|----|----|
|hadoop(put)|626.286s|583.836s|83.956s|10.2MB/sec|
|hadoop(get)|757.776s|544.516s|86.876s|8.4MB/sec|
|XiCluster(put)|159.886s|75.552s|3.516s|40.0MB/sec|
|XiCluster(get)|124.89s|27.812s|9.66s|51.2MB/sec|


## 考察
XiClusterの独自データ転送プロトコルはストリーミング圧縮＋暗号化＋データフロー制御を行っているので障害にセキュリティ面に強いがパフォーマンスを犠牲にしている。その為にスループットはhadoopに劣るが、ストリーミング圧縮している分、データ転送量が少なくなるのでネットワーク帯域消費量が少なく、パラレル転送する場合はXiClusterが有利になると思われる。  
hadoopは起動時の処理オーバヘッドが5秒ほどあるので、ファイル数の多い場合は処理時間に大きな影響を与える。
