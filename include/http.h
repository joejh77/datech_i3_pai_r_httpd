#ifndef http_H
#define http_H

#include <stdio.h>
#include <string.h>

#include "CivetServer.h"
#include "multipart_parser.h"
#include "json/json.h"

#include "datypes.h"

#define ASSERT(condition)	\
	if (!(condition)) {	\
		__tassert(#condition, LOG_TAG, __FUNCTION__, __FILE__,  __LINE__); \
	}

	
#ifdef __cplusplus
extern "C"
{
#endif


typedef int (*http_response_cb) (multipart_parser*, u8*data, int len);
	
// param condition : DLOG_WARN, DLOG_INFO, DLOG_ERROR ...
void __tassert(const char * condition, const char * tag, const char * func, const char * file, int line);
int http_printf(unsigned int condition, struct mg_connection *conn);

int http_mg_printf(struct mg_connection *conn, const char *msg);

int http_send(const char *host, int port, int use_ssl, const char * method, const char * url, const char * query_string, const char * pMessage, int length, int timeout, http_response_cb response_cb);
int http_send_request(const struct mg_request_info *req_info, const char * pMessage, int length, int timeout, http_response_cb response_cb);

////


#ifdef __cplusplus
}
#endif

#endif // http_H
