## インストール環境
Amazon Elastic MapReduceの各ノードにxiclusterを導入  

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
#rpm -ihv ./XiCluster/RPMS/x86_64/xicluster-*.*-*.x86_64.rpm
#ldconfig  
```

## XiClusterの設定
[パラメータ一覧](PARAMETER.md)を参考に設定変更を行う。
```
#vi /usr/local/xicluster/conf/xicluster.conf  
#vi /usr/local/xicluster/conf/server.lst
```

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

