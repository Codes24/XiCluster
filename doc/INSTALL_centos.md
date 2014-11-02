## インストール環境
RightImage_CentOS_6.5_x64_v14.0_EBS - ami-03793402 64bit

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
#cd XiCluster/RPMS/x86_64  
#rpm -ihv xicluster-*.*-*.x86_64.rpm  
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
#su - 
#service xicluster start  
```

## XiCluster停止
```
#su -   
#service xicluster stop  
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
$xicluster_client
XICLUSTER> status
XICLUSTER> ls
XICLUSTER> mkdir hoge
```
