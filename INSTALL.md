## 導入環境
EC2 t2.micro ×　3台(全マスタ構成）  
Amazon Linux 64bit  

## 必要なパッケージのインストール
```
yum install gcc  
yum install gcc-c++  
yum install openssl-devel  
yum install rpm-build    
yum install git  
```

## XiClusterのインストール
```
git clone https://github.com/takakusaki/XiCluster.git  
cd xicluster/RPMS/x86_64  
rpm -ihv xicluster-0.0-0.x86_64.rpm  
ldconfig  
```

## XiClusterの設定
```
vi /usr/local/xicluster/conf/xicluster.conf  
```

## XiCluster起動
```
su - xicluster  
xicluster_server start  
```

## XiCluster停止
```
su - xicluster  
xicluster_server stop  
```

## XiCluster稼動確認
```
xicluster_client status  
ps -ef | grep XICLUSTER  
ipcs -a  
```

## ログファイル
/usr/local/xicluster/log/YYYYMMDD.log

## 2台目以降のノードを追加する
(1)１台目のイメージ(AMI)を作成する  
(2)AMIを利用してEC2を作成する。  
(3)各ノード間の通信を通す。  
　　　　UDP9010　：　ノード情報転送  
　　　　TCP9020　：　サーバ間キャッシュ転送  
　　　　TCP9030　：　クライアント間通信  
(4)XiClusterサーバを起動する。  

