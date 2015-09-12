/*****************************************************************************
*  <システム>   分散処理フレームワーク
*  <名称>	パフォーマンス情報収集API
*  <目的>       
*  <機能>       
*  <開発環境>   UNIX
*  <特記事項>
*
*  VERSION    DATE        BY                CHANGE/COMMENT
*  -----------------------------------------------------------------------------
*  V0.00      2014/02/08  Takakusaki        新規作成
******************************************************************************/
#include "xi_common.h"

int perfGetCPUavg(int *cpu_cnt, float *cpu_user, float *cpu_nice, float *cpu_system, float *cpu_idle, float *cpu_iowait){
	int ret,i;
	static float keep_perf[PERF_CPU_AVG_KEEP][5];
	static int keep_flg=0;
	float total_perf[5];

	//現時点のパフォーマンス情報取得
	ret=perfGetCPU(cpu_cnt, cpu_user, cpu_nice, cpu_system, cpu_idle, cpu_iowait);
//printf("ret=%d %f %f %f %f %f\n",ret,*cpu_user, *cpu_nice, *cpu_system, *cpu_idle, *cpu_iowait);
	if ( ret < 0 ){ return ret; }

	//過去世代
	if ( keep_flg == 0 ){
		for ( i=0; i<PERF_CPU_AVG_KEEP; i++){
			keep_perf[i][0]=*cpu_user;
			keep_perf[i][1]=*cpu_nice;
			keep_perf[i][2]=*cpu_system;
			keep_perf[i][3]=*cpu_idle;
			keep_perf[i][4]=*cpu_iowait;
		}
	}else{
		for ( i=PERF_CPU_AVG_KEEP - 1; i>=1; i--){
			keep_perf[i][0]=keep_perf[i-1][0];
			keep_perf[i][1]=keep_perf[i-1][1];
			keep_perf[i][2]=keep_perf[i-1][2];
			keep_perf[i][3]=keep_perf[i-1][3];
			keep_perf[i][4]=keep_perf[i-1][4];
		}
		keep_perf[0][0]=*cpu_user;
		keep_perf[0][1]=*cpu_nice;
		keep_perf[0][2]=*cpu_system;
		keep_perf[0][3]=*cpu_idle;
		keep_perf[0][4]=*cpu_iowait;
	}
	keep_flg=1;
//for ( i=0; i<5; i++){ printf("total_perf[%d] = %f\n",i,keep_perf[0][i]); }

	//平均値を求める
	for ( i=0; i<5; i++){ total_perf[i]=0; }
	for ( i=0; i<PERF_CPU_AVG_KEEP; i++){
		total_perf[0] += keep_perf[i][0];
		total_perf[1] += keep_perf[i][1];
		total_perf[2] += keep_perf[i][2];
		total_perf[3] += keep_perf[i][3];
		total_perf[4] += keep_perf[i][4];
	}
//for ( i=0; i<5; i++){ printf("total_perf[%d] = %f\n",i,total_perf[i]); }
	for ( i=0; i<5; i++){ total_perf[i] = total_perf[i] / PERF_CPU_AVG_KEEP; }

	*cpu_user = total_perf[0];
	*cpu_nice = total_perf[1];
	*cpu_system = total_perf[2];
	*cpu_idle = total_perf[3];
	*cpu_iowait = total_perf[4];

//printf("%d %f %f %f %f %f\n", *cpu_cnt, *cpu_user, *cpu_nice, *cpu_system, *cpu_idle, *cpu_iowait);
	return ret;	
}

int perfGetCPU(int *cpu_cnt, float *cpu_user, float *cpu_nice, float *cpu_system, float *cpu_idle, float *cpu_iowait){
	FILE *fp;
	char buff[2048];
	char cpuname[1024];
	u_long ccnt=0;
	u_long c1,c2,c3,c4,c5;
	u_long h_c1,h_c2,h_c3,h_c4,h_c5, h_all;
	static int s_flg=0;
	static u_long s_c1=0,s_c2=0,s_c3=0,s_c4=0,s_c5=0;

	*cpu_user = 0;
	*cpu_nice = 0;
	*cpu_system = 0;
	*cpu_idle = 0;
	*cpu_iowait = 0;

	if ( (fp=fopen("/proc/stat","r")) == NULL ){
		return -1;
	}

	while( fgets(buff,sizeof(buff),fp) != NULL ){
		if ( strncmp(buff,"cpu",3) != 0 ){ continue; }
		if ( strncmp(buff,"cpu ",4) == 0 ){
			sscanf(buff,"%s %ld %ld %ld %ld %ld",cpuname, &c1, &c2, &c3, &c4, &c5);
			//printf("[%s][%ld][%ld][%ld][%ld][%ld]\n",cpuname, c1, c2, c3, c4, c5);
			continue; 
		}
		ccnt++;	
	}
	fclose(fp);

	//取得値設定
	*cpu_cnt = ccnt;
	if ( s_flg == 1 ){
		h_c1=c1 - s_c1;
		h_c2=c2 - s_c2;
		h_c3=c3 - s_c3;
		h_c4=c4 - s_c4;
		h_c5=c5 - s_c5;
		h_all=h_c1 + h_c2 + h_c3 + h_c4 + h_c5;
		//printf("[%ld][%ld][%ld][%ld][%ld]\n",h_c1, h_c2, h_c3, h_c4, h_c5);
		if ( h_all > 0 ){
			*cpu_user = (float)h_c1 / h_all * 100;
			*cpu_nice = (float)h_c2 / h_all * 100;
			*cpu_system = (float)h_c3 / h_all * 100;
			*cpu_idle = (float)h_c4 / h_all * 100;
			*cpu_iowait = (float)h_c5 / h_all * 100;
		}
	}
	s_c1=c1;
	s_c2=c2;
	s_c3=c3;
	s_c4=c4;
	s_c5=c5;
	s_flg=1;

	//printf("%d %f %f %f %f %f\n", *cpu_cnt, *cpu_user, *cpu_nice, *cpu_system, *cpu_idle, *cpu_iowait);

	return 0;
}

int perfGetMEM(u_long *mem_size, u_long *mem_free, u_long *mem_buffer, u_long *mem_cache, u_long *mem_active, u_long *mem_inactive){
	FILE *fp;
	char buff[2048];
	char c1[1024];
	char c2[1024];
	u_long val;

	if ( (fp=fopen("/proc/meminfo","r")) == NULL ){
		return -1;
	}

	while( fgets(buff,sizeof(buff),fp) != NULL ){
		utlStringSep(buff," \t",1,c1);	
		utlStringSep(buff," \t",2,c2);	
		if ( strcmp(c1,"MemTotal:") == 0){ *mem_size=atol(c2); }
		if ( strcmp(c1,"MemFree:") == 0){ *mem_free=atol(c2); }
		if ( strcmp(c1,"Buffers:") == 0){ *mem_buffer=atol(c2); }
		if ( strcmp(c1,"Cached:") == 0){ *mem_cache=atol(c2); }
		if ( strcmp(c1,"Active:") == 0){ *mem_active=atol(c2); }
		if ( strcmp(c1,"Inactive:") == 0){ *mem_inactive=atol(c2); }
	}
	fclose(fp);

//printf("[%ld][%ld][%ld][%ld][%ld][%ld]\n",*mem_size, *mem_free, *mem_buffer, *mem_cache, *mem_active, *mem_inactive);
	return 0;
}

int perfGetDISK(char *dev, u_long *f_bsize, u_long *f_blocks, u_long *f_use, u_long *f_free){
	struct statvfs fsbuf;

	//ディスク使用情報
	if ( statvfs(dev,&fsbuf) != 0 ){ return -1; }
	*f_bsize=fsbuf.f_bsize;
	*f_blocks=fsbuf.f_blocks;
	//*f_use= fsbuf.f_blocks - fsbuf.f_bfree;
	//*f_free=fsbuf.f_bfree;
	*f_use= fsbuf.f_blocks - fsbuf.f_bavail;
	*f_free=fsbuf.f_bavail;

	/*
	printf("%s bs=%d blocks=%d %d %d %d %d\n",dev, fsbuf.f_bsize, fsbuf.f_blocks, 
			fsbuf.f_bavail, fsbuf.f_favail,
			fsbuf.f_bfree, fsbuf.f_ffree);
	*/

	return 0;
}

int perfGetDISKSTAT(char *dev, T_DISKSTAT *stat_a, T_DISKSTAT *stat_s){
	FILE *fp;
	char devname[1024];
	char buff[2048];
	char cols[3][1024];
	u_long rd_ios;					//読み込みが成功した回数
	u_long rd_merges;				//読み書きの I/O がそれぞれまとめて行なわれた回数
	u_long rd_sec;					//読み込みに成功したセクタ数
	u_long rd_ticks;				//読み込みにかかった時間 (ミリ秒)
	u_long wr_ios;					//書き込みが成功した回数
	u_long wr_merges;				//書き込みの I/O がそれぞれまとめて行なわれた回数
	u_long wr_sec;					//書き込みに成功したセクタ数
	u_long wr_ticks;				//書き込みにかかった時間 (ミリ秒)
	u_long run_ios;					//実行中の I/O リクエスト数
	u_long tot_ticks;				//I/O リクエストが存在した時間(ミリ秒)
	u_long rq_ticks;				//I/O 数 x 経過時間 = I/O のべ時間

	utlStringSep(dev," /",3,devname);	

	//ディスク稼動統計
	if ( (fp=fopen("/proc/diskstats","r")) == NULL ){
		return -1;
	}
	while( fgets(buff,sizeof(buff),fp) != NULL ){
		sscanf(buff,"%s %s %s %lu %lu %lu %lu %lu %lu %lu %u %u %u %u",
			cols[0],cols[1],cols[2],
			&rd_ios, &rd_merges, &rd_sec, &rd_ticks,
			&wr_ios, &wr_merges, &wr_sec, &wr_ticks, &run_ios, &tot_ticks, &rq_ticks);
		if ( strcmp(devname,cols[2]) == 0 ){ break; }
	}
	fclose(fp);
	//printf("[%lu %lu %lu] [%lu %lu %lu]\n",rd_ios, wr_ios, run_ios, rd_ticks, wr_ticks, tot_ticks);

	if ( stat_a->tm.tv_sec != 0 ){
		stat_s->tm.tv_sec = stat_a->tm.tv_sec;
		stat_s->tm.tv_usec = stat_a->tm.tv_usec;
		stat_s->rd_ios = rd_ios - stat_a->rd_ios;
		stat_s->wr_ios = wr_ios - stat_a->wr_ios;
		stat_s->run_ios = run_ios - stat_a->run_ios;
		stat_s->rd_ticks = rd_ticks - stat_a->rd_ticks;
		stat_s->wr_ticks = wr_ticks - stat_a->wr_ticks;
		stat_s->tot_ticks = tot_ticks - stat_a->tot_ticks;
		//printf("SA [%lu %lu %lu] [%lu %lu %lu]\n",stat_s->rd_ios, stat_s->wr_ios, stat_s->run_ios, stat_s->rd_ticks, stat_s->wr_ticks, stat_s->tot_ticks);
	}
	gettimeofday(&stat_a->tm,NULL);
	stat_a->rd_ios=rd_ios;
	stat_a->wr_ios=wr_ios;
	stat_a->run_ios=run_ios;
	stat_a->rd_ticks=rd_ticks;
	stat_a->wr_ticks=wr_ticks;
	stat_a->tot_ticks=tot_ticks;

	return 0;
}

