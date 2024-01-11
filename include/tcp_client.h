#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include <stdio.h>
#include <string.h>

#include "CivetServer.h"
#include "json/json.h"

#include "datypes.h"

	
class CTcpClient
{
public:
	typedef int (*tcp_response_cb) (u8*data, int len);

	CTcpClient(void);
	virtual ~CTcpClient(void);

	short convert_short(short in);
	long convert_long(long in);
	int socket_open(const char *host, int port);
	int socket_write(unsigned char * pData, int length);
	int socket_write(unsigned int u32data, bool bicEndian = false);
	int socket_write(unsigned short u16data, bool bicEndian = false);
	int socket_write(const char * file_path, char * ret_file_hash, int file_hash_size);
	int socket_read(unsigned char * pData, int length, int timeout, tcp_response_cb response_cb);
	int socket_close(void);
	int socket_send(const char *host, int port, unsigned char * pData, int length, int timeout, tcp_response_cb response_cb);
	////

	struct mg_connection *m_conn;
	char last_error_buf[1024];
};

#endif // TCP_CLIENT_H