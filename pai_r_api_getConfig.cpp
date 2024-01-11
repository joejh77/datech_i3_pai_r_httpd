/*
4-09. �ɫ髤�֫쫳?��?�������� ����̺극�ڴ� ���� ���.
4-09a. ���
�ɫ髤�֫쫳?��?�����Ҫ����𪹪�
���ƫʫ���
����̺극�ڴ��� ������ ����Ѵ�.
���� ������

4-09b. API�ի�����٣��
getConfig

4-09c. POST������
No	??٣	��?����	���Ϊ���	������	��
������				
4-09d. JSON������
��������������������˽���Ȫ��ު�
����������ʦ����?黪���請����ʥ?��𶪷�ƪ�������
�α� ��ȯ���� �ۼ� �Ͻ� �������� �մϴ�.
���� ��� ������ ���뿡 ���߾� �߰��������� �ּ���.
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

