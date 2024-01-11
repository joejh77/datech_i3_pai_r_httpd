
#include "base64.h"

//-------------------------------------------------------------------------------------------------
/*
4-04b. API«Õ«¡«¤«ëÙ£äÐ
getMovieList

4-04c. POSTáêãáö·
widthªÈheightªÎò¦ïÒª¬ª¢ªëíÞùê¡¢?ßÀªÎ??ÝïªÏ«ª«ê«¸«Ê«ëªòë«ò¥ª·ª¿ªÞªÞª½ªÎÛô??ªË?ªÞªëªèª¦«µ«à«Í«¤«ëªòßæà÷ª·ªÞª¹
width¿Í heightÀÇ ÁöÁ¤ÀÌ ÀÖ´Â °æ¿ì, È­»óÀÇ Á¾È¾ºñ´Â ¿À¸®Áö³ÎÀ» À¯ÁöÇÑ Ã¤·Î ±× ¹üÀ§³»¿¡ µé¾î°¡µµ·Ï ¼¶³×ÀÏÀ» »ý¼ºÇÕ´Ï´Ù.
No	??Ù£	«Ç?«¿úþ	ù±âÎª«£¿	«³«á«ó«È	ÖÇ
1	is_event_only	boolean	Yes	G«»«ó«µ?Úã?ÔÑ?ªÎªßÚ÷ª¹ª«£¿true===G«»«ó«µ?Úã?ÔÑ?ªÎªßÚ÷ª¹G¼¾¼­ ¹ÝÀÀ µ¿¿µ»ó¸¸ µ¹·ÁÁÙ±î? true===G ¼¾¼­ ¹ÝÀÀ µ¿¿µ»ó¸¸ µ¹·ÁÁØ´Ù.	true
2	start_datetime	datetime	No	?ËìªËìéöÈª·ª¿ÔÑ?ªÎõÉç¯ËÒã·ìíãÁË½â÷ªÇª¤ªÄª«ªéÚ÷ª¹ª«£¿àýÕÔãÁªÏúÞî¤ÐññÞªËªÊªêªÞª¹ Á¶°Ç¿¡ ÀÏÄ¡ÇÑ µ¿¿µ»ó ÃÔ¿µ ½ÃÀÛ ÀÏ½Ã °­¼øÀ¸·Î ¾ðÁ¦ºÎÅÍ µ¹·ÁÁÙ±î? »ý·«½Ã´Â ÇöÀç ±âÁØÀÌ µË´Ï´Ù.	2020-02-20 11:11:11
3	camera_type	integer	No	«µ«à«Í«¤«ë?ßÀ:«««á«éÛã? àýÕÔãÁªÏ«á«¤«ó«««á«é£¨Ûã?===0£©½æ³×ÀÏ È­»ó:Ä«¸Þ¶ó ¹øÈ£ »ý·«½Ã´Â ¸ÞÀÎ Ä«¸Þ¶ó(¹øÈ£===0)	0
4	width	integer	No	«µ«à«Í«¤«ë?ßÀ:õÌÓÞ?øë àýÕÔãÁªÏ«ª«ê«¸«Ê«ëªÎªÞªÞ ½æ³×ÀÏ È­»ó : ÃÖ´ë °¡·ÎÆø »ý·«½Ã´Â ¿À¸®Áö³¯ ±×´ë·Î	240
5	height	integer	No	«µ«à«Í«¤«ë?ßÀ:õÌÓÞ?øë àýÕÔãÁªÏ«ª«ê«¸«Ê«ëªÎªÞªÞ ½æ³×ÀÏ È­»ó: ÃÖ´ë ¼¼·ÎÆø »ý·«½Ã´Â ¿À¸®Áö³¯ ±×´ë·Î	160 
6	count	integer	No	õÌÓÞªÇìé?ö¢ÔðªÇª­ªë? àýÕÔãÁªÏ20Ëì ÃÖ´ë·Î ÀÏ¶÷ ÃëµæÇÒ ¼ö ÀÖ´Â ¼ö »ý·«¼ºÀº 20°Ç
*/

#define PAI_R_API_GET_MOVIE_LIST		"/getMovieList"

u32 		m_last_autoid = 0;
u32 		m_last_read_item_no = 0;
time_t 	m_last_read_item_time = 0;


class pai_r_api_getMovieListHandler : public CivetHandler
{
  	private:
		
	bool
	handleAll(const char *method, CivetServer *server, struct mg_connection *conn)
	{
		const char * str_is_event_only = "is_event_only";
		const char * str_start_datetime = "start_datetime";
		const char * str_camera_type = "camera_type";
		const char * str_width = "width";
		const char * str_height = "height";
		const char * str_count = "count";
		Json::Value jsonMovieList;
		
		Json::Value error;
		
		bool IsSuccess = false;
		int	auto_id_error = 0;
		
		 const struct mg_request_info *ri = mg_get_request_info(conn);
	    //int query_len = 0;
		 //const char *query= ri->query_string;

		 mg_printf(conn,
	          "HTTP/1.1 200 OK\r\nContent-Type: "
	          "text/html\r\nConnection: close\r\n\r\n{"); //start http and json

		g_form_data.key_v.clear();
		
		if(strcmp("POST", ri->request_method) == 0){
			if(post_data_parser(conn)){
				if(strcmp("true", g_form_data.key_v[str_is_event_only].c_str()) == 0){
					IsSuccess = true;
					jsonMovieList[str_is_event_only] = true;
				}
				else if(strcmp("false", g_form_data.key_v[str_is_event_only].c_str()) == 0){
					IsSuccess = true;
					jsonMovieList[str_is_event_only] = false;
				}
				else {
					IsSuccess = false;
					error["code"] = ERROR_NO_NEED_ITEM;
					error["message"] = "no need item error.";
				}
			}
		}


		if(IsSuccess){
			int list_count = 0;
			time_t last_time = 0;
			
			 mg_printf(conn,"\"is_success\":true,\"error\":{\"code\":0},\"list\":[");
	          
			if(strlen(g_form_data.key_v[str_start_datetime].c_str()))
				jsonMovieList[str_start_datetime] = g_form_data.key_v[str_start_datetime].c_str();
			
			if(strlen(g_form_data.key_v[str_camera_type].c_str()))
				jsonMovieList[str_camera_type] = atoi(g_form_data.key_v[str_camera_type].c_str());

			if(strlen(g_form_data.key_v[str_width].c_str()))
				jsonMovieList[str_width] = atoi(g_form_data.key_v[str_width].c_str());

			if(strlen(g_form_data.key_v[str_height].c_str()))
				jsonMovieList[str_height] = atoi(g_form_data.key_v[str_height].c_str());

			if(strlen(g_form_data.key_v[str_count].c_str()))
				jsonMovieList[str_count] = atoi(g_form_data.key_v[str_count].c_str());

			if(jsonMovieList[str_count].asInt() == 0)
				jsonMovieList[str_count] = 20; // »ý·«¼ºÀº 20°Ç
			
			dprintf(HTTPD_INFO, "%s : %d\n", str_is_event_only, (int)jsonMovieList[str_is_event_only].asBool());
			dprintf(HTTPD_INFO, "%s : %s\n", str_start_datetime, (int)jsonMovieList[str_start_datetime].asString().c_str());
			dprintf(HTTPD_INFO, "%s : %d\n", str_camera_type, (int)jsonMovieList[str_camera_type].asInt());
			//dprintf(HTTPD_INFO, "%s : %d\n", str_width, (int)jsonMovieList[str_width].asInt());
			//dprintf(HTTPD_INFO, "%s : %d\n", str_height, (int)jsonMovieList[str_height].asInt());
			dprintf(HTTPD_INFO, "%s : %d\n", str_count, (int)jsonMovieList[str_count].asInt());

			if(pai_r_data.m_queue_index_init){
				u32 queue_max_count = 0;
				u32 read_queue_no = 0;
				bool is_event_only = false;
				
				//time_t current_time;
				time_t start_time = time(0);
				Json::Value list;

				int camera_id = jsonMovieList[str_camera_type].asInt();
				std::string str_list;

				if(jsonMovieList[str_is_event_only].asBool() == true){
					is_event_only = true;
				}

				read_queue_no = pai_r_data.Location_queue_auto_id_in_get((eUserDataType)is_event_only) - 1;
				queue_max_count = pai_r_data.Location_queue_max_count_get((eUserDataType)is_event_only);
				
				if(read_queue_no == 0){
					queue_max_count = 0;
				}
#if DEF_MOVIELIST_USE_HASH_STRING				
				if(read_queue_no > 1 && pai_r.file_hash_update_list.size())
					read_queue_no--;
#endif					
				if(jsonMovieList[str_start_datetime].isString())
					start_time = pai_r_time_string_to_time(jsonMovieList[str_start_datetime].asString());
				
				if(camera_id >= DEF_MAX_CAMERA_COUNT)
					camera_id = 0;

				if(m_last_read_item_time && m_last_read_item_time >= start_time){
					if(m_last_read_item_no < read_queue_no)
						read_queue_no = m_last_read_item_no;
				}
				else
					m_last_autoid = 0;
				
				m_last_read_item_no = 0;
				m_last_read_item_time = 0;

				PAI_R_BACKUP_DATA *pLocData = (PAI_R_BACKUP_DATA *)new char[ sizeof(PAI_R_BACKUP_DATA) ];
				if(pLocData == NULL) {
					dprintf(HTTPD_INFO, "%s() : buffer allocation error!! \r\n", __func__);
					queue_max_count = 0;
				}
				
				dprintf(HTTPD_INFO, "start auto id : %d \n", read_queue_no);
				
				
				for(int i = 0; i < queue_max_count && read_queue_no && list_count < (int)jsonMovieList[str_count].asInt(); i++) {
					
					u32 queue_no = read_queue_no;

					if(!pai_r_data.Location_pop(pLocData, queue_no, (eUserDataType)is_event_only, true))
						break;

					if(pLocData->location.autoid == queue_no || (is_event_only && pLocData->event_autoid == queue_no)){
						bool is_event = false;
						const char * file_name = strrchr(pLocData->location.file_path, '/');

						if(file_name != NULL){
					  		file_name++;
								
							if(strchr(file_name, REC_FILE_TYPE_EVENT))
								is_event = true;	

							last_time = recording_time_string_to_time(file_name);
						}
						
						//if(i == 0)
						//	current_time = last_time;
						
						read_queue_no--;

						if(last_time >= start_time || file_name == NULL)
							continue;
						
						if(jsonMovieList[str_is_event_only].asBool() == true && !is_event)
							continue;

						m_last_read_item_no = read_queue_no;
						m_last_read_item_time = last_time;
						
							
						if(access(pLocData->location.file_path, R_OK ) == 0){
							//std::string str_list;
			
							u32 thumbnail_size = pLocData->location.thumbnail.size[camera_id];
							
							u32 base64_buf_size = b64e_size(thumbnail_size) + 1 + 32; // 32 for add string "image/jpeg:base64,"
							char* base64_buf = new char[ base64_buf_size ];
							
							
							if(base64_buf){
								int str_length = 0;
								memset((void *)base64_buf, 0, base64_buf_size);
								strcpy((char *)base64_buf, "image/jpeg:base64,");
								str_length = strlen((const char *)base64_buf);
								str_length += b64_encode((const unsigned char *)pLocData->location.thumbnail.data[camera_id], thumbnail_size, (unsigned char*)&base64_buf[str_length]);
							}
#if 0
							if(list_count == 0)
								str_list = "{";
							else
								str_list= ",{";
							
							list_count++;
							m_last_autoid++;

							get_md5_file_hash(pLocData->file_hash, pLocData->location.file_path);

							//str_list.append(format_string("\"autoid\":%u", m_last_autoid));
							str_list.append(format_string("\"autoid\":%u", pLocData->location.autoid));
							str_list.append(format_string(",\"is_event\":%s", is_event ? "true" : "false"));
							str_list.append(format_string(",\"create_datetime\":\"%s\"", make_time_string(last_time).c_str()));
							
							if(base64_buf)
								str_list.append(format_string(",\"image\":\"%s\"", base64_buf));
							
							str_list.append(format_string(",\"latitude\":%f", pLocData->location.latitude));
							str_list.append(format_string(",\"longitude\":%f", pLocData->location.longitude));
							str_list.append(format_string(",\"accurate\":%u", (u32)pLocData->location.accurate));
							str_list.append(format_string(",\"direction\":%u", (u32)pLocData->location.direction));
							str_list.append(format_string(",\"altitude\":%u", (u32)pLocData->location.altitude));
							str_list.append(format_string(",\"path\":\"%s\"", pLocData->location.file_path));
							str_list.append(format_string(",\"hash\":\"%s\"", pLocData->file_hash));
							str_list.append(format_string(",\"upload_datetime\":\"%s\"", ""));

							str_list.append("}");

							printf("\r\n%u:%03u\r\n", get_tick_count()/1000, get_tick_count()%1000);
							mg_printf(conn,str_list.c_str());
							printf("%u:%03u\r\n", get_tick_count()/1000, get_tick_count()%1000);
#else
							if(list_count == 0)
								mg_printf(conn,"{");
							else
								mg_printf(conn,",{");
							
							list_count++;
							m_last_autoid++;
#if DEF_MOVIELIST_USE_HASH_STRING		
							if(strlen(pLocData->file_hash) == 0){
								get_md5_file_hash(pLocData->file_hash, pLocData->location.file_path);
								//pai_r_data.update_file_hash(queue_no, (const char *)pLocData->file_hash);
#ifdef MSGQ_PAIR_LOC_Q_OUT_IDKEY		
								ST_QMSG msg;
								msg.type = QMSG_UP_FILE_HASH;
								msg.data = eUserDataType_Normal;
								msg.data2 = queue_no;
								msg.time = (u32)time(0);
								strcpy(msg.string, (const char *)pLocData->file_hash);
								datool_ipc_msgsnd(m_http_msg_q_out_id, (void *)&msg, sizeof(msg));
#endif								
							}
#endif
							//mg_printf(conn,"\"autoid\":%u", m_last_autoid);
							//mg_printf(conn,"\"autoid\":%u", pLocData->location.autoid);
							mg_printf(conn,"\"autoid\":%u", pLocData->location.unique_local_autoid);
							mg_printf(conn,",\"is_event\":%s", is_event ? "true" : "false");
							mg_printf(conn,",\"create_datetime\":\"%s\"", make_time_string(last_time).c_str());
							
							if(base64_buf && thumbnail_size)
								mg_printf(conn,",\"image\":\"%s\"", base64_buf);
							
							mg_printf(conn,",\"latitude\":%f", pLocData->location.latitude);
							mg_printf(conn,",\"longitude\":%f", pLocData->location.longitude);
#ifdef DEF_MMB_SERVER
							mg_printf(conn,",\"gnssStatus\":\"%c\"", (char)pLocData->location.gnssStatus);
							mg_printf(conn,",\"eventType\":%u", (u32)pLocData->location.eventType);
#else
							mg_printf(conn,",\"accurate\":%u", (u32)pLocData->location.accurate);
#endif
							mg_printf(conn,",\"direction\":%u", (u32)pLocData->location.direction);
							mg_printf(conn,",\"altitude\":%u", (u32)pLocData->location.altitude);
							mg_printf(conn,",\"path\":\"%s\"", pLocData->location.file_path);
#if DEF_MOVIELIST_USE_HASH_STRING								
							mg_printf(conn,",\"hash\":\"%s\"", pLocData->file_hash);
#else
								if(access(pLocData->location.file_path, F_OK) == 0){
								if(strlen(pLocData->file_hash)){
									mg_printf(conn,",\"hash\":\"%s\"", pLocData->file_hash);
								}
								else {										
									mg_printf(conn,",\"hash\":\"%s.hash\"", pLocData->location.file_path);
								}
								}
#endif
							if(pLocData->upload_time)
								mg_printf(conn,",\"upload_datetime\":\"%s\"", make_time_string(pLocData->upload_time).c_str());
							else
								mg_printf(conn,",\"upload_datetime\":\"%s\"", "");

							mg_printf(conn,"}");
#endif
							if(base64_buf)
								delete [] base64_buf;
						}
						else{ // no file exist
							//break;
						}
					}else{
						dprintf(HTTPD_INFO, "%s() : error!! movie autoid is different.(%d:%d)\r\n", __func__,  is_event_only ? pLocData->event_autoid : pLocData->location.autoid, read_queue_no);
						//break;
						read_queue_no--;		
						if(auto_id_error++ > 5)
							break;
					}
				}		

				delete [] pLocData;
				pai_r_data.Location_pop_continue_read_close();
			}

			if(last_time)
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

