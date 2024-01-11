
#define PAI_R_API_UPDATE_FILE												"/updateFile"

class pai_r_api_updateFileHandler : public CivetHandler
{
	private:
	bool
	handleAll(const char *method, CivetServer *server, struct mg_connection *conn)
	{
		Json::Value error;
		bool IsSuccess = false;
		
		 const struct mg_request_info *ri = mg_get_request_info(conn);
	    //int query_len = 0;
		 //const char *query= ri->query_string;

		 //if(ri->query_string)
		 //	query_len = strlen(ri->query_string);
		g_form_data.key_v.clear();
			
		if(strcmp("POST", ri->request_method) == 0){
			int post_data_len;
			int post_data_size = KBYTE(500);
			char * post_data = (char *)sb_malloc(post_data_size);

			if(post_data == NULL)
			{
				printf("%s() : memory allocation error!\r\n", __func__);
				return false;
			}
			
			post_data_len = mg_read(conn, post_data, post_data_size);
			if(post_data_len){
				//printf("%s() : %s\r\n", __func__, post_data);

				multipart_parser* mp;

				char boundary[256] = { 0,};
				sscanf( post_data, "%s\r\n", boundary );
				//printf("%s() : %s\r\n", __func__, boundary);
				
				 multipart_parser_settings callbacks;

				memset(&callbacks, 0, sizeof(multipart_parser_settings));

				callbacks.on_header_field = read_header_name;
				callbacks.on_header_value = read_header_value;
				callbacks.on_part_data = read_part_data;

				callbacks.on_headers_complete = cb_on_headers_complete;
				callbacks.on_part_data_end = cb_on_part_data_end;

				mp= multipart_parser_init((const char *)boundary, &callbacks);
				//multipart_parser_set_data(mp, (void *)param);
				do{
					multipart_parser_execute(mp, (const char *) post_data, post_data_len);
					msleep(1);
					post_data_len = mg_read(conn, post_data, post_data_size);
				}while(post_data_len != -1 && post_data_len > 0);
				//printf("%s() : %s\r\n", __func__, mp->lookbehind);
				multipart_parser_free(mp);

				if(strcmp(g_form_data.key_v["file_hash"].c_str(), g_form_data.md5_string) == 0){
					IsSuccess = true;
				}
				else {
					IsSuccess = false;
					error["code"] = 999;
					error["message"] = "file hash error.";
				}
			}

			sb_free(post_data);
		}
		
		Json::Value root;
		
		root["is_success"] = IsSuccess;
		if(IsSuccess)
		{
			error["code"] = 0;
		}
		root["error"] = error;
		root["is_connect"] = httpd_cfg.server_connected ? true:false;
		
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



