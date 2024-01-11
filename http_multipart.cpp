/* Copyright (c) 2013-2014 the Civetweb developers
 * Copyright (c) 2013 No Face Press, LLC
 * License http://opensource.org/licenses/mit-license.php MIT License
 */

// Simple example program on how to use Embedded C++ interface.

 #include <fcntl.h>
 

#include "http.h"
#include "zip.h"
//#include "unzip.h"
#include "sysfs.h"
#include "base64.h"

#include "datools.h"
#include "SB_Network.h"
#include "CivetServer.h"
#include "multipart_parser.h"
#include "http_multipart.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif
#define MD5_STATIC static
#include "md5.inl"

#define MULTIPART_INFO		0
#define MULTIPART_ERROR	1


#define HTTP_RESPONSE_WAIT_TIME		40 //sec
const char * multipart_form_data_fmt_start = "\r\nContent-Disposition: form-data; name=\"%s\"\r\n\r\n";
const char * multipart_form_data_fmt_end = "\r\n--%s";

const char * multipart_form_data_file_fmt = "\r\nContent-Disposition: form-data; name=\"%s\"; filename=\"%s\"\r\n";
const char * multipart_form_data_content_type_fmt = "Content-Type: %s\r\n\r\n";

const char * content_type_zip = "application/zip";
const char * content_type_avi = "video/x-msvideo";
const char * content_type_mp4 = "video/mp4";
const char * content_type_csv = "text/csv";
const char * content_type_text = "text/plain";  // log, txt
const char * content_type_bin = "application/octet-stream";
const char * content_type_png = "image/png";
const char * content_type_jpg = "image/jpeg";	//jpg, jpeg
const char * content_type_none = "false";


CHttp_multipart::CHttp_multipart(void)
{
	m_conn = NULL;
	m_contents_length = 0;
	memset((void *)&m_hd, 0, sizeof(m_hd));
}

CHttp_multipart::CHttp_multipart(const char * url, int port, int is_ssl, const char *request_method, const char *request_uri, const char *boundary)
{
	m_conn = NULL;
	contents_header_init(url, port, is_ssl, request_method, request_uri, boundary);
}

CHttp_multipart::~CHttp_multipart(void)
{
	if(m_conn){
		mg_close_connection(m_conn);
		m_conn = NULL;
	}
}

void CHttp_multipart::contents_header_init(const char * url, int port, int is_ssl, const char *request_method, const char *request_uri, const char *boundary)
{
	if(m_conn){
		mg_close_connection(m_conn);
		m_conn = NULL;
	}
	m_contents_length= 0;
	memset((void *)&m_hd, 0, sizeof(m_hd));
	memset((void *)&last_error_buf, 0, sizeof(last_error_buf));
	
	m_hd.host = url;
	m_hd.port = port;
	m_hd.is_ssl = is_ssl;
	m_hd.request_method = request_method;
	m_hd.request_uri = request_uri;
	m_hd.boundary = boundary;
	m_hd.content_length = 0;
}

const char * CHttp_multipart::get_content_type(const char *file_ext)
{
	const char * ext = file_ext;
	const char *lastDot = strrchr(file_ext, '.');

	if(lastDot != NULL)
		ext =  lastDot + 1;
	
	if(strcmp("zip", ext) == 0)
		return content_type_zip;
	else if(strcmp("avi", ext) == 0)
		return content_type_avi;
	else if(strcmp("mp4", ext) == 0)
		return content_type_mp4;
	else if(strcmp("csv", ext) == 0)
		return content_type_csv;
	else if(strcmp("log", ext) == 0 || strcmp("hash", ext) == 0)
		return content_type_text;
	else if(strcmp("bin", ext) == 0)
		return content_type_bin;
	else if(strcmp("png", ext) == 0)
		return content_type_png;
	else if(strcmp("jpg", ext) == 0)
		return content_type_jpg;
	else
		return content_type_none;
}
	
int CHttp_multipart::update_contents_length(int item_count, int total_data_size)
{
	if(m_hd.content_length == 0)
		m_hd.content_length = 2 + strlen(m_hd.boundary); // start "--[boundary]"
		
	m_hd.content_length += (item_count * ((strlen(multipart_form_data_fmt_start)-2) + (strlen(multipart_form_data_fmt_end) - 2) + strlen(m_hd.boundary))) + total_data_size;
	return m_hd.content_length;
}

// calculation
int CHttp_multipart::update_contents_length(const char * key, const char * value)
{	
	if(m_hd.content_length == 0)
		m_hd.content_length = 2 + strlen(m_hd.boundary); // start "--[boundary]"
		
	m_hd.content_length += strlen(multipart_form_data_fmt_start)-2 + strlen(key);
	m_hd.content_length += strlen(value);
	m_hd.content_length += strlen(multipart_form_data_fmt_end) - 2 + strlen(m_hd.boundary); // end "\r\n--[boundary]"
	
	return m_hd.content_length;
}

int CHttp_multipart::update_contents_length(THUMBNAIL * th)
{	
	const char * key = "thumbnail";
	const char * thumbnail_json_fmt = "{\"camera_type\": 0,\"base64\": \"%s\"}";
	//const char * thumbnail_json_fmt2ch = "[{\"camera_type\": 0,\"base64\": \"%s\"},{\"camera_type\": 1,\"base64\": \"%s\"}]";

	if(th->size[0] == 0) {
		dprintf(MULTIPART_ERROR, " %s() : %d length error!\n", __func__, th->size[0]);
		return m_hd.content_length;
	}
	
	if(m_hd.content_length == 0)
		m_hd.content_length = 2 + strlen(m_hd.boundary); // start "--[boundary]"
		
	m_hd.content_length += strlen(multipart_form_data_fmt_start)-2 + strlen(key);

	m_hd.content_length += 2; // json array []
	for(int ch = 0 ; ch < DEF_MAX_CAMERA_COUNT; ch++){
		int base_64_size = 0;
		if( th->size[ch] == 0 || th->size[ch] > DEF_MAX_THUMBNAIL_DATA_SIZE){
			break;
		}
		if(ch != 0)
			m_hd.content_length ++; //json array ','
		
		m_hd.content_length += strlen(thumbnail_json_fmt) - 2;
		base_64_size = b64e_size(th->size[ch]);
		m_hd.content_length += base_64_size;
		
		dprintf(MULTIPART_INFO, "%s() : base64 encode size : %d \n", __func__, base_64_size);
	}
	
	m_hd.content_length += strlen(multipart_form_data_fmt_end) - 2 + strlen(m_hd.boundary); // end "\r\n--[boundary]"
	
	return m_hd.content_length;
}

int CHttp_multipart::update_contents_length(const char * file_path)
{
	int contents_length = 0;
	char buf[1024];
	int file_size = 0;
	
	file_size = sysfs_getsize(file_path);
	
	char file_name[128] = { 0,};
	char file_ext[32] = {0,};
	
	const char *lastSeparator = strrchr(file_path, '/');
	const char *lastDot = strrchr(file_path, '.');
    const char *endOfPath = file_path + strlen(file_path);
    const char *startOfName = lastSeparator ? lastSeparator + 1 : file_path;
    const char *startOfExt = lastDot > startOfName ? lastDot : endOfPath;

	sprintf(file_ext, "%s", startOfExt + 1);
	sprintf(file_name, "%.*s.%s",  startOfExt - startOfName, startOfName, file_ext);

	if(m_hd.content_length == 0)
		m_hd.content_length = 2 + strlen(m_hd.boundary); // start "--[boundary]"
		
	m_hd.content_length += strlen(multipart_form_data_file_fmt)-4 + strlen("file") + strlen(file_name);
	m_hd.content_length += strlen(multipart_form_data_content_type_fmt)-2 + strlen(get_content_type(file_ext));
	m_hd.content_length += file_size;
	m_hd.content_length += strlen(multipart_form_data_fmt_end) - 2 + strlen(m_hd.boundary); // end "\r\n--[boundary]"
	
	//update_contents_length("file_hash", "ff4dd58b07b170345acc9cfa5fe9082f");
	update_contents_length(1, strlen("file_hash") + 32);

	dprintf(MULTIPART_INFO, " %s() : [%s] file size = %d, content_length=%d \n", __func__, file_path, file_size, contents_length);
	
	return m_hd.content_length;
}

int CHttp_multipart::request_start(const char *query_string)
{
	http_multipart_head *hd = &m_hd;
	char msg_buf[1024 * 5] = {0,};
	int length = 0;
	
	m_boundary = hd->boundary;
	
	if(m_conn == NULL){
		memset((void *)&last_error_buf, 0, sizeof(last_error_buf));
		m_contents_length = 0;
		m_conn = mg_connect_client(hd->host, hd->port, hd->is_ssl, last_error_buf, sizeof(last_error_buf));
		ASSERT(m_conn != NULL);

		if(m_conn == NULL) {
			dprintf(MULTIPART_ERROR, "server connection error!\n");
			return -1;
		}
	}

	if(query_string)
		sprintf(msg_buf, "%s %s?%s HTTP/1.1\r\n", hd->request_method, hd->request_uri, query_string);
	else
		sprintf(msg_buf, "%s %s HTTP/1.1\r\n", hd->request_method, hd->request_uri);

	
	length += mg_printf(m_conn, msg_buf);
	length += mg_printf(m_conn, "Host: %s:%d\r\n", hd->host, hd->port);
	length += mg_printf(m_conn, "User-Agent: recorder\r\n");
	length += mg_printf(m_conn, "Content-Type: multipart/form-data; boundary=%s\r\n", hd->boundary);
	//length += mg_printf(m_conn, "Accept-Encoding: identity\r\n"); //gzip, deflate
	length += mg_printf(m_conn, "Content-Length: %d\r\n", hd->content_length + 4 ); //  --[end] string add
	length += mg_printf(m_conn, "Connection: keep-alive\r\n\r\n");

#if 1
	dprintf(MULTIPART_INFO, "%s() : %d\r\n\r\n", __func__, hd->is_ssl);
	dprintf(MULTIPART_INFO, msg_buf);
	dprintf(MULTIPART_INFO, "Host: %s:%d\r\n", hd->host, hd->port);
	dprintf(MULTIPART_INFO, "User-Agent: recorder\r\n");
	dprintf(MULTIPART_INFO, "Content-Type: multipart/form-data; boundary=%s\r\n", hd->boundary);
	//length += mg_printf(m_conn, "Accept-Encoding: identity\r\n"); //gzip, deflate
	dprintf(MULTIPART_INFO, "Content-Length: %d\r\n", hd->content_length + 4 ); //  --[end] string add
	dprintf(MULTIPART_INFO, "Connection: keep-alive\r\n\r\n");
#endif

	return length;
}

int CHttp_multipart::request(const char * key, const char * value)
{
	if(m_conn == NULL){

		if(m_hd.content_length == 0) {
			dprintf(MULTIPART_ERROR, "%s() : content length error!\n", __func__);
			return 0;
		}
		
		if(request_start() == -1)
			return -1;
	}
	
	if(m_contents_length == 0)
		m_contents_length += mg_printf(m_conn, "--%s", m_boundary); //first frame, // start "--[boundary]"
	
	m_contents_length += mg_printf(m_conn, multipart_form_data_fmt_start, key);
	m_contents_length += mg_printf(m_conn, value);
	m_contents_length += mg_printf(m_conn, multipart_form_data_fmt_end, m_hd.boundary); // end "\r\n--[boundary]"

	dprintf(MULTIPART_INFO, "%s : %s\r\n", key, value);
	
	return m_contents_length;
}

int CHttp_multipart::request(THUMBNAIL * th)
{
	int ch;
	const char * key = "thumbnail";
	const char * thumbnail_json_fmt_f = "{\"camera_type\": %d,\"base64\": \"";
	const char * thumbnail_json_fmt_r = "\"}";
	//const char * thumbnail_json_fmt2ch = "[{\"camera_type\": 0,\"base64\": \"%s\"},{\"camera_type\": 1,\"base64\": \"%s\"}]";

	if(th->size[0] == 0) {
		dprintf(MULTIPART_ERROR, " %s() : %d length error!\n", __func__, th->size[0]);
		return 0;
	}
	
	if(m_conn == NULL){

		if(m_hd.content_length == 0) {
			dprintf(MULTIPART_ERROR, "%s() : content length error!\n", __func__);
			return 0;
		}
		
		if(request_start() == -1)
			return -1;
	}
	
	if(m_contents_length == 0)
		m_contents_length += mg_printf(m_conn, "--%s", m_boundary); //first frame, // start "--[boundary]"
	
	m_contents_length += mg_printf(m_conn, multipart_form_data_fmt_start, key);
	
	m_contents_length += mg_printf(m_conn, "[");
	
	for( ch = 0 ; ch < DEF_MAX_CAMERA_COUNT; ch++){
		unsigned char buf[ 5 * 1024 ];
		int pos = 0, base_64_size = 0;
		int size;
		int en_size;
			
		if( th->size[ch] == 0 || th->size[ch] > DEF_MAX_THUMBNAIL_DATA_SIZE){
			break;
		}
		if(ch != 0)
			m_contents_length += mg_printf(m_conn, ",");
		
		m_contents_length += mg_printf(m_conn, thumbnail_json_fmt_f, ch);

		pos = 0;
		base_64_size = 0;
		while(pos < th->size[ch]){
			size = (3 * 1024);
			
			if(pos + size > th->size[ch])
				size = th->size[ch] % size;

			//dprintf(MULTIPART_INFO, "%s() : 0x%08x \n", __func__, *((int *)&th->data[ch][pos]));
			
			en_size = b64_encode((const unsigned char *)&th->data[ch][pos], size, buf);
			pos += size;

			//dprintf(MULTIPART_INFO, "%s() : %*s\n", __func__, 6, buf);
			
			mg_write(m_conn, buf, en_size);
			base_64_size += en_size;
		}
		dprintf(MULTIPART_INFO, "%s() : base64 encode size : %d \n", __func__, base_64_size);

		m_contents_length += base_64_size;
		m_contents_length += mg_printf(m_conn, thumbnail_json_fmt_r);
	}
	m_contents_length += mg_printf(m_conn, "]");
	
	m_contents_length += mg_printf(m_conn, multipart_form_data_fmt_end, m_hd.boundary); // end "\r\n--[boundary]"
	
	return m_contents_length;
}
	
int CHttp_multipart::request(const char * file_path, char * ret_file_hash, int file_hash_size)
{
	md5_state_t ctx;
	md5_byte_t hash[16];
	char md5_hash[33] = {0,};
	char * buf;
	int file_size = 0;
	
	int fd;
	int ret;
	
	char file_name[128] = { 0,};
	char file_ext[32] = {0,};
	
	const char *lastSeparator = strrchr(file_path, '/');
	const char *lastDot = strrchr(file_path, '.');
    const char *endOfPath = file_path + strlen(file_path);
    const char *startOfName = lastSeparator ? lastSeparator + 1 : file_path;
    const char *startOfExt = lastDot > startOfName ? lastDot : endOfPath;
		
	if(m_conn == NULL){

		if(m_hd.content_length == 0) {
			dprintf(MULTIPART_ERROR, "%s() : content length error!\n", __func__);
			return 0;
		}
			
		if(request_start() == -1)
			return -1;
	}

	buf = new char[ DEF_FILE_READ_BUFFER_SIZE ];
	if(buf == NULL)
	{
		dprintf(MULTIPART_ERROR, " %s() : buffer allocation error!!!\n", __func__);
		return 0;
	}
	
	sprintf(file_ext, "%s", startOfExt + 1);
	sprintf(file_name, "%.*s.%s",  startOfExt - startOfName, startOfName, file_ext);
		
	fd = open(file_path, O_RDONLY);
	if (fd < 0) {
		dprintf(MULTIPART_ERROR, " %s() : [%s] file open error!\n", __func__, file_path);
		return 0;
	}
	
	if(m_contents_length == 0)
		m_contents_length += mg_printf(m_conn, "--%s", m_boundary); //first frame, // start "--[boundary]"
		
	m_contents_length += mg_printf(m_conn, multipart_form_data_file_fmt, "file", file_name);
	m_contents_length += mg_printf(m_conn, multipart_form_data_content_type_fmt, get_content_type(file_ext));
	
	
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

	m_contents_length += mg_printf(m_conn, multipart_form_data_fmt_end, m_hd.boundary); // end "\r\n--[boundary]"

	md5_finish(&ctx, hash);
	bin2str(md5_hash, hash, sizeof(hash));
	strncpy(ret_file_hash, md5_hash, file_hash_size);
	m_contents_length += request("file_hash", md5_hash);

	dprintf(MULTIPART_INFO, "%s(size:%dKB, hash:%s)\r\n", file_path, file_size / 1024, md5_hash);
	
	return m_contents_length;
}

int CHttp_multipart::request_end(http_multipart_data_cb data_cb, int timeout_sec)
{
	char msg_buf[1024 * 10] = {0,}; 
	int read_length = 0;
	int result;
	time_t wait_sec = 0;

	u32 start_tick = get_tick_count();
	
	if(m_conn == NULL){
		dprintf(MULTIPART_ERROR, " connection error!!!\n");
		return 0;
	}

	m_contents_length += mg_printf(m_conn, "--\r\n");

	dprintf(MULTIPART_INFO, "%s() : waiting response ... (timeout : %d sec)\r\n", __func__, timeout_sec);
	
	do{
			memset((void *)&last_error_buf, 0, sizeof(last_error_buf));
			result = mg_get_response(m_conn, last_error_buf, sizeof(last_error_buf), 1000, 0);
			
			if(get_tick_count() >= (timeout_sec * 1000) + start_tick){
				dprintf(MULTIPART_ERROR, " timeout error!!! %d (%s)\n", result, last_error_buf);
				dprintf(MULTIPART_ERROR, " timeout error!!! %d (%s)\n", result, last_error_buf);
				break;
			}

			msleep(100);
		}while(result < 0);

		//dprintf(MULTIPART_INFO, "%s\n", msg_buf);
		//ASSERT(msg_buf[0] != 0);

		const struct mg_request_info *ri;
		ri = mg_get_request_info(m_conn);
		//ASSERT(ri->content_length == 0);
		
		read_length = mg_read(m_conn, msg_buf, sizeof(msg_buf));
		dprintf(MULTIPART_INFO, "%s\n", msg_buf);
		
		if(read_length && data_cb) {
			const char GzipArchiveFileSignatures[3] = {0x1f, 0x8b, 0x08};
			
			dprintf(1, "\r\nresponse length : %u bytes (%02x %02x %02x ...)\r\n", read_length, msg_buf[0], msg_buf[1], msg_buf[2]);

			//GZIP file check
			if(GzipArchiveFileSignatures[0] == msg_buf[0] && GzipArchiveFileSignatures[1] == msg_buf[1] && GzipArchiveFileSignatures[2] == msg_buf[2] ){
 #if 1
 				sysfs_write("/tmp/response.gz", msg_buf, read_length);
 				system("gzip -d /tmp/response.gz");
 				result = sysfs_read("/tmp/response", msg_buf, sizeof(msg_buf));
				
				dprintf(1, "GZIP File	: decompress length %d\r\n", result);
				
				if(result)
					data_cb(this, msg_buf, result);
 #elif 0
				int i;
 				int numitems;
 				ZRESULT zr;
 				ZIPENTRY ze;
 				HZIP hz = OpenZip((void *)msg_buf, read_length, 0);  
				//SetUnzipBaseDir(hz,"\\");

				i = 0;
				zr = GetZipItem(hz,-1,&ze); 
				
				dprintf(1, "GetZipItem	: result %x, index %d\r\n", zr, ze.index);

					
				numitems=ze.index;
				for (int zi=0; zi<numitems; zi++)
				{ 
					char *ibuf;
					zr = GetZipItem(hz,zi,&ze);

					dprintf(1, "GetZipItem	: result %x, attr %x, name %s(comp size %d, unc size %d)\r\n", zr, ze.attr, ze.name, ze.comp_size, ze.unc_size);
					
					if(ze.unc_size){
					 	ibuf = new char[ze.unc_size];
						
						UnzipItem(hz,zi, ibuf, ze.unc_size);
						data_cb(this, ibuf, ze.unc_size);

						delete[] ibuf;
					}
				}
				CloseZip(hz);
 #else				
 				int i;
				ri = mg_get_request_info(m_conn);
	
				dprintf(MULTIPART_ERROR, "request_method	: %s\r\n", ri->request_method);
				dprintf(MULTIPART_ERROR, "request_uri	: %s\r\n", ri->request_uri);
				dprintf(MULTIPART_ERROR, "local_uri	: %s\r\n", ri->local_uri);
				dprintf(MULTIPART_ERROR, "uri		: %s\r\n", ri->uri);
				dprintf(MULTIPART_ERROR, "http_version	: %s\r\n", ri->http_version);
				dprintf(MULTIPART_ERROR, "query_string	: %s\r\n", ri->query_string);
				dprintf(MULTIPART_ERROR, "remote_user	: %s\r\n", ri->remote_user);
				dprintf(MULTIPART_ERROR, "remote_addr	: %s\r\n", ri->remote_addr);
				dprintf(MULTIPART_ERROR, "content_length	: %d\r\n", ri->content_length);
				dprintf(MULTIPART_ERROR, "remote_port	: %d\r\n", ri->remote_port);
				dprintf(MULTIPART_ERROR, "num_headers	: %d\r\n", ri->num_headers);

				dprintf(MULTIPART_ERROR, "\r\nresponse string error!!!(%s)\n", last_error_buf);

				for(i=0; i < read_length; i++){
					if(i % 16 == 0)
							dprintf(MULTIPART_ERROR, "\r\n dump data %u : ", i / 16);

					dprintf(MULTIPART_ERROR, "%02x ", msg_buf[i]);
				}

				for(i=0; i < read_length; i++){
					if(msg_buf[i]){
						break;
					}
				}
				
				dprintf(MULTIPART_ERROR, "\r\n skip! %u bytes, length : %u bytes (%02x ...)\n", i, read_length - i, msg_buf[i]);
				data_cb(this, &msg_buf[i], read_length - i);
 #endif				
			}
			else{
				data_cb(this, msg_buf, read_length);
			}

		}
		
		mg_close_connection(m_conn);
		m_conn = NULL;
		dprintf(MULTIPART_INFO, "close connection \n");

		if(wait_sec >= timeout_sec)
			return -1;
		
		return 1;
}

int CHttp_multipart::response(http_multipart_head *hd, http_multipart_data_cb data_cb, int timeout_sec)
{
	int result;

	if(hd != &m_hd)
		m_hd = *hd;
	
	request_start(hd->query_string);

	mg_write(m_conn, hd->user_buffer, hd->content_length);

	dprintf(MULTIPART_INFO, hd->user_buffer);

	result = request_end(data_cb, timeout_sec);

	return result;
}

int CHttp_multipart::make_contents(http_multipart_head *hd, bool end_flag)
{
	int length = 0;
	
	
	if(m_conn == NULL){
		memset((void *)&last_error_buf, 0, sizeof(last_error_buf));
		m_boundary = hd->boundary;
		m_conn = mg_connect_client(hd->host, hd->port, hd->is_ssl, last_error_buf, sizeof(last_error_buf));
		ASSERT(m_conn != NULL);
		ASSERT(last_error_buf[0] == 0);

		if(m_conn == NULL)
			return -1;
		
		dprintf(MULTIPART_INFO, "%s connectd %s\n", hd->host, last_error_buf);
		
		hd->content_length += sprintf(&hd->user_buffer[hd->content_length], "--%s", m_boundary); //first frame

	}
	
	if(hd->user_name && hd->user_data && hd->user_data_size){
		make_contents(hd->user_name, hd->user_data, hd->user_data_size, NULL, false);
	}
	
	if(end_flag) {
		int result = response(hd, hd->data_cb, HTTP_RESPONSE_WAIT_TIME);
		m_conn = NULL;
		return result;
	}
	
	return length;
}

int CHttp_multipart::make_contents(const char * name, const void * data, size_t size, http_multipart_data_cb data_cb, bool end_flag)
{
	int length = 0;
#if 0
	if(m_hd.content_length + size + (strlen(m_boundary) * 2) > HTTP_MULTIPART_BUFFER_SIZE)
	{
		dprintf(MULTIPART_INFO, "buffer full!\n");
		return -1;
	}
#endif	
	if(m_conn == NULL){
		if(make_contents(&m_hd, 0) < 0)
			return -1;
	}
		
	m_hd.content_length += sprintf(&m_hd.user_buffer[m_hd.content_length], multipart_form_data_fmt_start, name);
	memcpy((void *)&m_hd.user_buffer[m_hd.content_length], data, size);
	m_hd.content_length += size;
	m_hd.content_length += sprintf(&m_hd.user_buffer[m_hd.content_length], multipart_form_data_fmt_end, m_boundary);

	if(end_flag) {
		int result = response(&m_hd, data_cb, HTTP_RESPONSE_WAIT_TIME);
		m_conn = NULL;
		return result;
	}
	
	return length;
}

int CHttp_multipart::make_contents(const char *k, const char *p, http_multipart_data_cb data_cb, bool end_flag)
{
	int length = 0;
#if 0
	if(m_hd.content_length + strlen(p) + (strlen(m_boundary) * 2) > HTTP_MULTIPART_BUFFER_SIZE)
	{
		dprintf(MULTIPART_INFO, "buffer full!\n");
		return -1;
	}
#endif

	if(m_conn == NULL){
		if(make_contents(&m_hd, 0) < 0)
			return -1;
	}
	
	m_hd.content_length += sprintf(&m_hd.user_buffer[m_hd.content_length], multipart_form_data_fmt_start, k);
	m_hd.content_length += sprintf(&m_hd.user_buffer[m_hd.content_length], p);
	m_hd.content_length += sprintf(&m_hd.user_buffer[m_hd.content_length], multipart_form_data_fmt_end, m_boundary);

	if(end_flag) {
		int result = response(&m_hd, data_cb, HTTP_RESPONSE_WAIT_TIME);
		m_conn = NULL;

		return result;
	}
	
	return length;
}

/*
name		// file
path // /mnt/extsd/speed.scv 
type //zip ...
*/
int CHttp_multipart::make_contents(Json::Value &form_file, CHttp_multipart::http_multipart_data_cb data_cb, bool end_flag)
{
	unsigned long length = 0;
#if 0
	if(m_hd.content_length + strlen(p) + (strlen(m_boundary) * 2) > HTTP_MULTIPART_BUFFER_SIZE)
	{
		dprintf(MULTIPART_INFO, "buffer full!\n");
		return -1;
	}
#endif

	if(m_conn == NULL){
		if(make_contents(&m_hd, 0) < 0)
			return -1;
	}
	//Content-Disposition: form-data; name=\"file\"; filename=\"speed1.zip\"\r\nContent-Type: application/zip
	//form_file["type"] = "zip";
	//form_file["path"] = file_path;
	//form_file["name"] = "file"
	//form_file["file_hash"] = "md5"
	
	char file_name[128] = { 0,};
	const char *lastSeparator = strrchr(form_file["path"].asString().c_str(), '/');
	const char *lastDot = strrchr(form_file["path"].asString().c_str(), '.');
    const char *endOfPath = form_file["path"].asString().c_str() + strlen(form_file["path"].asString().c_str());
    const char *startOfName = lastSeparator ? lastSeparator + 1 : form_file["path"].asString().c_str();
    const char *startOfExt = lastDot > startOfName ? lastDot : endOfPath;


	sprintf(file_name, "%.*s.%s",  startOfExt - startOfName, startOfName, form_file["type"].asString().c_str());
	m_hd.content_length += sprintf(&m_hd.user_buffer[m_hd.content_length], "\r\nContent-Disposition: form-data; name=\"%s\"; filename=\"%s\"\r\n", form_file["name"].asString().c_str(), file_name);
	m_hd.content_length += sprintf(&m_hd.user_buffer[m_hd.content_length], "Content-Type: %s\r\n\r\n", get_content_type(form_file["type"].asString().c_str()));
	
	//
	FILE* file=  fopen( form_file["path"].asString().c_str(), "r");

	if(!file){
		dprintf(MULTIPART_ERROR, " [%s] file open error!\n", form_file["path"].asString().c_str());
		return 0;
	}
	
	fseek( file, 0, SEEK_END );
	length = ftell( file );
	fseek( file, 0, SEEK_SET );

// Strange case, but good to handle up front.
	if ( length <= 0 )
	{
		fclose(file);
		dprintf(MULTIPART_ERROR, " [%s] file size error!\n", form_file["path"].asString().c_str());
		return 0;
	}

	char* buf = new char[ length+1 ];
	buf[0] = 0;

	if ( fread( buf, length, 1, file ) != 1 ) {
		delete [] buf;
		fclose(file);
		dprintf(MULTIPART_ERROR, " [%s] file read error!\n", form_file["path"].asString().c_str());
		return 0;
	}

	char md5_hash[33] = {0,};
	
	if(strcmp("zip", form_file["type"].asString().c_str()) == 0 && strcmp("zip", startOfExt+1) != 0)
	{
		unsigned long zip_buf_len =length+1024;
		char * zip_buf  = new char[ zip_buf_len ];
		unsigned long zip_length = 0;
		void *zbuf_get;

		//¾ÐÃà
		// afford to be generous: no address-space is allocated unless it's actually needed.
  		HZIP hz;
		hz = CreateZip(zip_buf, zip_buf_len, 0);
		if (hz==NULL) 
			dprintf(MULTIPART_ERROR,"%x Failed to create zip!\r\n", (unsigned int)hz);
		
		//ZRESULT zret = ZipAdd(hz, startOfName, buf, length);
		ZRESULT zret = ZipAdd(hz, startOfName, form_file["path"].asString().c_str());
		
		if (zret !=ZR_OK) 
			dprintf(MULTIPART_ERROR,"%x Failed to add file(%s)\r\n", (unsigned int)hz, startOfName);

		zret = ZipGetMemory(hz, (void **)&zbuf_get, &zip_length);

		dprintf(MULTIPART_INFO,"%x : %s(%ld), %s(%ld)\r\n", (unsigned int)zret, form_file["path"].asString().c_str(), length, file_name, zip_length);
		
		if(zret == ZR_OK) {
			mg_md5_data(md5_hash, zbuf_get, zip_length);
			memcpy((void *)&m_hd.user_buffer[m_hd.content_length], zbuf_get, zip_length);
			m_hd.content_length += zip_length;
		}

		CloseZip(hz);
		delete [] zip_buf;
	}
	else
	{
		mg_md5_data(md5_hash, buf, length);
		memcpy((void *)&m_hd.user_buffer[m_hd.content_length], buf, length);
		m_hd.content_length += length;
	}

	delete[] buf;
	fclose(file);

	
	m_hd.content_length += sprintf(&m_hd.user_buffer[m_hd.content_length], multipart_form_data_fmt_end, m_boundary);

	if(strcmp("md5", form_file["file_hash"].asString().c_str()) == 0)
	{
		dprintf(MULTIPART_INFO,"md5 : %s\r\n", md5_hash);
		make_contents("file_hash", (const char *)md5_hash, NULL, CHttp_multipart::HTTP_MP_BUFFER);
	}

	if(end_flag) {
		int result= response(&m_hd, data_cb, HTTP_RESPONSE_WAIT_TIME);
		m_conn = NULL;

		return result;
	}

	return (int)length;
}

/*
name		// file
file_name // speed.zip 
type //zip ...
file_hash //md5
*/
int CHttp_multipart::make_contents(Json::Value &form_file, const void * data, size_t size, http_multipart_data_cb data_cb, bool end_flag)
{
	if(m_conn == NULL){
		if(make_contents(&m_hd, 0) < 0)
			return -1;
	}

	std::string upload_file_name = format_string("%s.%s", form_file["file_name"].asString().c_str(), form_file["type"].asString().c_str());
	std::string current_file_name = format_string("%s.%s", form_file["file_name"].asString().c_str(), form_file["file_ext"].asString().c_str());
	
	//Content-Disposition: form-data; name=\"file\"; filename=\"speed1.zip\"\r\nContent-Type: application/zip
	//form_file["type"] = "zip";
	//form_file["path"] = file_path;
	
	m_hd.content_length += sprintf(&m_hd.user_buffer[m_hd.content_length], "\r\nContent-Disposition: form-data; name=\"%s\"; filename=\"%s\"\r\n", form_file["name"].asString().c_str(), upload_file_name.c_str());
	m_hd.content_length += sprintf(&m_hd.user_buffer[m_hd.content_length], "Content-Type: %s\r\n\r\n", get_content_type(form_file["type"].asString().c_str()));

	char md5_hash[33] = {0,};
	
	if(strcmp("zip", form_file["type"].asString().c_str()) == 0 && strcmp("zip", form_file["file_ext"].asString().c_str()) != 0) 
	{
		HZIP hz;
		unsigned long zip_buf_len = size+1024;
		unsigned long zip_length= 0;
		char * zip_buf  = new char[ zip_buf_len ];
		//char zip_buf[1024 * 1024];
		//zip_buf_len = sizeof(zip_buf);
		
		void *zbuf_get;
	
		hz = CreateZip((void *)zip_buf, zip_buf_len, 0);
		if (hz==NULL) 
			dprintf(MULTIPART_ERROR,"%x Failed to create zip!\r\n", (unsigned int)hz);
		
		ZRESULT zret = ZipAdd(hz, current_file_name.c_str(), (char *)data, size);
		
		if (zret !=ZR_OK) 
			dprintf(MULTIPART_ERROR,"%x Failed to add file(%s)\r\n", (unsigned int)hz, current_file_name.c_str());

		zret = ZipGetMemory(hz, (void **)&zbuf_get, &zip_length);

		dprintf(MULTIPART_INFO,"%x : %s(%u), %lu\r\n", (unsigned int)zret, upload_file_name.c_str(), size, zip_length);
		
		if(zret == ZR_OK)
		{
			mg_md5_data(md5_hash, zbuf_get, zip_length);
			memcpy((void *)&m_hd.user_buffer[m_hd.content_length], zbuf_get, zip_length);
			m_hd.content_length += zip_length;

			//sysfs_write(format_string("/mnt/extsd/%s.zip", current_file_name.c_str()).c_str(), (const char *)zbuf_get, zip_length);
		}

		CloseZip(hz);
	
		delete [] zip_buf;
	}
	else
	{
		mg_md5_data(md5_hash, (void *)data, size);
		memcpy((void *)&m_hd.user_buffer[m_hd.content_length], data, size);
		m_hd.content_length += size;
	}

	m_hd.content_length += sprintf(&m_hd.user_buffer[m_hd.content_length], multipart_form_data_fmt_end, m_boundary);

	if(strcmp("md5", form_file["file_hash"].asString().c_str()) == 0)
	{
		dprintf(MULTIPART_INFO,"md5 : %s\r\n", md5_hash);
		make_contents("file_hash", (const char *)md5_hash, NULL, CHttp_multipart::HTTP_MP_BUFFER);
	}
	
	if(end_flag) {
		int result = response(&m_hd, data_cb, HTTP_RESPONSE_WAIT_TIME);
		m_conn = NULL;

		return result;
	}

	return 0;
}
