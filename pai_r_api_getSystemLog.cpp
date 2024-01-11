
//-------------------------------------------------------------------------------------------------
/*
4-08. «·«¹«Æ«à«í«°ö¢Ôð
½Ã½ºÅÛ ·Î±× Ãëµæ
4-08a. ßÙá¬
ò¦ïÒìíªËªªª±ªëò¦ïÒª·ª¿«·«¹«Æ«à?Ìõ£¨£½«¢«Ã«×«Ç?«Èªòð¶ª¯£©«í«°ªòö¢Ôðª¹ªë
«á«ó«Æ«Ê«ó«¹éÄ
ÁöÁ¤ÀÏ¿¡ ÀÖ¾î¼­ ÁöÁ¤ÇÑ ½Ã½ºÅÛ °ü°è(=¾÷µ¥ÀÌÆ®¸¦ Á¦¿ÜÇÑ´Ù) ·Î±×¸¦ ÃëµæÇÑ´Ù. À¯Áö °ü¸®¿ë

4-08b. API«Õ«¡«¤«ëÙ£äÐ
getSystemLog

4-08c. POSTáêãáö·
No	??Ù£	«Ç?«¿úþ	ù±âÎª«£¿	«³«á«ó«È	ÖÇ
1	type	integer	Yes	«í«°«Ç?«¿ªÎðú×¾
·Î±× µ¥ÀÌÅÍÀÇ Á¾·ù	1
2	day_ago	integer	No	ù¼ìíîñªÎ«í«°ªòö¢Ôðª¹ªëª«£¿
àýÕÔãÁ = 0
¸çÄ¥ Àü ·Î±×¸¦ ÃëµæÇÒ °ÍÀÎ°¡?
»ý·«½Ã = 0
*/

#define PAI_R_API_GET_SYSTEMLOG	"/getSystemLog"


class pai_r_api_getSystemLogHandler : public CivetHandler
{
  	private:
		
	bool
	handleAll(const char *method, CivetServer *server, struct mg_connection *conn)
	{
		const char * str_type = "type";
		const char * str_day_ago = "day_ago"; // ¸çÄ¥ Àü ·Î±×¸¦ ÃëµæÇÒ °ÍÀÎ°¡?<br> »ý·«½Ã = 0
		const char * str_how_many_days_ago = "how_many_days_ago"; //¿À´ÃºÎÅÍ ¸îÀÏÀüÀÇ ·Î±×°¡ Á¸ÀçÇÏ´Â°¡?
		int day_ago = 0;
		int how_many_days_ago = 0;
		int request_type = 0;
		
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

		if(strlen(g_form_data.key_v[str_type].c_str())) {
			request_type = atoi(g_form_data.key_v[str_type].c_str());		
			IsSuccess = true;
		}
		else {
				error["code"] = ERROR_NO_NEED_ITEM;
				error["message"] = "There are no request_type variables.";
		}

		if(IsSuccess){

			 mg_printf(conn,"\"is_success\":true,\"error\":{\"code\":0},\"list\":[");

			if(strlen(g_form_data.key_v[str_day_ago].c_str()))
				day_ago = atoi(g_form_data.key_v[str_day_ago].c_str());
			
			dprintf(HTTPD_INFO, "%s : %d\n", str_type, request_type);
			dprintf(HTTPD_INFO, "%s : %d\n", str_day_ago, day_ago);

			u32 read_log_no = 0;
			int list_count = 0;
			const char * file_path = DEF_SYSTEM_LOG_PATH_SPI;

			ST_LOG_ITEM *p_log;
			int ret;
			int buf_size = SYSTEMLOG_MAX_COUNT * sizeof(ST_LOG_ITEM);
			u8* buf = new u8[ buf_size ];
			
			u32 current_log_no = 0;
			u32 last_log_no = SYSTEMLOG_MAX_COUNT - 1;
			
			FILE* file=  fopen( file_path, "r");
			//int length = 0;

			time_t current_time = time(0);
			current_time -= (current_time % (60*60*24));

			if(!file){
				dbg_printf(HTTPD_ERROR," [%s] file open error!\n", file_path);
			}
			else {					
				memset((void *)buf, 0, buf_size);
				
				ret =  fread( (void *)buf, 1, buf_size, file );
				if (ret != buf_size ) {
					dbg_printf(HTTPD_ERROR, "%s read failed: %d(%s) , ret = %d : %d\n", file_path, errno, strerror(errno), ret, sizeof(ST_LOG_ITEM));
				}

				for(int i = 0; i < SYSTEMLOG_MAX_COUNT; i++){
					p_log = (ST_LOG_ITEM *)&buf[i*sizeof(ST_LOG_ITEM)];

					if(p_log->log_no && p_log->log_no < last_log_no)
						last_log_no = p_log->log_no;
					
					if(p_log->log_no > current_log_no)
						current_log_no = p_log->log_no;
					else
						break;
				}

				p_log = (ST_LOG_ITEM *)&buf[(last_log_no % SYSTEMLOG_MAX_COUNT)*sizeof(ST_LOG_ITEM)];

				how_many_days_ago = (current_time / (60*60*24)) -  (p_log->create_time / (60*60*24));
				
				dbg_printf(HTTPD_ERROR, " %d (%u) Last Log NO:%d DATE:%s\r\n", last_log_no, current_time, p_log->log_no, make_time_string(p_log->create_time).c_str());
				
				read_log_no = (current_log_no - 1) % SYSTEMLOG_MAX_COUNT;
				
				for(int i = 0; i < SYSTEMLOG_MAX_COUNT; i++){
					char str_log_type[64];
					p_log = (ST_LOG_ITEM *)&buf[read_log_no*sizeof(ST_LOG_ITEM)];

					if(p_log->log_no == 0 && list_count)
						break;

					if(read_log_no == 0)
							read_log_no = SYSTEMLOG_MAX_COUNT - 1;
						else
							read_log_no--;
						
					if(p_log->create_time > (current_time - (60*60*24 * day_ago) + (60*60*24)))
						continue;
					
					if(p_log->create_time < (current_time - (60*60*24 * day_ago)))
						break;

					if(CSystemlog::get_item_type_string(str_log_type, p_log)) {
						
						if(request_type != 0 && request_type != p_log->type)
							continue;
						
						if(list_count != 0)
							mg_printf(conn,",");	
						
						list_count++;
						mg_printf(conn,"{\"autoid\":%u", p_log->log_no);
						mg_printf(conn,",\"type\":%u", p_log->type);
						mg_printf(conn,",\"create_datetime\":\"%s\"", make_time_string(p_log->create_time).c_str());

						p_log->data.byte[sizeof(p_log->data.byte)-1] = 0;
						
						mg_printf(conn,",\"system_log\":\"%s\\t\\t%s%s\"}", \
							str_log_type, \
							(char *)p_log->data.byte, \
							(p_log->log_sub & LOG_ITEM_DATA_ELLIPSIS_FLAG) ? "..." : "");
					}
				}
				
				fclose(file);	
			}

			delete [] buf;			

			mg_printf(conn,"],\"%s\":%d", str_how_many_days_ago, how_many_days_ago);			
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

