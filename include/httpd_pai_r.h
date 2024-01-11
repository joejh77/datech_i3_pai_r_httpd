#ifndef httpd_pai_r_H
#define httpd_pai_r_H
#include <unordered_map>
#include <vector>
#include "daappconfigs.h"
#include "json/json.h"

#include "pai_r_data.h"
#include "pai_r_updatelog.h"
#include "parallel_hashmap/phmap.h"

#ifdef DEF_MMB_SERVER
#define DEF_SERVER_TIMEOUT_INTERVAL								(30 * 1000) //30sec
#ifdef DEF_MMB_VER_2
#define DEF_SERVER_TIMEOUT_WAIT										(30 * 1000) //60sec 
#else
#define DEF_SERVER_TIMEOUT_WAIT										(60 * 1000) //60sec 
#endif
#define DEF_SERVER_POLLING_INTERVAL									(1 * 60 * 1000)
#define DEF_SERVER_UPLOAD_RETRY_MAX_COUNT				3
#define DEF_MAX_SERVER_ERROR_COUNT									6
#else
#define DEF_SERVER_TIMEOUT_INTERVAL								(45 * 1000) //45sec
#define DEF_MAX_SERVER_ERROR_COUNT									6
#endif

typedef phmap::flat_hash_map<std::string, std::string> key_value_map_t;

typedef std::list<u32>														REQUEST_ID_POOL;
typedef REQUEST_ID_POOL::iterator								ITER_REQUEST_ID;

typedef struct {
	char strPai_r_id[64];
	char strUUID[64];
	char strProductSN[16];

	ST_CFG_DAVIEW cfg; //recorder config

	u32 operation_autoid;
	
	Json::Value api_list_drv;
	Json::Value api_list_opr;
	Json::Value request;

	REQUEST_ID_POOL request_list_temp;		//queue no save
	REQUEST_ID_POOL request_list_unique_autoid;		//unique no save

	REQUEST_ID_POOL file_hash_update_list;

	CPai_r_updatelog updatelog;
}PAI_R_CONFIG;

typedef struct {
	int httpd_port;
	u32 time_interval;//data queue check time

	u8 rec_channel_count;

	u8 net_error_count; // 0xff value is "no module" sign.
	u8 wifi_cur_mode; //WIFI_APMODE, WIFI_STATIONMODE
	u8 wifi_mode_change; //
	u32 serial_no;//0 is AP_MODE

	u8 net_rndis_mode;
	u8 wifi_connected;
	u8 server_connected;
	u8 server_wait_success;
	u8 server_wait_success_movie;
	u8 wait_ack;
	u8 wifi_had_connected;
}HTTPD_CFG;

	
#ifdef __cplusplus
extern "C"
{
#endif

int http_pai_r_insert_recorder(const char *host, int port, const char *url, int is_ssl, const char * pai_r_id, const char * ssid);
int http_pai_r_insert_operation(const char *host, int port, int is_ssl, time_t start_time, const char * uuid);
int http_pai_r_insert_list_and_request(const char *host, int port, int is_ssl, time_t start_time, const char * operation_autoid, Json::Value &location, const char * file_path);
int http_pai_r_insert_list_and_request2(const char *host, int port, int is_ssl, LOCATION_INFO &info);
int http_pai_r_insert_movie(const char *host, int port, int is_ssl, u32 auto_id, bool ud_type);

int httpd_server_error_check(void);
int httpd_addRequestMovieID(u32 queue_no, bool is_event);
void httpd_main_work(void);

int httpd_start(int http_server_port);
int httpd_end(void);

bool httpd_cfg_get(ST_CFG_DAVIEW *p_cfg);
bool httpd_cfg_set(ST_CFG_DAVIEW *p_cfg);

int httpd_wifi_start(int mode, bool user1_start);

extern bool g_httpdExitNow;
extern PAI_R_CONFIG pai_r;
extern HTTPD_CFG httpd_cfg;

#ifdef MSGQ_PAIR_LOC_Q_IN_IDKEY
extern int m_http_msg_q_in_id;
extern int m_http_msg_q_out_id;
#endif

#ifdef __cplusplus
}
#endif

#endif // httpd_pai_r_H
