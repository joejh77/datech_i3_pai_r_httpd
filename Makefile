#soft-fp version
CXX=/home/${USER}/workspace/project/oasis/buildroot-oasis/output/firstview_nor/host/bin/arm-linux-gnueabi-g++
STRIP=/home/${USER}/workspace/project/oasis/buildroot-oasis/output/firstview_nor/host/bin/arm-linux-gnueabi-strip
#CXX=/home/${USER}/workspace/project/Oasis/tools/gcc-linaro-4.9.4-2017.01-x86_64_arm-linux-gnueabi/bin/arm-linux-gnueabi-g++
#STRIP=/home/${USER}/workspace/Oasis/tools/gcc-linaro-4.9.4-2017.01-x86_64_arm-linux-gnueabi/bin/arm-linux-gnueabi-strip
CXXFLAGS=-Wall -std=c++11 -march=armv7-a -mfloat-abi=softfp -mfpu=neon -DNDEBUG -fPIC -DZIP_STD

#hard-fp version
#CXX=/home/${USER}/workspace/Oasis/tools/gcc-linaro-4.9.4-2017.01-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-g++
#STRIP=CXX=/home/${USER}/workspace/Oasis/tools/gcc-linaro-4.9.4-2017.01-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-strip
#CXXFLAGS=-g -Wall -std=c++11 -march=armv7-a -mfloat-abi=hard -mfpu=neon -DNDEBUG -fPIC

CXXFLAGS+=-O0 -fstack-protector-strong
#CXXFLAGS+=-O3

APP_SRC = ../datech_i3_app/


INCLUDES=-I./include -I$(APP_SRC)arm/include -I$(APP_SRC)include -I$(APP_SRC)include/iio -I$(APP_SRC)/lib/tinyxml -I./lib/libhttp-1.8/include -I./lib/zip_utils
LDFLAGS=-L$(APP_SRC)arm/lib -L$(APP_SRC)lib/tinyxml -L./lib/libhttp-1.8

LDLIBS=-lpthread -ljsoncpp -lfreetype -lz -levdev -ltixml -lcivetweb -ldl

#non-gprof
#LDLIBS+=-loasis -lcdc_base -lcdc_vencoder -lcdc_ve -lcdc_memory -lcdc_vdecoder
#CXXFLAGS+=-DHTTPD_EMBEDDED
CXXFLAGS+=-DHTTPD_MSGQ_SNAPSHOT

#gprof
#LDLIBS+=-loasis-static -loffs-direct
#CXXFLAGS+=-pg

#LDLIBS += -lAdas
#LDLIBS += -Wl,--no-as-needed -ltcmalloc_debug

#CXXFLAGS+=-fno-omit-frame-pointer -fsanitize=address
#CXXFLAGS+=-static-libasan  -static-libstdc++
#LDLIBS += -lasan

#CXXFLAGS+=-fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free
#CXXFLAGS+=-fno-omit-frame-pointer
#LDLIBS += -Wl,--no-as-needed -ltcmalloc

#LDLIBS += libtcmalloc.a

#LDLIBS += -lprofiler

BUILD_DIR=./build/

.PHONY: clean
all: clean httpd_r3

stripped: httpd_r3
	$(STRIP) --strip-unneeded httpd_r3

httpd_r3: httpd_r3.cpp Makefile
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c httpd_r3.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c tcp_mmb_r.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c tcp_client.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c pai_r_updatelog.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c http.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c httpd_pai_r.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c http_multipart.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c multipart_parser.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c base64.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $(APP_SRC)pai_r_data.cpp	
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $(APP_SRC)system_log.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $(APP_SRC)ConfigTextFile.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $(APP_SRC)SB_Network.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $(APP_SRC)SB_System.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $(APP_SRC)sysfs.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $(APP_SRC)datools.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c ./lib/zip_utils/zip.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c ./lib/zip_utils/unzip.cpp
	
	$(CXX) $(CXXFLAGS) $(INCLUDES) *.o $(LDFLAGS) $(LDLIBS) -o $@

#	../../etc/file_extend httpd_r3
#	cp httpd_r3 ../buildroot-oasis/board/datech/oasis_nor/userfs/bin

	cp httpd_r3 ../buildroot-oasis/board/datech/oasis_nor/rootfs_overlay/datech/app/
	
	date > ../buildroot-oasis/board/datech/oasis_nor/userfs/reboot.txt
clean:
	$(RM) *.o httpd_r3
 
