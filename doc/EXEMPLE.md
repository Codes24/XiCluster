# コマンド実行例

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
$ps -ef | grep XICLUSTER  
11000     4320  4256  0 14:41 pts/0    00:00:00 grep XICLUSTER
11000     4616     1  0 14:15 pts/0    00:00:14 XICLUSTER_000_SMAN
11000     4617     1  0 14:15 pts/0    00:00:00 XICLUSTER_001_NLSR 172.31.29.135:9010
11000     4618     1  0 14:15 pts/0    00:00:00 XICLUSTER_002_CLSR 172.31.29.135:9020
11000     4619     1  0 14:15 pts/0    00:00:10 XICLUSTER_003_CACH
11000     4620     1  0 14:15 pts/0    00:00:00 XICLUSTER_004_CRCV 172.31.29.135:9030
11000     4621     1  0 14:15 pts/0    00:00:11 XICLUSTER_005_CSND
11000     4622     1  0 14:15 pts/0    00:00:11 XICLUSTER_006_CSND
11000     4623     1  0 14:15 pts/0    00:00:04 XICLUSTER_007_MSYN
11000     4624     1  3 14:15 pts/0    00:00:50 XICLUSTER_008_DSYN
11000     4625     1  0 14:15 pts/0    00:00:11 XICLUSTER_009_DISK /usr/local/xicluster/data
11000     4626     1  0 14:15 pts/0    00:00:04 XICLUSTER_010_DMNT
11000     4627     1  0 14:15 pts/0    00:00:08 XICLUSTER_011_TASK
11000     4628     1  0 14:15 pts/0    00:00:00 XICLUSTER_012_TASK
11000     4629     1  0 14:15 pts/0    00:00:00 XICLUSTER_013_TASK
11000     4630     1  0 14:15 pts/0    00:00:00 XICLUSTER_014_TASK
11000     4631     1  0 14:15 pts/0    00:00:00 XICLUSTER_015_TASK
11000     4632     1  0 14:15 pts/0    00:00:00 XICLUSTER_016_TASK
11000     4633     1  0 14:15 pts/0    00:00:00 XICLUSTER_017_TASK
11000     4634     1  0 14:15 pts/0    00:00:00 XICLUSTER_018_TASK
11000     4635     1  0 14:15 pts/0    00:00:00 XICLUSTER_019_TASK
11000     4636     1  0 14:15 pts/0    00:00:00 XICLUSTER_020_TASK

$ipcs -a  
------ Message Queues --------
key        msqid      owner      perms      used-bytes   messages

------ Shared Memory Segments --------
key        shmid      owner      perms      bytes      nattch     status
0x01005ad2 32768      xicluster  660        336334848  21

------ Semaphore Arrays --------
key        semid      owner      perms      nsems
0x02005ad2 32768      xicluster  660        25
```

## XiClusterクライアントで動作確認

[ノード情報表示]  
```
$xicluster_client status

[cluster information]
No  F nodename        ipaddress       sts start_tm   SCN                  KA   Max  Busy
--- - --------------- --------------- --- ---------- -------------------- ---- ---- ----
000 M ip-172-31-29-13 172.31.29.134   010 2014/10/20 20141020:00004:00150 1    10   0
001 S ip-172-31-29-13 172.31.29.135   010 2014/10/20 20141020:00004:00150 1    10   0
002 S ip-172-31-22-68 172.31.22.68    010 2014/10/20 20141020:00004:00150 1    10   0
```

[共有メモリ内訳表示]  
```
$xicluster_client mem

[Shared Memory]
memory header Area       = 1024byte
system configration Area = 1024byte
cluster information Area = 133120byte
process information Area = 111616byte
Disk Management Area     = 5120byte
Meta Cache Area          = 536576byte
Index Cache Area         = 1024byte
Data Cache Area          = 335545344byte
TOTAL                    = 336334848byte

[Data Cache information]
Memory Block Size   = 67108864byte
Memory Total Blocks = 5
Memory Free Blocks  = 0
```

[キャッシュ情報表示]  
```
$xicluster_client cache

[META CACHE]
No   sts         pid      dcnt ncnt write time          FileID
---- ----------- -------- ---- ---- ------------------- -----------------------------
0000 20:10:10:00     3339   16   16 2014/10/20 14:26:22 54451B8E095E87.54451B8E095E87
0001 20:10:10:00     3339   16   16 2014/10/20 14:26:59 54451BB307375C.54451BB307375C
0002 20:10:10:00     3339   16   16 2014/10/20 14:27:35 54451BD7001AF4.54451BD7001AF4

[DATA CACHE]
No   sts         pid      size     write time          BlockID
---- ----------- -------- -------- ------------------- --------------------------------
0000 20:10:10:00     3339 67108864 2014/10/20 14:27:55 54451BD7001AF4.54451BD7001AF4:14
0001 20:10:10:00     3339 67108864 2014/10/20 14:27:56 54451BD7001AF4.54451BD7001AF4:15
0002 20:10:10:00     3339 67108864 2014/10/20 14:27:58 54451BD7001AF4.54451BD7001AF4:16
0003 20:10:10:00     3339 67108864 2014/10/20 14:27:51 54451BD7001AF4.54451BD7001AF4:12
0004 20:10:10:00     3339 67108864 2014/10/20 14:27:53 54451BD7001AF4.54451BD7001AF4:13
```

[ディスク情報表示]  
```
$xicluster_client disk

DISK disks=1 devices=1 total=10079MB use=7080MB(70%) free=2998MB

FilePath                  DeviceName           STS SCN                  Size(MB)  Used(MB)  Free(MB)  Use% Busy%
------------------------- -------------------- --- -------------------- --------- --------- --------- ---- ------
/usr/local/xicluster/data /dev/xvda            010 20141020:00004:00150     10079      7080      2999  70%  69.9%
```

[プロセス情報表示]  
```
$xicluster_client process

[process information]
No  pid   sts start time          REQ IRD fork KA   process name
--- ----- --- ------------------- --- --- ---- ---- -------------------------
000 3328  010 2014/10/20 14:15:49 000 ---    1    0 XICLUSTER_000_SMAN
001 3329  010 2014/10/20 14:15:49 000 ---    1    3 XICLUSTER_001_NLSR 172.31.29.134:9010
002 3330  010 2014/10/20 14:15:49 000 ---    1    0 XICLUSTER_002_CLSR 172.31.29.134:9020
003 3331  010 2014/10/20 14:15:49 000 ---    1    0 XICLUSTER_003_CACH
004 3332  010 2014/10/20 14:15:49 000 ---    1    1 XICLUSTER_004_CRCV 172.31.29.134:9030
005 3333  010 2014/10/20 14:15:49 000 ---    1    0 XICLUSTER_005_CSND
006 3334  010 2014/10/20 14:15:49 000 ---    1    0 XICLUSTER_006_CSND
007 3335  010 2014/10/20 14:15:49 000 ---    1    0 XICLUSTER_007_MSYN
008 3336  010 2014/10/20 14:15:49 000 ---    1    0 XICLUSTER_008_DSYN
009 3337  010 2014/10/20 14:15:49 000 ---    1    0 XICLUSTER_009_DISK /usr/local/xicluster/data
010 3338  010 2014/10/20 14:15:49 000 ---    1    0 XICLUSTER_010_DMNT
011 3339  011 2014/10/20 14:15:49 010 ---    1    0 XICLUSTER_011_TASK
012 3340  010 2014/10/20 14:15:49 000 ---    1    0 XICLUSTER_012_TASK
013 3341  010 2014/10/20 14:15:49 000 ---    1    0 XICLUSTER_013_TASK
014 3342  010 2014/10/20 14:15:49 000 ---    1    0 XICLUSTER_014_TASK
015 3343  010 2014/10/20 14:15:49 000 ---    1    0 XICLUSTER_015_TASK
016 3344  010 2014/10/20 14:15:49 000 ---    1    0 XICLUSTER_016_TASK
017 3345  010 2014/10/20 14:15:49 000 ---    1    0 XICLUSTER_017_TASK
018 3346  010 2014/10/20 14:15:49 000 ---    1    0 XICLUSTER_018_TASK
019 3347  010 2014/10/20 14:15:49 000 ---    1    0 XICLUSTER_019_TASK
020 3348  010 2014/10/20 14:15:49 000 ---    1    0 XICLUSTER_020_TASK
```

[パフォーマンス情報表示]  
```
$xicluster_client perf

[performance information]

[CPU]
cpus=2 user=0.7% nice=0.0% sys=0.3% idle=98.9% wait=0.1%

[Memory]
total=7454MB free=2757MB buff=93MB cache=3469MB

[Disk]
DISK disks=1 devices=1 total=10079MB use=7080MB(70%) free=2998MB

FilePath                  DeviceName           STS SCN                  Size(MB)  Used(MB)  Free(MB)  Use% Busy%
------------------------- -------------------- --- -------------------- --------- --------- --------- ---- ------
/usr/local/xicluster/data /dev/xvda            010 20141020:00004:00150     10079      7080      2999  70%  89.9%
```

