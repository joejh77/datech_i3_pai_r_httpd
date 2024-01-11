
//-------------------------------------------------------------------------------------------------
/*
4-07. «·«¹«Æ«àï×ÜÃö¢Ôð
4-07a. ßÙá¬
«·«¹«Æ«àï×ÜÃªòö¢Ôðª¹ªë
Îý?îÜªËªÏ«Õ«¡?«à«¦«§«¢«Ð?«¸«ç«óªÊªÉªòö¢Ôðª¹ªë
½Ã½ºÅÛ Á¤º¸¸¦ ÃëµæÇÑ´Ù.
±¸Ã¼ÀûÀ¸·Î´Â Æß¿þ¾î ¹öÀü µîÀ» ÃëµæÇÑ´Ù.

4-07b. API«Õ«¡«¤«ëÙ£äÐ
getSystem

4-07c. POSTáêãáö·
No	??Ù£	«Ç?«¿úþ	ù±âÎª«£¿	«³«á«ó«È	ÖÇ
£¨Ùíª·£©				
4-07d. JSONÚ÷ªêö·

{
    "is_success": true,
    "error": {
        "code": 0
    },
    "version": "1.0.0",
    "serial": "1234567890",
    "model": "VFPR",
    "manufacturer": "WiOPEN",
    "mac": "A0:B2:D5:7F:81:B3",
    "camera_type": [
        0,
        1
    ]
}
*/

#define PAI_R_API_GET_SYSTEM			"/getSystem"

class pai_r_api_getSystemHandler : public CivetHandler
{
	private:
	bool
	handleAll(const char *method, CivetServer *server, struct mg_connection *conn)
	{
		const char * strVer = DA_FIRMWARE_VERSION;
		char mac_addr[64] = {0,};
		char serial[64] = {0, };
		Json::Value root;
		Json::Value error;
		Json::Value camera_type(Json::arrayValue);
		

		const char * tmp_sn_path = DEF_DEFAULT_SSID_TMP_PATH;
		if(access(tmp_sn_path, R_OK ) == 0){
			u32 sn = 0;
			if(sysfs_scanf(tmp_sn_path, "%u", &sn)){
				sprintf(serial, "%u", sn);
			}
		}
		
//		if(httpd_cfg.serial_no != 2) //ÀÎÁõ ¹Þ¾Æ¾ß  Á¢¼Ó µÊ
			SB_GetMacAddress(mac_addr);
//		else //for test
//			strcpy(mac_addr, "70:3E:AC:EE:8B:17");

		root["is_success"] = true;
		{
			error["code"] = 0;
		}
		root["error"] = error;
		root["version"] = strchr(strVer, '/') + 1;
		root["serial"] = serial;
		root["model"] = "VRHD";
		root["mac"] = mac_addr;

		camera_type[0] = 0;
		
		if(httpd_cfg.rec_channel_count > 1)
			camera_type[1] = 1;

		root["camera_type"] = camera_type;
		
		Json::FastWriter writer;
		std::string content = writer.write(root);
		http_mg_printf(conn,content.c_str());

		dprintf(HTTPD_INFO, "RESPONSE: %s\n", content.c_str());
		http_printf(HTTPD_WARN, conn);
		return true;
	}

	public:
	bool
	handleGet(CivetServer *server, struct mg_connection *conn)
	{
		return handleAll("GET", server, conn);
	}
	bool
	handlePost(CivetServer *server, struct mg_connection *conn)
	{
		return handleAll("POST", server, conn);
	}
	bool
	handlePut(CivetServer *server, struct mg_connection *conn)
	{
		return handleAll("PUT", server, conn);
	}
};

