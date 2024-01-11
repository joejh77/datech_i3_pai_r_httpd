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
#include "pai_r_error_list.h"
//#include "OasisAPI.h"
#include "TCPClient.h"
#include "tcp_client.h"
#include "tcp_mmb_r.h"
#include "httpd_pai_r.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif


#define HTTPD_INFO		1
#define HTTPD_ERROR		1
#define HTTPD_WARN		1

#define DEF_HOST_SSL_USE		0

#define DEF_MOVIELIST_USE_HASH_STRING		0	// 0 : "hash":"/mnt/extsd/NORMAL/20200505_130142_I2.hash" , 1 : "hash":"[md5 file hash]" 

#if 1
/* 
	<item value="mirec.info" id="CloudServerName" />
	<item value="/api_driverecorder/firstview/1/insert_driverecorder.php" id="CloudServerApiUrl" />
*/
 #ifdef DEF_TEST_SERVER_USE
 #define DEF_HOST_ADDR 					"da-tech.com"
 #define DEF_DRIVERECORDER_URL	"/driverecorder/firstview/insert_driverecorder.php"
 #define STR_COMPANY_ID  			"company_id"
 #else
 #define DEF_HOST_ADDR 					"mirec.info"
 #define DEF_DRIVERECORDER_URL	"/api_driverecorder/firstview/1/insert_driverecorder.php"
 #define STR_COMPANY_ID		"company_pai_r_id"
 #endif

#else
#define DEF_HOST_ADDR 					"mar-i.com"
#define DEF_DRIVERECORDER_URL	"/driverecorder/api_driverecorder/firstview/1/insert_driverecorder.php"
#endif

#if DEF_HOST_SSL_USE
#define DEF_HOST_PORT		443
#else
#define DEF_HOST_PORT		80
#endif

#define SSL_CERT_PATH		"/datech/etc/ssl_cert.pem"

#define DOCUMENT_ROOT "/mnt/extsd/"
#define PORT "8081"
#define EXAMPLE_URI "/example"
#define EXIT_URI "/exit"

#define DEF_APPLICATION_PORT			31284

bool g_httpdExitNow = false;

#define DEF_SERVER_REGISTRATION_FAILED_INTERVAL		(1 * 60 * 1000)
#define DEFAULT_NETWORK_CHECK_INTERVAL		30000 //30sec
#define DEFAULT_STATION_MODE_TIMEOUT_COUNT		6 // 3min



HTTPD_CFG httpd_cfg = {80, DEFAULT_NETWORK_CHECK_INTERVAL, 2, 0, WIFI_STATIONMODE, 0, 0, 0, 0, 0, 0, 0, 0,0};

CPai_r_data pai_r_data;

#define IPC_MSG_USE 		0
#if IPC_MSG_USE
int m_http_msg_q_loc_id = -1;
int m_http_msg_q_spd_id = -1;
#endif
#ifdef MSGQ_PAIR_LOC_Q_IN_IDKEY
int m_http_msg_q_in_id = -1;
int m_http_msg_q_out_id = -1;
#endif

///////////////////////////////////////////////////////
/*
https://mar-i.com/driverecorder/api_driverecorder/firstview/1/insert_driverecorder.php
{
    "api_list": {
        "insert_driverecorder": "https://mar-i.com/driverecorder/api_driverecorder/firstview/1/insert_driverecorder.php",
        "insert_operation": "https://mar-i.com/driverecorder/api_driverecorder/firstview/1/insert_operation.php",
        "insert_list_and_request": "https://mar-i.com/driverecorder/api_driverecorder/firstview/1/insert_list_and_request.php",
        "insert_movie": "https://mar-i.com/driverecorder/api_driverecorder/firstview/1/insert_movie.php"
    },
    "sensor": {
        "level": 2,
        "update_datetime": "2019-09-06 18:30:00"
    },
    "IsSuccess": true,
    "error": {
        "code": 0
    }
}

https://mar-i.com/driverecorder/api_driverecorder/firstview/1/insert_operation.php
{
    "operation_autoid": 40,
    "now_datetime": "2019-09-26 11:11:44",
    "api_list": {
        "insert_driverecorder": "https://mar-i.com/driverecorder/api_driverecorder/firstview/1/insert_driverecorder.php",
        "insert_operation": "https://mar-i.com/driverecorder/api_driverecorder/firstview/1/insert_operation.php",
        "insert_list_and_request": "https://mar-i.com/driverecorder/api_driverecorder/firstview/1/insert_list_and_request.php",
        "insert_movie": "https://mar-i.com/driverecorder/api_driverecorder/firstview/1/insert_movie.php"
    },
    "latest_version": "0.0.1",
    "sensor": {
        "level": 2,
        "update_datetime": "2019-09-06 18:30:00"
    },
    "IsSuccess": true,
    "error": {
        "code": 0
    }
}

https://mar-i.com/driverecorder/api_driverecorder/firstview/1/insert_list_and_request.php",
{
	"IsSuccess":true,
	"error":{"code":0},
	"request_driverecorder_autoid":[],"cancel_driverecorder_autoid":[],
	"sensor":{
		"level":2,
		"update_datetime":"2019-09-06 18:30:00"
	}
}
*/


PAI_R_CONFIG pai_r = {0,};

int _http_json_parse(const char * doc, Json::Value& root)
{
	Json::Reader reader;
	
	dprintf(HTTPD_WARN, "response json : %s\r\n", doc);
	
	if(!reader.parse(doc, root))
		dprintf(HTTPD_ERROR, "json error\r\n");

	dprintf(HTTPD_INFO, "type : %d\r\n", root.type());

	if(root.type() == Json::nullValue){
		const char * json_str_start = strchr(doc, '{');
		const char * json_str_end= strrchr(doc, '}');
		if(json_str_end) json_str_end++;
		
		if(!reader.parse(json_str_start, json_str_end, root))
			dprintf(HTTPD_ERROR, "2 json error\r\n");

		dprintf(HTTPD_INFO, "2 type : %d\r\n", root.type());

		if(root.type() == Json::nullValue) {
			return 0;
		}
	}

	if(root.type() == Json::objectValue) {
		httpd_cfg.wait_ack = 0;
		
		if((root["IsSuccess"].isBool() && root["IsSuccess"].asBool()) || (root["is_success"].isBool() && root["is_success"].asBool()) ){
			httpd_cfg.server_wait_success = 0;
		}
	}

	return 1;
}


int pai_r_information_send_to_app(CHttp_multipart *p, const char *data, int length)
{
	Json::Value list_keepalive;
	bool success = false;
	
	if(_http_json_parse(data, list_keepalive) == 0) {
		pai_r.updatelog.save(UL_RESULT_TYPE_API_ERROR, "", "", data);
		return 0;
	}
	
	if(list_keepalive.isObject()){
		if(list_keepalive["IsSuccess"].asBool() == true || pai_r.request["is_success"].asBool() == true){

			success = true;
		}
		else{
			Json::Value error = list_keepalive.get("error", list_keepalive);

			if(error.isObject()){
				//int error_code = error.get("code", &error);
				
				if(error["message"].isString())
					dprintf(HTTPD_ERROR, "message : %s\r\n", error["message"].asString().c_str());
			}	
		}
	}

	Json::FastWriter writer;
	std::string content = writer.write(list_keepalive);
	
	pai_r.updatelog.save(success == true ? UL_RESULT_TYPE_SUCCESS : UL_RESULT_TYPE_API_ERROR, "", "",content.c_str());
	
	return 0;
}

 #ifndef 	DEF_MMB_SERVER
 
int _http_host_string_parser(const char * src_url, const char * default_host, int default_port, char * ret_url, char * ret_host, int *ret_port)
{
	char strHttp[128] = {0,};
	const char * host_s = NULL;
	const char * host_e = NULL;
#if 0	
	const char * url_s = strstr( src_url, "/driverecorder");
#else
	const char * url_s = strstr( src_url, "//");

	if(url_s)
		url_s = strstr( (const char *)(url_s + 2), "/");
	else
		url_s = strstr( src_url, "/");
#endif


	
	if(strstr( src_url, "http") || strstr( src_url, "https")){
		host_s = strstr( src_url, "//");
	}

	if(host_s){
		host_s += 2;
		strncpy(strHttp, src_url, host_s - src_url);

		//dprintf(HTTPD_INFO, "http : %s\r\n", strHttp);
	}
	else {
		host_s = src_url;
		strcpy(strHttp, "http://");
	}

	host_e = strchr( host_s, ':');
	
	if(host_s && host_e){
		strncpy(ret_host, host_s, host_e - host_s);
		//dprintf(HTTPD_INFO, "host : %s\r\n", ret_host);
	}
	else {
		if(strstr( default_host, "http") && strstr( default_host, "//"))
			strcpy(ret_host, strstr( default_host, "//") + 2);
		else
			strcpy(ret_host, default_host);
	}

	*ret_port = 0;
	if(host_e){
		char strPort[32] = {0,};
		host_e++;
			
		if(host_e < url_s){
			strncpy(strPort, host_e, url_s - host_e);
			*ret_port = atoi(strPort);

			//dprintf(HTTPD_INFO, "port : %s(%d)\r\n", strPort, *ret_port);
		}
	}
	
	if(*ret_port == 0){
		*ret_port = default_port;
	}
	
	strcpy(ret_url, url_s);
	
	dprintf(HTTPD_WARN, "url : %s:%d%s\r\n", ret_host, *ret_port, ret_url);
	return 1;	
}


int _http_json_sensor_parse(const Json::Value& sensor)
{
	if(sensor.size()){
		const char * item = "level";
		
		if(sensor[item].isInt()){
			const char * time = sensor["update_datetime"].asString().c_str();	
			if(pai_r.cfg.iGsensorSensi != sensor[item].asInt() || strcmp(pai_r.cfg.strLastSetupTime, time) != 0){
				
				dprintf(HTTPD_INFO, "%s() : %d => %d (%s)\r\n", __func__, pai_r.cfg.iGsensorSensi, sensor[item].asInt(), time);
				
				pai_r.cfg.iGsensorSensi = sensor[item].asInt();

				if(strlen(time))
					strcpy(pai_r.cfg.strLastSetupTime, time);
				
				httpd_cfg_set(&pai_r.cfg);
			}
			return 1;
		}
	}

	return 0;
}

void _http_pa_r_insert_list_and_request_queue_clear(void)
{
	if(pai_r_data.m_tmp_pop_count) {
			pai_r_data.Location_queue_clear(pai_r_data.m_tmp_pop_count);
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

bool pai_r_backup_list_drv_read(Json::Value& list_drv, const char * host, int port, const char * api_url){
	bool skip_list_drv = false;
	char doc[2048] = { 0,};
	
	if(sysfs_read(DEF_SERVER_INFO_JSON_PATH, doc, sizeof(doc)) > 0){
		Json::Value wifi_info;
		if(_http_json_parse(doc, list_drv)){						
			wifi_info = list_drv.get("wifi_info", list_drv);
			if(wifi_info.size()){

				skip_list_drv = true;
				
				if(strcmp(wifi_info[STR_COMPANY_ID].asString().c_str(), pai_r.cfg.strSeriarNo) != 0)
					skip_list_drv = false;

				if(strcmp(wifi_info["ap_ssid"].asString().c_str(), pai_r.cfg.strApSsid) != 0)
					skip_list_drv = false;

				if(strcmp(wifi_info["wifi_ssid"].asString().c_str(), pai_r.cfg.strWiFiSsid[0]) != 0)
					skip_list_drv = false;

				if(strcmp(wifi_info["wifi_password"].asString().c_str(), pai_r.cfg.strWiFiPassword[0]) != 0)
					skip_list_drv = false;

				if(strcmp(wifi_info["tel_number"].asString().c_str(), pai_r.cfg.strTelNumber[0]) != 0)
					skip_list_drv = false;

				if(skip_list_drv){
#if 1
					if(!strstr( list_drv["server_url"].asString().c_str(), format_string("%s:%d%s", host, port, api_url).c_str()))
						skip_list_drv = false;
#else
					Json::Value api_list; 
					api_list = list_drv.get("api_list", list_drv);
					if(!strstr( api_list["insert_driverecorder"].asString().c_str(), format_string("%s:%d%s", host, port, api_url).c_str()))
						skip_list_drv = false;
#endif					
				}
			}						
		}
	}

	if(!skip_list_drv)
		list_drv.clear();
		
	return skip_list_drv;
}

 int pai_r_insert_driverecorder_func(CHttp_multipart *p, const char *data, int length)
{
	Json::FastWriter writer;
	std::string content;
	
	Json::Value list_drv;
	bool success = false;
	
	if(_http_json_parse(data, list_drv) == 0) {
		pai_r.updatelog.save(UL_RESULT_TYPE_API_ERROR, "", "", data);
		return 0;
	}
	
	if(list_drv.isObject()){
		if((list_drv["IsSuccess"].isBool()  && list_drv["IsSuccess"].asBool()) || (list_drv["is_success"].isBool() && list_drv["is_success"].asBool())){
			Json::Value api_list = list_drv.get("api_list", list_drv);

	//
			list_drv["server_url"] = pai_r.api_list_drv["server_url"];
	//
			pai_r.api_list_drv = list_drv;
			
			
			dprintf(HTTPD_INFO, "insert_driverecorder : %s\r\n", api_list["insert_driverecorder"].asString().c_str());
			dprintf(HTTPD_INFO, "insert_operation : %s\r\n",  api_list["insert_operation"].asString().c_str());
			dprintf(HTTPD_INFO, "insert_list_and_request : %s\r\n", api_list["insert_list_and_request"].asString().c_str());
			dprintf(HTTPD_INFO, "insert_movie : %s\r\n", api_list["insert_movie"].asString().c_str());

			success = true;

			//gsensor setup
			Json::Value sensor;
			_http_json_sensor_parse(list_drv.get("sensor", list_drv));

			// server information save
			Json::Value wifi_info;
			
			wifi_info[STR_COMPANY_ID] = (const char *)pai_r.cfg.strSeriarNo;
			wifi_info["ap_ssid"] = (const char *)pai_r.cfg.strApSsid;
			wifi_info["wifi_ssid"] = (const char *)pai_r.cfg.strWiFiSsid[0];
			wifi_info["wifi_password"] = (const char *)pai_r.cfg.strWiFiPassword[0];;

			pai_r.api_list_drv["wifi_info"] = wifi_info;


	
			content = writer.write(pai_r.api_list_drv);
			sysfs_write(DEF_SERVER_INFO_JSON_PATH, content.c_str(), content.length() + 1);
		}
		else{
			Json::Value error = list_drv.get("error", list_drv);

			if(error.isObject()){
				//int error_code = error.get("code", &error);
				
				if(error["message"].isString())
					dprintf(HTTPD_ERROR, "message : %s\r\n", error["message"].asString().c_str());
			}	
		}
	}

	content = writer.write(list_drv);
	
	pai_r.updatelog.save(success == true ? UL_RESULT_TYPE_SUCCESS : UL_RESULT_TYPE_API_ERROR, "", "",content.c_str());

	if(!success){
		httpd_cfg.time_interval = DEF_SERVER_REGISTRATION_FAILED_INTERVAL;
#ifdef MSGQ_PAIR_LOC_Q_IN_IDKEY
		datool_ipc_msgsnd2(m_http_msg_q_out_id, (long)QMSG_SOUND, kMixwavDriverecorder_registration_failed);
#endif		
	}
	return 0;
}

 int pai_r_insert_operation_func(CHttp_multipart *p, const char *data, int length)
{
	Json::Value list_opr;
	bool success = false;
	
	if(_http_json_parse(data, list_opr) == 0) {
		pai_r.updatelog.save(UL_RESULT_TYPE_API_ERROR, "", "",data);
		return 0;
	}
	
	if(list_opr.isObject()){
		if((list_opr["IsSuccess"].isBool() && list_opr["IsSuccess"].asBool() ) || (list_opr["is_success"].isBool() && list_opr["is_success"].asBool())){
			Json::Value api_list = list_opr.get("api_list", list_opr);

			pai_r.api_list_opr = list_opr;
			
			dprintf(HTTPD_INFO, "oper json - type : %d\r\n", api_list.type());
			
#if 1
			dprintf(HTTPD_INFO, "operation_autoid : %u\r\n", pai_r.api_list_opr["operation_autoid"].asUInt());
			dprintf(HTTPD_INFO, "now_datetime : %s\r\n", pai_r.api_list_opr["now_datetime"].asString().c_str());

			
			dprintf(HTTPD_INFO, "insert_driverecorder : %s\r\n", api_list["insert_driverecorder"].asString().c_str());
			dprintf(HTTPD_INFO, "insert_operation : %s\r\n",  api_list["insert_operation"].asString().c_str());
			dprintf(HTTPD_INFO, "insert_list_and_request : %s\r\n", api_list["insert_list_and_request"].asString().c_str());
			dprintf(HTTPD_INFO, "insert_movie : %s\r\n", api_list["insert_movie"].asString().c_str());
#endif	

			// time set
			{
				time_t     t_rtc;
				const char * value = pai_r.api_list_opr["now_datetime"].asString().c_str();
				time_t pai_r_server_time_t = pai_r_time_string_to_time(value) + 1;

				printf("SERVER Time : %ld\n", (long) pai_r_server_time_t);
				// Get current time
			    time(&t_rtc);
				printf("RTC Time : %ld\n", (long) t_rtc);
				
				if((value && strlen(value) && pai_r_server_time_t > pai_r_time_string_to_time("2020-01-01 09:00:00")) && (pai_r_server_time_t > t_rtc + 15 ||pai_r_server_time_t < t_rtc - 15))
				{
					char szCmd[64];
					SYSTEMLOG(LOG_TYPE_SYSTEM, LOG_EVENT_DATETIMECHANGE , RTC_SRC_SETUP_SERVER, "Before");
					sprintf(szCmd, "date -s \"%s\"", make_time_string(pai_r_server_time_t).c_str());
					system(szCmd);

					//cfg_time.nTimeSet = 0;
					//CConfigText::Save(&cfg_time);

					system("hwclock -w");
					SYSTEMLOG(LOG_TYPE_SYSTEM, LOG_EVENT_DATETIMECHANGE , RTC_SRC_SETUP_SERVER, "After");
				}
			}

#ifdef MSGQ_PAIR_LOC_Q_IN_IDKEY
			ST_QMSG msg;
			msg.type = QMSG_HTTPD_OPERID;
			msg.data = pai_r.api_list_opr["operation_autoid"].asUInt();
			msg.data2 = 0;
			msg.time = (u32)time(0);
			strcpy(msg.string, pai_r.strUUID);
			datool_ipc_msgsnd(m_http_msg_q_out_id, (void *)&msg, sizeof(msg));
			
			datool_ipc_msgsnd2(m_http_msg_q_out_id, (long)QMSG_SOUND, kMixwaveDingdong); // kMixwaveDingdong;
			datool_ipc_msgsnd2(m_http_msg_q_out_id, (long)QMSG_LED, WIFI_LED_SERVER_OK);		
#endif
			httpd_cfg.server_connected = 1;	
			success = true;

			//gsensor setup
			Json::Value sensor;
			_http_json_sensor_parse(list_opr.get("sensor", list_opr));
		}else{
			Json::Value error = list_opr.get("error", list_opr);

			if(error.isObject()){
				//int error_code = error.get("code", &error);
				
				if(error["message"].isString())
					dprintf(HTTPD_ERROR, "message : %s\r\n", error["message"].asString().c_str());
			}
		}
	}

	Json::FastWriter writer;
	std::string content = writer.write(list_opr);
	
	pai_r.updatelog.save(success == true ? UL_RESULT_TYPE_SUCCESS : UL_RESULT_TYPE_API_ERROR, "", "",content.c_str());

	if(!success){
		httpd_cfg.time_interval = DEF_SERVER_REGISTRATION_FAILED_INTERVAL;
#ifdef MSGQ_PAIR_LOC_Q_IN_IDKEY
		datool_ipc_msgsnd2(m_http_msg_q_out_id, (long)QMSG_SOUND, kMixwavDriverecorder_registration_failed);
#endif		
	}
	
	return 0;
}

int pai_r_insert_list_and_request_func(CHttp_multipart *p, const char *data, int length)
{
		u32 i, j;
		Json::Value insert_list;
		bool success = false;

	if(_http_json_parse(data, insert_list) == 0){
		pai_r.updatelog.save(UL_RESULT_TYPE_API_ERROR, "", "", data);
		return 0;
	}

	pai_r.request = insert_list;
	
	if(pai_r.request.isObject()){
		//dprintf(HTTPD_INFO, "request_driverecorder_autoid : %s\r\n", pai_r.requet["request_driverecorder_autoid"].asCString());
		//dprintf(HTTPD_INFO, "cancel_driverecorder_autoid : %s\r\n", pai_r.requet["cancel_driverecorder_autoid"].asCString());

		if((pai_r.request["IsSuccess"].isBool()&& pai_r.request["IsSuccess"].asBool()) || (pai_r.request["is_success"].isBool() && pai_r.request["is_success"].asBool()) == true){
			//{"IsSuccess":true,"error":{"code":0},"request_driverecorder_autoid":[],"cancel_driverecorder_autoid":[],"sensor":{"level":2,"update_datetime":"2019-10-11 15:16:23"}}
			const char * str_req = "request_driverecorder_autoid";
			const char * str_cancel = "cancel_driverecorder_autoid";
			
			_http_pa_r_insert_list_and_request_queue_clear();
			success = true;

			if(pai_r.request[str_req].isArray()){
				for(i = 0; i < pai_r.request[str_req].size(); i++){
					u32 id = pai_r.request[str_req].get(i, Json::Value(0)).asUInt();

					httpd_addRequestMovieID(id, true);
				}
			}
			
			if(pai_r.request[str_cancel].isArray()){
				for(i = 0; i < pai_r.request[str_cancel].size(); i++){
					u32 id = pai_r.request[str_cancel].get(i, Json::Value(0)).asUInt();

					REQUEST_ID_POOL * list = &pai_r.request_list_unique_autoid;
					int count;
					ITER_REQUEST_ID iData;

					dprintf(HTTPD_INFO, "cancel id (%d).\r\n", id);
					
					count = list->size();
					iData = list->begin();
	
					for(j = 0; j < count; j++, iData++){
						if(*iData == id){
							list->erase(iData);
							dprintf(HTTPD_INFO, "erase cancel id (%d).\r\n", id);
							break;
						}
					}				
				}
			}

			//gsensor setup
			Json::Value sensor;
			_http_json_sensor_parse(pai_r.request.get("sensor", insert_list));
		}
		else {
			Json::Value error = pai_r.request.get("error", pai_r.request);

			if(error.isObject()){
				//int error_code = error.get("code", &error);
				
				if(error["message"].isString())
					dprintf(HTTPD_ERROR, "message : %s\r\n", error["message"].asString().c_str());


				//
				if(error["code"].asInt() == 40) {//There is no location associated with specified operation_autoid or driverecorder_autoid.
					_http_pa_r_insert_list_and_request_queue_clear();
				}
				else if(pai_r_data.m_tmp_pop_count) {
					if(httpd_cfg.server_wait_success >= DEF_MAX_SERVER_ERROR_COUNT){
						//pai_r_data.Location_queue_clear(pai_r_data.m_tmp_pop_count);
						//pai_r_data.m_tmp_pop_count = 0;
					}
				}
				else if(pai_r.request_list_unique_autoid.size()){
					if(httpd_cfg.server_wait_success_movie++ >= DEF_MAX_SERVER_ERROR_COUNT/2){
						httpd_cfg.server_wait_success_movie = 0;
						dprintf(HTTPD_ERROR, "server response error. deleted insert_movie file id %d. \r\n", pai_r.request_list_unique_autoid.front());
						pai_r.request_list_unique_autoid.pop_front();
					}
				}		
				else if(pai_r.request_list_temp.size()){
					if(httpd_cfg.server_wait_success_movie++ >= DEF_MAX_SERVER_ERROR_COUNT/2){
						httpd_cfg.server_wait_success_movie = 0;
						dprintf(HTTPD_ERROR, "server response error. deleted insert_movie file id %d. \r\n", pai_r.request_list_temp.front());
						pai_r.request_list_temp.pop_front();
					}
				}
			}
		}
	}

	Json::FastWriter writer;
	std::string content = writer.write(insert_list);
	
	pai_r.updatelog.save(success == true ? UL_RESULT_TYPE_SUCCESS : UL_RESULT_TYPE_API_ERROR, "", "", content.c_str());

	if(success){
		httpd_cfg.time_interval = 1000;
	}
	else {
		httpd_cfg.time_interval = DEF_SERVER_TIMEOUT_INTERVAL;
	}
	
	return 0;
}

int http_pai_r_insert_recorder(const char *host, int port, const char * url, int is_ssl, const char * pai_r_id, const char * ssid)
{
	char mac_addr[64] = { 0,};
	char server_url[256] = { 0,};
	char server_name[128] = { 0,};
	int server_port;
	int result;

	if(!pai_r_data.m_queue_index_init){
		return 0;
	}
	
	pai_r.updatelog.make(UL_DATA_TYPE_AUTHENTICATION, pai_r_data.Location_queue_auto_id_out_get());
	httpd_server_error_check();

	//dprintf(HTTPD_INFO, "insert_recorder url : %s\r\n", url);
	_http_host_string_parser(url, host, port, server_url, server_name, &server_port);

	CHttp_multipart http2(server_name, server_port, is_ssl, "POST", server_url,  "----WebKitFormBoundary7MA4YWxkTrZu0gW");		
	if(http2.make_contents(STR_COMPANY_ID, pai_r_id, NULL, CHttp_multipart::HTTP_MP_BUFFER) == -1){
		pai_r.updatelog.save(UL_RESULT_TYPE_CONNECTION_FAILURE, "Connection failure", http2.last_error_buf);
		return -1;
	}

	http2.make_contents("ssid", ssid, NULL, CHttp_multipart::HTTP_MP_BUFFER);
		
//	if(httpd_cfg.serial_no != 2) //인증 받아야  접속 됨
		SB_GetMacAddress(mac_addr);
//	else //for test
//		strcpy(mac_addr, "70:3E:AC:EE:8B:17");
		
	http2.make_contents("mac", mac_addr, NULL, CHttp_multipart::HTTP_MP_BUFFER);
	http2.make_contents("sensor_level", format_string("%d", pai_r.cfg.iGsensorSensi).c_str(), NULL, CHttp_multipart::HTTP_MP_BUFFER);
	http2.make_contents("sensor_datetime", pai_r.cfg.strLastSetupTime, NULL, CHttp_multipart::HTTP_MP_BUFFER);
	result = http2.make_contents("environment", pai_r.cfg.strFWVersion, pai_r_insert_driverecorder_func, CHttp_multipart::HTTP_MP_SEND);

	if(result == -1)
		pai_r.updatelog.save(UL_RESULT_TYPE_CONNECTION_FAILURE, "Timeout", http2.last_error_buf);
	
	return result;
}


 int http_pai_r_insert_operation(const char *host, int port, int is_ssl, time_t start_time, const char * uuid)
{
	int result;
	char mac_addr[64] = { 0,};
	char server_url[256] = { 0,};
	char server_name[128] = { 0,};
	int server_port;

	Json::Value api_list = pai_r.api_list_drv.get("api_list", pai_r.api_list_drv);
	
	pai_r.updatelog.make(UL_DATA_TYPE_AUTHENTICATION, pai_r_data.Location_queue_auto_id_out_get());
	httpd_server_error_check();
	
	if(!api_list.size()){
		dprintf(HTTPD_ERROR, "not find insert_operation server_url!. try insert_driverecorder.\r\n");
		return 0;
	}

	//dprintf(HTTPD_INFO, "insert_operation url : %s\r\n", api_list["insert_operation"].asString().c_str());
	_http_host_string_parser(api_list["insert_operation"].asString().c_str(), host, port, server_url, server_name, &server_port);
	
	
	CHttp_multipart http2(server_name, server_port, is_ssl, "POST", server_url,  "----WebKitFormBoundary7MA4YWxkTrZu0gW");		

//	if(strlen(pai_r.strPai_r_id) > 1) // 10 이상  , //인증 받아야  접속 됨
		SB_GetMacAddress(mac_addr);
//	else //for test
//		strcpy(mac_addr, "70:3E:AC:EE:8B:17");

	if(http2.make_contents("mac", mac_addr, NULL, CHttp_multipart::HTTP_MP_BUFFER) == -1){
		pai_r.updatelog.save(UL_RESULT_TYPE_CONNECTION_FAILURE, "Connection failure", http2.last_error_buf);
		return -1;
	}
	http2.make_contents("environment", pai_r.cfg.strFWVersion, NULL, CHttp_multipart::HTTP_MP_BUFFER);
	http2.make_contents("create_datetime", make_time_string(start_time).c_str(), NULL, CHttp_multipart::HTTP_MP_BUFFER);
	http2.make_contents("local_autoid", uuid, NULL, CHttp_multipart::HTTP_MP_BUFFER);
	http2.make_contents("sensor_level", format_string("%d", pai_r.cfg.iGsensorSensi).c_str(), NULL, CHttp_multipart::HTTP_MP_BUFFER);
	http2.make_contents("sensor_datetime", pai_r.cfg.strLastSetupTime, NULL, CHttp_multipart::HTTP_MP_BUFFER);
	result = http2.make_contents("tel", pai_r.cfg.strTelNumber[0], pai_r_insert_operation_func, CHttp_multipart::HTTP_MP_SEND);

	if(result == -1)
		pai_r.updatelog.save(UL_RESULT_TYPE_CONNECTION_FAILURE, "Timeout", http2.last_error_buf);
	
	if(pai_r.api_list_opr.isObject()){
		return result;
	}
	return 0;
}

 int http_pai_r_insert_list_and_request(const char *host, int port, int is_ssl, time_t start_time, const char * operation_autoid, Json::Value &location, const char * file_path)
{
	int result;
	char mac_addr[64] = { 0,};
	char server_url[256] = { 0,};
	char server_name[128] = { 0,};
	int server_port;

	Json::Value api_list = pai_r.api_list_opr.get("api_list", pai_r.api_list_opr);
	
	pai_r.updatelog.make(UL_DATA_TYPE_LOCATION, pai_r_data.Location_queue_auto_id_out_get());
	httpd_server_error_check();
	
	if(!api_list.size()){
		dprintf(HTTPD_ERROR, "not find insert_list_and_request url!. try insert_operation.\r\n");
		return 0;
	}

	//dprintf(HTTPD_INFO, "insert_list_and_request url : %s\r\n", api_list["insert_list_and_request"].asString().c_str());
	_http_host_string_parser(api_list["insert_list_and_request"].asString().c_str(), host, port, server_url, server_name, &server_port);
	
	CHttp_multipart http2(server_name, server_port, is_ssl, "POST", server_url,  "----WebKitFormBoundary7MA4YWxkTrZu0gW");		

//	if(httpd_cfg.serial_no != 2) //인증 받아야  접속 됨
		SB_GetMacAddress(mac_addr);
//	else //for test
//		strcpy(mac_addr, "70:3E:AC:EE:8B:17");

	if(http2.make_contents("mac", mac_addr, NULL, CHttp_multipart::HTTP_MP_BUFFER) == -1){
		pai_r.updatelog.save(UL_RESULT_TYPE_CONNECTION_FAILURE, "Connection failure", http2.last_error_buf);
		return -1;
	}
	http2.make_contents("environment", pai_r.cfg.strFWVersion, NULL, CHttp_multipart::HTTP_MP_BUFFER);
	http2.make_contents("operation_autoid", operation_autoid, NULL, CHttp_multipart::HTTP_MP_BUFFER);

	Json::FastWriter writer;
	std::string content = writer.write(location);
	http2.make_contents("location", content.c_str(), NULL, CHttp_multipart::HTTP_MP_BUFFER);

	//add file send function
	Json::Value form_file;
	form_file["name"] = "file";
	form_file["type"] = "zip";
	form_file["path"] = file_path;
	form_file["file_hash"] = "md5";
	http2.make_contents(form_file, NULL, CHttp_multipart::HTTP_MP_BUFFER);
	
	//http2.send("file_hash", (const char *)md5_hash, NULL, CHttp_multipart::HTTP_MP_BUFFER);
	
	http2.make_contents("sensor_level", format_string("%d", pai_r.cfg.iGsensorSensi).c_str(), NULL, CHttp_multipart::HTTP_MP_BUFFER);
	result = http2.make_contents("sensor_datetime", pai_r.cfg.strLastSetupTime, pai_r_insert_list_and_request_func, CHttp_multipart::HTTP_MP_BUFFER);

	if(result == -1)
		pai_r.updatelog.save(UL_RESULT_TYPE_CONNECTION_FAILURE, "Timeout", http2.last_error_buf);
	
	return result;
}

 int http_pai_r_insert_list_and_request2(const char *host, int port, int is_ssl, LOCATION_INFO &info)
{
	int result;
	char mac_addr[64] = { 0,};
	char server_url[256] = { 0,};
	char server_name[128] = { 0,};
	int server_port;

	Json::Value api_list = pai_r.api_list_opr.get("api_list", pai_r.api_list_opr);

	std::string operation_autoid;
	std::string file_name = format_string("speed%d", info.file_auto_id);

	if(info.operation_id)
		operation_autoid = format_string("%u", info.operation_id);
	else if(strlen(info.operation_uuid))
		operation_autoid = format_string("%s", info.operation_uuid);
	else {
		operation_autoid = format_string("%s", pai_r.strUUID);
		dprintf(HTTPD_INFO, " warning!! operation autoid empty. use to current UUID(%s).\r\n", pai_r.strUUID);
	}
	
	
	pai_r.updatelog.make(UL_DATA_TYPE_LOCATION, info.file_auto_id, info.create_time);
	httpd_server_error_check();
	
	if(!api_list.size()){
		dprintf(HTTPD_ERROR, "not find insert_list_and_request url!. try insert_operation.\r\n");
		return 0;
	}


	//dprintf(HTTPD_INFO, "%s\r\n %s\r\n", info.loc.c_str(), info.speed.c_str());
	
	//dprintf(HTTPD_INFO, "insert_list_and_request url : %s\r\n", api_list["insert_list_and_request"].asString().c_str());
	_http_host_string_parser(api_list["insert_list_and_request"].asString().c_str(), host, port, server_url, server_name, &server_port);
	
	CHttp_multipart http2(server_name, server_port, is_ssl, "POST", server_url,  "----WebKitFormBoundary7MA4YWxkTrZu0gW");		

//	if(httpd_cfg.serial_no != 2) //인증 받아야  접속 됨
		SB_GetMacAddress(mac_addr);
//	else //for test
//		strcpy(mac_addr, "70:3E:AC:EE:8B:17");

	if(http2.make_contents("mac", mac_addr, NULL, CHttp_multipart::HTTP_MP_BUFFER) == -1){
		pai_r.updatelog.save(UL_RESULT_TYPE_CONNECTION_FAILURE, "Connection failure", http2.last_error_buf);
		return -1;
	}
	http2.make_contents("environment", pai_r.cfg.strFWVersion, NULL, CHttp_multipart::HTTP_MP_BUFFER);
	http2.make_contents("operation_autoid", (const char *)operation_autoid.c_str(), NULL, CHttp_multipart::HTTP_MP_BUFFER);

	http2.make_contents("location", (const char *)info.loc.c_str(), NULL, CHttp_multipart::HTTP_MP_BUFFER);

	//add file send function
	Json::Value form_file;
	form_file["name"] = "file";
	form_file["type"] = "zip";
	form_file["file_name"] = (const char *)file_name.c_str();
	form_file["file_ext"] = "csv";
	form_file["file_hash"] = "md5";
	http2.make_contents(form_file, (const void *)info.speed.c_str(), strlen(info.speed.c_str()), NULL, CHttp_multipart::HTTP_MP_BUFFER);
	
	//http2.send("file_hash", (const char *)md5_hash, NULL, CHttp_multipart::HTTP_MP_BUFFER);
	
	http2.make_contents("sensor_level", format_string("%d", pai_r.cfg.iGsensorSensi).c_str(), NULL, CHttp_multipart::HTTP_MP_BUFFER);
	result = http2.make_contents("sensor_datetime", pai_r.cfg.strLastSetupTime, pai_r_insert_list_and_request_func, CHttp_multipart::HTTP_MP_SEND);

	if(result == -1)
		pai_r.updatelog.save(UL_RESULT_TYPE_CONNECTION_FAILURE, "Timeout", http2.last_error_buf);
	
	return result;
}

int http_pai_r_insert_movie(const char *host, int port, int is_ssl, u32 queue_no, bool is_unique_autoid)
{
	int result;
	char mac_addr[64] = { 0,};
	char server_url[256] = { 0,};
	char server_name[128] = { 0,};
	int server_port;

	httpd_server_error_check();
	
	if(!pai_r.api_list_opr.size()){
		dprintf(HTTPD_ERROR, "not find insert_operation url!. try insert_driverecorder.\r\n");
		return 0;
	}

	Json::Value api_list = pai_r.api_list_opr.get("api_list", pai_r.api_list_opr);

	//dprintf(HTTPD_INFO, "insert_movie url : %s\r\n", api_list["insert_movie"].asString().c_str());
	_http_host_string_parser(api_list["insert_movie"].asString().c_str(), host, port, server_url, server_name, &server_port);

	if(strlen(server_url) == 0){
		dprintf(HTTPD_ERROR, "error url!. try insert_driverecorder.\r\n");
		return 0;
	}

//	if(httpd_cfg.serial_no != 2) //인증 받아야  접속 됨
		SB_GetMacAddress(mac_addr);
//	else //for test
//		strcpy(mac_addr, "70:3E:AC:EE:8B:17");

	u32 status = 90; //동영상이 드라이브 레코더에서 삭제된 경우"90:오류:블랙 박스에 해당 데이터는 더 이상 존재하지 않는다"
	char operation_autoid[64] = { 0,};
	u32 unique_autoid = queue_no + pai_r_data.m_local_auto_id_offset;
	
	PAI_R_BACKUP_DATA *pLoc = (PAI_R_BACKUP_DATA *)new char [sizeof(PAI_R_BACKUP_DATA)];

	memset((void *)pLoc, 0, sizeof(PAI_R_BACKUP_DATA));

	if(is_unique_autoid) {
		unique_autoid = queue_no;
		
		if(queue_no > pai_r_data.m_local_auto_id_offset)
			queue_no -= pai_r_data.m_local_auto_id_offset;
		else
			queue_no = 0;
	}
	
	if(pai_r_data.Location_pop(pLoc, queue_no, eUserDataType_Normal)){
		u32 index = pLoc->location.autoid;
		//if(is_unique_autoid == eUserDataType_Event){
		//	index= pLoc->event_autoid;
		//}
		
		if(index == queue_no){
			if(unique_autoid != pLoc->location.unique_local_autoid){
				dprintf(HTTPD_INFO, "%s() : error!! movie unique local autoid is different.(%d:%d)\r\n", __func__,  unique_autoid, pLoc->location.unique_local_autoid);
			}

			if(access(pLoc->location.file_path, R_OK ) == 0){
					status =  MOVIE_STATUS_FILE_EXIST; // 업로드하는 동영상이 존재할 경우"4:업로드 완료"를 보내
			}
		}else{
			dprintf(HTTPD_INFO, "%s() : error!! movie queue_no is different.(%d:%d)\r\n", __func__,  index, queue_no);
		}
	}
	else{
		dprintf(HTTPD_INFO, "%s() : error!! not find location info : %d(queue_no %d)\r\n", __func__,  queue_no, queue_no);
	}
	
	CHttp_multipart http3(server_name, server_port, is_ssl, "POST", server_url,  "----WebKitFormBoundary7MA4YWxkTrZu0gW");	

	const char * file_name = strrchr(pLoc->location.file_path, '/');
	time_t movie_time = 0;

	if(file_name != NULL){
  		file_name++;

		movie_time = recording_time_string_to_time(file_name);
	}

	pai_r.updatelog.make(UL_DATA_TYPE_MOVIE, queue_no, movie_time);
	
	if(pLoc->operation_id){
		sprintf(operation_autoid, "%u", pLoc->operation_id);
		unique_autoid = pLoc->location.unique_local_autoid;
	}
	else if(strlen(pLoc->operation_uuid)){
		sprintf(operation_autoid, "%s", pLoc->operation_uuid);
		unique_autoid = pLoc->location.unique_local_autoid;
	}
	else {
		sprintf(operation_autoid, "%u", pai_r.api_list_opr["operation_autoid"].asUInt());
		//pai_r.updatelog.save(UL_RESULT_TYPE_API_ERROR, "not find location data!", NULL);
		//pai_r.request_list_temp.pop_front();
		//delete[] pLoc;
		//return -1;
	}

	//////////////////////////////////////////////////////////////////
	http3.update_contents_length("mac", mac_addr);
	http3.update_contents_length("environment", pai_r.cfg.strFWVersion);
	http3.update_contents_length("operation_autoid", operation_autoid);
	http3.update_contents_length("driverecorder_autoid", format_string("%u", unique_autoid).c_str());
	//http3.update_contents_length("driverecorder_autoid", format_string("%u", pLoc->location.unique_local_autoid).c_str()); 
	http3.update_contents_length("status", format_string("%d", status).c_str());

	if(status == MOVIE_STATUS_FILE_EXIST){
		http3.update_contents_length(pLoc->location.file_path);
		http3.update_contents_length(&pLoc->location.thumbnail);
	}
	
	http3.update_contents_length("sensor_level", format_string("%d", pai_r.cfg.iGsensorSensi).c_str());
	http3.update_contents_length("sensor_datetime", pai_r.cfg.strLastSetupTime);
	
	////////////////////////////////////////////////////////////////////
	if(http3.request("mac", mac_addr) == -1){
		pai_r.updatelog.save(UL_RESULT_TYPE_CONNECTION_FAILURE, "Connection failure", NULL);
		delete[] pLoc;
		return -1;
	}
	
	http3.request("environment", pai_r.cfg.strFWVersion);
	http3.request("operation_autoid", operation_autoid);
	http3.request("driverecorder_autoid", format_string("%u", unique_autoid).c_str());
	//http3.request("driverecorder_autoid", format_string("%u", pLoc->location.unique_local_autoid).c_str());
	http3.request("status", format_string("%d", status).c_str());

	char file_hash[33] = {0,};
	
	if(status == MOVIE_STATUS_FILE_EXIST){
		http3.request(pLoc->location.file_path, file_hash, sizeof(file_hash));
		http3.request(&pLoc->location.thumbnail);
	}

	http3.request("sensor_level", format_string("%d", pai_r.cfg.iGsensorSensi).c_str());
	http3.request("sensor_datetime", pai_r.cfg.strLastSetupTime);
	
	result = http3.request_end(pai_r_insert_list_and_request_func, 40);

	if(result == -1)
		pai_r.updatelog.save(UL_RESULT_TYPE_CONNECTION_FAILURE, "Timeout", http3.last_error_buf);


	if(status == MOVIE_STATUS_FILE_EXIST){
		if(strlen(pLoc->file_hash) == 0) {
			//pai_r_data.update_file_hash(queue_no, (const char *)file_hash, is_unique_autoid);

#ifdef MSGQ_PAIR_LOC_Q_OUT_IDKEY		
			ST_QMSG msg;
			msg.type = QMSG_UP_FILE_HASH;
			msg.data = eUserDataType_Normal;
			msg.data2 = queue_no;
			msg.time = (u32)time(0);
			strcpy(msg.string, file_hash);
			datool_ipc_msgsnd(m_http_msg_q_out_id, (void *)&msg, sizeof(msg));
#endif
		}
	}
	
	delete[] pLoc;
	
	dprintf(HTTPD_INFO, "%s() : %d:%d \r\n", __func__, http3.m_hd.content_length, http3.m_contents_length);
	
	if(pai_r.api_list_opr.isObject()){
		return result;
	}
	return 0;
}

int httpd_insert_list_send(CPai_r_data &pai_r_data, bool send_require)
{
	const char * host = DEF_HOST_ADDR;
	const char * driverecorder_api_url = DEF_DRIVERECORDER_URL;
	int host_port = DEF_HOST_PORT;
	int is_ssl = DEF_HOST_SSL_USE;

	if(strlen(pai_r.cfg.strCloudServerName) && strlen(pai_r.cfg.strCloudServerApiUrl) && pai_r.cfg.iCloudServerPort ){
		host = (const char *)pai_r.cfg.strCloudServerName;
		driverecorder_api_url = (const char *)pai_r.cfg.strCloudServerApiUrl;
		host_port = pai_r.cfg.iCloudServerPort;
		is_ssl = pai_r.cfg.bCloudServerSSL;
	}
		
	if(pai_r.cfg.iDebugServer_PORT){
		Json::Value api_list;
#if 0
		api_list["insert_list_and_request"] 	= "/driverecorder/api_driverecorder/firstview/1/insert_list_and_request.php";
		api_list["insert_movie"] 						= "/driverecorder/api_driverecorder/firstview/1/insert_movie.php";

		pai_r.api_list_drv["api_list"] = api_list;
		pai_r.api_list_opr["api_list"] = api_list;
#endif
	  host = pai_r.cfg.strApplication_IP;
	  host_port = pai_r.cfg.iDebugServer_PORT;
	}
	
	if(!pai_r.api_list_drv["api_list"].size()){
		static bool list_drv_init = false;
		
		httpd_cfg_get(&pai_r.cfg);
		strcpy(pai_r.strPai_r_id, pai_r.cfg.strSeriarNo);


		if(list_drv_init == false){
			pai_r_backup_list_drv_read(pai_r.api_list_drv, host, host_port, driverecorder_api_url);
			list_drv_init = true;
		}

		if(!pai_r.api_list_drv["api_list"].size()) {
			pai_r.api_list_drv["server_url"] = format_string("%s:%d%s", host, host_port, driverecorder_api_url ).c_str();
			http_pai_r_insert_recorder(host, host_port, driverecorder_api_url, is_ssl, (const char *)pai_r.cfg.strSeriarNo, (const char *)pai_r.cfg.strApSsid);
		}
	}
	
	if(pai_r.api_list_drv["api_list"].size() && !pai_r.api_list_opr["api_list"].size()){
		//(const char *host, int port, int is_ssl, time_t start_time, const char * uuid, int gsensor_level, time_t seensor_datetime)
		if(strlen(pai_r.strUUID) == 0){
			strcpy(pai_r.strUUID, datool_generateUUID().c_str());
		}
		
		http_pai_r_insert_operation(host, host_port, is_ssl, time(0), pai_r.strUUID);
	}

	
	if(pai_r.api_list_opr["api_list"].size())
	{
#if 1
		//(const char *host, int port, int is_ssl, time_t start_time, const char * operation_autoid, Json::Value &location, const char * file_path, int gsensor_level, time_t seensor_datetime)
		
		if(!pai_r.api_list_opr["operation_autoid"].isUInt()){
			dprintf(HTTPD_ERROR, "api_list_opr error\r\n");
			pai_r.api_list_opr["operation_autoid"] = 1;

			//Json::Value api_list = pai_r.api_list_opr.get("api_list", pai_r.api_list_opr);
			//pai_r.api_list_opr["insert_list_and_request"] = "/driverecorder/api_driverecorder/firstview/1/insert_list_and_request.php";
			dprintf(HTTPD_INFO, "operation autoid : %s\r\n", format_string("%u", pai_r.api_list_opr["operation_autoid"].asUInt()).c_str());
		}
#if 0
		PAI_R_LOCATION loc;
			strncpy(loc.file_type, "I2", sizeof(loc.file_type));
		loc.create_time = time(0);
		loc.move_filesize = 12345;
		loc.latitude = 37.233;
		loc.longitude = 127.235;
		loc.accurate = 10;
		loc.direction = 180;
		loc.altitude= 100;
		loc.autoid = 1;
	
		
		pai_r_data.addLocation(loc);
		
		for(int i=0; i < 10; i++){
			PAI_R_SPEED_CSV spd= {time(0), get_tick_count(), 10 * i, 100 * i };
			pai_r_data.addSpeed(spd);
		}
#endif			
		if(pai_r_data.Location_queue_count()) {
 			LOCATION_INFO info;
			if(pai_r_data.BackupLocationGetJson(info))
			{
				//http_pai_r_insert_movie(host, host_port, 0, operation_autoid.c_str(), info.file_auto_id);  //for test
				if(http_pai_r_insert_list_and_request2(host, host_port, is_ssl, info) > 0) {
					if(info.is_event) {
						httpd_addRequestMovieID(info.queue_no, false);
					}
				}
			}
		}

		if(pai_r_data.Location_queue_count() == 0) {
			if(pai_r.request_list_unique_autoid.size()){
				u32 unique_autoid = pai_r.request_list_unique_autoid.front();
				dprintf(HTTPD_INFO, " movie file send : unique autoid no : %u\r\n", unique_autoid);
				http_pai_r_insert_movie(host, host_port, is_ssl, unique_autoid, true);
			}
			else
			if(pai_r.request_list_temp.size())
			{
				u32 queue_no = pai_r.request_list_temp.front();
				dprintf(HTTPD_INFO, " movie file send : queue no : %d\r\n", queue_no);
				http_pai_r_insert_movie(host, host_port, is_ssl, queue_no, false);
			}
		}
#else
		//(const char *host, int port, int is_ssl, time_t start_time, const char * operation_autoid, Json::Value &location, const char * file_path, int gsensor_level, time_t seensor_datetime)
		const char *file_path = "/mnt/extsd/speed1.csv";
		const char *json_doc = "[{\"driverecorder_autoid\":\"1\",\"create_datetime\":\"2019-09-08 00:08:00.598\",\"movie_filesize\":6907955,\"g_sensortype_id\":0,\"latitude\":34.668555,\"longitude\":135.518744,\"accurate\":10,\"direction\":90,\"altitude\":6}]";
		Json::Value location;
		Json::Reader reader;
		if(!reader.parse(json_doc, location))
			dprintf(HTTPD_ERROR, "json error\r\n");
		
		if(!pai_r.api_list_opr["operation_autoid"].isString()){
			dprintf(HTTPD_ERROR, "api_list_opr error\r\n");
			pai_r.api_list_opr["operation_autoid"] = "1";

			Json::Value api_list = pai_r.api_list_opr.get("api_list", pai_r.api_list_opr);
			//pai_r.api_list_opr["insert_list_and_request"] = "/driverecorder/api_driverecorder/firstview/1/insert_list_and_request.php";
		}
		dprintf(HTTPD_INFO, "autoid : %s\r\n", pai_r.api_list_opr["operation_autoid"].asString().c_str());
		http_pai_r_insert_list_and_request(host, host_port, 0, time(0), format_string("%u", pai_r.api_list_opr["operation_autoid"].asUInt()).c_str(), location, file_path, 2, time(0));
#endif			
	}


	//file_hash_update_list_check();
#if 0		
	char mac_addr[64] = { 0,};

	SB_GetMacAddress(mac_addr);

	//CHttp_multipart http2("192.168.35.19", 4000, 0, "POST", "/getLive",  "----WebKitFormBoundary7MA4YWxkTrZu0gW");		
	//http2.send("cmd", "snapshot", NULL, 1);
	
	CHttp_multipart http2("mar-i.com", 80, 0, "POST", "/driverecorder/api_driverecorder/firstview/1/insert_driverecorder.php",  "----WebKitFormBoundary7MA4YWxkTrZu0gW");		
	http2.send("company_pai_r_id", "1", NULL, CHttp_multipart::HTTP_MP_BUFFER);
	http2.send("ssid", "test_wifi16", NULL, CHttp_multipart::HTTP_MP_BUFFER);
	http2.send("mac", /*mac_addr*/"70:3E:AC:EE:8B:10", NULL, CHttp_multipart::HTTP_MP_BUFFER);
	http2.send("sensor_level", "2", NULL, CHttp_multipart::HTTP_MP_BUFFER);
	http2.send("sensor_datetime", make_time_string(time(NULL)).c_str(), NULL, CHttp_multipart::HTTP_MP_BUFFER);
	http2.send("environment", pai_r.cfg.strFWVersion, pai_r_insert_driverecorder_func, CHttp_multipart::HTTP_MP_SEND);

	dprintf(HTTPD_INFO, "type : %d\r\n", pai_r.api_list_drv.type());
	
	if(pai_r.api_list_drv.isObject()){
		Json::Value api_list = pai_r.api_list_drv.get("api_list", &pai_r.api_list_drv);
		
		dprintf(HTTPD_INFO, "insert_driverecorder : %s\r\n", api_list["insert_driverecorder"].asCString());
		dprintf(HTTPD_INFO, "insert_operation : %s\r\n",  api_list["insert_operation"].asCString());
		dprintf(HTTPD_INFO, "insert_list_and_request : %s\r\n", api_list["insert_list_and_request"].asCString());
		dprintf(HTTPD_INFO, "insert_movie : %s\r\n", api_list["insert_movie"].asCString());
	}
#endif
#if 0
	CHttp_multipart http("mar-i.com", 80, 0, "POST", "/driverecorder/api_driverecorder/firstview/1/insert_list_and_request.php",  "----WebKitFormBoundary7MA4YWxkTrZu0gW");
	http.send("mac", /*mac_addr*/"70:3E:AC:EE:8B:10", NULL, 0);
	http.send("environment", "Tester/1.0.1", NULL, 0);
	http.send("operation_autoid", "1", NULL, 0);
	http.send("location", "[{\"driverecorder_autoid\":\"1\",\"create_datetime\":\"2019-09-08 00:08:00.598\",\"movie_filesize\":6907955,\"g_sensortype_id\":0,\"latitude\":34.668555,\"longitude\":135.518744,\"accurate\":10,\"direction\":90,\"altitude\":6}]", NULL, 0);
	http.send("file\"; filename=\"speed1.zip\"\r\nContent-Type: application/zip", "70:3E:AC:EE:8B:10", NULL, 0);
	http.send("file_hash", "e12369f2735d340e757952a81a74f6c5", NULL, 0);
	http.send("sensor_level", "3", NULL, 0);
	http.send("sensor_datetime", make_time_string(time(NULL)).c_str(), pai_r_insert_list_and_request_func, 1);
#endif		

	return 0;
}

#endif // DEF_MMB_SERVER


int http_pai_r_information_send_to_app(const char *host, int port, int is_ssl, const char * pai_r_id, const char * ssid)
{
	char host_ip_addr[128] = { 0,};
	char mac_addr[64] = { 0,};
	char ip_addr[32] = { 0,};
	const char * str_url = "information";
	int result;

	if(!pai_r_data.m_queue_index_init){
			return 0;
	}
	
	getIPAddress(ip_addr);

	SYSTEMLOG(LOG_TYPE_COMM, LOG_COMM_WIFI, 0, "IP %s", ip_addr);
	
	if(strlen(host) == 0){
		char * last_ip;
		strcpy(host_ip_addr, ip_addr);
		last_ip = strrchr(host_ip_addr, '.');
		
		strcpy(last_ip, ".1");
	}
	else {
		strcpy(host_ip_addr, host);
	}

	if(port == 0)
		port = DEF_APPLICATION_PORT;
	
	dprintf(HTTPD_INFO, "url : http://%s:%d/%s\r\n", host_ip_addr, port, str_url);
	sysfs_printf("/mnt/extsd/System/ip_address.txt", "local IP address : %s\r\napplication IP address : %s:%d\r\n", ip_addr, host_ip_addr,port);
	
	pai_r.updatelog.make(UL_DATA_TYPE_INFORMATION, pai_r_data.Location_queue_auto_id_out_get(), time(0));
	
	CHttp_multipart http2(host_ip_addr, port, is_ssl, "POST", str_url,  "----WebKitFormBoundary7MA4YWxkTrZu0gW");	
//	if(httpd_cfg.serial_no != 2) //인증 받아야  접속 됨
		SB_GetMacAddress(mac_addr);
//	else //for test
//		strcpy(mac_addr, "70:3E:AC:EE:8B:17");

	if(http2.make_contents(STR_COMPANY_ID, pai_r_id, NULL, CHttp_multipart::HTTP_MP_BUFFER) == -1){
		pai_r.updatelog.save(UL_RESULT_TYPE_CONNECTION_FAILURE, "Connection failure", http2.last_error_buf);
		return -1;
	}
	
	http2.make_contents("ssid", ssid, NULL, CHttp_multipart::HTTP_MP_BUFFER);
	http2.make_contents("mac", mac_addr, NULL, CHttp_multipart::HTTP_MP_BUFFER);

	
	http2.make_contents("ip_address", ip_addr, NULL, CHttp_multipart::HTTP_MP_BUFFER);

	//http2.make_contents("is_connect", httpd_cfg.server_connected ? "true":"false", NULL, CHttp_multipart::HTTP_MP_BUFFER);
	http2.make_contents("now_datetime", make_time_string(time(0)).c_str(), NULL, CHttp_multipart::HTTP_MP_BUFFER);
	
	result = http2.make_contents("environment", pai_r.cfg.strFWVersion, pai_r_information_send_to_app, CHttp_multipart::HTTP_MP_SEND);

	if(result == -1)
		pai_r.updatelog.save(UL_RESULT_TYPE_CONNECTION_FAILURE, "Timeout", http2.last_error_buf);
	
	return result;
}

///////////////////////////////////////////////////////
#define MD5_STATIC static
#include "md5.inl"

typedef struct {
	key_value_map_t key_v;
	char name[128];
	char filename[128];

	bool file_mode;
	char filepath[128];
	
	char write_buffer[KBYTE(10)];
	int write_length;
	int write_filesize;
	char md5_string[33];
	md5_state_t ctx;
}ST_FORM_DATA;

ST_FORM_DATA g_form_data;

int get_md5_file_hash(char * md5_hash, const char * path)
{
	md5_state_t ctx;
	md5_byte_t hash[16];
	char * buf;
	int fd;
	int file_size = 0;
	int ret;

	md5_init(&ctx);
	fd = open(path, O_RDONLY);
	
	if (fd < 0) {
		dprintf(HTTPD_ERROR, " %s() : [%s] file open error!\n", __func__, path);
		return 0;
	}

	buf = new char[ DEF_FILE_READ_BUFFER_SIZE ];
	
	if(buf){
		do{
			ret = read(fd, buf, DEF_FILE_READ_BUFFER_SIZE);
			
			if(ret) {
				if(g_httpdExitNow)
					return 0;
				
				md5_append(&ctx, (const md5_byte_t *)buf, ret);
				file_size += ret;
				usleep(1000);
			}
		}while(ret > 0);
		
		delete[] buf;
	}
	else{
		dprintf(HTTPD_ERROR, " %s() : buffer allocation error!!!\n", __func__);
	}
	
	close(fd);

	md5_finish(&ctx, hash);
	bin2str(md5_hash, hash, sizeof(hash));

	return file_size;
}

#if DEF_MOVIELIST_USE_HASH_STRING
void file_hash_update_list_check(void)
{
	// file md5_hash update
	if(pai_r.file_hash_update_list.size()){
		u32 queue_no = pai_r.file_hash_update_list.front();
		PAI_R_BACKUP_DATA *pLoc = (PAI_R_BACKUP_DATA *)new char [sizeof(PAI_R_BACKUP_DATA)];
		
		if(pai_r_data.Location_pop(pLoc, queue_no)){
			if(pLoc->location.autoid == queue_no){
				if(strlen(pLoc->file_hash) == 0){
					get_md5_file_hash(pLoc->file_hash, pLoc->location.file_path);
					//pai_r_data.update_file_hash(queue_no, (const char *)pLoc->file_hash);
					//dprintf(HTTPD_INFO, " file hash update : %s (%s)\r\n", pLoc->file_hash, pLoc->location.file_path);
#ifdef MSGQ_PAIR_LOC_Q_OUT_IDKEY		
					ST_QMSG msg;
					msg.type = QMSG_UP_FILE_HASH;
					msg.data = eUserDataType_Normal;
					msg.data2 = queue_no;
					msg.time = (u32)time(0);
					strcpy(msg.string, (const char *)pLoc->file_hash);
					datool_ipc_msgsnd(m_http_msg_q_out_id, (void *)&msg, sizeof(msg));
#endif					
				}
			}
		}

		pai_r.file_hash_update_list.pop_front();

		delete[] pLoc;
	}
}
#endif

int read_header_name(multipart_parser* p, const char *at, size_t length)
{
   printf("name = %.*s\n", length, at);
   return 0;
}

int read_header_value(multipart_parser* p, const char *at, size_t length)
{
	int len;
	int result = 0;
	char * data = g_form_data.name;
		
	if((result = sscanf( at, "form-data; name=\"%4s\"; filename=\"%s\"\r\n", g_form_data.name, g_form_data.filename)) == 2){
		dprintf(HTTPD_INFO, "%d [%s], [%s]\n", result, g_form_data.name, g_form_data.filename);
		data = g_form_data.filename;
		g_form_data.write_filesize = 0;
		g_form_data.write_length = 0;
	}
	else {
		result = sscanf( at, "form-data; name=\"%s\"\r\n", g_form_data.name );
	}

	if(result){
		len = strlen(data);

		if(len)
			data[len -1] = 0;
	}
	
	dprintf(HTTPD_INFO, "value = %.*s\n", length, at);
	dprintf(HTTPD_INFO, "key = %s\n", g_form_data.name);
   return 0;
}

int read_part_data(multipart_parser* p, const char *at, size_t length)
{
	if(g_form_data.file_mode){
		md5_append(&g_form_data.ctx, (const md5_byte_t *)at, length);
		
		if(g_form_data.write_length + length < sizeof(g_form_data.write_buffer)){
			memcpy((void *)&g_form_data.write_buffer[g_form_data.write_length], (void *)at, length);
			g_form_data.write_length += length;
		}
		else {
			if(g_form_data.write_length){
				//dprintf(HTTPD_INFO, "%d ", g_form_data.write_length);
				if(g_form_data.write_filesize == 0)
					sysfs_write(g_form_data.filepath, (const char *)g_form_data.write_buffer, g_form_data.write_length);
				else
					sysfs_writeAppend(g_form_data.filepath, (const char *)g_form_data.write_buffer, g_form_data.write_length);
				
				g_form_data.write_filesize += g_form_data.write_length;
				g_form_data.write_length = 0;
			}
			sysfs_writeAppend(g_form_data.filepath, at, length);
			g_form_data.write_filesize += length;
		}
	}
	else {
	   //dprintf(HTTPD_INFO, "data = %.*s\n", length, at);		
		g_form_data.key_v[g_form_data.name] = format_string("%.*s", length, at);
	}
   return 0;
}

int cb_on_headers_complete(multipart_parser* p)
{
	if(strcmp(g_form_data.name, "file") == 0){
		sprintf(g_form_data.filepath, "%s%s", g_form_data.key_v["target_path"].c_str(), g_form_data.filename);
		dprintf(HTTPD_INFO, "%s file create\r\n", g_form_data.filepath);

		md5_init(&g_form_data.ctx);
		g_form_data.file_mode = true;
	}
	else{
		g_form_data.file_mode = false;
		dprintf(HTTPD_INFO, "STRING MODE\r\n");
	}

	return 0;
}

int cb_on_part_data_end(multipart_parser* p)
{
	if(g_form_data.file_mode){
		md5_byte_t hash[16];
		
		if(g_form_data.write_length){
			sysfs_writeAppend(g_form_data.filepath, (const char *)g_form_data.write_buffer, g_form_data.write_length);
			g_form_data.write_filesize += g_form_data.write_length;
			g_form_data.write_length = 0;

			dprintf(HTTPD_INFO, "END %d\r\n", g_form_data.write_length);
		}
			
		g_form_data.file_mode = false;

		md5_finish(&g_form_data.ctx, hash);
		bin2str(g_form_data.md5_string, hash, sizeof(hash));
		
		dprintf(HTTPD_INFO, "MD5 : %s\r\n", g_form_data.md5_string);
	}
	dprintf(HTTPD_INFO, "PART DATA END\r\n");
	return 0;
}

int post_data_parser(struct mg_connection *conn)
{
	char post_data[2048] = {0,};
	int post_data_len = mg_read(conn, post_data, sizeof(post_data));
	if(post_data_len){
		//printf("%s() : %s\r\n", __func__, post_data);

		multipart_parser* mp;

		char boundary[256] = { 0,};
		sscanf( post_data, "%s\r\n", boundary );
		//printf("%s() : %s\r\n", __func__, boundary);
		
		 multipart_parser_settings callbacks;

		memset(&callbacks, 0, sizeof(multipart_parser_settings));
		g_form_data.key_v.clear();

		callbacks.on_header_field = read_header_name;
		callbacks.on_header_value = read_header_value;
		callbacks.on_part_data = read_part_data;

		mp= multipart_parser_init((const char *)boundary, &callbacks);
		//multipart_parser_set_data(mp, (void *)param);
		multipart_parser_execute(mp, (const char *) post_data, post_data_len);
		//printf("%s() : %s\r\n", __func__, mp->lookbehind);
		multipart_parser_free(mp);
	}

	return post_data_len;
}
		
//////////////////////////////////////////////////////////////////////////

bool http_wifi_config_save(const char * id, const char * pw, const char * no, const char * sn)
{
	ST_CFG_DAVIEW cfg;
	
	if(httpd_cfg_get(&cfg))
	{	
		int pre_sn = atoi(cfg.strSeriarNo);
		int cur_sn = atoi(sn);

		httpd_cfg.serial_no = cur_sn;
		strcpy(cfg.strSeriarNo, sn);

		if(pre_sn != cur_sn) // check wifi mode changed
		{			
			pai_r.api_list_opr.clear();
			pai_r.api_list_drv.clear();
			
			if (pre_sn == 0 || cur_sn == 0)
				httpd_cfg.wifi_mode_change = 1;
		}

		if(cur_sn != 0 && httpd_cfg.wifi_cur_mode == WIFI_APMODE)
			httpd_cfg.wifi_mode_change = 1;			

		if(cur_sn != 0 && (strcmp(cfg.strWiFiSsid[0], id) != 0 || strcmp(cfg.strWiFiPassword[0], pw) != 0 || strcmp(cfg.strTelNumber[0], no) != 0 ))
			httpd_cfg.wifi_mode_change = 1;
		
			
		CConfigText::updateTethering(&cfg, id, pw, no);
		
		return httpd_cfg_set(&cfg);
	}

	return false;
}

int httpd_server_error_check(void)
{
	httpd_cfg.wait_ack++;
	httpd_cfg.server_wait_success++;
	
	httpd_cfg.time_interval = DEF_SERVER_TIMEOUT_INTERVAL;


	if(httpd_cfg.wait_ack > 2 && httpd_cfg.server_wait_success >= DEF_MAX_SERVER_ERROR_COUNT){
		dprintf(HTTPD_ERROR, "server registration error\r\n");
		pai_r.api_list_opr.clear();
		httpd_cfg.server_wait_success = 1;
		httpd_cfg.server_connected = 0;
#ifdef MSGQ_PAIR_LOC_Q_IN_IDKEY
		if(httpd_cfg.wifi_connected){
			datool_ipc_msgsnd2(m_http_msg_q_out_id, (long)QMSG_LED, WIFI_LED_CONNECTED);
		}
#endif		
	}

	if(httpd_cfg.wait_ack == DEF_MAX_SERVER_ERROR_COUNT - 2){
		pai_r.api_list_opr.clear();
		httpd_cfg.server_connected = 0;
#ifdef MSGQ_PAIR_LOC_Q_IN_IDKEY
		if(httpd_cfg.wifi_connected){
			datool_ipc_msgsnd2(m_http_msg_q_out_id, (long)QMSG_LED, WIFI_LED_CONNECTED);
			datool_ipc_msgsnd2(m_http_msg_q_out_id, (long)QMSG_SOUND, kMixwaveServer_connection_failed); // kMixwaveServer_connection_failed
		}		
#endif
	}	
	else if(httpd_cfg.wait_ack >= DEF_MAX_SERVER_ERROR_COUNT){
		dprintf(HTTPD_ERROR, "server ack error\r\n");
		httpd_cfg.wait_ack = 1;
		
		if(SB_Get_Stations_count() == 0)
		{
			httpd_cfg.wifi_connected = 0;
#ifdef MSGQ_PAIR_LOC_Q_IN_IDKEY
			datool_ipc_msgsnd2(m_http_msg_q_out_id, (long)QMSG_LED, WIFI_LED_NOT_CON);
#endif	
			//
			httpd_wifi_start(WIFI_STATIONMODE, 0);
			return 1;
		}
	}
	return 0;
}

#define DEF_BACKUP_REQUEST_MOVIEID_MAX_SIZE		20
#define DEF_BAKUP_REQ_ID_FILE_PATH		"/data/movieid"


int httpd_updateRequestMovieID(bool update)
{
	u32 aMovidID[DEF_BACKUP_REQUEST_MOVIEID_MAX_SIZE ] = { 0,};
	int count = 0;
	if(sysfs_read(DEF_BAKUP_REQ_ID_FILE_PATH, (char *)&aMovidID, sizeof(aMovidID)) == sizeof(aMovidID)){
		int i;
		for( i = 0; i < DEF_BACKUP_REQUEST_MOVIEID_MAX_SIZE; i++){
			if(aMovidID[i]){
				count++;
				
				if(update)
					httpd_addRequestMovieID(aMovidID[i], true);
					
			}
			else
				break;
		}
	}

	return count;
}

int httpd_backupRequestMovieID(void)
{
	u32 aMovidID[DEF_BACKUP_REQUEST_MOVIEID_MAX_SIZE ] = { 0,};
	int count = pai_r.request_list_unique_autoid.size();
	
	if(count){
		REQUEST_ID_POOL * list = &pai_r.request_list_unique_autoid;
		ITER_REQUEST_ID iData = list->begin();
		int j;
		
		for(j = 0; j < count && j <DEF_BACKUP_REQUEST_MOVIEID_MAX_SIZE ; j++, iData++){
			aMovidID[j] = *iData;
			dprintf(HTTPD_INFO, "backup request id : %u\r\n", aMovidID[j]);
		}

		if(sysfs_write(DEF_BAKUP_REQ_ID_FILE_PATH, (const char *)&aMovidID, sizeof(aMovidID)) != sizeof(aMovidID)){
		}
	}
	else {
		if(httpd_updateRequestMovieID(false) != 0) {
			memset((void *)&aMovidID, 0, sizeof(aMovidID));
			
			if(sysfs_write(DEF_BAKUP_REQ_ID_FILE_PATH, (const char *)&aMovidID, sizeof(aMovidID)) != sizeof(aMovidID)){
			}
		}
	}

	return count;
}

int httpd_addRequestMovieID(u32 queue_no, bool is_unique_autoid)
{
	bool new_no = true;
	int j;

	REQUEST_ID_POOL * list = &pai_r.request_list_temp;
	int count;
	ITER_REQUEST_ID iData;

	if(is_unique_autoid){
		list = &pai_r.request_list_unique_autoid;
	}

	count = list->size();
	iData = list->begin();
	
	//dprintf(HTTPD_INFO, "request queue_no (%d).\r\n", queue_no);
	
	for(j = 0; j < count; j++, iData++){
		if(*iData == queue_no){
			dprintf(HTTPD_ERROR, "error !! duplication request %s no (%d).\r\n", is_unique_autoid ? "unique":"queue", queue_no);
			new_no = false;
			break;
		}
	}

	if(new_no){
		dprintf(HTTPD_INFO, "new request %s no (%d).\r\n", is_unique_autoid ? "unique":"queue", queue_no);

		list->push_back(queue_no);
		return 1;
	}

	return 0;
}

void httpd_wifi_mode_change(void)
{
		if(httpd_cfg.wifi_mode_change++ > 4){
			httpd_cfg_get(&pai_r.cfg);
			pai_r.api_list_drv.clear();
			pai_r.api_list_opr.clear();

			if(strlen(pai_r.cfg.strSeriarNo))
				httpd_cfg.serial_no = atoi(pai_r.cfg.strSeriarNo);
			
			if(httpd_cfg.serial_no == WIFI_APMODE){//ap mode
				if(httpd_wifi_start(WIFI_APMODE, FALSE)){
					httpd_cfg.wifi_cur_mode = WIFI_APMODE;
#ifdef MSGQ_PAIR_LOC_Q_IN_IDKEY
					datool_ipc_msgsnd2(m_http_msg_q_out_id, (long)QMSG_LED, WIFI_LED_AP_MODE);
					datool_ipc_msgsnd2(m_http_msg_q_out_id, (long)QMSG_SOUND, kMixwavWiFi_setting_mode);
#endif
				}
			}
			else {
				httpd_cfg.wifi_cur_mode = WIFI_STATIONMODE;
#ifdef MSGQ_PAIR_LOC_Q_IN_IDKEY
				datool_ipc_msgsnd2(m_http_msg_q_out_id, (long)QMSG_LED, WIFI_LED_NOT_CON);
#endif				
				httpd_wifi_start(WIFI_STATIONMODE, TRUE);
				
			}

			httpd_cfg.net_error_count = 0;
			httpd_cfg.wifi_connected = 0;
			httpd_cfg.server_connected = 0;
			httpd_cfg.wifi_mode_change = 0;
		}
}

int httpd_network_check(void)
{
	bool net_error = false;

	if(SB_Net_Is_RNDIS())
		httpd_cfg.net_rndis_mode = 1;
		
	if(access("/sys/class/net/wlan0", F_OK) != 0 && !httpd_cfg.net_rndis_mode){
		httpd_cfg.wifi_connected = 0;
		
		if(httpd_cfg.net_error_count == 0){
			SYSTEMLOG(LOG_TYPE_COMM, LOG_COMM_WIFI, 0, "No module.");
#ifdef MSGQ_PAIR_LOC_Q_IN_IDKEY
			datool_ipc_msgsnd2(m_http_msg_q_out_id, (long)QMSG_LED, WIFI_LED_NOT_CON);
#endif
		}
		dprintf(HTTPD_WARN, "WiFi module not connected.\r\n");
		httpd_cfg.net_error_count = 0xff;
		httpd_cfg.time_interval = DEF_SERVER_REGISTRATION_FAILED_INTERVAL * 5;
		return 0;
	}
	else if(httpd_cfg.net_error_count == 0xff){
		httpd_cfg.time_interval = 3000;
		httpd_cfg.net_error_count = 0;
		httpd_cfg.wifi_mode_change = 1;
		return 0;
	}
	
	if(!SB_Net_Connectd_check()){
		net_error = true;
	}
	else if(httpd_cfg.wait_ack > 0 || httpd_cfg.time_interval >= DEFAULT_NETWORK_CHECK_INTERVAL)
	{
		char ip_addr[32] = { 0,};
		getIPAddress(ip_addr);
		//dprintf(HTTPD_WARN, "ip : %s		is_time_out_ms %d \r\n",ip_addr, httpd_cfg.time_interval);
		
		if(strlen(ip_addr) < 7)
			net_error = true;
	}

	if(net_error)
	{
		if(httpd_cfg.wifi_cur_mode == WIFI_STATIONMODE || SB_Get_Stations_count() == 0 || httpd_cfg.net_rndis_mode) {
			dprintf(HTTPD_WARN, "NET Not connected. (%d) \r\n", httpd_cfg.net_error_count);
#ifdef MSGQ_PAIR_LOC_Q_IN_IDKEY
			if(httpd_cfg.serial_no == 0) {
 #if defined(DEF_NO_SOUND_NETWORK_CHECK)				
				datool_ipc_msgsnd2(m_http_msg_q_out_id, (long)QMSG_LED, WIFI_LED_AP_MODE);	
 #else
 				datool_ipc_msgsnd2(m_http_msg_q_out_id, (long)QMSG_SOUND, kMixwavWiFi_setting_mode);
 #endif
			}
#endif			
		}
#if defined(MSGQ_PAIR_LOC_Q_IN_IDKEY) && defined(DEF_NO_SOUND_NETWORK_CHECK)
		else {
			datool_ipc_msgsnd2(m_http_msg_q_out_id, (long)QMSG_LED, WIFI_LED_SERVER_OK);		
		}
#endif
		
		httpd_cfg.time_interval = DEFAULT_NETWORK_CHECK_INTERVAL;

		httpd_cfg.net_error_count++;
	
 #if !defined(DEF_TEST_SERVER_USE) && !defined(DEF_USB_LTE_MODEM_USE)
		if(httpd_cfg.net_error_count == DEFAULT_STATION_MODE_TIMEOUT_COUNT/2){

				//httpd_cfg_get(&pai_r.cfg);
				if(strlen(pai_r.cfg.strSeriarNo))
					httpd_cfg.serial_no = atoi(pai_r.cfg.strSeriarNo);
				
				if(httpd_cfg.serial_no != 0) {
					pai_r.api_list_opr.clear();
			
					httpd_cfg.wifi_connected = 0;
					httpd_cfg.server_connected = 0;

#ifdef MSGQ_PAIR_LOC_Q_IN_IDKEY
					if(httpd_cfg.wifi_cur_mode == WIFI_STATIONMODE)
						datool_ipc_msgsnd2(m_http_msg_q_out_id, (long)QMSG_SOUND, kMixwaveWiFi_connection_failed); // kMixwaveWiFi_connection_failed
#endif

					//if(httpd_cfg.wifi_cur_mode == WIFI_STATIONMODE) //ver 0.0.11
					if(httpd_cfg.wifi_cur_mode == WIFI_STATIONMODE && !httpd_cfg.wifi_had_connected) // ver 0.0.12
					{
						if(httpd_wifi_start(WIFI_APMODE, FALSE)){
							httpd_cfg.wifi_cur_mode = WIFI_APMODE;
#ifdef MSGQ_PAIR_LOC_Q_IN_IDKEY
							datool_ipc_msgsnd2(m_http_msg_q_out_id, (long)QMSG_LED, WIFI_LED_AP_MODE);
							datool_ipc_msgsnd2(m_http_msg_q_out_id, (long)QMSG_SOUND, kMixwavWiFi_setting_mode);
#endif		
						}
					}
					else if(SB_Get_Stations_count() == 0){
#ifdef MSGQ_PAIR_LOC_Q_IN_IDKEY
						datool_ipc_msgsnd2(m_http_msg_q_out_id, (long)QMSG_LED, WIFI_LED_NOT_CON);
#endif
						httpd_wifi_start(WIFI_STATIONMODE, FALSE);
						httpd_cfg.wifi_cur_mode = WIFI_STATIONMODE;
					}
				}
		}
		else 
#endif //end of 	DEF_TEST_SERVER_USE
		if(httpd_cfg.net_error_count >= DEFAULT_STATION_MODE_TIMEOUT_COUNT){
			int sn = 0;
			
			httpd_cfg.net_error_count = 0;
			
			//httpd_cfg_get(&pai_r.cfg);
			if(strlen(pai_r.cfg.strSeriarNo))
				httpd_cfg.serial_no = atoi(pai_r.cfg.strSeriarNo);
			
			if(httpd_cfg.serial_no != 0){
				httpd_cfg.wifi_connected = 0;
				httpd_cfg.server_connected = 0;

				if(httpd_cfg.wifi_cur_mode == WIFI_STATIONMODE || SB_Get_Stations_count() == 0)
				{
					httpd_cfg.wifi_cur_mode = WIFI_STATIONMODE;
#ifdef MSGQ_PAIR_LOC_Q_IN_IDKEY
					datool_ipc_msgsnd2(m_http_msg_q_out_id, (long)QMSG_LED, WIFI_LED_NOT_CON);
#endif						
					httpd_wifi_start(WIFI_STATIONMODE, FALSE);
				}
			}
		}
		else if(httpd_cfg.net_rndis_mode){
			SB_RNDIS_Start();
		}

		//file_hash_update_list_check();
		httpd_cfg.wait_ack = 1;
		httpd_cfg.server_wait_success = 1;
		return 0;
	}
	else if(httpd_cfg.wifi_connected == 0){	
		httpd_cfg.wifi_connected = 1;
#ifdef MSGQ_PAIR_LOC_Q_IN_IDKEY
		//datool_ipc_msgsnd2(m_http_msg_q_out_id, (long)QMSG_SOUND, 9); // kMixwaveDingdong;
		datool_ipc_msgsnd2(m_http_msg_q_out_id, (long)QMSG_LED, WIFI_LED_CONNECTED);
#endif
		http_pai_r_information_send_to_app(pai_r.cfg.strApplication_IP, pai_r.cfg.iApplication_PORT, DEF_HOST_SSL_USE, (const char *)pai_r.cfg.strSeriarNo, (const char *)pai_r.cfg.strApSsid);
	}

	httpd_cfg.wifi_had_connected = 1;
	httpd_cfg.net_error_count = 0;

	return 1;
}

void httpd_main_work(void)
{
#if DEF_MOVIELIST_USE_HASH_STRING
	file_hash_update_list_check();
#endif
}

void httpd_msg_work(void)
{
#ifdef MSGQ_PAIR_LOC_Q_IN_IDKEY
	ST_QMSG msg;
	int first_msg = 0;
	
	while(datool_ipc_msgrcv(m_http_msg_q_in_id, (void *)&msg, sizeof(msg)) != -1){
		dprintf(HTTPD_INFO, "%s() : queue type:%d data1:%d data2:%d\r\n", __func__, (int)msg.type, (int)msg.data, (int)msg.data2);

		if(msg.type == QMSG_IN){
			pai_r_data.Location_queue_auto_id_in_set(msg.data, eUserDataType_Normal);
			pai_r_data.Location_queue_auto_id_in_set(msg.data2, eUserDataType_Event);

		   pai_r_data.m_local_auto_id_offset = msg.time - msg.data;
#if DEF_MOVIELIST_USE_HASH_STRING			
			pai_r.file_hash_update_list.push_back(msg.data - 1);
			
			//else if(httpd_cfg.wifi_connected == 0 || httpd_cfg.server_connected == 0)
				//file_hash_update_list_check();
#endif			
			dprintf(HTTPD_INFO, "%s() : queue %u:%u %d [event %u:%u %d] [autoid %u, offset %d]\r\n", __func__, msg.data, \
			pai_r_data.Location_queue_auto_id_out_get(eUserDataType_Normal), \
			pai_r_data.Location_queue_count(), \
			msg.data2, \
			pai_r_data.Location_queue_auto_id_out_get(eUserDataType_Event), \
			pai_r_data.Location_queue_count(eUserDataType_Event), \
			msg.time, \
			pai_r_data.m_local_auto_id_offset);
		}
		else if(msg.type == QMSG_OUT) {
			pai_r_data.Location_queue_auto_id_out_set(msg.data, eUserDataType_Normal);
			pai_r_data.Location_queue_auto_id_out_set(msg.data2, eUserDataType_Event);
		}
		else if(msg.type == QMSG_LOC_MAX) {
			pai_r_data.Location_queue_max_count_set(msg.data, eUserDataType_Normal);
			pai_r_data.Location_queue_max_count_set(msg.data2, eUserDataType_Event);
			pai_r_data.m_queue_index_init = true;
		}
		else if(msg.type == QMSG_EXIT){
			g_httpdExitNow = true;
			httpd_backupRequestMovieID();
		}
		else if(msg.type == QMSG_CHANNEL){
			httpd_cfg.rec_channel_count = msg.data;
			dprintf(HTTPD_INFO, "%s() : channel %d \r\n", __func__, (int)httpd_cfg.rec_channel_count);
		}
		else if(msg.type == QMSG_HTTPD_SNAPSHOT_DONE) {
			dprintf(HTTPD_INFO, "%s() : snapshot %d \r\n", __func__, (int)msg.data);
			if(first_msg == 0) {
				datool_ipc_msgsnd(m_http_msg_q_in_id, (void *)&msg, sizeof(msg));
				msleep(200);
			}
			first_msg++;
		}
	}
#else
	PAI_R_LOCATION loc;
	PAI_R_SPEED_CSV spd;

	
	if(m_http_msg_q_loc_id == -1)
		return 0;
	
	if(datool_ipc_msgrcv(m_http_msg_q_loc_id, (void *)&loc, sizeof(loc)) != -1)
	{
		pai_r_data.addLocation(loc);
	}
	while(datool_ipc_msgrcv(m_http_msg_q_spd_id, (void *)&spd, sizeof(spd)) != -1)
	{
		pai_r_data.addSpeed(spd);
	}
#endif

}

void httpd_thread_work(void)
{
	static u32 time_out_tm;
	bool send_require = false;
	

	if(httpd_cfg.wifi_mode_change) // 5초후 전환 
	{
		httpd_wifi_mode_change();
		return;
	}


#ifdef MSGQ_PAIR_LOC_Q_IN_IDKEY
#ifdef DEF_MMB_SERVER
	eUserDataType ud_type  = eUserDataType_Event;
	static u32 time_out_polling_tm;
	if(httpd_cfg.wifi_connected){
		if(is_time_out_ms(&time_out_polling_tm, DEF_SERVER_POLLING_INTERVAL)){
			printf("\n##### Ryunni tcp_mmb_send send rqruire 0 !!!  \n\n");
			tcp_mmb_send(pai_r_data, 0, ud_type);
		}
	}
	if(httpd_cfg.wifi_connected && pai_r_data.Location_queue_count(ud_type))
		send_require = true;
#else
	if(httpd_cfg.wifi_connected  && httpd_cfg.server_connected && pai_r_data.Location_queue_count())
		send_require = true;
#endif
#else
	if(pai_r_data.m_data_list[0].size() > 1 || pai_r_data.m_data_list[1].size())
		send_require = true;
#endif

	if(is_time_out_ms(&time_out_tm, httpd_cfg.time_interval) || (send_require && httpd_cfg.server_wait_success == 0))
	{
		time_out_tm = get_tick_count();
		
		if(httpd_network_check() && !g_httpdExitNow) {
#ifdef DEF_MMB_SERVER
			if(pai_r_data.Location_queue_count(ud_type)) {
				printf("\n##### Ryunni tcp_mmb_send send rqruire 1 Event File!!!  \n\n");
				if(tcp_mmb_send(pai_r_data, 1, ud_type) < 0){
					time_out_tm = get_tick_count();
				}
			}
#else
			httpd_insert_list_send(pai_r_data, send_require);
#endif
		}
	}
}


#include "pai_r_api_default.cpp"
#include "pai_r_api_updateTethering.cpp"
#include "pai_r_api_getCouldConnectToServer.cpp"
#include "pai_r_api_getLive.cpp"
#include "pai_r_api_updateFile.cpp"
#include "pai_r_api_getMovieList.cpp"
#include "pai_r_api_getUpdateLog.cpp"
#include "pai_r_api_getSystem.cpp"
#include "pai_r_api_getSystemLog.cpp"
#include "pai_r_api_getConfig.cpp"
#include "pai_r_api_updateConfig.cpp"

#if  defined(HTTPD_EMBEDDED) || defined(HTTPD_MSGQ_SNAPSHOT)
#include "../datech_i3_httpd_recorder/pai_r_api_command.cpp"
#endif


static struct mg_callbacks CALLBACKS;

static int log_message_cb(const struct mg_connection *conn, const char *msg)
{
	(void)conn;
	printf("%s\n", msg);
	return 0;
}

static void init_CALLBACKS()
{
	memset(&CALLBACKS, 0, sizeof(CALLBACKS));

	CALLBACKS.log_message = log_message_cb;

};

//-------------------------------------------------------------------------------------------------

int
_set_sock_timeout(int sock, int timeout)
{
    fd_set set;
		
	struct timeval tv;

    if (sock == -1) {
        cerr << "Could not receive data: No connection to server" << endl;
        return false;
    }

    // set up the file descriptor set
    FD_ZERO(&set);
    FD_SET(sock, &set);

    // set up the timeout value
    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    // wait for data to be received or timeout
    int rv = select(sock, &set, NULL, NULL, (timeout > 0) ? &tv : NULL);
    if (rv == -1) {
        cerr << "Error in select()" << endl;
        return false;
    } else if (rv == 0) {
        cerr << "Timeout occurred." << endl;
        return false;
    }

	return 1;
}
	
static void * httpd_send_thread(void * param)
{
	char option_port[12] = PORT;
	const char *options[] = {
	    	"document_root", DOCUMENT_ROOT, 
			"listening_ports", PORT, 
#ifndef NO_SSL
		    "ssl_certificate",
		    SSL_CERT_PATH,
#endif
			0};
    
    std::vector<std::string> cpp_options;
 
	httpd_cfg.serial_no = atoi(pai_r.cfg.strSeriarNo);

	httpd_cfg.httpd_port = (int)param;

	//pai_r_data.m_auto_id = format_string( "%s", pai_r.strUUID);

	int delay = 0;
	do{
		sleep(1);
	}while(!SB_Net_Connectd_check() && delay++ < 35);

	while(access(DEF_LOCATION_QUEUE_CHECK_FILE, R_OK ) != 0) {
		//dprintf(HTTPD_INFO, "%s() : wait!\r\n", __func__);
		sleep(1);
		if(g_httpdExitNow)
			return 0;
	}
	
    for (unsigned int i=0; i<(sizeof(options)/sizeof(options[0])-1); i++) {
		if(i==3 && httpd_cfg.httpd_port){
			sprintf(option_port, "%d", httpd_cfg.httpd_port);
			cpp_options.push_back(option_port);
		}
		else
        	cpp_options.push_back(options[i]);
    }

#if 1
	struct mg_context *ctx;

	init_CALLBACKS();
	ctx = mg_start(&CALLBACKS, NULL, options);
#endif

	// CivetServer server(options); // <-- C style start
    CivetServer server(cpp_options); // <-- C++ style start

	ExitHandler h_exit;
	server.addHandler(EXIT_URI, h_exit);

	FooHandler h_foo;
	server.addHandler("", h_foo);
	
//---------------------------------
	pai_r_api_updateTetheringHandler h_Tethering;
	server.addHandler(PAI_R_API_UPDATE_TETHERING, h_Tethering);

	pai_r_api_getCouldConnectToServerHandler h_GetConnect;
	server.addHandler(PAI_R_API_GET_COULD_CONNECT_TO_SERVER, h_GetConnect);
	
	pai_r_api_getLiveHandler h_getLive;
	server.addHandler(PAI_R_API_GET_LIVE, h_getLive);

	pai_r_api_updateFileHandler h_updateFile;
	server.addHandler(PAI_R_API_UPDATE_FILE, h_updateFile);

	pai_r_api_getMovieListHandler h_getMovieList;
	server.addHandler(PAI_R_API_GET_MOVIE_LIST, h_getMovieList);

	pai_r_api_getUpdateLogHandler h_getUpdateLog;
	server.addHandler(PAI_R_API_GET_UPDATELOG, h_getUpdateLog);

	pai_r_api_getSystemHandler	h_getSystem;
	server.addHandler(PAI_R_API_GET_SYSTEM, h_getSystem);

	pai_r_api_getSystemLogHandler h_getSystemLog;
	server.addHandler(PAI_R_API_GET_SYSTEMLOG, h_getSystemLog);
	
	pai_r_api_getConfigHandler	h_getConfig;
	server.addHandler(PAI_R_API_GET_CONFIG, h_getConfig);

	pai_r_api_updateConfigHandler	h_updateConfig;
	server.addHandler(PAI_R_API_UPDATE_CONFIG, h_updateConfig);	

#if  defined(HTTPD_EMBEDDED) || defined(HTTPD_MSGQ_SNAPSHOT)
	pai_r_api_commandHandler	h_command;
	server.addHandler(PAI_R_API_COMMAND, h_command);	
#endif
//--------------------------------

	//printf("Browse files at http://localhost:%s/\n", option_port);
	//printf("Run example at http://localhost:%s%s\n", option_port, EXAMPLE_URI);
	//printf("Exit at http://localhost:%s%s\n", option_port, EXIT_URI);

	//SYSTEMLOG(LOG_TYPE_COMM, LOG_COMM_SERVER, 0, "START (%s)", option_port);

	//upload 못한 동영상 처리를 위해 미리 업로드할 리스트를 생성한다.
	if(pai_r_data.Location_queue_count(eUserDataType_Event))
	{
		u32 queue_no = pai_r_data.Location_queue_first_autid(eUserDataType_Event);
		int count = pai_r_data.Location_queue_count(eUserDataType_Event);
		PAI_R_BACKUP_DATA *pLoc = (PAI_R_BACKUP_DATA *)new char [sizeof(PAI_R_BACKUP_DATA)];
								
		for(int i = 0; i < count; i++){
			if(!pai_r_data.Location_pop(pLoc, queue_no, eUserDataType_Event, true))
						break;

			if(pLoc->event_autoid == queue_no){
				if(pLoc->location.autoid < pai_r_data.Location_queue_auto_id_out_get())
					if(pLoc->location.autoid < pai_r_data.Location_queue_max_count_get() || \
						pLoc->location.autoid > (pai_r_data.Location_queue_auto_id_in_get() - pai_r_data.Location_queue_max_count_get()))
					{
						httpd_addRequestMovieID(pLoc->location.autoid, false);
					}
			}
			queue_no++;
		}
	
		pai_r_data.Location_pop_continue_read_close();

		delete[] pLoc;
	}

	//upload 못한 request id를 생성
	httpd_updateRequestMovieID(true);

#ifdef DEF_MMB_SERVER
	tcp_mmb_init();
#endif

	while (!g_httpdExitNow) {
#ifdef _WIN32
		Sleep(1000);
#else
		sleep(1);
#endif

#if 0
		http_send("mar-i.com", 80, 0, "POST", \
		"/driverecorder/api_driverecorder/firstview/1/insert_driverecorder.php", \
		"?company_pai_r_id=1231&ssid=pair-recoder12345678&mac=A0:B2:D5:7F:81:B3	&sensor_level=4&sensor_datetime=2019-09-00 10:10:10&environment=VFPR/1.0.0", \
		NULL, 0, 10000);
#elif 0
	{
		//unsigned char eBuf[1024];
		//tcpClient.setup("192.168.35.19", 4000);
		//tcpClient.sendData("test 1234");
		//tcpClient.mmb_recv(eBuf, sizeof(eBuf), 10);
		tcp_send("192.168.35.19", 4000, (unsigned char *)"test 1234", strlen("test 1234"), 40, analyzeData);
	}
#else
		//sleep(20);
		httpd_thread_work();
#endif
	}
	//SYSTEMLOG(LOG_TYPE_COMM, LOG_COMM_SERVER, 0, "END (%s)", option_port);

	mg_stop(ctx);
	printf("Bye!\n");
	return (void *)0;
}

static void * httpd_msg_thread(void * param)
{
	u32 time_out_tm = 0;

	datool_ipc_msgsnd2(m_http_msg_q_out_id, (long)QMSG_HTTPD_START, 0);
	
	strcpy(pai_r.strUUID, datool_generateUUID().c_str());

	if(strlen(pai_r.strUUID))
	{
		ST_QMSG msg;
		msg.type = QMSG_HTTPD_OPERID;
		msg.data = 0;
		msg.data2 = 0;
		msg.time = (u32)time(0);
		strcpy(msg.string, pai_r.strUUID);
		datool_ipc_msgsnd(m_http_msg_q_out_id, (void *)&msg, sizeof(msg));
	}
	
	
	while (!g_httpdExitNow) {
		msleep(100);

		if(is_time_out_ms(&time_out_tm, 5000)){		
			datool_ipc_msgsnd2(m_http_msg_q_out_id, (long)QMSG_HTTPD_ALIVE, 0);
		}
		
		httpd_msg_work();
	}
	
	return (void *)0;
}

int httpd_start(int http_server_port) // 0 is test mode
{
#if IPC_MSG_USE
		m_http_msg_q_loc_id = datool_ipc_msgget(MSGQ_PAIR_LOC_IDKEY);
		m_http_msg_q_spd_id = datool_ipc_msgget(MSGQ_PAIR_SPD_IDKEY);
#endif	
#ifdef MSGQ_PAIR_LOC_Q_IN_IDKEY
		m_http_msg_q_out_id = datool_ipc_msgget(MSGQ_PAIR_LOC_Q_OUT_IDKEY);
		m_http_msg_q_in_id = datool_ipc_msgget(MSGQ_PAIR_LOC_Q_IN_IDKEY);

		dprintf(HTTPD_INFO, "%s() : in:%d out:%d\r\n", __func__, m_http_msg_q_in_id, m_http_msg_q_out_id);

		mg_start_thread(httpd_msg_thread, (void *)NULL);
#endif
	
	while(access(DA_FORMAT_FILE_NAME, R_OK ) == 0 || access(DA_FORMAT_FILE_NAME2, R_OK ) == 0 || access(DA_SETUP_FILE_PATH_SD, R_OK ) != 0) {
		sleep(1);
		if(g_httpdExitNow)
			return 0;
	}

	const char * tmp_sn_path = DEF_DEFAULT_SSID_TMP_PATH;
	strcpy(pai_r.strProductSN, "0");
	
	if(access(tmp_sn_path, R_OK ) == 0){
		u32 number  = 0;
		if(sysfs_scanf(tmp_sn_path, "%u", &number)){
			sprintf(pai_r.strProductSN, "%u", number);
		}
		else
			strcpy(pai_r.strProductSN, "0");
	}
		

	httpd_cfg_get(&pai_r.cfg);

	dprintf(HTTPD_INFO, "%s() : Seriar No : %s \r\n", __func__, pai_r.cfg.strSeriarNo);
	dprintf(HTTPD_INFO, "%s() : AP SSID : %s \r\n", __func__, pai_r.cfg.strApSsid);
	dprintf(HTTPD_INFO, "%s() : WiFi SSID : %s \r\n", __func__, pai_r.cfg.strWiFiSsid[0]);
	dprintf(HTTPD_INFO, "%s() : WiFi Password : %s \r\n", __func__, pai_r.cfg.strWiFiPassword[0]);
	dprintf(HTTPD_INFO, "%s() : Tel Number : %s \r\n", __func__, pai_r.cfg.strTelNumber[0]);

	dprintf(HTTPD_INFO, "%s() : APP IP : %s \r\n", __func__, pai_r.cfg.strApplication_IP);
	dprintf(HTTPD_INFO, "%s() : APP PORT : %d \r\n", __func__, pai_r.cfg.iApplication_PORT);

	dprintf(HTTPD_INFO, "-------------------------------\r\n");
	dprintf(HTTPD_INFO, "%s() : Debug Server Port : %d \r\n", __func__, pai_r.cfg.iDebugServer_PORT);
	
	dprintf(HTTPD_INFO, "%s() : CloudServerName : %s \r\n", __func__, pai_r.cfg.strCloudServerName);
	dprintf(HTTPD_INFO, "%s() : CloudServerApiUrl : %s \r\n", __func__, pai_r.cfg.strCloudServerApiUrl);
	dprintf(HTTPD_INFO, "%s() : CloudServerPort : %d \r\n", __func__, pai_r.cfg.iCloudServerPort);
	dprintf(HTTPD_INFO, "%s() : CloudServerPollingPort : %d \r\n", __func__, pai_r.cfg.iCloudServerPollingPort);
	dprintf(HTTPD_INFO, "%s() : CloudServerSSL : %d \r\n", __func__, pai_r.cfg.bCloudServerSSL);

#ifdef MSGQ_PAIR_LOC_Q_IN_IDKEY
	datool_ipc_msgsnd2(m_http_msg_q_out_id, (long)QMSG_LED, WIFI_LED_NOT_CON);
#endif

	int delay = 0;
	do{
		if(access("/sys/class/net/wlan0", F_OK) == 0)
			break;
		
		sleep(1);
	}while(!SB_Net_Connectd_check() && delay++ < 35);

#if 0
	if(http_server_port == 0) {
		httpd_cfg.wifi_cur_mode = WIFI_STATIONMODE;
		if(!SB_Net_Connectd_check()){
			SB_WiFi_Start(WIFI_STATIONMODE, "SK_WiFiGIGA6989",  "1603078128");
			sleep(3);
		}
		pai_r.api_list_drv["insert_list_and_request"] = "/driverecorder/api_driverecorder/firstview/1/insert_list_and_request.php";
		pai_r.api_list_opr["insert_list_and_request"] = "/driverecorder/api_driverecorder/firstview/1/insert_list_and_request.php";
	}
	else 
#endif
	{
		if(atoi(pai_r.cfg.strSeriarNo) == 0){
#ifdef MSGQ_PAIR_LOC_Q_IN_IDKEY			
			datool_ipc_msgsnd2(m_http_msg_q_out_id, (long)QMSG_LED, WIFI_LED_AP_MODE);
 #if !defined(DEF_NO_SOUND_NETWORK_CHECK)
			datool_ipc_msgsnd2(m_http_msg_q_out_id, (long)QMSG_SOUND, kMixwavWiFi_setting_mode);	
 #endif
#endif

			if(httpd_wifi_start(WIFI_APMODE, FALSE)){
				httpd_cfg.wifi_cur_mode = WIFI_APMODE;
			}
		}
		else {			
			httpd_cfg.wifi_cur_mode = WIFI_STATIONMODE;
			
			if(!SB_Net_Connectd_check() || SB_Net_Is_RNDIS()) {
				httpd_wifi_start(WIFI_STATIONMODE, FALSE);
				sleep(3);
			}
		}
		
		while(g_httpdExitNow) {
			if(get_tick_count() > SAFETY_START_RECORDING_SECONDS * 1000)
				break;
			msleep(100);
		}

		if(!g_httpdExitNow)
			mg_start_thread(httpd_send_thread, (void *)http_server_port);
	}
	
	return 0;
}


int httpd_end(void)
{
	//dprintf(HTTPD_INFO, "%s() : Bye!\r\n", __func__);
	g_httpdExitNow = true;
	sleep(1);
	return 0;
}
const char * httpd_sd_setup_file = DA_SETUP_FILE_PATH_SD;
const char * httpd_backup_setup_file = DA_SETUP_FILE_PATH_BACKUP;

bool httpd_cfg_get(ST_CFG_DAVIEW *p_cfg)
{
	ST_CFG_DAVIEW	back_cfg;
	ST_CFG_DAVIEW	sd_cfg;
	
	memset((void *)&back_cfg, 0, sizeof(ST_CFG_DAVIEW));
	memset((void *)&sd_cfg, 0, sizeof(ST_CFG_DAVIEW));
	CConfigText::CfgDefaultSet(&back_cfg);
	CConfigText::CfgDefaultSet(&sd_cfg);

	//setup.XML file check
	const char * xmlcheck_exe = "/datech/app/da-start";
	if ( access(xmlcheck_exe, R_OK ) == 0) {                             // 파일이 있음 
		system(xmlcheck_exe);
	}

	if(!CConfigText::Load(httpd_backup_setup_file, &back_cfg)){
		CConfigText::CfgDefaultSet(httpd_backup_setup_file);
	}

	
	if(!CConfigText::Load(httpd_sd_setup_file, &sd_cfg)){
		//sd card에 setup.XML이 없으면 backup된 xml 파일을 저장한다
		sd_cfg = back_cfg;
	}
	else {
		if(sd_cfg.bWifiSet){
			sd_cfg.bWifiSet = 0;
			httpd_cfg_set(&sd_cfg);
			dprintf(HTTPD_INFO, "%s() : Wi-Fi settings Update at internal values.\r\n", __func__);
		}
		else {
			//recprder의 daSystemSetup.cpp cfg_get 코드도 함께 수정해야 함
			dprintf(HTTPD_INFO, "%s() : WiFi setting use internal values.\r\n", __func__);
			strcpy(sd_cfg.strApSsid, back_cfg.strApSsid);
			memcpy((void *)sd_cfg.strWiFiSsid, (void *)back_cfg.strWiFiSsid, sizeof(sd_cfg.strWiFiSsid));
			memcpy((void *)sd_cfg.strWiFiPassword, (void *)back_cfg.strWiFiPassword, sizeof(sd_cfg.strWiFiPassword));
			memcpy((void *)sd_cfg.strTelNumber, (void *)back_cfg.strTelNumber, sizeof(sd_cfg.strTelNumber));
			strcpy(sd_cfg.strSeriarNo, back_cfg.strSeriarNo);
		}
	}
		

	*p_cfg = sd_cfg;
	return true;
}

bool httpd_cfg_set(ST_CFG_DAVIEW *p_cfg)
{	
	bool result = false;
	
	//strcpy(p_cfg->strFWVersion, pai_r.strAppVer);
	result = CConfigText::Save(httpd_sd_setup_file, p_cfg);
	result = CConfigText::Save(httpd_backup_setup_file, p_cfg);

	return result;
}

//RNDIS

int httpd_wifi_start(int mode, bool user1_start)
{
	int result = 0;
	if(SB_Net_Is_RNDIS()){
		if(SB_RNDIS_Start())
			return 1;
	}
	
	if(mode == WIFI_APMODE){
		const char * tmp_sn_path = DEF_DEFAULT_SSID_TMP_PATH;
		char strApSsid[128];

		strcpy(strApSsid, pai_r.cfg.strApSsid);

		if(strcmp(strApSsid, DEF_DEFAULT_SSID_STRING) == 0){
			if(access(tmp_sn_path, R_OK ) == 0){
				u32 sn = 0;
				if(sysfs_scanf(tmp_sn_path, "%u", &sn)){
					sprintf(strApSsid, DEF_DEFAULT_SSID_FMT, sn);
				}
			}
		}

		result = SB_WiFi_Start(mode, (const char *)strApSsid, NULL);
	}
	else {
		if(user1_start) {
			result = SB_WiFi_Start(mode, pai_r.cfg.strWiFiSsid[0], pai_r.cfg.strWiFiPassword[0]);
		}
		else{
			int i,j;
			int user_no = -1;
			WIFI_STATIONS_POOL list_st;
			double dTick;
			int tr_size;
			ITER_WIFI_STATIONS iTI_S;
			
			SB_GetStations(list_st);

RETRY_SCAN:
	
			tr_size = list_st.size();
			
			if(tr_size){
				iTI_S = list_st.begin();
								
				for(j = 0; j < DEF_MAX_TETHERING_INFO; j++){
					for(i = 0; i < tr_size ; i++, iTI_S++){
						if(strlen(pai_r.cfg.strWiFiSsid[j]) && strcmp(pai_r.cfg.strWiFiSsid[j], iTI_S->strSSID) == 0){
							dprintf(HTTPD_INFO, "[WiFi] Access User %d (%d: BSS %s	freq: %d	signal: %lf	SSID: %s)\r\n", j+1, i, iTI_S->strBSS, iTI_S->iFreq, iTI_S->dSignal, iTI_S->strSSID);
							user_no = j;
							j = DEF_MAX_TETHERING_INFO; // for break
							break;
						}						
					}
				}
			}

			if(user_no == -1 || user_no >=  DEF_MAX_TETHERING_INFO){
				user_no = 0;
				dprintf(HTTPD_INFO, "[WiFi] Accessible ssid does not exist.\r\n");
			}

			dTick = ((double)get_tick_count()) / 1000.0;
			
			result = SB_WiFi_Start(mode, pai_r.cfg.strWiFiSsid[user_no], pai_r.cfg.strWiFiPassword[user_no]);

			if(i < tr_size){
				sleep(2);
				
				if(SB_StationsStatus(iTI_S->strBSS, dTick) == 3){ // 3 is reason=WRONG_KEY
					dprintf(HTTPD_INFO, "[WiFi] SKIP! user %d [ %s (reason=WRONG_KEY)]\r\n", user_no + 1, iTI_S->strSSID);
					list_st.erase(iTI_S);

					goto RETRY_SCAN;
				}
				else if(user_no) {
					CConfigText::updateTethering(&pai_r.cfg, pai_r.cfg.strWiFiSsid[user_no], pai_r.cfg.strWiFiPassword[user_no], pai_r.cfg.strTelNumber[user_no]);		
					httpd_cfg_set(&pai_r.cfg);
				}
			}
			
			list_st.clear();
		}
	}		

	return result;
}
