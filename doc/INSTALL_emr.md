## インストール環境
Amazon Elastic MapReduceの各ノードにxiclusterを導入  
下記IPアドレスのm1.largeインスタンス３台に導入する手順を記載  
```
172.31.29.221
172.31.18.89
172.31.18.90
```


## 必要なパッケージのインストール
```
#yum install gcc-c++  
#yum install openssl-devel  
#yum install rpm-build    
#yum install git  
```

## XiClusterのインストール
```
#git clone https://github.com/takakusaki/XiCluster.git
#cd ./XiCluster/RPMS/x86_64
#rpm -ihv xicluster-*.*-*.x86_64.rpm
#ldconfig  
```

## XiClusterの設定
[パラメータ一覧](PARAMETER.md)を参考に設定変更を行う。
```
#vi /usr/local/xicluster/conf/xicluster.conf  
#cat /usr/local/xicluster/conf/xicluster.conf
network_if_svr = eth0
network_if_clt = eth0
network_port_svr = 9030
network_port_clt = 9020
network_port_cache = 9030
data_dir = /usr/local/xicluster/data

#vi /usr/local/xicluster/conf/server.lst
#cat /usr/local/xicluster/conf/server.lst
172.31.29.221
172.31.18.89
172.31.18.90
```

#セキュリティーグループ設定
ElasticMapReduce-masterセキュリティグループとElasticMapReduce-slaveセキュリティグループのinboundに通信許可設定を行う。  
|Type|Protocol|Port Range|Source|
|----|--------|----------|------|
|Custom TCP Rule|TCP|9020|172.31.29.221/32|
|Custom TCP Rule|TCP|9020|172.31.18.89/32|
|Custom TCP Rule|TCP|9020|172.31.18.90/32|
|Custom TCP Rule|TCP|9030|172.31.29.221/32|
|Custom TCP Rule|TCP|9030|172.31.18.89/32|
|Custom TCP Rule|TCP|9030|172.31.18.90/32|


## XiCluster起動
```
#su - xicluster  
$xicluster_server start  
```

## XiCluster停止
```
#su - xicluster  
$xicluster_server stop  
```

## XiClusterサーバ稼動確認
```
$xicluster_server status  
$ps -ef | grep XICLUSTER
$ipcs -a  
```
詳しくは[実行例](EXEMPLE.md)を参照して下さい。  

## XiClusterクライアントで動作確認
```
$xicluster_client status
XICLUSTER> ls
XICLUSTER> mkdir hoge
```

