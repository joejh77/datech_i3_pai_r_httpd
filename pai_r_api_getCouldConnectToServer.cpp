
//-------------------------------------------------------------------------------------------------
/*
4-02a. ßÙá¬
«É«é«¤«Ö«ì«³?«À?ªÏ«¢«×«êªË?ª·¡¢«¢«×«êª«ªéÍìêóªµªìª¿ïÈ?ï×ÜÃªòéÄª¤ªÆ«µ?«Ğ?ªËïÈ?ªÇª­ª¿ª«ªÉª¦ª«ªòÚ÷ª·ªŞª¹ µå¶óÀÌºê·¹ÄÚ´õ´Â ¾Û¿¡ ´ëÇØ, ¾ÛÀ¸·ÎºÎÅÍ °øÀ¯µÈ Á¢¼Ó Á¤º¸¸¦ ÀÌ¿ëÇØ ¼­¹ö¿¡ Á¢¼ÓÇÒ ¼ö ÀÖ¾ú´ÂÁö¸¦ µ¹·ÁÁİ´Ï´Ù.

4-02b. API«Õ«¡«¤«ëÙ£äĞ
getCouldConnectToServer

4-02c. POSTáêãáö·
No	??Ù£	«Ç?«¿úş	ù±âÎª«£¿	«³«á«ó«È	ÖÇ
£¨Ùíª·£©				
4-02d. JSONÚ÷ªêö· JSON ¹İÈ¯°ª
APIÑÃÔÑªËà÷Ííª·ª¿ª¬¡¢«É«é«¤«Ö«ì«³?«À?ª¬«µ?«Ğ?ªËïÈ?ªÇª­ªÊª¤íŞùê¡¢¡ºis_success === true¡»ªÇ¡ºIsConnect === false¡»ªËªÊªêªŞª¹
API ±âµ¿¿¡ ¼º°øÇßÁö¸¸, µå¶óÀÌºê·¹ÄÚ´õ°¡ ¼­¹ö¿¡ Á¢¼ÓÇÒ ¼ö ¾ø´Â °æ¿ì, 'is_success == true'·Î 'Is Connect === false'°¡ µË´Ï´Ù.

No	??Ù£	«Ç?«¿úş	«³«á«ó«È	ÖÇ
1	is_success	boolean	à÷Ííª·ª¿ª«£¿true===à÷Íí	true
2	error	Ö§ßÌÛÕÖª	«¨«é??é»	
2a	error.code	string	«¨«é?Ûã?	999
2b	error.message	string	«¨«é?«á«Ã«»?«¸	Syntaxerror
3	is_connect	boolean	«É«é«¤«Ö«ì«³?«À?ª¬«µ?«Ğ?ªËïÈ?ªÇª­ª¿ª«£¿ µå¶óÀÌºê·¹ÄÚ´õ°¡ ¼­¹ö¿¡ Á¢¼ÓÇÒ ¼ö ÀÖ¾ú½À´Ï±î?	true
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

