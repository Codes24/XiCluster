
#########################
#パス定義
#########################
GCC=/usr/bin/g++
INC_DIR=/usr/local/cdoop/src
LIB_DIR=/usr/local/cdoop/lib

#########################
#フラグ
#########################
CFLAGS=-c -g -w -I$(INC_DIR)
LINKS=-lssl -lz -L$(LIB_DIR) 

#########################
#ファイル定義
#########################
SRC1=sample.c
OBJ1=$(SRC1:%.c=%.o)
TARGETOUT1=./sample

####################################################################
all:$(TARGETOUT1)

.c.o:
	$(GCC) $(CFLAGS) $*.c

$(TARGETOUT1): $(OBJ1)
	$(GCC) -o $(TARGETOUT1) $(LINKS) -lcdoop_common -lcdoop_client $(OBJ1)

clean:
	rm -f *.o $(TARGETOUT1) core

