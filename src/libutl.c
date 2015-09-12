/*****************************************************************************
*  <システム>   分散処理フレームワーク
*  <名称>	ユーティリティAPI
*  <目的>	
*  <機能>
*  <開発環境>	UNIX
*  <特記事項>
*
*  VERSION	  DATE		BY			   CHANGE/COMMENT
*  -----------------------------------------------------------------------------
*  V0.00	2014/01/18  Takakusaki	   新規作成
******************************************************************************/
#include "xi_common.h"

char *utlStrStr(char *moto, int pos, int len, char *out){
	out[0]=NULL;

	if( pos < 0 || len < 0 || len > strlen(moto) ){ return out; }

    for( moto += pos; *moto != NULL && len > 0; len-- ){
        *out++ = *moto++;
	}
    *out = NULL;

	return out;
}

void utlStrCut(char *moto, char *taisyo){
	int i,j,k, flg;

	for(i=0,j=0; moto[i]!=NULL; i++){
		flg=0;
		for(k=0; taisyo[k]!=NULL; k++){
			if ( moto[i] == taisyo[k] ){ flg=1; } 
		}
		if ( flg == 1 ){ continue; }
		
		moto[j++]=moto[i];	
		moto[j]=NULL;
	}
}

void utlStrCutRear(char *moto, int len){
	int i;
	int l=strlen(moto);
	int s=l-len;
	if ( s <= 0 ){ return; }

	for(i=0; moto[s]!=NULL; i++){
		moto[i]=moto[s++];
		moto[i+1]=NULL;
	}
}

void chomp(char *s){
	int i;
	for(i=0; ; i++){
		if ( s[i]==NULL || s[i]=='\r' || s[i]=='\n'){
			s[i]=(char)NULL; return; 
		}
	}
}

int utlName2Uid(char *nm){
	static char wk[64];
	struct passwd *pw;

	pw=getpwnam(nm);
	if ( pw == NULL ){
		//return atoi(nm);
		return -1;
	}else{
		return pw->pw_uid;
	}
}

int utlName2Gid(char *nm){
	static char wk[64];
	FILE *fp;
	char buff[2049];
	char uname[2048];
	char uid[2048];
	int  retid=(-2);

	if ( (fp=fopen("/etc/group","r")) == NULL ){ return -1;	}
	while( fgets(buff,sizeof(buff),fp) != NULL ){
		utlStringSep(buff,":",1,uname);
		utlStringSep(buff,":",3,uid);
		//printf("%s,%s\n",uname,uid);
		if ( strcmp(uname,nm) == 0 ){
			retid=atoi(uid);
			break;
		}
	}
	fclose(fp);

	return retid;
}

char *utlUid2Name(int id){
	static char wk[64];
	struct passwd *pwd;

	pwd=getpwuid(id);
	if ( pwd == NULL ){
		sprintf(wk, "%d",id); 
	}else{
		strcpy(wk,pwd->pw_name);
	}
	return wk;
}

char *utlGid2Name(int id){
	struct group *grp;
	static char wk[64];

	grp = getgrgid( id );	
	if ( grp == NULL ){
		sprintf(wk,"%d",id);
	}else{
		strcpy(wk,grp->gr_name);
	}
	return wk;
}

u_long utlCeil(u_long c1, u_long c2){
	double a = (double)c1;
	double b = (double)c2;
	return (u_long)ceil(a/b);
}

/*****************************************************************************
*  <関数名>	 utlCurNewLine
*  <機能>	   改行削除
*  <説明>	   文字列中の改行を削除する
*  <引数>	   str:I/O:文字列
*  <リターン値> なし
*  <備考>
******************************************************************************/
void utlCurNewLine(char *mbuf)
{
	int i;
	int mlen;

	/* 最終ＬＦを取る */
	mlen=strlen(mbuf);
	if ( mbuf[mlen-1] == 0x0A ) mbuf[mlen-1]=(char)NULL;

	/* CR/LFを取る */
	mlen=strlen(mbuf);
	if ( mbuf[mlen-2]==0x0D && mbuf[mlen-1]==0x0A ) mbuf[mlen-2]=(char)NULL;

}

/*****************************************************************************
*  <関数名>	 utlStrChangeSmall
*  <機能>	   小文字変換
*  <説明>	   大文字アルファベットを小文字に変換する
*  <引数>	   str:I/O:文字列
*  <リターン値> なし
*  <備考>
******************************************************************************/
void utlStrChangeSmall(char *str)
{
	int i;
	for (i=0; str[i]!=(char)NULL; i++){
		if ( str[i]>='A' && str[i]<='Z') str[i]=str[i]+0x20;
	}
}

/*****************************************************************************
*  <関数名>	 utlStrChangeBig
*  <機能>	   大文字変換
*  <説明>	   小文字アルファベットを大文字に変換する
*  <引数>	   str:I/O:文字列
*  <リターン値> なし
*  <備考>
******************************************************************************/
void utlStrChangeBig(char *str)
{
	int i;
	for (i=0; str[i]!=(char)NULL; i++){
		if ( str[i]>='a' && str[i]<='z') str[i]=str[i]-0x20;
	}
}

/*****************************************************************************
*  <関数名>	 utlStrExtracts
*  <機能>	   <>で括られた文字列を抜き出す。
*  <説明>	   ＜＞で括られた中身の文字列を抜き出して返す
*  <引数>	   str:I:入力文字列
*		resbuf:O:出力文字列
*  <リターン値> 出力文字列
*  <備考>
******************************************************************************/
char *utlStrExtracts(char *str, char *resbuf)
{
	int i,cnt;

	resbuf[0]=(char)NULL;	
	for (i=0,cnt=0; str[i]!=(char)NULL; i++){
		if ( str[i]==' ' ) continue;
		if ( str[i]=='<' ){ cnt=1; continue; }
		if ( str[i]=='>' ) break;
		if ( cnt != 0 ){
			resbuf[cnt-1]=str[i];
			resbuf[cnt]=(char)NULL;
			cnt++;
		}
	}
	return resbuf;
}

/*****************************************************************************
*  <関数名>	 utlStrExtractsPre
*  <機能>	   <>文字以前の文字列を抜き出す。
*  <説明>	   ＜＞以前にある文字列を抜き出して返す。
*  <引数>	   str:I:入力文字列
*		resbuf:O:出力文字列
*  <リターン値> 出力文字列
*  <備考>
******************************************************************************/
char *utlStrExtractsPre(char *str, char *resbuf)
{
	int i,cnt;

	resbuf[0]=(char)NULL;	
	for (i=0,cnt=0; str[i]!=(char)NULL; i++){
		if ( str[i]==' ' ) continue;
		if ( str[i]=='<' ) break;
		if ( str[i]=='>' ) break;
		resbuf[cnt++]=str[i];
	}
	resbuf[cnt]=(char)NULL;
	return resbuf;
}

/*****************************************************************************
*  <関数名>	 utlTargetWordCount
*  <機能> 	特定文字カウント
*  <説明>	   パラメータで指定された文字の出現数を返す
*  <引数>	   
*		str:I:入力文字列
*		sep:I:検索文字
*  <リターン値> 出力文字列
*  <備考>
******************************************************************************/
int utlTargetWordCount(char *str, char *sep){
	int i,j;
	int cnt=0;

	for ( i=0; str[i]!=(char)NULL; i++){
		for ( j=0; sep[j]!=(char)NULL; j++ ){
			if ( sep[j] == str[i] ){ cnt++; }
		}
	}
	return cnt;
}

/*****************************************************************************
*  <関数名>	 utlStringSep
*  <機能> 	文字列セパレート
*  <説明>	   セパレート文字列で区切られた項目を抜き出す。
*  <引数>	   str:I:入力文字列
*		sep:I:セパレート文字列
*		gc:I:何項目か
*		resbuf:O:出力文字列
*  <リターン値> 出力文字列
*  <備考>
******************************************************************************/
char *utlStringSep(char *str, char *sep, int gc, char *resbuf)
{
	int i,j;
	int flg;
	int cnt;
	int bufcnt;

	/* 初期化 */
	flg=0;
	cnt=0;
	bufcnt=0;
	resbuf[0]=(char)NULL;

	for ( i=0; str[i]!=(char)NULL; i++){
		/* セパレート文字か? */
		for ( j=0; str[i]!=sep[j] && sep[j]!=(char)NULL; j++ );

		/* セパレート文字以外 */
		if ( sep[j] == (char)NULL ){
			resbuf[bufcnt++]=str[i];
			resbuf[bufcnt]=(char)NULL;
			flg=1;
			continue;
		}

		/* セパレート文字 */
		if ( flg != (-1) ){
			cnt++;
			if ( cnt == gc ) return resbuf;
		}
		bufcnt=0;
		resbuf[bufcnt]=(char)NULL;
		flg=(-1);
	}
	cnt++;
	if ( gc != cnt ) resbuf[0]=(char)NULL;
	return resbuf;
}

/*****************************************************************************
*  <関数名>	 utlStringSep2
*  <機能> 	文字列セパレート
*  <説明>	   セパレート文字列で区切られた項目を抜き出す。
*  <引数>	   str:I:入力文字列
*		sep:I:セパレート文字列
*		gc:I:何項目か
*		resbuf:O:出力文字列
*  <リターン値> 出力文字列
*  <備考>	１つでもセパレート文字列が存在したら次項目へ
******************************************************************************/
char *utlStringSep2(char *str, char *sep, int gc, char *resbuf)
{
	int i,j;
	int cnt;
	int bufcnt;

	/* 初期化 */
	cnt=0;
	bufcnt=0;
	resbuf[0]=(char)NULL;

	for ( i=0; str[i]!=(char)NULL; i++){
		/* セパレート文字か? */
		for ( j=0; str[i]!=sep[j] && sep[j]!=(char)NULL; j++ );

		/* セパレート文字以外 */
		if ( sep[j] == (char)NULL ){
			resbuf[bufcnt++]=str[i];
			resbuf[bufcnt]=(char)NULL;
			continue;
		}

		/* セパレート文字 */
		cnt++;
		if ( gc == cnt ) return resbuf;
		bufcnt=0;
		resbuf[bufcnt]=(char)NULL;
	}
	cnt++;
	if ( gc != cnt ) resbuf[0]=(char)NULL;
	return resbuf;
}

/*****************************************************************************
*  <関数名>	 utlStringSep3
*  <機能> 	文字列セパレート
*  <説明>	   セパレート文字列で区切られた文字列のgc項目までの文字列を抜き出す
*  <引数>	   str:I:入力文字列
*		sep:I:セパレート文字列
*		gc:I:何項目か
*		resbuf:O:出力文字列
*  <リターン値> 出力文字列
*  <備考>	
******************************************************************************/
char *utlStringSep3(char *str, char *sep, int gc, char *resbuf)
{
	int i,j;
	int cnt;
	int bufcnt;

	/* 初期化 */
	cnt=0;
	bufcnt=0;
	resbuf[0]=(char)NULL;

	for ( i=0; str[i]!=(char)NULL; i++){
		/* セパレート文字か? */
		for ( j=0; str[i]!=sep[j] && sep[j]!=(char)NULL; j++ );

		/* セパレート文字 */
		if ( sep[j] != (char)NULL ){
			cnt++;
			if ( gc == cnt ) return resbuf;
		}

		resbuf[bufcnt++]=str[i];
		resbuf[bufcnt]=(char)NULL;

	}
	cnt++;
	if ( gc != cnt ) resbuf[0]=(char)NULL;
	return resbuf;
}

char *utlStringSepLast(char *str, char *sep, char *resbuf){
	int i,j;
	int bufcnt=0;
	int bufcnt_last=0;

	resbuf[0]=(char)NULL;

	for(i=0; str[i]!=NULL; i++){
		for ( j=0; str[i]!=sep[j] && sep[j]!=(char)NULL; j++ );
		if ( sep[j] != (char)NULL ){
			bufcnt_last=i+1;
			bufcnt=0;
			resbuf[0]=(char)NULL;
			continue;
		}
		resbuf[bufcnt++]=str[i];
		resbuf[bufcnt]=(char)NULL;
	}

	return str + bufcnt_last;
}

/*****************************************************************************
*  <関数名>	 utlDisplyIP
*  <機能>	   IPアドレス文字列変換
*  <説明>	   ４バイト文字列を１バイトずつのコロンで区切った文字列に変換する
*  <引数>	   ip:I:IPアドレス
*		resbuf:O:出力文字列
*  <リターン値> 出力文字列
*  <備考>
******************************************************************************/
char *utlDisplyIP(u_char *ip, char *resbuf)
{
	sprintf(resbuf,"%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
	return resbuf;
}

char *utlDisplyIP(u_char *ip)
{
	static char resbuf[128];
	sprintf(resbuf,"%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
	return resbuf;
}

//ホスト名からIPアドレスに変換する
u_long utlHostnameToAddress(char *ip){
	u_long addr=0;
	char ip_str[1024];
	
	//改行除去
	for(int i=0; ip[i]!=NULL; i++){
		if ( ip[i] == 0xa || ip[i] == 0xd ){ ip[i]=NULL; break; }
	}

	//IPアドレス変換
	if ( utlCheckIPAddress(ip) == 0 ){
		strcpy(ip_str, ip);
	}else{
		struct hostent *he;
		struct in_addr **addr_list;
		he = gethostbyname(ip);
		if( he != NULL ){
			addr_list = (struct in_addr **) he->h_addr_list;
			strcpy(ip_str, inet_ntoa(*addr_list[0]));
		}
	}

	addr=utlGetIPAddress(ip_str);
	//printf("[%s][%s]=%X\n",ip,ip_str,addr);
	return addr;
}

//パラメータの文字列がIPアドレスかチェックする
int utlCheckIPAddress(char *ip){
	int ret;
	ret=inet_addr(ip);
	if ( ret == INADDR_NONE ){ return -1; }
	return 0;
}

u_long utlGetIPAddress(char *ip){
	if ( utlCheckIPAddress(ip) == 0 ){
		return utlStrIP2long(ip);
	}else{
		return utlMyIpAddr(ip);
	}
}

/*****************************************************************************
*  <関数名>	 utlStrIP2long
*  <機能>	   IPアドレス文字列変換
*  <説明>	   コロンで区切ったIPをバイナリに変換する
*  <引数>	   
*		ip:I:IPアドレス文字列
*  <リターン値> 
*		IPアドレス
*  <備考>
******************************************************************************/
u_long utlStrIP2long(char *ip)
{
	struct in_addr inp;
	inet_aton(ip,&inp);
	return inp.s_addr;
}

/*****************************************************************************
*  <関数名>	 utlMyIpAddr
*  <機能>	   IPアドレス取得
*  <説明>	   パラメータで指定されたデバイス名のIPアドレスを取得する
*  <引数>	   
*		ip:I:デバイス名
*  <リターン値> 
*		IPアドレス
*  <備考>
******************************************************************************/
u_long utlMyIpAddr(char* device_name) {
	int ret;

	//指定したデバイス名のIPアドレスを取得します。
	int s = socket(AF_INET, SOCK_STREAM, 0);

	struct ifreq ifr;
	ifr.ifr_addr.sa_family = AF_INET;
	strcpy(ifr.ifr_name, device_name);
	ret=ioctl(s, SIOCGIFADDR, &ifr);
	close(s);
	if ( ret != 0 ){ return (u_int32_t)0; }

	struct sockaddr_in addr;
	memcpy( &addr, &ifr.ifr_ifru.ifru_addr, sizeof(struct sockaddr_in) );
	//return inet_ntoa(addr.sin_addr);
	return (u_int32_t)(addr.sin_addr.s_addr);
}

/*****************************************************************************
*  <関数名>	 utlGetSHA1
*  <機能>	 SHA1ハッシュ値取得
*  <説明>	 パラメータで指定された文字列のハッシュ値を取得し結果を返す
*  <引数>	   
*		ip:I:デバイス名
*  <リターン値> 
*		IPアドレス
*  <備考>
******************************************************************************/
void utlGetSHA1(char *in, unsigned char *out)
{
	char *type = "blob";
	int hdrlen;
	char hdr[256];
	unsigned long len;
	SHA_CTX c;

	len = strlen(in);
	sprintf(hdr, "%s %ld", type, len);
	hdrlen = strlen(hdr) + 1;

	SHA1_Init(&c);
	SHA1_Update(&c, hdr, hdrlen);
	SHA1_Update(&c, in, len);
	SHA1_Final(out, &c);
}

char *utlSHA1_to_HEX(unsigned char *sha1)
{
	int i,j;
	char w[256];
	static char out[256];
	out[0]=(char)NULL;

	for (i=0,j=0; i < 20; i++) {
		sprintf(w,"%x",sha1[i]);
		strcat(out,w);
	}
	
	return out;
}
