
#########################
#パス定義
#########################
GCC=/usr/bin/g++
AR=/usr/bin/ar

BASE_DIR=/usr/local/xicluster
SRC_DIR=$(BASE_DIR)/src
BIN_DIR=$(BASE_DIR)/bin
LIB_DIR=$(BASE_DIR)/lib
PKG_DIR=$(BASE_DIR)/pkg
TMP_DIR=$(BASE_DIR)/temp

#########################
#フラグ
#########################
CFLAGS=-c -w -fPIC
LFLAGS=-shared -fPIC 
LINKS=-lssl -lz -L./

#########################
#ファイル定義
#########################
HDF=xi_common.h xi_server.h xi_client.h
LINKSRC1=libutl.c \
libtm.c \
libtcp.c \
libudp.c \
libipc.c \
libsem.c \
libshm.c \
libmqu.c \
libpipe.c \
libmmap.c \
libfil.c \
libud.c \
libperf.c \
libdl.c \
libpara.c \
libcomm.c \
libztrans.c

LINKSRC2=svr_daemon.c \
svr_cache.c \
svr_disp.c \
svr_diskreq.c \
svr_disknm.c \
svr_diskrd.c \
svr_diskwt.c \
svr_disksync.c \
svr_diskrep.c \
svr_log.c \
svr_hlog.c \
svr_trans.c

LINKSRC3=xi_client.c

SRC1=xicluster_server.c xi_server.c

SRC2=xicluster_daemon.c \
process_clireq.c \
process_sman.c \
process_nlsr_tcp.c \
process_nlsr_udp.c \
process_clsr.c \
process_cach.c \
process_crcv.c \
process_csnd.c \
process_disk.c \
process_task.c \
process_msyn.c \
process_dsyn.c \
process_dmnt.c

SRC3=xicluster_client.c

#Object Files
LINKOBJ1=$(LINKSRC1:%.c=%.o)
LINKOBJ2=$(LINKSRC2:%.c=%.o)
LINKOBJ3=$(LINKSRC3:%.c=%.o)
OBJ1=$(SRC1:%.c=%.o)
OBJ2=$(SRC2:%.c=%.o)
OBJ3=$(SRC3:%.c=%.o)

LINKFILE1=libxicluster_common.so
LINKFILE2=libxicluster_server.so
LINKFILE3=libxicluster_client.so
TARGETFILE1=xicluster_server
TARGETFILE2=xicluster_daemon
TARGETFILE3=xicluster_client

#Out Files
LINKOUT1=./$(LINKFILE1)
LINKOUT2=./$(LINKFILE2)
LINKOUT3=./$(LINKFILE3)
TARGETOUT1=./$(TARGETFILE1)
TARGETOUT2=./$(TARGETFILE2)
TARGETOUT3=./$(TARGETFILE3)

TARGETS=$(LINKOUT1) $(LINKOUT2) $(LINKOUT3) $(TARGETOUT1) $(TARGETOUT2) $(TARGETOUT3)

####################################################################
all:$(TARGETS) 

.c.o:$(HDF) $(LINKSRC1) $(LINKSRC2) $(LINKSRC3)
	$(GCC) $(CFLAGS) $*.c

$(LINKOUT1):$(HDF) $(LINKOBJ1)
	$(GCC) $(LFLAGS) -o $(LINKOUT1) $(LINKOBJ1)

$(LINKOUT2):$(HDF) $(LINKOBJ2)
	$(GCC) $(LFLAGS) -o $(LINKOUT2) $(LINKOBJ2)

$(LINKOUT3):$(HDF) $(LINKOBJ3)
	$(GCC) $(LFLAGS) -o $(LINKOUT3) $(LINKOBJ3)

$(TARGETOUT1): $(LINKOUT1) $(OBJ1) $(HDF)
	$(GCC) -o $(TARGETOUT1) $(LINKS) -lxicluster_common -lxicluster_server $(OBJ1)

$(TARGETOUT2): $(LINKOUT1) $(OBJ2) $(HDF)
	$(GCC) -o $(TARGETOUT2) $(LINKS) -lxicluster_common -lxicluster_server $(OBJ2)

$(TARGETOUT3): $(LINKOUT1) $(OBJ3) $(HDF)
	$(GCC) -o $(TARGETOUT3) $(LINKS) -lxicluster_common -lxicluster_client $(OBJ3)

install:
	xicluster_server stop
	cp $(LINKOUT3)   $(LIB_DIR)
	cp $(LINKOUT2)   $(LIB_DIR)
	cp $(LINKOUT1)   $(LIB_DIR)
	cp $(TARGETOUT3) $(BIN_DIR)
	cp $(TARGETOUT2) $(BIN_DIR)
	cp $(TARGETOUT1) $(BIN_DIR)

clean:
	rm -f *.o $(TARGETS) core

pkg:
	echo "%_topdir $(BASE_DIR)/pkg" > ~/.rpmmacros
	tar cvfz $(PKG_DIR)/SOURCES/xicluster-0.0.tar.gz -C /usr/local \
		xicluster/src/server.lst \
		xicluster/src/xicluster.conf \
		xicluster/src/xicluster.init \
		xicluster/bin/$(TARGETFILE1) xicluster/bin/$(TARGETFILE2) xicluster/bin/$(TARGETFILE3) \
		xicluster/lib/$(LINKFILE1) xicluster/lib/$(LINKFILE2) xicluster/lib/$(LINKFILE3)

	rpmbuild -bb $(PKG_DIR)/SPECS/xicluster-0.0.spec

