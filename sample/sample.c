#include "xi_client.h"

int file_put(char *in_file,char *out_file){
	int ifd,ofd,ret;
	char buff[1024];

	if ( (ifd=open(in_file,O_RDONLY)) < 0 ){
		printf("open error\n");
		exit(1);
	}

	if ( (ofd=xi_open(out_file,XI_WRITE)) < 0 ){
		printf("open error errno=%d [%s]\n",xi_errno,xi_message);
		exit(1);
	}

	while( (ret=read(ifd,buff,sizeof(buff))) > 0 ){
		printf("read()=%d\n",ret);
		ret=xi_write(ofd,buff,ret);	
		if ( ret < 0 ){
			printf("write error errno=%d [%s]\n",xi_errno,xi_message);
			break;
		}
	}

	xi_close(ofd);
	close(ifd);

	return 0;
}

int file_get(char *in_file,char *out_file){
	int ifd,ofd,ret;
	char buff[1024];

	if ( (ifd=xi_open(in_file,XI_READ)) < 0 ){
		printf("open error errno=%d [%s]\n",xi_errno,xi_message);
		exit(1);
	}

	if ( (ofd=open(out_file,O_CREAT|O_RDWR|O_TRUNC,0622)) < 0 ){
		printf("open error\n");
		exit(1);
	}

	while( 1 ){
		ret=xi_read(ifd,buff,sizeof(buff));
		printf("xi_read()=%d\n",ret);
		if ( ret <= 0 ){ break; }

		ret=write(ofd,buff,ret);	
		if ( ret < 0 ){
			printf("write error\n");
			break;
		}
	}

	close(ofd);
	xi_close(ifd);

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

