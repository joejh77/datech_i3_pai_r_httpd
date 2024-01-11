
//-------------------------------------------------------------------------------------------------
/*
4-05b. API«Õ«¡«¤«ëÙ£äÐ
getUpdateLog

4-05c. POSTáêãáö·
No	??Ù£	«Ç?«¿úþ	ù±âÎª«£¿	«³«á«ó«È	ÖÇ
1	start_datetime	datetime	No	?ËìªËìéöÈª·ª¿ÔÑ?ªÎõÉç¯ËÒã·ìíãÁË½â÷ªÇª¤ªÄª«ªéÚ÷ª¹ª«£¿àýÕÔãÁªÏúÞî¤ÐññÞªËªÊªêªÞª¹	2020-02-20 11:11:11
2	count	integer	No	õÌÓÞªÇìé?ö¢ÔðªÇª­ªë?àýÕÔãÁªÏ20Ëì	30
3	data_type integer YES «í«°:«Ç?«¿ðúÜ¬  0===ìã? 1===?ÖØ 2===ÔÑ?

4-05d. JSONÚ÷ªêö·
«í«°ªÎÚ÷ªêö·ªÏíÂà÷ìíãÁË½â÷ªÈª·ªÞª¹

No	??Ù£	«Ç?«¿úþ	«³«á«ó«È	ÖÇ
1	is_success	boolean	à÷Ííª·ª¿ª«£¿true===à÷Íí	true
2	error	Ö§ßÌÛÕÖª	«¨«é??é»	
2a	error.code	string	«¨«é?Ûã?	999
2b	error.message	string	«¨«é?«á«Ã«»?«¸	Syntaxerror
3	max	integer	?ËìªËìéöÈª¹ªë«í«°:?? 
Á¶°Ç¿¡ ÀÏÄ¡ÇÏ´Â ·Î±×: ÃÑ¼ö	1
4	end_datetime	datetime	listªÎª¦ªÁõÌý­ªÎ«í«°ªËªªª±ªëíÂà÷ìíãÁ
list Áß ¸¶Áö¸· ·Î±×¿¡¼­ ÀÛ¼º ÀÏ½Ã	2020-02-20 11:31:11
5	list	ÛÕÖª	«í«°	
5a	list.autoid	integer	«í«°:ÔÑ?í»ÔÑID
·Î±×: µ¿¿µ»ó ÀÚµ¿ ¾ÆÀÌµð	1
5b	list.create_datetime	datetime	«í«°:«í«°íÂà÷ìíãÁ
·Î±× : ·Î±× ÀÛ¼º ÀÏ½Ã	2020-02-20 11:31:11
5c	list.movie_create_datetime	datetime	«í«°:ÔÑ?íÂà÷ìíãÁ
¡Ølist.data_type===0:ìã?ªÊªéNULLªòàâïÒ
·Î±×:µ¿¿µ»ó ÀÛ¼º ÀÏÀÚ
(list.data_type===0:ÀÎÁõÀº NULLÀ» ¼³Á¤	2020-02-20 11:31:11
5d	list.send_datetime	datetime	«í«°:áêãáËÒã·ìíãÁ
·Î±×: Àü¼Û ½ÃÀÛ ÀÏ½Ã	2020-02-20 11:41:11
5e	list.data_type	integer	«í«°:«Ç?«¿ðúÜ¬
0===ìã?
1===?ÖØ
2===ÔÑ?
·Î±×:µ¥ÀÌÅÍ Á¾º°
0===ÀÎÁõ
1===°æ·Î
2===µ¿¿µ»ó	1
5f	list.result_type	integer	«í«°:Ì¿ÍýðúÜ¬
0===à÷Íí
1===«¨«é?:ïÈ?ã÷ø¨
2===«¨«é?:ïÈ?à÷Íí?API«¨«é?
·Î±×:°á°ú Á¾º°
0===¼º°ø
1===¿¡·¯:Á¢¼Ó ½ÇÆÐ
2===¿¡·¯:Á¢¼Ó ¼º°ø, API¿À·ù	1
5g	list.result_json	JSON	«í«°:APIª«ªéªÎÚ÷ªêö·
¡Ølist.result_type===1ªÊªéàýÕÔ
·Î±×:API¿¡¼­ ¹ÝÈ¯ °ª
(list.result_type===1ÀÌ¸é »ý·«	£¨àýÕÔ
»ý·«£©
5h	list.comment	string	«í«°:«³«á«ó«È
¡ØÖÇ£º÷×ãá«¿«¤«à«¢«¦«ÈªËªÊªÃª¿ãÁ¡¢result_jsonªÏÍöªÇª³ªÁªéªËßÙá¬ªòÑÀî°ª¹ªë
·Î±×: ÄÚ¸àÆ®
¡Ø¿¹: Åë½Å Å¸ÀÓ¾Æ¿ôÀÌ µÇ¾úÀ» ¶§, result_jsonÀº °øÀ¸·Î ÀÌÂÊ¿¡ ¼¼ºÎ»çÇ×À» ±âÀçÇÑ´Ù.
*/

#define PAI_R_API_GET_UPDATELOG	"/getUpdateLog"

u32 		m_last_log_no = 0;
u32 		m_last_read_log_item_no = 0;
time_t 	m_last_read_log_item_time = 0;

class pai_r_api_getUpdateLogHandler : public CivetHandler
{
  	private:
		
	bool
	handleAll(const char *method, CivetServer *server, struct mg_connection *conn)
	{
		const char * str_start_datetime = "start_datetime";
		const char * str_count = "count";
		const char * str_data_type = "data_type";
		Json::Value jsonUpdateLog;
		
		Json::Value error;
		
		bool IsSuccess = false;
		
		 const struct mg_request_info *ri = mg_get_request_info(conn);
	    //int query_len = 0;
		//const char *query= ri->query_string;

		 mg_printf(conn,
	          "HTTP/1.1 200 OK\r\nContent-Type: "
	          "text/html\r\nConnection: close\r\n\r\n{"); //start http and json

		g_form_data.key_v.clear();
		
		if(strcmp("POST", ri->request_method) == 0){
			post_data_parser(conn);
		}

		IsSuccess = true;

		if(IsSuccess){

			 mg_printf(conn,"\"is_success\":true,\"error\":{\"code\":0},\"list\":[");
	          
			if(strlen(g_form_data.key_v[str_start_datetime].c_str()))
				jsonUpdateLog[str_start_datetime] = g_form_data.key_v[str_start_datetime].c_str();

			if(strlen(g_form_data.key_v[str_count].c_str()))
				jsonUpdateLog[str_count] = atoi(g_form_data.key_v[str_count].c_str());

			if(jsonUpdateLog[str_count].asInt() == 0)
				jsonUpdateLog[str_count] = 20; // »ý·«¼ºÀº 20°Ç

			if(strlen(g_form_data.key_v[str_data_type].c_str()))
				jsonUpdateLog[str_data_type] = atoi(g_form_data.key_v[str_data_type].c_str());
			else
				jsonUpdateLog[str_data_type] = UL_DATA_TYPE_ALL;
			

			dprintf(HTTPD_INFO, "%s : %s\n", str_start_datetime, jsonUpdateLog[str_start_datetime].asString().c_str());
			dprintf(HTTPD_INFO, "%s : %d\n", str_count, (int)jsonUpdateLog[str_count].asInt());

			u32 read_log_no = pai_r.updatelog.m_log_no - 1;
			int list_count = 0;
			time_t last_time = 0;
//			time_t current_time;
			time_t start_time = time(0);			

			if(jsonUpdateLog[str_start_datetime].isString())
				start_time = pai_r_time_string_to_time(jsonUpdateLog[str_start_datetime].asString());
			
			if(m_last_read_log_item_time && m_last_read_log_item_time > start_time){
				if(m_last_read_log_item_no < read_log_no)
					read_log_no = m_last_read_log_item_no;
			}
			else
				m_last_log_no = 0;
			
			m_last_read_log_item_no = 0;
			m_last_read_log_item_time = 0;

			FILE *fp = fopen(DEF_UPDATELOG_PATH, "rb");
	
			for(int i = 0; i < UPDATELOG_MAX_COUNT && list_count < jsonUpdateLog[str_count].asInt(); i++) {
				u32 queue_no = read_log_no;
				PAI_R_UPDATELOG log_data;
				
				if(!pai_r.updatelog.get_updatelog(queue_no, &log_data, fp))
					break;

				last_time = log_data.create_time;
				
//				if(i == 0)
//					current_time = last_time;
				
				read_log_no--;

				if(last_time >= start_time)
					continue;
				
				if(log_data.date_type >= UL_DATA_TYPE_END)
					continue;

				if(jsonUpdateLog[str_data_type].asInt() != UL_DATA_TYPE_ALL){
					if(jsonUpdateLog[str_data_type].asInt() != (int)log_data.date_type)
						continue;
				}
					
				m_last_read_log_item_no = read_log_no;
				m_last_read_log_item_time = last_time;		
				
				if(list_count == 0)
					mg_printf(conn,"{");
				else
					mg_printf(conn,",{");
				
				list_count++;
				m_last_log_no++;
				
				mg_printf(conn,"\"autoid\":%u", log_data.auto_id);
				mg_printf(conn,",\"create_datetime\":\"%s\"", last_time == 0 ? "" : make_time_string(last_time).c_str());
				mg_printf(conn,",\"movie_create_datetime\":\"%s\"", log_data.movie_create_time == 0 ? "" : make_time_string(log_data.movie_create_time).c_str());
				mg_printf(conn,",\"send_datetime\":\"%s\"", log_data.send_time == 0 ? "" : make_time_string(log_data.send_time).c_str());
				
				mg_printf(conn,",\"data_type\":%d", (int)log_data.date_type);
				mg_printf(conn,",\"result_type\":%d", (int)log_data.result_type);
				
				if(log_data.result_type < UL_RESULT_TYPE_END && strlen(log_data.result_json)){
					Json::Reader reader;
					Json::Value root;
					reader.parse(log_data.result_json, root);

					if(root.type() == Json::objectValue){
						mg_printf(conn,",\"result_json\":%s", log_data.result_json);
					}
				}
				
				mg_printf(conn,",\"comment\":\"%s\"", log_data.comment);

				mg_printf(conn,"}");

			}		

			fclose(fp);
			
			if(list_count)
				mg_printf(conn,"],\"max\":%u,\"end_datetime\":\"%s\"", list_count, make_time_string(last_time).c_str());
			else
				mg_printf(conn,"],\"max\":%u", list_count);			
		}

		if(!IsSuccess) {
			 if(!error["code"].size()){
				error["code"] = ERROR_UNDEFINED;
				error["message"] = "undefined error.";
			}

			 mg_printf(conn,"\"is_success\":false,\"error\":{\"code\":%u,\"message\":\"%s\"}", error["code"].asUInt(), error["message"].asString().c_str());
		}
	
		mg_printf(conn, "}\r\n"); //end json and http
	
		//Json::FastWriter writer;
		//std::string content = writer.write(root);
		//http_mg_printf(conn,content.c_str());

		//dprintf(HTTPD_INFO, "RESPONSE: %s\n", content.c_str());
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

