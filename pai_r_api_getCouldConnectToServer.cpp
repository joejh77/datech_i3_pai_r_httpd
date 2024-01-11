
//-------------------------------------------------------------------------------------------------
/*
4-02a. ���
�ɫ髤�֫쫳?��?�ϫ��׫��?�������׫ꪫ�����󪵪쪿��?���ê��Ī��ƫ�?��?����?�Ǫ������ɪ����������ު� ����̺극�ڴ��� �ۿ� ����, �����κ��� ������ ���� ������ �̿��� ������ ������ �� �־������� �����ݴϴ�.

4-02b. API�ի�����٣��
getCouldConnectToServer

4-02c. POST������
No	??٣	��?����	���Ϊ���	������	��
������				
4-02d. JSON������ JSON ��ȯ��
API���Ѫ������������ɫ髤�֫쫳?��?����?��?����?�Ǫ��ʪ����ꡢ��is_success === true���ǡ�IsConnect === false���˪ʪ�ު�
API �⵿�� ����������, ����̺극�ڴ��� ������ ������ �� ���� ���, 'is_success == true'�� 'Is Connect === false'�� �˴ϴ�.

No	??٣	��?����	������	��
1	is_success	boolean	����������true===����	true
2	error	֧����֪	����??�	
2a	error.code	string	����?��?	999
2b	error.message	string	����?��ë�?��	Syntaxerror
3	is_connect	boolean	�ɫ髤�֫쫳?��?����?��?����?�Ǫ������� ����̺극�ڴ��� ������ ������ �� �־����ϱ�?	true
{
    "is_success": true,
    "error": {
        "code": 0
    },
    "is_connect": true
}
*/

#define PAI_R_API_GET_COULD_CONNECT_TO_SERVER			"/getCouldConnectToServer"

class pai_r_api_getCouldConnectToServerHandler : public CivetHandler
{
	private:
	bool
	handleAll(const char *method, CivetServer *server, struct mg_connection *conn)
	{
		Json::Value root;
		Json::Value error;
		
		root["is_success"] = true;
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

