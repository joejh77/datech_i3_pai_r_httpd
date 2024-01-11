/* Copyright (c) 2013-2014 the Civetweb developers
 * Copyright (c) 2013 No Face Press, LLC
 * License http://opensource.org/licenses/mit-license.php MIT License
 */

// Simple example program on how to use Embedded C++ interface.

#include "http.h"

#include "datools.h"
#include "SB_Network.h"


#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

void __tassert(const char * condition, const char * tag, const char * func, const char * file, int line)
{
	dprintf(1, "%s:%d:%s() assert %s\r\n", file, line, func, condition);
}


// param condition : DLOG_WARN, DLOG_INFO, DLOG_ERROR ...
int
http_printf(unsigned int condition, struct mg_connection *conn)
{
	/* Handler may access the request info using mg_get_request_info */
	const struct mg_request_info *req_info = mg_get_request_info(conn);

	dprintf(condition, "%s%s %s HTTP/%s\n",
	          req_info->request_method,
	          req_info->uri,
	          req_info->query_string,
	          req_info->http_version);
	return true;
}

int http_mg_printf(struct mg_connection *conn, const char *msg)
{
	int len = 0;
	len = mg_printf(conn,
	          "HTTP/1.1 200 OK\r\nContent-Type: "
	          "text/html\r\nConnection: close\r\n\r\n");
	
	len += mg_printf(conn, "%s\r\n", msg);

	return len;
}

int http_send(const char *host, int port, int use_ssl, const char * method, const char * url, const char * query_string, const char * pMessage, int length, int timeout, http_response_cb response_cb)
{
	struct mg_request_info mg_req;
	memset((void *)&mg_req, 0, sizeof(mg_req));

	mg_req.uri = host;
	mg_req.remote_port = port;
	mg_req.is_ssl = use_ssl;
	mg_req.request_method = method;
	mg_req.request_uri = url;
	mg_req.query_string = query_string;
	
	http_send_request(&mg_req, pMessage, length, timeout, response_cb);
	
	return 0;
}

int http_send_request(const struct mg_request_info *req_info, const char * pMessage, int length, int timeout, http_response_cb response_cb)
{
	char ebuf[1024] = {0,}; 
	int i;
	u32 start_tick = 0;
	
	struct mg_connection *conn = mg_connect_client(req_info->uri, req_info->remote_port, req_info->is_ssl, ebuf, sizeof(ebuf));
	ASSERT(conn != NULL);
	ASSERT(ebuf[0] == 0);
	printf("%s():%s\n",__func__, ebuf);

	mg_printf(conn, "%s %s%s HTTP/1.1\r\n", req_info->request_method, req_info->request_uri, req_info->query_string);
	mg_printf(conn, "Host: %s:%d\r\n", req_info->uri, req_info->remote_port);
	mg_printf(conn, "User-Agent: PAI-R\r\n");
	mg_printf(conn, "Accept: */*");
	mg_printf(conn, "content-Length: %d", length);
	mg_printf(conn, "Connection: keep-alive\r\n\r\n");
	
	if(pMessage && length)
		mg_write(conn, pMessage, length);

	start_tick = get_tick_count();
	
	do{
		i = mg_get_response(conn, ebuf, sizeof(ebuf), 1000, 0);
		
		if(get_tick_count() >= timeout + start_tick)
			break;

		msleep(100);
	}while(i < 0);
	
	printf("%s():%s\n",__func__, ebuf);
	ASSERT(ebuf[0] != 0);

	const struct mg_request_info *ri;
	ri = mg_get_request_info(conn);
	ASSERT(ri->content_length == 0);
	
	mg_read(conn, ebuf, sizeof(ebuf));
	printf("%s():%s\n",__func__, ebuf);
	
	mg_close_connection(conn);

	return 0;
}
