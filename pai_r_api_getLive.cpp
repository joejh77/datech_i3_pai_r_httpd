/*
4-03. «««á«éðàïÚ Ä«¸Þ¶ó Á¶Á¤
4-03a. ßÙá¬
«É«é«¤«Ö«ì«³?«À?ªÏ«¢«×«êªË?ª·¡¢«««á«éªÎÊÇÓøðàïÚªÎª¿ªáªÎ«ê«¢«ë«¿«¤«àªÊç±ßÀªòøúãÆª·ªÞª¹
¡ØªÇª­ªìªÐ«¹«ï«¤«×ªÇ«««á«éªòï·ªêôðª¨ª¿ª¤
µå¶óÀÌºê·¹ÄÚ´õ´Â ¾Û¿¡ ´ëÇØ, Ä«¸Þ¶óÀÇ °¢µµ Á¶Á¤À» À§ÇÑ ¸®¾ó Å¸ÀÓ ¿µ»óÀ» Ç¥½ÃÇÕ´Ï´Ù.
°¡´ÉÇÏ¸é ½º¿ÍÀÌÇÁ·Î Ä«¸Þ¶ó¸¦ ¹Ù²Ù°í ½Í´Ù.
ÖÇ:AbemaTVªßª¿ª¤ªÊUI

4-03b. API«Õ«¡«¤«ëÙ£äÐ
getLive
*/

#define PAI_R_API_GET_LIVE														"/getLive"

class pai_r_api_getLiveHandler : public CivetHandler
{
  private:
	
	bool
	handleAll(const char *method, CivetServer *server, struct mg_connection *conn)
	{
		Json::Value root;
		Json::Value error;

		char post_data[2048] = {0,};
		
		const struct mg_request_info *req_info = mg_get_request_info(conn);
		int param_len = 0;
		const char *param = req_info->query_string;

		int32_t width = 1280, height = 720, camera = 0;
		g_form_data.key_v.clear();
		g_form_data.key_v["width"] = std::to_string(width);
		g_form_data.key_v["height"] = std::to_string(height);
		g_form_data.key_v["camera"] = std::to_string(camera);
		
		printf("%s() : %s\r\n", __func__, req_info->request_method);
	
		if(strcmp("POST", req_info->request_method) == 0){
			post_data_parser(conn);
		}

		if(req_info->query_string) {
		 	param_len = strlen(req_info->query_string);
		
			if(param_len) {
				if(mg_get_var(param, param_len, "cmd", post_data, sizeof(post_data)) > 0)
					g_form_data.key_v["cmd"] = post_data;
				
				if(mg_get_var(param, param_len, "width", post_data, sizeof(post_data)) > 0)
					g_form_data.key_v["width"] = post_data;

				if(mg_get_var(param, param_len, "height", post_data, sizeof(post_data)) > 0)
					g_form_data.key_v["height"] = post_data;

				if(mg_get_var(param, param_len, "camera", post_data, sizeof(post_data)) > 0)
					g_form_data.key_v["camera"] = post_data;

				if(mg_get_var(param, param_len, "file", post_data, sizeof(post_data)) > 0)
					g_form_data.key_v["file"] = post_data;
			}
		}

#if 0 // for test
		if(strcmp("download", g_form_data.key_v["cmd"].c_str()) == 0) {
				const char * file_name = g_form_data.key_v["file"].c_str();
				
				dprintf(HTTPD_INFO, "++ %s %s\r\n", g_form_data.key_v["cmd"].c_str(), file_name );
								
				if(access(file_name, R_OK ) != 0) {
					mg_printf(conn, "HTTP/1.1 404 Not Found (request file : %s)\r\n", file_name);
				} else {
					std::string msgs;

									 
					 {

						FILE *fp = fopen(file_name, "r");
						int ret;
						int file_size = 0;
						char buf[1024];
						
						if (fp < 0) {
							mg_printf(conn, "HTTP/1.1 404 Not Found (file open error!: %s)\r\n", file_name);
						}else {

							int length = 0;
							fseek( fp, 0, SEEK_END );
							length = ftell( fp );
							fseek( fp, 0, SEEK_SET );
							
							mg_printf(conn, "HTTP/1.1 200 OK\r\n");
							mg_printf(conn, "Accept-Ranges: bytes\r\n");
							mg_printf(conn, "Content-Length: ");
							mg_printf(conn, "%d", length);
							mg_printf(conn, "\r\nContent-Type: %s\r\n\r\n", CHttp_multipart::get_content_type(file_name) );
								
							do{
								ret = fread((void *)buf, 1, sizeof(buf), fp);
									
								if(ret) {
									mg_write(conn, buf, ret);
									file_size += ret;
								}
							}while(ret > 0);

							fclose(fp);
							
							mg_printf(conn, "\r\n");

							dprintf(HTTPD_INFO, "-- %s %s(%d:%d Byte)\r\n", g_form_data.key_v["cmd"].c_str(), file_name, length, file_size);
						}
					 }
				}
			
		}
		else 
#endif			
#if 0			
		if(strcmp("snapshot", g_form_data.key_v["cmd"].c_str()) == 0) {
				struct timeval timestamp;
				std::vector<char> image_data;
				
				dprintf(HTTPD_INFO, "%s[%d] %s, %s, %s\r\n", g_form_data.key_v["cmd"].c_str(), strcmp("snapshot", g_form_data.key_v["cmd"].c_str()), g_form_data.key_v["camera"].c_str(), g_form_data.key_v["width"].c_str(), g_form_data.key_v["height"].c_str() );
								
				if(oasis::takePicture(camera/*camera id*/, g_form_data.key_v, image_data, &timestamp) < 0) {
					mg_printf(conn, "HTTP/1.1 408 Request Timeout\r\n");
				} else {
					std::string msgs;
					
					mg_printf(conn, "HTTP/1.1 200 OK\r\n");
					mg_printf(conn, "Accept-Ranges: bytes\r\n");
					mg_printf(conn, "Content-Length: ");
					mg_printf(conn, "%d", image_data.size());
					mg_printf(conn, "\r\nContent-Type: image/jpeg\r\n\r\n");
					mg_write(conn, image_data.data(), image_data.size());
					mg_printf(conn, "\r\n");
				}
			
		}
		else 
#endif			
		{
			char ip_addr[32];

			//strcpy(ip_addr, inet_ntoa(SB_GetIp("wlan0")));
			getIPAddress(ip_addr);

			for(int ch = 0 ; ch < httpd_cfg.rec_channel_count ; ch ++) {
				char url_name[PATH_MAX];
				char url_cam[PATH_MAX];
				sprintf(url_name, "url_rtsp%d", ch);
				sprintf(url_cam, "rtsp://%s:%d/resolution=360p&bitrate=1500000/Live/%d", ip_addr, 554 /*_rtsp_server_port*/, ch);
				root[url_name] = url_cam;

				sprintf(url_name, "url_jpg%d", ch);
#if  defined(HTTPD_EMBEDDED) || defined(HTTPD_MSGQ_SNAPSHOT)
				sprintf(url_cam, "http://%s:%d/command?cmd=snapshot&width=640&height=360&camera=%d", ip_addr, httpd_cfg.httpd_port /*_http_server_port*/, ch);
#else
				sprintf(url_cam, "http://%s:%d/command?cmd=snapshot&width=640&height=360&camera=%d", ip_addr, 8080 /*_http_server_port*/, ch);
#endif
				root[url_name] = url_cam;
			}	
			
			root["is_success"] = true;
			{
				error["code"] = 0;
			}
			root["error"] = error;
		
			Json::FastWriter writer;
			std::string content = writer.write(root);
			http_mg_printf(conn,content.c_str());

			dprintf(HTTPD_INFO, "RESPONSE: %s\n", content.c_str());
			http_printf(HTTPD_WARN, conn);
		}
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

