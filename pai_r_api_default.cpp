/////////////////////////////////////////////////////////////////////////////
#if 0
static const char *html_form =
    "<html><body>WiFi SSID Setting"
    "<form method=\"POST\" action=\"/handle_post_request\">"
    "«Æ«¶«ê«ó«°-SSID	: <input type=\"text\" name=\"input_1\" value=\"%s\" /> <br/>"
    "«Æ«¶«ê«ó«°-PW		: <input type=\"password\" name=\"input_2\" value=\"%s\" /> <br/>"
    "Î·×â-ID   				: <input type=\"text\" name=\"input_3\" value=\"%s\" /> <br/>"
    "<input type=\"submit\" />"
    "</form></body></html>";
#endif

void __printf_ri(const struct mg_request_info *ri)
{
	dprintf(HTTPD_INFO, "request_method : %s \r\n", ri->request_method);
	dprintf(HTTPD_INFO, "request_uri : %s \r\n", ri->request_uri);
	dprintf(HTTPD_INFO, "local_uri : %s \r\n", ri->local_uri);
	dprintf(HTTPD_INFO, "uri : %s \r\n", ri->uri);
	dprintf(HTTPD_INFO, "http_version : %s \r\n", ri->http_version);
	dprintf(HTTPD_INFO, "query_string : %s \r\n", ri->query_string);
	dprintf(HTTPD_INFO, "remote_user : %s \r\n", ri->remote_user);
	dprintf(HTTPD_INFO, "remote_addr : %s \r\n", ri->remote_addr);
}

void __hashfile_write(char *md5_hash, const char * file_path)
{
	FILE *fp = fopen(file_path, "wb");

	if(fp){
		fwrite(md5_hash, 1, strlen(md5_hash), fp);
		fclose(fp);
		dprintf(HTTPD_INFO, "%s  : %s\r\n", file_path, md5_hash);
	} else {
		dprintf(HTTPD_ERROR, "%s creation failed: %d(%s)\n", file_path, errno, strerror(errno));
	}
}

static int begint_file_upload_handler(struct mg_connection *conn)
{	
	const struct mg_request_info *ri = mg_get_request_info(conn);
	if(strcmp("GET", ri->request_method) == 0) {
			const char * str_sd_root = "/mnt/extsd/";
			const char * str_hash = "hash";
			const char * file_path = ri->request_uri;

			char hash_file_path[128] = { 0,};
			char file_ext[32] = { 0,};
			
			const char *lastSeparator = strrchr(file_path, '/');
			const char *lastDot = strrchr(file_path, '.');
		    const char *endOfPath = file_path + strlen(file_path);
		    const char *startOfName = lastSeparator ? lastSeparator + 1 : file_path;
		    const char *startOfExt = lastDot > startOfName ? lastDot : endOfPath;

			sprintf(file_ext, "%s", startOfExt + 1);
			
			
			if(strncmp(str_sd_root, file_path, strlen(str_sd_root)) != 0 && strcmp(file_ext, str_hash) != 0)
				return 0;
				
			dprintf(HTTPD_INFO, "++ GET : %s\r\n", file_path);

			if(strcmp(file_ext, str_hash) == 0){
				sprintf(hash_file_path, "/tmp/%s", startOfName);
				
				if(access(hash_file_path, R_OK ) != 0) { // ÆÄÀÏ ¾øÀ½
					char md5_hash[33] = {0,};
					char 	target_file_path[64];
					char *target_file_lastDot;
					
					strcpy(target_file_path, file_path);
					target_file_lastDot = strrchr(target_file_path, '.');

					if(lastDot)
						target_file_lastDot[0] = 0;

					if(get_md5_file_hash(md5_hash, target_file_path)){
						__hashfile_write(md5_hash, hash_file_path);
					}					
				}

				file_path = hash_file_path;
			}
			else {
				sprintf(hash_file_path, "/tmp/%s.%s", startOfName, str_hash);
			}
							
			if(access(file_path, R_OK ) == 0) {
					std::string msgs;

					FILE *fp = fopen(file_path, "r");
					int ret;
					int file_size = 0;
					
					if (fp > 0) {
						md5_state_t ctx;
						md5_byte_t hash[16];
						char md5_hash[33] = {0,};
						
						int length = 0;
						char * buf = new char[ DEF_FILE_READ_BUFFER_SIZE ];

						if(buf == NULL){
							fclose(fp);

							mg_printf(conn, "HTTP/1.1 500 internal server error\r\n");
							dprintf(HTTPD_INFO, "-- HTTP/1.1 500 internal server error\r\n");
							return 1;
						}

						md5_init(&ctx);
						
						fseek( fp, 0, SEEK_END );
						length = ftell( fp );
						fseek( fp, 0, SEEK_SET );
						
						mg_printf(conn, "HTTP/1.1 200 OK\r\n");
						mg_printf(conn, "Accept-Ranges: bytes\r\n");
						mg_printf(conn, "Content-Length: ");
						mg_printf(conn, "%d", length);
						mg_printf(conn, "\r\nContent-Type: %s\r\n\r\n", CHttp_multipart::get_content_type(file_path) );
							
						do{
							ret = fread((void *)buf, 1, DEF_FILE_READ_BUFFER_SIZE, fp);
								
							if(ret) {
								mg_write(conn, buf, ret);
								file_size += ret;
								
								md5_append(&ctx, (const md5_byte_t *)buf, ret);
								
								msleep(1);								
							}
						}while(ret > 0);

						fclose(fp);
						delete[] buf;
						
						mg_printf(conn, "\r\n");

						if(file_path != hash_file_path) {
							system("rm /tmp/*.hash");
							
							md5_finish(&ctx, hash);
							bin2str(md5_hash, hash, sizeof(hash));
							
							__hashfile_write(md5_hash, hash_file_path);
						}
							
						dprintf(HTTPD_INFO, "-- %s(%d:%d Byte)\r\n", file_path, length, file_size);
						return 1;
				 }
			}

			mg_printf(conn, "HTTP/1.1 404 Not Found (request file : %s)\r\n",  ri->request_uri);
			dprintf(HTTPD_INFO, "-- HTTP/1.1 404 Not Found (request file : %s)\r\n",  ri->request_uri);
			return 1;
	}	

	return 0;
}

static int begin_request_handler(struct mg_connection *conn)
{
#if 0
    const struct mg_request_info *ri = mg_get_request_info(conn);
    char post_data[1024], input1[sizeof(post_data)], input2[sizeof(post_data)], input3[sizeof(post_data)];
    int post_data_len;

	__printf_ri(ri);
	
    if (!strcmp(ri->uri, "/handle_post_request")) {
        // User has submitted a form, show submitted data and a variable value
        post_data_len = mg_read(conn, post_data, sizeof(post_data));

		printf("%s(): %s\r\n", __func__, post_data);
        // Parse form data. input1 and input2 are guaranteed to be NUL-terminated
        if(mg_get_var(post_data, post_data_len, "input_1", input1, sizeof(input1)) > 0 && \
        	mg_get_var(post_data, post_data_len, "input_2", input2, sizeof(input2)) > 0 && \
		 	mg_get_var(post_data, post_data_len, "input_3", input3, sizeof(input3)) > 0) {

			int pw_len = strlen(input2);
			char pw_star[1024];
			memset((void *)pw_star, '*', pw_len);
			pw_star[pw_len] = 0;
			
	        // Send reply to the client, showing submitted form values.
	        mg_printf(conn, "HTTP/1.0 200 OK\r\n"
	                  "Content-Type: text/plain\r\n\r\n"
	                  "WiFi SSID Setting is completed\n\n"
	                  "SSID : [%s]\n"
	                  "PW   : [%s]\n"
	                  "PAI-R_ID  : [%s]\n",
	                  input1, pw_star, input3);

				http_wifi_config_save(input1, input2, input3);
				return 1;
        }
    }
	
  	int strlen = sprintf(post_data, html_form, pai_r.cfg.strWiFiSsid[0], pai_r.cfg.strWiFiPassword[0], pai_r.cfg.strSeriarNo);
    // Show HTML form.
    mg_printf(conn, "HTTP/1.0 200 OK\r\n"
              "Content-Length: %d\r\n"
              "Content-Type: text/html\r\n\r\n%s",
              strlen, post_data);
#endif

    return 0;  // Mark request as processed
}

//-------------------------------------------------------------------------------------------------
class FooHandler : public CivetHandler
{
  public:
	bool
	handleGet(CivetServer *server, struct mg_connection *conn)
	{
#if 1
		if(!begint_file_upload_handler(conn))
			begin_request_handler(conn);
#else
		/* Handler may access the request info using mg_get_request_info */
		const struct mg_request_info *req_info = mg_get_request_info(conn);

		mg_printf(conn,
		          "HTTP/1.1 200 OK\r\nContent-Type: "
		          "text/html\r\nConnection: close\r\n\r\n");

		mg_printf(conn, "<html><body>\n");
		mg_printf(conn, "<h2>This is the Foo GET handler!!!</h2>\n");
		mg_printf(conn,
		          "<p>The request was:<br><pre>%s %s %s HTTP/%s</pre></p>\n",
		          req_info->request_method,
		          req_info->uri,
		          req_info->query_string,
		          req_info->http_version);
		mg_printf(conn, "</body></html>\n");
#endif
		return true;
	}
	bool
	handlePost(CivetServer *server, struct mg_connection *conn)
	{
#if 1
		begin_request_handler(conn);
#else	
		/* Handler may access the request info using mg_get_request_info */
		const struct mg_request_info *req_info = mg_get_request_info(conn);
		long long rlen, wlen;
		long long nlen = 0;
		long long tlen = req_info->content_length;
		char buf[1024];

		mg_printf(conn,
		          "HTTP/1.1 200 OK\r\nContent-Type: "
		          "text/html\r\nConnection: close\r\n\r\n");

		mg_printf(conn, "<html><body>\n");
		mg_printf(conn, "<h2>This is the Foo POST handler!!!</h2>\n");
		mg_printf(conn,
		          "<p>The request was:<br><pre>%s %s HTTP/%s</pre></p>\n",
		          req_info->request_method,
		          req_info->uri,
		          req_info->http_version);
		mg_printf(conn, "<p>Content Length: %li</p>\n", (long)tlen);
		mg_printf(conn, "<pre>\n");

		while (nlen < tlen) {
			rlen = tlen - nlen;
			if (rlen > sizeof(buf)) {
				rlen = sizeof(buf);
			}
			rlen = mg_read(conn, buf, (size_t)rlen);
			if (rlen <= 0) {
				break;
			}
			wlen = mg_write(conn, buf, (size_t)rlen);
			if (rlen != rlen) {
				break;
			}
			nlen += wlen;
		}

		mg_printf(conn, "\n</pre>\n");
		mg_printf(conn, "</body></html>\n");
#endif
		return true;
	}

    #define fopen_recursive fopen

    bool
        handlePut(CivetServer *server, struct mg_connection *conn)
    {
    #if 0
        /* Handler may access the request info using mg_get_request_info */
        const struct mg_request_info *req_info = mg_get_request_info(conn);
        long long rlen, wlen;
        long long nlen = 0;
        long long tlen = req_info->content_length;
        FILE * f;
        char buf[1024];
        int fail = 0;
			
        snprintf(buf, sizeof(buf), "./%s/%s", req_info->remote_user, req_info->local_uri);
        buf[sizeof(buf)-1] = 0; /* TODO: check overflow */

		 printf("%s", buf);
		 //http_send((char *)req_info->query_string, strlen(req_info->query_string));
		
        f = fopen_recursive(buf, "wb");

        if (!f) {
            fail = 1;
        } else {
            while (nlen < tlen) {
                rlen = tlen - nlen;
                if (rlen > sizeof(buf)) {
                    rlen = sizeof(buf);
                }
                rlen = mg_read(conn, buf, (size_t)rlen);
                if (rlen <= 0) {
                    fail = 1;
                    break;
                }
                wlen = fwrite(buf, 1, (size_t)rlen, f);
                if (rlen != rlen) {
                    fail = 1;
                    break;
                }
                nlen += wlen;
            }
            fclose(f);
        }

        if (fail) {
            mg_printf(conn,
                "HTTP/1.1 409 Conflict\r\n"
                "Content-Type: text/plain\r\n"
                "Connection: close\r\n\r\n");
            //MessageBeep(MB_ICONERROR);
        } else {
            mg_printf(conn,
                "HTTP/1.1 201 Created\r\n"
                "Content-Type: text/plain\r\n"
                "Connection: close\r\n\r\n");
            //MessageBeep(MB_OK);
        }
#endif
        return true;
    }
};

class ExitHandler : public CivetHandler
{
  public:
	bool
	handlePost(CivetServer *server, struct mg_connection *conn)
	{
		mg_printf(conn,
		          "HTTP/1.1 200 OK\r\nContent-Type: "
		          "text/plain\r\nConnection: close\r\n\r\n");
		mg_printf(conn, "Bye!\n");
		g_httpdExitNow = true;
		return true;
	}
};