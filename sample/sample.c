#include "xi_client.h"

int file_put(char *in_file,char *out_file){
	int ifd,ofd,ret;
	char buff[1024];
	struct stat fbuff;

	if ( stat(in_file,&fbuff) != 0 ){
		printf("no file (%s)\n",in_file);
		exit(1);
	}

	if ( (ifd=open(in_file,O_RDONLY)) < 0 ){
		printf("open error\n");
		exit(1);
	}

	if ( (ofd=xc_open(out_file,XC_WRITE,fbuff.st_size)) < 0 ){
		printf("open error errno=%d [%s]\n",xc_errno,xc_message);
		exit(1);
	}

	while( (ret=read(ifd,buff,sizeof(buff))) > 0 ){
		printf("read()=%d\n",ret);
		ret=xc_write(ofd,buff,ret);	
		if ( ret < 0 ){
			printf("write error errno=%d [%s]\n",xc_errno,xc_message);
			break;
		}
	}

	xc_close(ofd);
	close(ifd);

	return 0;
}

int file_get(char *in_file,char *out_file){
	int ifd,ofd,ret;
	char buff[1024];
	struct stat fbuff;

	if ( stat(out_file,&fbuff) == 0 ){
		printf("file exist (%s)\n", out_file);
		exit(1);
	}

	if ( (ifd=xc_open(in_file,XC_READ)) < 0 ){
		printf("open error errno=%d [%s]\n",xc_errno,xc_message);
		exit(1);
	}

	if ( (ofd=open(out_file,O_CREAT|O_RDWR|O_TRUNC,0622)) < 0 ){
		printf("open error\n");
		exit(1);
	}

	while( 1 ){
		ret=xc_read(ifd,buff,sizeof(buff));
		printf("xc_read()=%d\n",ret);
		if ( ret <= 0 ){ break; }

		ret=write(ofd,buff,ret);	
		if ( ret < 0 ){
			printf("write error\n");
			break;
		}
	}

	close(ofd);
	xc_close(ifd);

	return 0;
}

main(int argc, char **argv){

	if ( argc != 4 ){
		printf("%s <get|put> <local file name> <remote file name>\n",argv[0]);
		exit(1);
	}

	if ( strcmp(argv[1],"put") == 0 ){
		file_put(argv[2],argv[3]);
	}else if ( strcmp(argv[1],"get") == 0 ){
		file_get(argv[2],argv[3]);
	}else{
		printf("%s <get|put> <local file name> <remote file name>\n",argv[0]);
		exit(1);
	}

	exit(0);
}

