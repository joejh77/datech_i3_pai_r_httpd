/*
4-09. «É«é«¤«Ö«ì«³?«À?àâïÒö¢Ôğ µå¶óÀÌºê·¹ÄÚ´õ ¼³Á¤ Ãëµæ.
4-09a. ßÙá¬
«É«é«¤«Ö«ì«³?«À?ªÎàâïÒªòö¢Ôğª¹ªë
«á«ó«Æ«Ê«ó«¹éÄ
µå¶óÀÌºê·¹ÄÚ´õÀÇ ¼³Á¤À» ÃëµæÇÑ´Ù.
À¯Áö °ü¸®¿ë

4-09b. API«Õ«¡«¤«ëÙ£äĞ
getConfig

4-09c. POSTáêãáö·
No	??Ù£	«Ç?«¿úş	ù±âÎª«£¿	«³«á«ó«È	ÖÇ
£¨Ùíª·£©				
4-09d. JSONÚ÷ªêö·
«í«°ªÎÚ÷ªêö·ªÏíÂà÷ìíãÁË½â÷ªÈª·ªŞª¹
¡ØàâïÒõóÕôÊ¦ÒöªÊ?é»ªËùêªïª»ªÆõÚÊ¥?Şûğ¶ª·ªÆª¯ªÀªµª¤
·Î±× ¹İÈ¯°ªÀº ÀÛ¼º ÀÏ½Ã °­¼øÀ¸·Î ÇÕ´Ï´Ù.
¼³Á¤ Ãâ·Â °¡´ÉÇÑ ³»¿ë¿¡ ¸ÂÃß¾î Ãß°¡¡¤»èÁ¦ÇØ ÁÖ¼¼¿ä.
*/

#define PAI_R_API_GET_CONFIG														"/getConfig"

class pai_r_api_getConfigHandler : public CivetHandler
{
	private:
	bool
	handleAll(const char *method, CivetServer *server, struct mg_connection *conn)
	{
		//const char * strVer = DA_FIRMWARE_VERSION;
		int i;
		Json::Value root;
		Json::Value error;
		Json::Value sensor;
		Json::Value pulse;
		Json::Value resolution;
		Json::Value net;

		ST_CFG_DAVIEW cfg;
		
		httpd_cfg_get(&cfg);

		root["is_success"] = true;
		{
			error["code"] = 0;
		}
		root["error"] = error;
		root["datetime"] = make_time_string(time(0)).c_str();
		

		root["timezone"] = cfg.iGmt;
		root["audio_rec"] = (bool)cfg.iAudioRecEnable;
		root["speaker_vol"] = cfg.iSpeakerVol;
#if !DEF_VIDEO_QUALITY_ONLY
		{
				resolution["cam0"] = cfg.iFrontResolution;
				resolution["cam1"] = cfg.iRearResolution;
		}
		root["resolution"] = resolution;
#endif		
		root["video_quality"] = cfg.iVideoQuality;
		root["event_mode"] = cfg.iEventMode;

		{
			pulse["reset"] = (bool)cfg.iPulseReset;
			pulse["brake"] = (bool)cfg.bBrake;
			pulse["input1"] = (bool)cfg.bInput1;
			pulse["input2"] = (bool)cfg.bInput2;
		}
		root["pulse"] = pulse;
		
		{
			sensor["level_a"] = cfg.iGsensorSensi;
			sensor["level_b"] = cfg.G_Sensi_B;
			sensor["level_c"] = cfg.G_Sensi_C;
			sensor["update_datetime"] =  cfg.strLastSetupTime;
		}	
		root["sensor"] = sensor;

//++{VMD
		root["event_acceleration_level"] = cfg.iSuddenAccelerationSensi;
		root["event_deacceleration_level"] = cfg.iSuddenDeaccelerationSensi;
		root["event_rapid_rotation_level"] = cfg.iRapidRotationSensi;
//++}

		{
			net["seriar_no"] = atoi(cfg.strSeriarNo);
			net["ap_ssid"] = cfg.strApSsid;

			for(i=0; i < DEF_MAX_TETHERING_INFO; i++){
				char id_key[32];
				char pw_key[32];
				char no_key[32];
				sprintf(id_key, "wifi_ssid%d", i+1);
				sprintf(pw_key, "wifi_password%d", i+1);
				sprintf(no_key, "tel_number%d", i+1);
				
				net[id_key] = cfg.strWiFiSsid[i];
				net[pw_key] = cfg.strWiFiPassword[i];
				net[no_key] = cfg.strTelNumber[i];
			}
			net["application_ip"] = cfg.strApplication_IP;
			net["application_port"] = cfg.iApplication_PORT;
		}
		root["net"] = net;

		root["engine_cylinders"] = cfg.iEngine_cylinders;
		root["speed_limit"] = cfg.iSpeed_limit_kmh;
		root["osd_speed"] = cfg.bOsdSpeed;
		root["osd_rpm"] = cfg.bOsdRpm;
		
		root["factory_reset"] = cfg.bFactoryReset;
		
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

