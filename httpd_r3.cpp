#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <signal.h>

#include "json/json.h"
#include "datools.h"
#include "SB_System.h"
#include "SB_Network.h"
#include "ConfigTextFile.h"
#include "multipart_parser.h"
#include "http_multipart.h"
#include "pai_r_data.h"
#include "pai_r_updatelog.h"
#include "httpd_pai_r.h"
#include "httpd_r3.h"



const char *g_app_name = "PAI_R_HTTPD";

#define	DBG_ERR_HTTPD		DBG_ERR
#define	DBG_WRN_HTTPD	DBG_WRN
#define	DBG_MSG_HTTPD		DBG_MSG


int test_function();

int lockfile(int fd)
{
       struct flock fl;


       fl.l_type = F_WRLCK;
       fl.l_start = 0;
       fl.l_whence = SEEK_SET;
       fl.l_len = 0;
       return(fcntl(fd, F_SETLK, &fl));
}


int check_single_running(char *pos, char *progm)
{
       int fd;
       char PROCESS_ID[16];
       char LOCK_FILE_PATH[128];
       char PROCESS_NAME[32];


       memset(PROCESS_ID, 0x00, sizeof(PROCESS_ID));
       memset(PROCESS_NAME, 0x00, sizeof(PROCESS_NAME));
       memset(LOCK_FILE_PATH, 0x00, sizeof(LOCK_FILE_PATH));


       sprintf(LOCK_FILE_PATH, "%s/%s.pid", "/tmp", strrchr(pos, '/'));
		printf( "Debug LOCK_FILE_PATH:[%s]\n", LOCK_FILE_PATH);

		fd = open(LOCK_FILE_PATH, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		if (fd < 0) {
			printf( "Can't Open:[%s]/[%s]\n", LOCK_FILE_PATH, strerror(errno));
			exit(1);
		}


       if(lockfile(fd) < 0){
               if(errno == EACCES || errno == EAGAIN){
                       close(fd);
                       printf( "Already Running:[%s]/[%s]\n", LOCK_FILE_PATH, strerror(errno));
                       return (1);
               }
               printf("Can't Lock:[%s]/[%s]\n", LOCK_FILE_PATH, strerror(errno));
               exit(1);
       }


       ftruncate(fd, 0);
       sprintf(PROCESS_ID, "%d", getpid());
       write(fd, PROCESS_ID, strlen(PROCESS_ID)+1);
       return (0);
}

void _sig_handler(int sig)
{
	if (sig == SIGTERM) {
		g_httpdExitNow = true;
	}
}

int main(int argc, char **argv)
{
	int appnum;
	int ret = 0;
	int delay_time;

	if(test_function())
		return 0;
	
	if(argc < 2){
		printf( "USAGE : %s <checking time sec> <port>", argv[0]);
		exit(0);
	}

	printf( "%s : START(ver : %s [%s %s])\r\n", argv[0], HTTPD_R3_VERSION, __DATE__, __TIME__);

	printf( "################### Ryunni build httpd_r3 ####################\n");

	delay_time = atoi(argv[1]);
	if(delay_time == 0)
		delay_time = 1;
	
	ret = check_single_running(argv[0], argv[0]);
	if(ret !=0){
		printf( "duplicate Running!!");
		exit(0);
	}else{
		printf( "Single Running!!");
	}

	printf("%s START\n", argv[0]);

	signal(SIGTERM, _sig_handler);

	int port = 80;
	if(argc > 2)
		port = atoi(argv[2]);

	httpd_start(port);
	
	while (!g_httpdExitNow) {
		sleep(delay_time);
		
		httpd_main_work();		
		
		datool_is_appexe_stop("ps -e | grep -c \"[r]ecorder\"", 0, &appnum);

		//printf("appnum=%d\n", appnum);

		if (appnum == 0) {
			if(delay_time < 5)
				g_httpdExitNow = true;
		}
		else {
			// if app has been running
			
		}
	}

	httpd_end();
	printf("%s : END\n\n", argv[0]);

	return 1;
}

int test_function()
{
#if 0
	printf("%d \r\n", sizeof(PAI_R_UPDATELOG));
	return 1;
#endif

#if 0
	CPai_r_data pai_r_data;

	const char * host = "192.168.35.19";
	int host_port = 4000;
	httpd_start(0);
	
	for(int i = 0; i < 1000; i++) {
		char result[128];
		std::string file_path("/mnt/extsd/NORMAL/20191014_143828_I1.avi");
		
		PAI_R_LOCATION loc;
		const char * file = strrchr(file_path.c_str(), '/');
  		file++;
			
		sprintf(loc.file_type, "%c%d", 'I', 1);
		loc.create_time = recording_time_string_to_time(file);
		loc.create_time_ms = (get_tick_count()%1000);
		loc.move_filesize = 16*1024*1024; //BYTE
#if 0
		loc.latitude = gps.m_fLat;
		loc.longitude = gps.m_fLng;
		loc.accurate = (u16)gps.m_fPdop;
		loc.direction = (u16)gps.m_nCog/100;
		loc.altitude= (u16)gps.m_fAltitude;
#else
		loc.latitude = 37.233;
		loc.longitude = 127.235;
		loc.accurate = 10;
		loc.direction = 180;
		loc.altitude= 100;
#endif
		pai_r_data.addLocation(loc, 0);
		for(int i=0; i < 100; i++){
			PAI_R_SPEED_CSV spd= {time(0), get_tick_count(), 10 * i, 100 * i };
			pai_r_data.addSpeed(spd);
		}

		LOCATION_INFO info;
		if(pai_r_data.getLastLocation_json(info, 0)){
			http_pai_r_insert_list_and_request2(host, host_port, 0, info.create_time, (const char *)info.file_type.c_str(), "2", (const char*)info.loc.c_str(), (const char*)info.speed.c_str());
		}
				
		SB_Cat("grep \"[M]emFree\" /proc/meminfo", result, sizeof(result));
		
		printf("%s\r\n", result);
	}
	return 1;
#endif
#if 0
	const char * file = strrchr("/mnt/extsd/NORMAL/20001114_162711_I2.avi", '/');
  file++;
	time_t t= recording_time_string_to_time(file);
	
	printf("%s %d \r\n%s\r\n", file, t, make_recording_time_string(t).c_str());
	return 1;
#endif
#if 0
	const char * url2;
	const char * url = "http://mar-i.com:80/driverecorder/api_driverecorder/firstview/1/insert_list_and_request.php";
	url2 = strstr( url, "/driverecorder");
	printf(url2);
	return 1;
#endif
#if 0
	static int msg_q_loc_id = 0;
	static int msg_q_spd_id = 0;

	PAI_R_LOCATION loc;
	PAI_R_SPEED_CSV spd;
	
	if(msg_q_loc_id == 0)
		msg_q_loc_id = datool_ipc_msgget(MSGQ_PAIR_LOC_IDKEY);

	if(msg_q_spd_id == 0)
		msg_q_spd_id = datool_ipc_msgget(MSGQ_PAIR_SPD_IDKEY);

	static CPai_r_data pai_r_daata;

	strcpy(loc.file_type, "I2");
	loc.create_time = time(0);
	loc.move_filesize = 12345;
	loc.latitude = 37.233;
	loc.longitude = 127.235;
	loc.accurate = 10;
	loc.direction = 180;
	loc.altitude= 100;
	
	datool_ipc_msgsnd(msg_q_loc_id, (void *)&loc, sizeof(loc));

	for(int i=0; i < 1000; i++){
		PAI_R_SPEED_CSV spd= {time(0), get_tick_count(), 10 * i, 100 * i };
		datool_ipc_msgsnd(msg_q_spd_id, (void *)&spd, sizeof(spd));
	}

	memset((void *)&loc, 0 , sizeof(loc));
	memset((void *)&spd, 0 , sizeof(spd));
	
	if(datool_ipc_msgrcv(msg_q_loc_id, (void *)&loc, sizeof(loc)) != -1)
	{
		pai_r_daata.addLocation(loc);
	}
	
	while(datool_ipc_msgrcv(msg_q_spd_id, (void *)&spd, sizeof(spd))!= -1)
	{
		pai_r_daata.addSpeed(spd);
	}
	
	std::string location, speed, file_type;
	pai_r_daata.getLastLocation_json( location, speed, file_type);
	return 1;
#endif
#if 0
	static CPai_r_data pai_r_daata;
	
	PAI_R_LOCATION loc;
	loc.create_time = time(0);
	loc.move_filesize = 12345;
	loc.latitude = 37.233;
	loc.longitude = 127.235;
	loc.accurate = 10;
	loc.direction = 180;
	loc.altitude= 100;
	
	pai_r_daata.m_auto_id = "abcdef";

	pai_r_daata.addLocation(loc);
	
	for(int i=0; i < 10; i++){
		PAI_R_SPEED_CSV spd= {time(0), get_tick_count(), 10 * i, 100 * i };
		pai_r_daata.addSpeed(spd);
	}
	std::string location, speed;
	pai_r_daata.getLastLocation_json( location, speed);
	return 1;
#endif

#if 0
		syslog_printf(g_app_name, 1, "%s\r\n", datool_generateUUID().c_str());
		return 1;
#endif
	return 0;
}
