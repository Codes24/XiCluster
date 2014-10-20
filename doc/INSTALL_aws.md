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
[xicluster@ip-172-31-7-46 doc]$ ps -ef | grep XICLUSTER
11000     3045     1  0 09:58 pts/0    00:00:00 XICLUSTER_000_SMAN
11000     3046     1  0 09:58 pts/0    00:00:00 XICLUSTER_001_NLSR 172.31.7.46:9010
11000     3047     1  0 09:58 pts/0    00:00:00 XICLUSTER_002_CLSR 172.31.7.46:9020
11000     3048     1  0 09:58 pts/0    00:00:00 XICLUSTER_003_CACH
11000     3049     1  0 09:58 pts/0    00:00:00 XICLUSTER_004_CRCV 172.31.7.46:9030
11000     3050     1  0 09:58 pts/0    00:00:00 XICLUSTER_005_CSND
11000     3051     1  0 09:58 pts/0    00:00:00 XICLUSTER_006_CSND
11000     3052     1  0 09:58 pts/0    00:00:00 XICLUSTER_007_MSYN
11000     3053     1  0 09:58 pts/0    00:00:00 XICLUSTER_008_DSYN
11000     3054     1  0 09:58 pts/0    00:00:00 XICLUSTER_009_DISK /usr/local/xicluster/data
11000     3055     1  0 09:58 pts/0    00:00:00 XICLUSTER_010_DMNT
11000     3056     1  0 09:58 pts/0    00:00:00 XICLUSTER_011_TASK
11000     3057     1  0 09:58 pts/0    00:00:00 XICLUSTER_012_TASK
11000     3058     1  0 09:58 pts/0    00:00:00 XICLUSTER_013_TASK
11000     3059     1  0 09:58 pts/0    00:00:00 XICLUSTER_014_TASK
11000     3060     1  0 09:58 pts/0    00:00:00 XICLUSTER_015_TASK
11000     3061     1  0 09:58 pts/0    00:00:00 XICLUSTER_016_TASK
11000     3062     1  0 09:58 pts/0    00:00:00 XICLUSTER_017_TASK
11000     3063     1  0 09:58 pts/0    00:00:00 XICLUSTER_018_TASK
11000     3064     1  0 09:58 pts/0    00:00:00 XICLUSTER_019_TASK
11000     3065     1  0 09:58 pts/0    00:00:00 XICLUSTER_020_TASK
11000     3067  2237  0 09:58 pts/0    00:00:00 grep XICLUSTER

$ipcs -a  
------ メッセージキュー --------
キー     msqid      所有者  権限     使用バイト数 メッセージ

------ 共有メモリセグメント --------
キー     shmid      所有者  権限     バイト  nattch     状態
0x01012396 32768      xicluster  660        336334848  21

------ セマフォ配列 --------
キー     semid      所有者  権限     nsems
0x02012396 32768      xicluster  660        25

```

## XiClusterクライアントで動作確認
```
$xicluster_client
XICLUSTER> status
No  F nodename        ipaddress       sts start_tm   SCN                  KA   Max  Busy
--- - --------------- --------------- --- ---------- -------------------- ---- ---- ----
000 M ip-172-31-7-46  172.31.7.46     010 2014/10/19 20141019:00002:00000 1    10   0
001 S ip-172-31-12-23 172.31.12.232   010 2014/10/19 20141019:00002:00000 1    10   0

XICLUSTER> ls

XICLUSTER> mkdir hoge
```
