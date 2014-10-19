#API一覧

[int xi_open(char *path, int mod);](https://github.com/takakusaki/XiCluster/blob/master/doc/API_open.md)  
int xi_read(int fd, char *buff, int size);  
int xi_write(int fd, char *buff, int size);  
int xi_close(int fd);  
int xi_stat(char *path, struct xi_stat *buf);  
int xi_mkdir(char *path);  
int xi_rmdir(char *path);  
int xi_rename(char *path1, char *path2);  
int xi_unlink(char *path);  
int xi_chmod(int mode, char *path);  
int xi_chown(int uid, char *path);  
int xi_chgrp(int gid, char *path);  


