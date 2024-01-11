/* Copyright (c) 2013-2014 the Civetweb developers
 * Copyright (c) 2013 No Face Press, LLC
 * License http://opensource.org/licenses/mit-license.php MIT License
 */

// Simple example program on how to use Embedded C++ interface.
#include <stdio.h>
#include <string.h>
 #include <fcntl.h>
 #include <stdarg.h>
#include "sysfs.h"

#include "ConfigTextFile.h"
#include "mixwav.h"
#include "SB_System.h"
#include "SB_Network.h"
#include "CivetServer.h"
#include "multipart_parser.h"
#include "http.h"

#include "http_multipart.h"
#include "system_log.h"
#include "pai_r_data.h"
#include "pai_r_updatelog.h"
#include "pai_r_error_list.h"
//#include "OasisAPI.h"
#include "TCPClient.h"
#include "tcp_client.h"
#include "httpd_pai_r.h"
#include "tcp_mmb_r.h"

#ifdef DEF_MMB_VER_2
#include <iostream>
#include <vector>
#include <dirent.h>
#endif

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif


#define TCPD_INFO			1
#define TCPD_ERROR		1
#define TCPD_WARN		1

#define DOCUMENT_ROOT "/mnt/extsd/"
#define DEF_DEVICETYPE 'F'

#define DEF_MMB_ENDIAN		1 // 0 is little, 1 is bic

#ifdef DEF_MMB_SERVER
TCPClient l_mmb;
CTcpClient l_Tcp;

#ifdef DEF_MMB_VER_2
CTcpClient l_PollingTcp;

#define VIDEO_DIR	"/NORMAL/"

struct filelist {
	unsigned short length;
	char filepath[256];
};
vector<filelist> files;
unsigned short file_count;
uint32_t filelist_dataSize;

#endif

 void _mmb_send_queue_clear(CPai_r_data &pai_r_data, eUserDataType ud_type)
{
	if(pai_r_data.m_tmp_pop_count) {
		pai_r_data.Location_queue_clear(pai_r_data.m_tmp_pop_count, ud_type);
		pai_r_data.m_tmp_pop_count = 0;
	}
	else if(pai_r.request_list_unique_autoid.size()){
		pai_r.request_list_unique_autoid.pop_front();
		httpd_cfg.server_wait_success_movie = 0;
	}	
	else if(pai_r.request_list_temp.size()){
#ifdef MSGQ_PAIR_LOC_Q_OUT_IDKEY		
		ST_QMSG msg = {QMSG_UP_TIME, eUserDataType_Normal, pai_r.request_list_temp.front() , (u32)time(0), 0};
		datool_ipc_msgsnd(m_http_msg_q_out_id, (void *)&msg, sizeof(msg));
		pai_r_data.Location_queue_clear(1, eUserDataType_Event);
#endif
		pai_r.request_list_temp.pop_front();
		httpd_cfg.server_wait_success_movie = 0;
	}
	
#ifdef MSGQ_PAIR_LOC_Q_OUT_IDKEY
	datool_ipc_msgsnd3(m_http_msg_q_out_id, (long)QMSG_OUT, pai_r_data.Location_queue_auto_id_out_get(eUserDataType_Normal), pai_r_data.Location_queue_auto_id_out_get(eUserDataType_Event));
#endif	
}

void _mmbSetEventData(time_t t_event, unsigned short eventTimeMs, unsigned char eventType, unsigned char gnssStatus, double eventLongitude, double eventLatitude, bool bicEndian)
{
#ifdef DEF_MMB_VER_2
	struct tm tmThis;
	char eventTime[17] =  { 0,};
		
	localtime_r(&t_event, &tmThis);
	sprintf(eventTime, "%04d%02d%02d%02d%02d%02d%03d", \
		tmThis.tm_year + 1900, tmThis.tm_mon + 1, tmThis.tm_mday, \
		tmThis.tm_hour, tmThis.tm_min, tmThis.tm_sec, eventTimeMs);
// 서버 GPS 좌표와 오차 발생 : HH.HHHHHH => HH.MMMMMM 형태로 전송(x 0.6)_230821 Joe
	int degrees = (int) eventLongitude;
	double minutes = ((double) eventLongitude - degrees) * 0.6;
	eventLongitude = degrees + minutes;

	int degrees1 = (int) eventLatitude;
	double minutes1 = ((double) eventLatitude - degrees1) * 0.6;
	eventLatitude = degrees1 + minutes1;

	//printf("Latitude : %0.6f, Longitude : %0.6f\r\n", eventLatitude, eventLongitude);
////////////////////////////////////////////////////////////////////////////		
	l_mmb.setEventData(eventTime, eventType, gnssStatus, (int)(eventLongitude * 1000000), (int)(eventLatitude * 1000000), bicEndian);
	//l_mmb.setEventData(eventTime, eventType, gnssStatus, (int)(eventLongitude * 10000.0), (int)(eventLatitude * 10000.0), bicEndian);
#else
	time_t     t_rtc;
	struct tm tmThis;
	char accOnTime[14] =  { 0,};
	char eventTime[17] =  { 0,};
	
  	time(&t_rtc);
	t_rtc -= (get_tick_count() / 1000); //get acc on time
	localtime_r(&t_rtc, &tmThis);
	sprintf(accOnTime, "%04d%02d%02d%02d%02d%02d", \
		tmThis.tm_year + 1900, tmThis.tm_mon + 1, tmThis.tm_mday, \
		tmThis.tm_hour, tmThis.tm_min, tmThis.tm_sec);


	localtime_r(&t_event, &tmThis);
	sprintf(eventTime, "%04d%02d%02d%02d%02d%02d%03d", \
		tmThis.tm_year + 1900, tmThis.tm_mon + 1, tmThis.tm_mday, \
		tmThis.tm_hour, tmThis.tm_min, tmThis.tm_sec, eventTimeMs);

	l_mmb.setEventData(accOnTime, eventTime, eventType, gnssStatus, (int)(eventLongitude * 1000000), (int)(eventLatitude * 1000000), bicEndian);
	//l_mmb.setEventData(accOnTime, eventTime, eventType, gnssStatus, (int)(eventLongitude * 10000.0), (int)(eventLatitude * 10000.0), bicEndian);
#endif
}

int _mmbUploadResponseFunc(unsigned char *data, int length)
{
	httpd_cfg.wait_ack = 0;
	if(l_mmb.analyzeUploadAckData(data, length) != MMB_ACK){
		char msg[128];
		sprintf(msg, "Length : %d - %0x %02x %02x", length, data[0], data[1], data[2]); 
		pai_r.updatelog.save(UL_RESULT_TYPE_API_ERROR, "", "", msg);
		return -1;
	}
	
	pai_r.updatelog.save(UL_RESULT_TYPE_SUCCESS, "", "", "ACK");
	httpd_cfg.server_wait_success = 0;	
	httpd_cfg.time_interval = DEF_SERVER_POLLING_INTERVAL;

#ifdef MSGQ_PAIR_LOC_Q_IN_IDKEY
	if(httpd_cfg.server_connected == 0){			
		datool_ipc_msgsnd2(m_http_msg_q_out_id, (long)QMSG_SOUND, kMixwaveDingdong); // kMixwaveDingdong;
		datool_ipc_msgsnd2(m_http_msg_q_out_id, (long)QMSG_LED, WIFI_LED_SERVER_OK);		
		httpd_cfg.server_connected = 1;	
	}
#endif

	return 0;
}

int _mmbPollingResponseFunc(unsigned char *data, int length)
{
	int ack = l_mmb.analyzePollingAckData(data, length);

	httpd_cfg.wait_ack = 0;
#ifdef DEF_MMB_VER_2
	if(ack == MMB_ERR || ack > MMB_NOTHING )
#else	
	if(ack != MMB_REQUEST_FILE && ack != MMB_NOTHING)
#endif
	{
		char msg[128];
		sprintf(msg, "Request frame Error!(%02x %02x : %d)", data[0], data[1], length);
		
		pai_r.updatelog.save(UL_RESULT_TYPE_API_ERROR, "", "", msg);
		return -1;
	}
	
	pai_r.updatelog.save(UL_RESULT_TYPE_SUCCESS, "", "", ack == MMB_REQUEST_FILE ? "Rquest File" : "Nothing");
	httpd_cfg.server_wait_success = 0;		
		
	httpd_cfg.time_interval = DEF_SERVER_POLLING_INTERVAL;

#ifdef MSGQ_PAIR_LOC_Q_IN_IDKEY
	if(httpd_cfg.server_connected == 0){			
		datool_ipc_msgsnd2(m_http_msg_q_out_id, (long)QMSG_SOUND, kMixwaveDingdong); // kMixwaveDingdong;
		datool_ipc_msgsnd2(m_http_msg_q_out_id, (long)QMSG_LED, WIFI_LED_SERVER_OK);		
		httpd_cfg.server_connected = 1;	
	}
#endif

	return 0;
}

#ifdef DEF_MMB_VER_2
int _mmbCreateFileList()
{
	files.clear();
	file_count = 0;
	filelist_dataSize = 0;

	// 상시녹화 파일 목록 구성
	// number of files 2byte + file path length 2byte + file path + ... +  file path length 2byte + file path 로 구성하여 전송
	DIR *dir; 
	struct dirent *diread;
	
	filelist file_item;

	dprintf(TCPD_INFO, "%s() \n", __func__);

	if ((dir = opendir(VIDEO_NORMAL_DIR)) != nullptr) {
		while ((diread = readdir(dir)) != nullptr) {
			file_item.length = strlen(diread->d_name);
			if(file_item.length < 3)
				continue;
			
			strcpy(file_item.filepath, VIDEO_DIR);
			strcat(file_item.filepath, diread->d_name);
			file_item.length = strlen(file_item.filepath);

			filelist_dataSize += sizeof(file_item.length);
			filelist_dataSize += file_item.length;

			//printf("length = %d, ", file_item.length);
			//printf("path : %s ", file_item.filepath);
			//printf("filelist_dataSize : %d\r\n", filelist_dataSize);
			files.push_back(file_item);
			file_count++;
		}
		closedir (dir);
	} else {
		dprintf(TCPD_ERROR, "%s() : ERROR \n", __func__);
		return EXIT_FAILURE;
	}
	return 0;
}

int __mmbKeepRequestFilePath(unsigned char* buf, int length)
{
	unsigned char* data;
	unsigned char imei_len = *(buf + 2);
	int32_t data_len;
	unsigned short i;

	filelist file_item;

	file_count = 0;
	files.clear();

	data = buf + 3 + imei_len;

	data_len =  (int32_t)(0xff & *data) << 24 | 
				(int32_t)(0xff & *(data+1)) << 16 |
				(int32_t)(0xff & *(data+2)) << 8 |
				(int32_t)(0xff & *(data+3)) << 0;
	data += 4;
	dprintf(TCPD_INFO, "%s() : data_len = %d \n", __func__, data_len);
	
	file_count = (unsigned short)(0xff & *data) << 8 | (unsigned short)(0xff & *(data+1)) << 0;
	data += 2;
	dprintf(TCPD_INFO, "%s() : file_count = %d \n", __func__, file_count);

	for(i=0; i<file_count; i++) {
		file_item.length = 0;
		memset(file_item.filepath, 0x00, sizeof(file_item.filepath));

		file_item.length = (unsigned short)(0xff & *data) << 8 | (unsigned short)(0xff & *(data+1)) << 0;
		data += 2;
		strncpy(file_item.filepath, (char*)data, file_item.length);
		data += file_item.length;

		dprintf(TCPD_INFO, "%s() : %dth filepath_len = %d, filepath = %s \n", __func__, i+1, file_item.length, file_item.filepath);

		files.push_back(file_item);			
	}
	return 0;
}

int _mmbRequestFrameFunc(unsigned char* data, int length)
{
	int ack = l_mmb.analyzeRequestFrameData(data, length);

	if(ack == MMB_REQUEST_FILELIST) {
		if(_mmbCreateFileList() != 0) {
			l_mmb.mmb_ack = MMB_NAK;
			return -1;
		}
		
	}else if (ack == MMB_REQUEST_FILE) {
		/* 요청받은 파일 패스 저장
		number of files 2byte + file path length 2byte + file path + ... +  file path length 2byte + file path 로 수신됨
		각각의 파일에 대하여 파일패스를 저장하여 업로드 하자.
		*/
		if(__mmbKeepRequestFilePath(data, length) != 0) {
			l_mmb.mmb_ack = MMB_NAK;
			return -1;
		}
	} else {
		char msg[128];
		sprintf(msg, "_mmbRequestFrameFunc Error!(%02x, %02x : %d)", data[0], data[1], length);
		pai_r.updatelog.save(UL_RESULT_TYPE_API_ERROR, "", "", msg);
		l_mmb.mmb_ack = MMB_NAK;
		return -1;
	}

	pai_r.updatelog.save(UL_RESULT_TYPE_SUCCESS, "", "", ack == MMB_REQUEST_FILE ? "Request File" : "Request FileList");

	httpd_cfg.server_wait_success = 0;	
	httpd_cfg.time_interval = DEF_SERVER_POLLING_INTERVAL;

	dprintf(TCPD_INFO, "%s() file_count: %d \n", __func__, file_count);

	return 0;
}
#endif


int _mmbSendPollingData(CPai_r_data &pai_r_data, const char *host, int host_port)
{
	if(l_PollingTcp.socket_open(host, host_port) == 0) {
		int writeSize = 0;
		time_t     t_rtc;
	  	time(&t_rtc);
		
		pai_r.updatelog.make(UL_DATA_TYPE_POLLING, 0, t_rtc);
			
		// Data Header Send
		writeSize += l_PollingTcp.socket_write((unsigned char *)&l_mmb.frame_polling, l_mmb.frame_size);

		if(writeSize == l_mmb.frame_size){
			l_mmb.mmb_ack = MMB_ERR;
			l_PollingTcp.socket_read(NULL, 0, DEF_SERVER_TIMEOUT_WAIT, _mmbPollingResponseFunc);
#ifdef DEF_MMB_VER_2
			dprintf(TCPD_INFO, "%s() : polling mmb_ack = %d\n", __func__, l_mmb.mmb_ack);
			if(l_mmb.mmb_ack == MMB_ACK || l_mmb.mmb_ack == MMB_NAK) { // 30초 대기 후 filelist등 request frame 이 있는 경우 처리
				unsigned char ack_buf[3] = {MMB_CLIENT_RESPONSE_H0, MMB_CLIENT_RESPONSE_H1, MMB_SERVER_ACK};
				l_mmb.mmb_ack = MMB_ERR;
				l_PollingTcp.socket_read(NULL, 0, DEF_SERVER_TIMEOUT_WAIT, _mmbRequestFrameFunc);
				if(l_mmb.mmb_ack == MMB_REQUEST_FILELIST) {

					// Ack 전송 후 업로드 서버로 리스트 전송 
					l_PollingTcp.socket_write(ack_buf, sizeof(ack_buf));
					
				} else if(l_mmb.mmb_ack == MMB_REQUEST_FILE) {

					// Ack 전송 후 업로드 서버로 파일 전송 
					l_PollingTcp.socket_write(ack_buf, sizeof(ack_buf));
					
				} else if(l_mmb.mmb_ack == MMB_NAK) {
					ack_buf[2] = MMB_SERVER_NAK;
					l_PollingTcp.socket_write(ack_buf, sizeof(ack_buf));
					l_mmb.mmb_ack = MMB_POLLING_WAIT;
				}
			}
			return 0;
#else
			if(l_mmb.mmb_ack == MMB_REQUEST_FILE)
				return 0;			
#endif				
		}
		else {
			dprintf(TCPD_ERROR, "%s() : write size error!(%d:%d)\r\n", __func__, writeSize, l_mmb.frame_size);
		}
	}
	return -1; //timeout
}

#ifdef DEF_MMB_VER_2
int _mmbPollingWait(const char *host, int host_port)
{
	unsigned char ack_buf[3] = {MMB_CLIENT_RESPONSE_H0, MMB_CLIENT_RESPONSE_H1, MMB_SERVER_ACK};
	l_mmb.mmb_ack = MMB_ERR;

	if(l_PollingTcp.socket_open(host, host_port) == 0) {
		l_PollingTcp.socket_read(NULL, 0, DEF_SERVER_TIMEOUT_WAIT, _mmbRequestFrameFunc);
		if(l_mmb.mmb_ack == MMB_REQUEST_FILELIST) {
			// Ack 전송 후 업로드 서버로 리스트 전송 
			l_PollingTcp.socket_write(ack_buf, sizeof(ack_buf));
			return 0;
		} else if(l_mmb.mmb_ack == MMB_REQUEST_FILE) {
			// Ack 전송 후 업로드 서버로 파일 전송 
			l_PollingTcp.socket_write(ack_buf, sizeof(ack_buf));
			return 0;
		} else if(l_mmb.mmb_ack == MMB_NAK) {
			ack_buf[2] = MMB_SERVER_NAK;
			l_PollingTcp.socket_write(ack_buf, sizeof(ack_buf));
			l_mmb.mmb_ack = MMB_POLLING_WAIT;
			return 0;
		}
	}
	return -1; //timeout
}

int _mmbSendUploadDataVer2(CPai_r_data &pai_r_data, const char *host, int host_port, eUserDataType ud_type, eUpLoadDataType upload_type)
{
	int result = -1;
	int foundInfo = 0;

	u32 uploadDataLength = 0;
	u32 writeSize = 0;
	u32 dataFrameSize = 0;
	u8   numberOfImages = 0;
	time_t create_time = 0;
	u32 file_size = 0;
	
	PAI_R_BACKUP_DATA * pInfo = (PAI_R_BACKUP_DATA *)new char[  sizeof(PAI_R_BACKUP_DATA) * DEF_MMB_MAX_EVENTDATA_COUNT ];

	if(pInfo == NULL){
		dprintf(TCPD_ERROR, "%s() : memory allocation error!(size:%d)", __func__, sizeof(PAI_R_BACKUP_DATA));
		return -1;
	}

	if(l_Tcp.socket_open(host, host_port) != 0) {
		delete [] pInfo;
		return -1;
	}

	if(upload_type == UPLOAD_TYPE_EVENT && !pai_r_data.Location_queue_count(ud_type)) {
		delete [] pInfo;
		return -1;
	}

	l_mmb.updateHeaderFrameUpload(upload_type);

	if(upload_type == UPLOAD_TYPE_EVENT){
		foundInfo = pai_r_data.BackupLocationGet(pInfo, ud_type);

		if(foundInfo)
		{
			const char * file = strrchr(pInfo->location.file_path, '/');		

			if(file && access(pInfo->location.file_path, R_OK ) == 0){
				file++;
				create_time = recording_time_string_to_time(file);
			}
			else
				create_time = pInfo->location.create_time;

			pai_r.updatelog.make(UL_DATA_TYPE_LOCATION, pai_r_data.Location_queue_auto_id_out_get(ud_type));
			
			_mmbSetEventData(create_time, pInfo->location.create_time_ms, pInfo->location.eventType, pInfo->location.gnssStatus, pInfo->location.longitude, pInfo->location.latitude, DEF_MMB_ENDIAN);

			dprintf(TCPD_INFO, "%s() : %u, event type : %d, gnssStatus : %c (%0.4f, %0.4f)\r\n", __func__, create_time, \
				pInfo->location.eventType, pInfo->location.gnssStatus, \
				pInfo->location.longitude, pInfo->location.latitude);
			
			uploadDataLength += sizeof(CommonHeader);
			uploadDataLength += sizeof(EventData);

			file_size = sysfs_getsize(pInfo->location.file_path);
			dprintf(TCPD_INFO, "%s() : filename : %s, filesize : %d\r\n", __func__, pInfo->location.file_path, file_size);
			if(file_size) {
				numberOfImages = 1;

				uploadDataLength += sizeof(file_size);
				uploadDataLength += file_size;
			}

			if(numberOfImages)
				uploadDataLength += sizeof(numberOfImages);

			// Data Header Send
			writeSize += l_Tcp.socket_write((unsigned char *)&l_mmb.frame_upload, l_mmb.frame_size);
				
			writeSize += l_Tcp.socket_write(uploadDataLength, DEF_MMB_ENDIAN);


			dataFrameSize = l_mmb.frame_size + sizeof(uploadDataLength) + uploadDataLength;

			dprintf(TCPD_INFO, "%s() : write - header %d, data %d\r\n", __func__, l_mmb.frame_size, uploadDataLength);
			
			//Common Header Data Send
			writeSize += l_Tcp.socket_write((unsigned char *)&l_mmb.header, sizeof(l_mmb.header));
			
			//EventData Send
			writeSize += l_Tcp.socket_write((unsigned char *)&l_mmb.event, sizeof(l_mmb.event));

			dprintf(TCPD_INFO, "%s() : write - comm %d, event %d\r\n", __func__, sizeof(l_mmb.header), sizeof(l_mmb.event));


			//Event Movie Data Send
			if(access(pInfo->location.file_path, R_OK ) == 0){
				char 	file_hash[36] = { 0, };
				numberOfImages = 1;
				writeSize += l_Tcp.socket_write((unsigned char *)&numberOfImages, sizeof(numberOfImages)); // number of cameras
				writeSize += l_Tcp.socket_write(file_size, DEF_MMB_ENDIAN); // file size
				writeSize += l_Tcp.socket_write(pInfo->location.file_path, file_hash, sizeof(file_hash));

				dprintf(TCPD_INFO, "%s() : write - video %d \r\n", __func__, file_size);
				dprintf(TCPD_INFO, "%s() : writeSize %d,  dataFrameSize %d\r\n", __func__, writeSize, dataFrameSize);
			}

			if(writeSize == dataFrameSize){
				l_mmb.mmb_ack = MMB_ERR;
				l_Tcp.socket_read(NULL, 0, DEF_SERVER_TIMEOUT_WAIT, _mmbUploadResponseFunc);

				if(l_mmb.mmb_ack == MMB_ACK) {
					httpd_cfg.server_wait_success_movie = 0;
					_mmb_send_queue_clear(pai_r_data, ud_type);
					result  = 0;				
					l_mmb.retry_cnt = 0;
				}				
				else if(l_mmb.mmb_ack == MMB_ERR) {
					if(l_mmb.retry_cnt >= 2) { //30초 간격으로 2회 retry를 시도 후 종료
						l_mmb.retry_cnt = 0;
					} else {
						l_mmb.retry_cnt ++;
					}										
				}								
			}
			else {
				dprintf(TCPD_ERROR, "%s() : write size error!(%d:%d)\r\n", __func__, writeSize, dataFrameSize);
			}				

		}
	} else if(upload_type == UPLOAD_TYPE_FILELIST) {

		//uploadDataLength (헤더를 제외한 파일리스트 데이터 크기) 를 구하고
		uploadDataLength = sizeof(file_count) + filelist_dataSize;

		//header + imei len + IMEI
		writeSize += l_Tcp.socket_write((unsigned char *)&l_mmb.frame_upload, l_mmb.frame_size);

		
		writeSize += l_Tcp.socket_write(uploadDataLength, DEF_MMB_ENDIAN);


		// uploadDataLength 길이만큼의 data 전송
		vector<filelist>::iterator iter;

		writeSize += l_Tcp.socket_write(file_count, DEF_MMB_ENDIAN);
		for(iter = files.begin(); iter != files.end(); iter++) {
			writeSize += l_Tcp.socket_write(iter->length, DEF_MMB_ENDIAN);
			writeSize += l_Tcp.socket_write((unsigned char*)iter->filepath, iter->length);
		}

		dataFrameSize = l_mmb.frame_size + sizeof(uploadDataLength) + uploadDataLength;

		// 전송 완료 후 ack/nak 응답 대기
		if(writeSize == dataFrameSize){
			l_mmb.mmb_ack = MMB_ERR;
			l_Tcp.socket_read(NULL, 0, DEF_SERVER_TIMEOUT_WAIT, _mmbUploadResponseFunc);

			if(l_mmb.mmb_ack == MMB_ACK || l_mmb.mmb_ack == MMB_NAK) {
				//httpd_cfg.server_wait_success_movie = 0;			
				l_mmb.retry_cnt = 0;

				files.clear();
				file_count = 0;
				filelist_dataSize = 0;
				l_mmb.mmb_ack = MMB_POLLING_WAIT;
				result = 0;
			}				
			else if(l_mmb.mmb_ack == MMB_ERR) {
				if(l_mmb.retry_cnt >= 2) { //30초 간격으로 2회 retry를 시도 후 종료
					l_mmb.retry_cnt = 0;

					files.clear();
					file_count = 0;
					filelist_dataSize = 0;
					l_mmb.mmb_ack = MMB_POLLING_WAIT;
					result = 0;
				} else {
					l_mmb.retry_cnt ++;
					l_mmb.mmb_ack = MMB_REQUEST_FILELIST;
				}										
			}								
		}
		else {
				dprintf(TCPD_ERROR, "%s() : write size error!(%d:%d)\r\n", __func__, writeSize, dataFrameSize);
		}

	} else if(upload_type == UPLOAD_TYPE_FILE) {

		if(files.size() == 0) {

			l_mmb.mmb_ack = MMB_ERR;
			dprintf(TCPD_ERROR, "%s() ERROR : file_count %d !!!\r\n", __func__, file_count);
			delete [] pInfo;
			return -1;
		}

		char path[256] = "/mnt/extsd";
		unsigned char filename[128] = {0,};
		const char* pfilename;
		filelist file_item;
		unsigned short filename_len;
		uint32_t filesize;
		char file_hash[36] = { 0, };

		file_item = files[0];

		strncat(path, file_item.filepath, file_item.length); // fullpath
#if 0
		pfilename = strrchr(path, '/');
		strcpy((char*)filename, pfilename + 1); //20230718_140000_I2.fvfs
#else
		pfilename = file_item.filepath;
		strcpy((char*)filename, pfilename); // /NORMAL/20230718_140000_I2.fvfs
#endif
		filename_len = strlen((char*)filename); 

		if(access(path, R_OK ) == 0) {

			filesize = sysfs_getsize(path);

			//uploadDataLength (헤더를 제외한 파일리스트 데이터 크기) 를 구하고
			// filepath_len 2byte +  filename + filesize 4byte + filedata
			uploadDataLength = sizeof(filename_len) + filename_len + sizeof(filesize) + filesize;

			//header + imei len + IMEI
			writeSize += l_Tcp.socket_write((unsigned char *)&l_mmb.frame_upload, l_mmb.frame_size);

			
			writeSize += l_Tcp.socket_write(uploadDataLength, DEF_MMB_ENDIAN);

			dataFrameSize = l_mmb.frame_size + sizeof(uploadDataLength) + uploadDataLength;

			dprintf(TCPD_INFO, "%s() : path = %s \n", __func__, path);
			dprintf(TCPD_INFO, "%s() : filename_len = %d \n", __func__, filename_len);
			dprintf(TCPD_INFO, "%s() : filename = %s \n", __func__, filename);
			dprintf(TCPD_INFO, "%s() : filesize = %d \n", __func__, filesize);

			// uploadDataLength 길이만큼의 data 전송

			writeSize += l_Tcp.socket_write(filename_len, DEF_MMB_ENDIAN); // 파일경로 길이
			writeSize += l_Tcp.socket_write(filename, filename_len); // 파일경로
			writeSize += l_Tcp.socket_write(filesize, DEF_MMB_ENDIAN); // 파일 크기
			writeSize += l_Tcp.socket_write(path, file_hash, sizeof(file_hash)); // 영상파일

			// 전송 완료 후 ack/nak 응답 대기
			if(writeSize == dataFrameSize){
				l_mmb.mmb_ack = MMB_ERR;
				l_Tcp.socket_read(NULL, 0, DEF_SERVER_TIMEOUT_WAIT, _mmbUploadResponseFunc);

				if(l_mmb.mmb_ack == MMB_ACK || l_mmb.mmb_ack == MMB_NAK) {
					httpd_cfg.server_wait_success_movie = 0;			
					l_mmb.retry_cnt = 0;

					files.erase(files.begin());
					file_count--;
				}				
				else if(l_mmb.mmb_ack == MMB_ERR) {
					if(l_mmb.retry_cnt >= 2) { //30초 간격으로 2회 retry를 시도 후 종료
						l_mmb.retry_cnt = 0;

						files.clear();
						file_count = 0;
					} else {
						l_mmb.retry_cnt ++;
						l_mmb.mmb_ack = MMB_REQUEST_FILE;
					}										
				}								
			}else {
				dprintf(TCPD_ERROR, "%s() : write size error!(%d:%d)\r\n", __func__, writeSize, dataFrameSize);
			}

			if(files.size() > 0) {
				l_mmb.mmb_ack = MMB_REQUEST_FILE;
				result = 0;
			}else {
				l_mmb.mmb_ack = MMB_POLLING_WAIT;
				result = 0;
			}

		} else {
			//file access fail.
			dprintf(TCPD_ERROR, "%s() file access fail : path %s !!!\r\n", __func__, path);
			l_mmb.mmb_ack = MMB_POLLING_WAIT;
			result = 0;
		}
	}
	
	delete [] pInfo;
	
	return result;
}

#endif

int _mmbSendUploadData(CPai_r_data &pai_r_data, const char *host, int host_port, eUserDataType ud_type, time_t event_time)
{
	int result = -1;
	int foundInfo = 0;
	
	PAI_R_BACKUP_DATA * pInfo = (PAI_R_BACKUP_DATA *)new char[  sizeof(PAI_R_BACKUP_DATA) * DEF_MMB_MAX_EVENTDATA_COUNT ];

	if(pInfo == NULL){
		dprintf(TCPD_ERROR, "%s() : memory allocation error!(size:%d)", __func__, sizeof(PAI_R_BACKUP_DATA));
		return -1;
	}

	if(l_Tcp.socket_open(host, host_port) != 0) {
		delete [] pInfo;
		return -1;
	}

	 if(event_time == 0 && !pai_r_data.Location_queue_count(ud_type)) {
		delete [] pInfo;
		return -1;
	}
	if(event_time){
		foundInfo = pai_r_data.BackupLocationGetEvent(pInfo, ud_type, event_time);
	}
	else {
		foundInfo = pai_r_data.BackupLocationGet(pInfo, ud_type);
	}
	
	if(foundInfo)
	{
		u32 uploadDataLength = 0;
		u32 writeSize = 0;
		u32 dataFrameSize = 0;
		u8   numberOfImages = 0;
		time_t create_time = 0;
		u32 file_size = 0;
		const char * file = strrchr(pInfo->location.file_path, '/');		

		if(file && access(pInfo->location.file_path, R_OK ) == 0){
			file++;
			create_time = recording_time_string_to_time(file);
		}
		else
			create_time = pInfo->location.create_time;

		pai_r.updatelog.make(UL_DATA_TYPE_LOCATION, pai_r_data.Location_queue_auto_id_out_get(ud_type));
		
		_mmbSetEventData(create_time, pInfo->location.create_time_ms, pInfo->location.eventType, pInfo->location.gnssStatus, pInfo->location.longitude, pInfo->location.latitude, DEF_MMB_ENDIAN);

		dprintf(TCPD_INFO, "%s() : %u, event type : %d, gnssStatus : %c (%0.4f, %0.4f)\r\n", __func__, create_time, \
			pInfo->location.eventType, pInfo->location.gnssStatus, \
			pInfo->location.longitude, pInfo->location.latitude);
		
		uploadDataLength += sizeof(CommonHeader);
		uploadDataLength += sizeof(EventData);

		if(event_time) {
			file_size = sysfs_getsize(pInfo->location.file_path);
			if(file_size) {
				numberOfImages = 1;

				uploadDataLength += sizeof(file_size);
				uploadDataLength += file_size;
			}
		}
		else {
#ifdef DEF_MMB_VER_2
			file_size = sysfs_getsize(pInfo->location.file_path);
			dprintf(TCPD_INFO, "%s() : filename : %s, filesize : %d\r\n", __func__, pInfo->location.file_path, file_size);
			if(file_size) {
				numberOfImages = 1;

				uploadDataLength += sizeof(file_size);
				uploadDataLength += file_size;
			}

#else
			for(int ch = 0; ch < DEF_MAX_CAMERA_COUNT; ch++){
				u32 size= pInfo->location.thumbnail.size[ch];
				if(size){
					numberOfImages++;
					uploadDataLength += sizeof(size);
					uploadDataLength += size;
				}
			}
#endif
		}

		if(numberOfImages)
			uploadDataLength += sizeof(numberOfImages);

		// Data Header Send
		if(event_time)
#ifdef DEF_MMB_VER_2
			dprintf(TCPD_ERROR, "%s() : This line not support MMB_VER_2 \r\n", __func__);
#else		
			writeSize += l_Tcp.socket_write((unsigned char *)&l_mmb.frame_request, l_mmb.frame_size);
#endif			
		else
			writeSize += l_Tcp.socket_write((unsigned char *)&l_mmb.frame_upload, l_mmb.frame_size);
			
		writeSize += l_Tcp.socket_write(uploadDataLength, DEF_MMB_ENDIAN);


		dataFrameSize = l_mmb.frame_size + sizeof(uploadDataLength) + uploadDataLength;

		dprintf(TCPD_INFO, "%s() : write - header %d, data %d\r\n", __func__, l_mmb.frame_size, uploadDataLength);
		
		//Common Header Data Send
		writeSize += l_Tcp.socket_write((unsigned char *)&l_mmb.header, sizeof(l_mmb.header));
		
		//EventData Send
		writeSize += l_Tcp.socket_write((unsigned char *)&l_mmb.event, sizeof(l_mmb.event));

		dprintf(TCPD_INFO, "%s() : write - comm %d, event %d\r\n", __func__, sizeof(l_mmb.header), sizeof(l_mmb.event));

		if(event_time == false){
#ifdef DEF_MMB_VER_2
			//Event Movie Data Send
			if(access(pInfo->location.file_path, R_OK ) == 0){
				char 	file_hash[36] = { 0, };
				numberOfImages = 1;
				writeSize += l_Tcp.socket_write((unsigned char *)&numberOfImages, sizeof(numberOfImages)); // number of cameras
				writeSize += l_Tcp.socket_write(file_size, DEF_MMB_ENDIAN); // file size
				writeSize += l_Tcp.socket_write(pInfo->location.file_path, file_hash, sizeof(file_hash));

				dprintf(TCPD_INFO, "%s() : write - video %d \r\n", __func__, file_size);
				dprintf(TCPD_INFO, "%s() : writeSize %d,  dataFrameSize %d\r\n", __func__, writeSize, dataFrameSize);
			}
#else
			//Event Image Data Send
			if(numberOfImages){
				writeSize += l_Tcp.socket_write((unsigned char *)&numberOfImages, sizeof(numberOfImages)); // number of cameras
				for(int ch = 0; ch < DEF_MAX_CAMERA_COUNT; ch++){
					u32 size= pInfo->location.thumbnail.size[ch];
					if(size){
						writeSize += l_Tcp.socket_write(size, DEF_MMB_ENDIAN); // image size
						writeSize += l_Tcp.socket_write(pInfo->location.thumbnail.data[ch], size); // image data

						dprintf(TCPD_INFO, "%s() : write - image %d \r\n", __func__, size);
					}
				}
			}
#endif

			if(writeSize == dataFrameSize){
				l_mmb.mmb_ack = MMB_ERR;
				l_Tcp.socket_read(NULL, 0, DEF_SERVER_TIMEOUT_WAIT, _mmbUploadResponseFunc);

				if(l_mmb.mmb_ack == MMB_ACK) {
					httpd_cfg.server_wait_success_movie = 0;
					_mmb_send_queue_clear(pai_r_data, ud_type);
					result  = 0;
#ifdef DEF_MMB_VER_2					
					l_mmb.retry_cnt = 0;
#endif
				}
#ifdef DEF_MMB_VER_2				
				else if(l_mmb.mmb_ack == MMB_ERR) {
					if(l_mmb.retry_cnt >= 2) { //30초 간격으로 2회 retry를 시도 후 종료
						l_mmb.retry_cnt = 0;
					} else {
						l_mmb.retry_cnt ++;
					}										
				}
#endif									
			}
			else {
				dprintf(TCPD_ERROR, "%s() : write size error!(%d:%d)\r\n", __func__, writeSize, dataFrameSize);
			}				
		}
		else { //movie file upload
			l_mmb.mmb_ack = MMB_ERR;
			pai_r.updatelog.make(UL_DATA_TYPE_MOVIE, pInfo->event_autoid, create_time);

			if(access(pInfo->location.file_path, R_OK ) == 0){
				char 	file_hash[36] = { 0, };
				numberOfImages = 1;
				writeSize += l_Tcp.socket_write((unsigned char *)&numberOfImages, sizeof(numberOfImages)); // number of cameras
				writeSize += l_Tcp.socket_write(file_size, DEF_MMB_ENDIAN); // file size
				writeSize += l_Tcp.socket_write(pInfo->location.file_path, file_hash, sizeof(file_hash));
					
				if(strlen(pInfo->file_hash) == 0) {
#ifdef MSGQ_PAIR_LOC_Q_OUT_IDKEY		
					ST_QMSG msg;
					msg.type = QMSG_UP_FILE_HASH;
					msg.data = eUserDataType_Normal;
					msg.data2 = pInfo->location.autoid;
					msg.time = (u32)time(0);
					strcpy(msg.string, file_hash);
					datool_ipc_msgsnd(m_http_msg_q_out_id, (void *)&msg, sizeof(msg));
#endif
				}
			}

			if(writeSize == dataFrameSize){
				l_mmb.mmb_ack = MMB_ERR;
				l_Tcp.socket_read(NULL, 0, DEF_SERVER_TIMEOUT_WAIT, _mmbPollingResponseFunc);

				if(l_mmb.mmb_ack == MMB_NOTHING || l_mmb.mmb_ack == MMB_REQUEST_FILE){
					httpd_cfg.server_wait_success_movie = 0;
					//_mmb_send_queue_clear(pai_r_data, ud_type);

					if(l_mmb.mmb_ack == MMB_REQUEST_FILE)
						result = 0;
				}						
			}
			else {
				dprintf(TCPD_ERROR, "%s() : write size error!(%d:%d)\r\n", __func__, writeSize, dataFrameSize);
			}		
		}
	}

	delete [] pInfo;
	
	return result;
}

int tcp_mmb_init(void)
{
	l_mmb.setHeaderFrameUpload(strlen(pai_r.strProductSN), (unsigned char*)pai_r.strProductSN);
	l_mmb.setHeaderFramePolling(strlen(pai_r.strProductSN), (unsigned char*)pai_r.strProductSN);
#ifndef DEF_MMB_VER_2	
	l_mmb.setHeaderFrameRequest(strlen(pai_r.strProductSN), (unsigned char*)pai_r.strProductSN);
#endif
	l_mmb.setCommonHeader(DEF_DEVICETYPE, __FW_VERSION_MAJOR__, __FW_VERSION_MINOR__, DA_FIRMWARE_VERSION, pai_r.strProductSN);

	return 0;
}

int tcp_mmb_send(CPai_r_data &pai_r_data, bool send_require, eUserDataType ud_type)
{
	const char * host = TCPIP_SERVER_ADDRESS;
	const char * driverecorder_api_url = "";
	int host_port = TCPIP_PORT_UPLOAD;
	int host_polling_port = TCPIP_PORT_POLLING;
	int result = 0;
	bool send_movie = false;

	if(strlen(pai_r.cfg.strCloudServerName))
		host = (const char *)pai_r.cfg.strCloudServerName;
	if( pai_r.cfg.iCloudServerPort )
		host_port = pai_r.cfg.iCloudServerPort;
	if( pai_r.cfg.iCloudServerPollingPort)
		host_polling_port = pai_r.cfg.iCloudServerPollingPort;
		
	if(pai_r.cfg.iDebugServer_PORT){
	  host = pai_r.cfg.strApplication_IP;
	  host_port = pai_r.cfg.iDebugServer_PORT;
	}

	httpd_server_error_check();

#ifdef DEF_MMB_VER_2
	l_mmb.retry_cnt = 0;

	if(send_require) {
		do {
			result = _mmbSendUploadDataVer2(pai_r_data, host, host_port, ud_type, UPLOAD_TYPE_EVENT);
		}while ((result == 0 && pai_r_data.Location_queue_count(ud_type)) || (l_mmb.retry_cnt > 0 && pai_r_data.Location_queue_count(ud_type)));
	}
	else {
		result = _mmbSendPollingData(pai_r_data, host, host_polling_port);
		while( result == 0 || l_mmb.retry_cnt > 0) {
			//send file
			if(l_mmb.mmb_ack == MMB_REQUEST_FILELIST) {
				result = _mmbSendUploadDataVer2(pai_r_data, host, host_port, ud_type, UPLOAD_TYPE_FILELIST);
			} else if(l_mmb.mmb_ack == MMB_REQUEST_FILE) {
				result = _mmbSendUploadDataVer2(pai_r_data, host, host_port, ud_type, UPLOAD_TYPE_FILE);
			} else if(l_mmb.mmb_ack == MMB_POLLING_WAIT) { // polling 대기
				result = _mmbPollingWait(host, host_polling_port);
			} else {
				break;
			}
		}
	}
#else
	if(send_require) {
		do {
			result = _mmbSendUploadData(pai_r_data, host, host_port, ud_type, 0);
		}while (result == 0 && pai_r_data.Location_queue_count(ud_type));		
	}
	else {
		result = _mmbSendPollingData(pai_r_data, host, host_polling_port);
		while(result == 0) {
			//send file
			result = _mmbSendUploadData(pai_r_data, host, host_polling_port, ud_type, l_mmb.request_t);
		}
	}
#endif

	l_Tcp.socket_close();
#ifdef DEF_MMB_VER_2	
	l_PollingTcp.socket_close();
#endif
	if(httpd_cfg.server_wait_success >= DEF_SERVER_UPLOAD_RETRY_MAX_COUNT)
		httpd_cfg.time_interval = DEF_SERVER_TIMEOUT_WAIT * 10; // 업로드서버 응답이 없는 경우 5분후 다시시도 하도록 한다. 
		
	
	return result;
}

 #endif	

