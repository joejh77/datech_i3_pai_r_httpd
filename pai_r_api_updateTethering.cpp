
//-------------------------------------------------------------------------------------------------
/*
4-01b. API´’´°´§´ÎŸ£‰– API ∆ƒ¿œ ∏Ìæ»
updateTethering
4-01c. POST·Í„·ˆ∑
company_pai_r_id™œ´∆´∂´Í´Û´∞™À™œﬁ≈Èƒ™∑™ﬁ™ª™Û
DBﬂæ™« ¿ﬁ‰™»´…´È´§´÷´Ï´≥?´¿?“Ô™≈™±™Î™Œ™Àﬁ≈Èƒ™∑™ﬁ™π
company_pai_r_id¥¬ ≈◊¥ı∏µø°¥¬ ªÁøÎ«œ¡ˆ æ Ω¿¥œ¥Ÿ.
DB ªÛø°º≠ ∞¢ »∏ªÁøÕ µÂ∂Û¿Ã∫Í∑∫¥ı ø¨∞·¿ª «œ¥¬µ• ªÁøÎ«’¥œ¥Ÿ.

No	??Ÿ£	´«?´ø˙˛	˘±‚Œ™´£ø	´≥´·´Û´»	÷«
1	id	string	Yes	´∆´∂´Í´Û´∞:ID	admin
2	password	string	Yes	´∆´∂´Í´Û´∞:PASSWORD	dummypass
3	company_pai_r_id	integer	Yes	?ﬁ‰:Pai-RŒ∑◊‚ID	1
4-01d. JSON⁄˜™Íˆ∑
No	??Ÿ£	´«?´ø˙˛	´≥´·´Û´»	÷«
1	is_success	boolean	‡˜ÕÌ™∑™ø™´£øtrue===‡˜ÕÌ	true
2	error	÷ßﬂÃ€’÷™	´®´È??Èª	
2a	error.code	string	´®´È?€„?	999
2b	error.message	string	´®´È?´·´√´ª?´∏	Syntaxerror
3	mac	string	´…´È´§´÷´Ï´≥?´¿?™ŒMAC´¢´…´Ï´π	?4C:ED:FB:78:A5:AC
{
    "is_success": true,
    "error": {
        "code": 0
    },
    "mac": "?4C:ED:FB:78:A5:AC"
}
*/

#define PAI_R_API_UPDATE_TETHERING 									"/updateTethering"

class pai_r_api_updateTetheringHandler : public CivetHandler
{
  	private:
	bool
	handleAll(const char *method, CivetServer *server, struct mg_connection *conn)
	{
		const char * cmd_id = "id";
		const char * cmd_pw = "password";
		const char * cmd_pi = STR_COMPANY_ID;
		const char * cmd_tel = "tel";
		const char * cmd_ip = "application_ip";
		//const char * cmd_port = "application_port";
		
		bool IsSuccess = false;
		//int post_data_len;
		char post_data[2048] = {0,};
		const struct mg_request_info *ri = mg_get_request_info(conn);
		int query_len = 0;
		int query_cnt = 0;
		 const char *query= ri->query_string;

		 if(ri->query_string)
		 	query_len = strlen(ri->query_string);

		 g_form_data.key_v.clear();
		 
		if(strcmp("POST", ri->request_method) == 0){		
			post_data_parser(conn);
		}

		if(query_len ){
			if(mg_get_var(query, query_len, cmd_id, post_data, sizeof(post_data))  > 0){
				g_form_data.key_v[cmd_id] = post_data;
			}
			if(mg_get_var(query, query_len, cmd_pw, post_data, sizeof(post_data)) > 0){
				g_form_data.key_v[cmd_pw] = post_data;
			}
			if(mg_get_var(query, query_len, cmd_pi, post_data, sizeof(post_data)) > 0){
				g_form_data.key_v[cmd_pi] = post_data;
			}
			if(mg_get_var(query, query_len, cmd_tel, post_data, sizeof(post_data)) > 0){
				g_form_data.key_v[cmd_tel] = post_data;
			}
		}

		if(strlen(g_form_data.key_v[cmd_id].c_str()))
				query_cnt++;
		if(strlen(g_form_data.key_v[cmd_pw].c_str()))
				query_cnt++;
		if(strlen(g_form_data.key_v[cmd_pi].c_str()))
				query_cnt++;
		if(strlen(g_form_data.key_v[cmd_tel].c_str()))
				query_cnt++;
				
		if(query_cnt >= 4 || (query_cnt == 3 && strlen(g_form_data.key_v[cmd_pw].c_str())== 0))
			IsSuccess = true;
		
		Json::Value root;
		Json::Value error;

		char mac_addr[64] = { 0,};

//		if(httpd_cfg.serial_no != 2) //¿Œ¡ı πﬁæ∆æﬂ  ¡¢º” µ 
			SB_GetMacAddress(mac_addr);
//		else //for test
//			strcpy(mac_addr, "70:3E:AC:EE:8B:17");

		if(IsSuccess){
			dprintf(HTTPD_INFO, "id: %s\n", g_form_data.key_v[cmd_id].c_str());
			dprintf(HTTPD_INFO, "password: %s\n", g_form_data.key_v[cmd_pw].c_str());
			dprintf(HTTPD_INFO, STR_COMPANY_ID": %s\n", g_form_data.key_v[cmd_pi].c_str());
			dprintf(HTTPD_INFO, "tel: %s\n", g_form_data.key_v[cmd_tel].c_str());

			if(http_wifi_config_save(g_form_data.key_v[cmd_id].c_str(), g_form_data.key_v[cmd_pw].c_str(), g_form_data.key_v[cmd_tel].c_str(), g_form_data.key_v[cmd_pi].c_str()) == false)
			{	
				IsSuccess = false;
				error["message"] = "Config Save Error.";
			}
		}
		else 
		{
			error["message"] = "Syntax error";
		}
		
		root["is_success"] = IsSuccess;
		if(IsSuccess) {
			error["code"] = 0;
		}
		else {
			error["code"] = 999;
		}
		
		root["error"] = error;
		root["mac"] = mac_addr;
		
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

