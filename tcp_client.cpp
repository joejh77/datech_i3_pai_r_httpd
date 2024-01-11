/* Copyright (c) 2013-2014 the Civetweb developers
 * Copyright (c) 2013 No Face Press, LLC
 * License http://opensource.org/licenses/mit-license.php MIT License
 */

// Simple example program on how to use Embedded C++ interface.

#include <fcntl.h>
#include "tcp_client.h"

#include "datools.h"
#include "SB_Network.h"


#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#define MD5_STATIC static
#include "md5.inl"

#define DEF_FILE_READ_BUFFER_SIZE	(128 * 1024)

#define TCPCLIENT_ERROR		1

#define ASSERT(condition)	\
if (!(condition)) {	\
	__tassert_tcp(#condition, LOG_TAG, __FUNCTION__, __FILE__,  __LINE__); \
}
	
void __tassert_tcp(const char * condition, const char * tag, const char * func, const char * file, int line)
{
	dprintf(1, "%s:%d:%s() assert %s\r\n", file, line, func, condition);
}


CTcpClient::CTcpClient(void)
{
	m_conn = NULL;
	memset((void *)last_error_buf, 0, sizeof(last_error_buf));
}

CTcpClient::~CTcpClient(void)
{
	if(m_conn){
		mg_close_connection(m_conn);
		m_conn = NULL;
	}
}
short CTcpClient::convert_short(short in)
{
  short out;
  char *p_in = (char *) &in;
  char *p_out = (char *) &out;
  p_out[0] = p_in[1];
  p_out[1] = p_in[0];  
  return out;
}

long CTcpClient::convert_long(long in)
{
  long out;
  char *p_in = (char *) &in;
  char *p_out = (char *) &out;
  p_out[0] = p_in[3];
  p_out[1] = p_in[2];
  p_out[2] = p_in[1];
  p_out[3] = p_in[0];  
  return out;
}

int CTcpClient::socket_open(const char *host, int port)
{
	if(m_conn == NULL) {
		memset((void *)last_error_buf, 0, sizeof(last_error_buf));
		m_conn = mg_connect_client(host, port, 0, last_error_buf, sizeof(last_error_buf));
		ASSERT(m_conn != NULL);

		if(!m_conn)
			printf("%s():%s\n",__func__, last_error_buf);
	}

	return (m_conn != NULL) ? 0 : -1;
}

int CTcpClient::socket_write(unsigned char * pData, int length)
{
	if(m_conn && pData && length)
		return mg_write(m_conn, pData, length);
	
	return 0;
}

int CTcpClient::socket_write(unsigned int u32data, bool bicEndian)
{
	if(m_conn)
		if(bicEndian)
			u32data = convert_long(u32data);
		
		return mg_write(m_conn, (unsigned char *)&u32data, sizeof(u32data));
	
	return 0;
}

int CTcpClient::socket_write(unsigned short u16data, bool bicEndian)
{
	if(m_conn)
		if(bicEndian)
			u16data = convert_short(u16data);
		
		return mg_write(m_conn, (unsigned char *)&u16data, sizeof(u16data));
	
	return 0;
}


int CTcpClient::socket_write(const char * file_path, char * ret_file_hash, int file_hash_size)
{
	md5_state_t ctx;
	md5_byte_t hash[16];
	char md5_hash[33] = {0,};
	char * buf;
	int file_size = 0;
	
	int fd;
	int ret;
	
		
	if(m_conn == NULL){
		dprintf(TCPCLIENT_ERROR, " %s() : [%s] Error! Socket dose not opend!\n", __func__, file_path);
		return -1;
	}

	buf = new char[ DEF_FILE_READ_BUFFER_SIZE ];
	if(buf == NULL)
	{
		dprintf(TCPCLIENT_ERROR, " %s() : buffer allocation error!!!\n", __func__);
		return 0;
	}
	
		
	fd = open(file_path, O_RDONLY);
	if (fd < 0) {
		dprintf(TCPCLIENT_ERROR, " %s() : [%s] file socket_open error!\n", __func__, file_path);
		return 0;
	}
	
	md5_init(&ctx);
	
	do{
		ret = read(fd, buf, DEF_FILE_READ_BUFFER_SIZE);
		
		if(ret) {
			mg_write(m_conn, buf, ret);
			
			md5_append(&ctx, (const md5_byte_t *)buf, ret);
			file_size += ret;
			usleep(1000);
		}
	}while(ret > 0);

	close(fd);

	delete[] buf;

	md5_finish(&ctx, hash);
	bin2str(md5_hash, hash, sizeof(hash));
	strncpy(ret_file_hash, md5_hash, file_hash_size);

	dprintf(TCPCLIENT_ERROR, "%s(size:%dKB, hash:%s)\r\n", file_path, file_size / 1024, md5_hash);
	
	return file_size;
}

int CTcpClient::socket_read(unsigned char * pData, int length, int timeout, CTcpClient::tcp_response_cb response_cb)
{
	int readlength = 0;
	
	if(m_conn) {
		int i;
		u32 start_tick = get_tick_count();

		memset((void *)last_error_buf, 0, sizeof(last_error_buf));
		
		do{
			i = mg_get_response(m_conn, last_error_buf, sizeof(last_error_buf), 1000, 1);
			
			if(get_tick_count() >= timeout + start_tick)
				break;

			msleep(100);
		}while(i < 0);
		
		const struct mg_request_info *ri;
		ri = mg_get_request_info(m_conn);
		
		if(ri->content_length > 0) {
			//i = mg_read(conn, ebuf, sizeof(ebuf));

			if(response_cb)
				response_cb((u8 *)last_error_buf, ri->content_length);

			if(pData && ri->content_length < length){
				readlength = ri->content_length;
				memcpy(pData, last_error_buf, readlength);
			}
				
		}
		else {
			printf("%s():%s\n",__func__, last_error_buf);
		}
	}
	return readlength;
}

int CTcpClient::socket_close(void)
{
	printf("%s\n\n", __func__);

	if(m_conn)
		mg_close_connection(m_conn);

	m_conn = NULL;

	return 0;
}

int CTcpClient::socket_send(const char *host, int port, unsigned char * pData, int length, int timeout, tcp_response_cb response_cb)
{	
	socket_open(host, port);
	
	if(m_conn && pData && length) {
		mg_write(m_conn, pData, length);
		socket_read(NULL, 0, timeout, response_cb);
	}
	
	socket_close();
	return 0;
}
