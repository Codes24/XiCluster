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
[cluster information]
No  F nodename        ipaddress       sts start_tm   SCN                  KA   Max  Busy
--- - --------------- --------------- --- ---------- -------------------- ---- ---- ----
000 M ip-172-31-0-114 172.31.0.114    010 2014/10/20 20141020:00001:00000 1    10   0

$ps -ef | grep XICLUSTER  
11000     2346     1  0 13:31 pts/0    00:00:00 XICLUSTER_000_SMAN
11000     2347     1  0 13:31 pts/0    00:00:00 XICLUSTER_001_NLSR 172.31.0.114:9010
11000     2348     1  0 13:31 pts/0    00:00:00 XICLUSTER_002_CLSR 172.31.0.114:9020
11000     2349     1  0 13:31 pts/0    00:00:00 XICLUSTER_003_CACH
11000     2350     1  0 13:31 pts/0    00:00:00 XICLUSTER_004_CRCV 172.31.0.114:9030
11000     2351     1  0 13:31 pts/0    00:00:00 XICLUSTER_005_CSND
11000     2352     1  0 13:31 pts/0    00:00:00 XICLUSTER_006_CSND
11000     2353     1  0 13:31 pts/0    00:00:00 XICLUSTER_007_MSYN
11000     2354     1  0 13:31 pts/0    00:00:00 XICLUSTER_008_DSYN
11000     2355     1  0 13:31 pts/0    00:00:00 XICLUSTER_009_DISK /usr/local/xicluster/data
11000     2356     1  0 13:31 pts/0    00:00:00 XICLUSTER_010_DMNT
11000     2357     1  0 13:31 pts/0    00:00:00 XICLUSTER_011_TASK
11000     2358     1  0 13:31 pts/0    00:00:00 XICLUSTER_012_TASK
11000     2359     1  0 13:31 pts/0    00:00:00 XICLUSTER_013_TASK
11000     2360     1  0 13:31 pts/0    00:00:00 XICLUSTER_014_TASK
11000     2361     1  0 13:31 pts/0    00:00:00 XICLUSTER_015_TASK
11000     2362     1  0 13:31 pts/0    00:00:00 XICLUSTER_016_TASK
11000     2363     1  0 13:31 pts/0    00:00:00 XICLUSTER_017_TASK
11000     2364     1  0 13:31 pts/0    00:00:00 XICLUSTER_018_TASK
11000     2365     1  0 13:31 pts/0    00:00:00 XICLUSTER_019_TASK
11000     2366     1  0 13:31 pts/0    00:00:00 XICLUSTER_020_TASK
11000     2370  2079  0 13:32 pts/0    00:00:00 grep XICLUSTER

$ipcs -a  

------ Shared Memory Segments --------
key        shmid      owner      perms      bytes      nattch     status
0x0101e533 32768      xicluster  660        336334848  21

------ Semaphore Arrays --------
key        semid      owner      perms      nsems
0x0201e533 98306      xicluster  660        25

------ Message Queues --------
key        msqid      owner      perms      used-bytes   messages

```

## XiClusterクライアントで動作確認
```
$xicluster_client
XICLUSTER> status
XICLUSTER> ls
XICLUSTER> mkdir hoge
```
