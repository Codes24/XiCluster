## インストール環境
Amazon Linux AMI 2014.09 (HVM) - ami-35072834  64bit  

## 必要なパッケージのインストール
```
#yum install gcc  
#yum install gcc-c++  
#yum install openssl-devel  
#yum install rpm-build    
#yum install git  
```

## XiClusterのインストール
```
#git clone https://github.com/takakusaki/XiCluster.git  
#cd xicluster/RPMS/x86_64  
#rpm -ihv xicluster-*.*-*.x86_64.rpm  
#ldconfig  
```

## XiClusterの設定
[パラメータ一覧](https://github.com/takakusaki/XiCluster/blob/master/doc/PARAMETER.md)を参考に設定変更を行う。
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

## XiClusterクライアントで動作確認
```
$xicluster_client
XICLUSTER> status
XICLUSTER> ls
XICLUSTER> mkdir hoge
```
