/*
-10. «É«é«¤«Ö«ì«³?«À?àâïÒÌÚãæ µå¶óÀÌºê·¹ÄÚ´õ ¼³Á¤ °»½Å.
4-10a. ßÙá¬
«É«é«¤«Ö«ì«³?«À?ªÎàâïÒªòÌÚãæª¹ªë µå¶óÀÌ¹öÀÇ ¼³Á¤À» °»½ÅÇÑ´Ù

«á«ó«Æ«Ê«ó«¹éÄ À¯Áö °ü¸®¿ë

4-10b. API«Õ«¡«¤«ëÙ£äÐ
updateConfig
*/
#define DBG_UC_INFO		1

#define PAI_R_API_UPDATE_CONFIG														"/updateConfig"



class pai_r_api_updateConfigHandler : public CivetHandler
{
	private:

	bool update_value(const char * key, int *ret_value)
	{
		const char * value = g_form_data.key_v[key].c_str();
		
		if(value && strlen(value)){
			if(strcmp(value, "true") == 0)
				*ret_value = 1;
			else if(strcmp(value, "false") == 0)
				*ret_value = 0;
			else
				*ret_value = atoi(value);
			dprintf(DBG_UC_INFO, " %s : %d\n", key, *ret_value);
			return true;
		}

		return false;
	}

	bool update_value(const char *key, bool *ret_value)
	{
		int value = 0;
		if(update_value(key, &value)){
			*ret_value = value ? true : false;
			return true;
		}

		return false;
	}

	bool update_value(const char *key, char *ret_value)
	{
		const char * value = g_form_data.key_v[key].c_str();
		
		if(value && strlen(value)){
			strcpy(ret_value, value);
			dprintf(DBG_UC_INFO, " %s : %s\n", key, ret_value);
			return true;
		}

		return false;
	}

	bool update_time(const char * key)
	{
		const char * value = g_form_data.key_v[key].c_str();
		if(value && strlen(value) && pai_r_time_string_to_time(value) > pai_r_time_string_to_time("2020-01-01 09:00:00"))
		{
			char szCmd[64];
			SYSTEMLOG(LOG_TYPE_SYSTEM, LOG_EVENT_DATETIMECHANGE , RTC_SRC_SETUP_APP, "Before");
			sprintf(szCmd, "date -s \"%s\"", value);
			system(szCmd);

			//cfg_time.nTimeSet = 0;
			//CConfigText::Save(&cfg_time);

			system("hwclock -w");
			SYSTEMLOG(LOG_TYPE_SYSTEM, LOG_EVENT_DATETIMECHANGE , RTC_SRC_SETUP_APP, "After");
			return true;
		}
		return false;
	}
		
	bool
	handleAll(const char *method, CivetServer *server, struct mg_connection *conn)
	{
		int i;
		Json::Value root;
		Json::Value error;
		bool IsSuccess = false;
		
		const struct mg_request_info *ri = mg_get_request_info(conn);
		if(strcmp("POST", ri->request_method) == 0){		
			g_form_data.key_v.clear();
			if(post_data_parser(conn)){
				const char * strCmdFormat = "echo \"\" > /mnt/extsd/format.txt";
				Json::Value sensor;
				Json::Value speed_pulse;
				Json::Value wifi;

				int iValue;
				//bool bValue;
				
				ST_CFG_DAVIEW cfg;
				
		
				httpd_cfg_get(&cfg);

				update_time("datetime");

				update_value("timezone", &cfg.iGmt);
				update_value("audio_rec", &cfg.iAudioRecEnable);
				if(update_value("speaker_vol", &cfg.iSpeakerVol )) {
#ifdef MSGQ_PAIR_LOC_Q_IN_IDKEY
					datool_ipc_msgsnd2(m_http_msg_q_out_id, (long)QMSG_SPEAKER, (u32)cfg.iSpeakerVol);
#endif									
				}

#if !DEF_VIDEO_QUALITY_ONLY			
				if(update_value("resolution_cam0", &iValue)) { // 0 : 720p, 1 1080p
					if(cfg.iFrontResolution != iValue){
						cfg.iFrontResolution = iValue;
						system(strCmdFormat);
					}
				}

				if(update_value("resolution_cam1", &iValue)) { // 0 : 720p, 1 1080p
					if(cfg.iRearResolution != iValue){
						cfg.iRearResolution = iValue;
						system(strCmdFormat);
					}
				}
#endif				
				if(update_value("video_quality", &iValue)){
					if(cfg.iVideoQuality != iValue){
						cfg.iVideoQuality = iValue;
						system(strCmdFormat);
					}
				}
					
				if(update_value("event_mode", &iValue)){
					if(cfg.iEventMode != iValue){
						cfg.iEventMode = iValue;
						system(strCmdFormat);
					}
				}
				
				{
					update_value("pulse_reset", &cfg.iPulseReset);
					update_value("pulse_brake", &cfg.bBrake);
					update_value("pulse_input1", &cfg.bInput1);
					update_value("pulse_input2", &cfg.bInput2);
				}
				
				{	
					update_value("sensor_level", &cfg.iGsensorSensi);
					update_value("sensor_level_a", &cfg.iGsensorSensi);
					update_value("sensor_level_b", &cfg.G_Sensi_B);
					update_value("sensor_level_c", &cfg.G_Sensi_C);
					update_value("sensor_update_datetime", cfg.strLastSetupTime);
				}	

//++{VMD
				update_value("event_acceleration_level" , &cfg.iSuddenAccelerationSensi);
				update_value("event_deacceleration_level", &cfg.iSuddenDeaccelerationSensi);
				update_value("event_rapid_rotation_level", &cfg.iRapidRotationSensi);
//++}

				{
					update_value("net_seriar_no", cfg.strSeriarNo);
					update_value("net_ap_ssid", cfg.strApSsid);
					for(i=0; i < DEF_MAX_TETHERING_INFO; i++){
						char id_key[32];
						char pw_key[32];
						char no_key[32];
						sprintf(id_key, "net_wifi_ssid%d", i+1);
						sprintf(pw_key, "net_wifi_password%d", i+1);
						sprintf(no_key, "net_tel_number%d", i+1);
						
						update_value(id_key, cfg.strWiFiSsid[i]);
						update_value(pw_key, cfg.strWiFiPassword[i]);
						update_value(no_key, cfg.strTelNumber[i]);
					}
					update_value("net_application_ip", cfg.strApplication_IP);
					update_value("net_application_port", &cfg.iApplication_PORT);
				}

				update_value("engine_cylinders", &cfg.iEngine_cylinders);
				update_value("speed_limit", &cfg.iSpeed_limit_kmh);
				update_value("osd_speed", &cfg.bOsdSpeed);
				update_value("osd_rpm", &cfg.bOsdRpm);
		
				if(update_value("factory_reset", &cfg.bFactoryReset)){
					if(cfg.bFactoryReset){
						//Add factory reset function
						CConfigText::CfgDefaultSet(&cfg);

						cfg.bFactoryReset = false;
					}
				}
				
				httpd_cfg_set(&cfg);
				IsSuccess = true;
				error["code"] = 0;
			}
		}

		
		if(!IsSuccess){
			error["message"] = "Syntax error";
			error["code"] = 999;
		}
		
		root["is_success"] = IsSuccess;
		root["error"] = error;
		
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

