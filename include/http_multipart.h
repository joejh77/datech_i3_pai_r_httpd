#ifndef http_http_multipart_H
#define http_http_multipart_H

#include <stdio.h>
#include <string.h>

#include "daappconfigs.h"
#include "CivetServer.h"
#include "multipart_parser.h"
#include "pai_r_data.h"

#include "json/json.h"
#include "datypes.h"
#include "datools.h"

#define DEF_FILE_READ_BUFFER_SIZE	(128 * 1024)

class CHttp_multipart
{
public:
	enum {
		HTTP_MP_BUFFER = 0,
		HTTP_MP_SEND
	};
	
public:
	typedef int (*http_multipart_data_cb) (CHttp_multipart *p, const char *data, int length);
	//typedef int (*http_multipart_data_cb) (struct mg_connection *p, const char *data, int length);
	
	struct http_multipart_head {
		const char *host;            /* Client's address */
		int port;          /* Client's port */
		int is_ssl;               /* 1 if SSL-ed, 0 if not */
		
		const char *request_method; /* "GET", "POST", etc */
		const char *request_uri;    /* URL-decoded URI (absolute or relative,
	                             * as in the request) */
		
		const char *http_version;   /* E.g. "1.0", "1.1" */
		const char *query_string;   /* URL part after '?', not including '?', or NULL */
		const char *user_agent;
		const char *boundary;

		int content_length;
		
		const char *user_name;
		const void *user_data;
		size_t user_data_size;

		char user_buffer[HTTP_MULTIPART_BUFFER_SIZE];
		http_multipart_data_cb data_cb;
	};
	
	struct http_multipart_auto {
	  const char *key;
	  const char *type;
	  const char *filename;
	  const char *data_string;
	  bool		data_bool;
	  int			data_int;
	  time_t		data_time_t;
	  http_multipart_data_cb data_cb;
	};

	//http_multipart_auto auto;
	struct mg_connection *m_conn;
	int m_contents_length;
	
	const char * m_boundary;
	http_multipart_head m_hd;
	char last_error_buf[1024];
	
  	CHttp_multipart(void);
	CHttp_multipart(const char * url, int port, int is_ssl, const char *request_method, const char *request_uri, const char *boundary);
	virtual ~CHttp_multipart(void);

	void contents_header_init(const char * url, int port, int is_ssl, const char *request_method, const char *request_uri, const char *boundary);
	
public:
	static const char * get_content_type(const char * file_ext);
	int update_contents_length(int item_count, int total_data_size);
	int update_contents_length(const char * key, const char * value);
	int update_contents_length(THUMBNAIL * th);
	int update_contents_length(const char * file_path);

	//without buffer, update_contents_length use
	//m_hd.content_length = update_contents_length( ...
	int request_start(const char *query_string = NULL);
	int request(const char * key, const char * value);
	int request(THUMBNAIL * th);
	int request(const char * file_path, char * ret_file_hash, int file_hash_size);
	int request_end(http_multipart_data_cb data_cb, int timeout_sec);

	//buffer use
	int response(http_multipart_head *hd, http_multipart_data_cb data_cb, int timeout_sec);
	int make_contents(http_multipart_head *hd, bool end_flag);
	int make_contents(const char * name, const void * data, size_t size, http_multipart_data_cb data_cb, bool end_flag);
	int make_contents(const char *k, const char *p, http_multipart_data_cb data_cb, bool end_flag);

	/*
	file make_contents or width zip make_contents
	name		// file
	path // /mnt/extsd/speed.scv 
	type //zip ...
	*/
	int make_contents(Json::Value &form_file, http_multipart_data_cb data_cb, bool end_flag);
	// zip data make_contents
	/*
	name		// file
	file_name // speed.zip 
	type //zip ...
	*/
	int make_contents(Json::Value &form_file, const void * data, size_t size, http_multipart_data_cb data_cb, bool end_flag);

};

#endif // http_http_multipart_H
