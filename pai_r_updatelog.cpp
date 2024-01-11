// pai_r_data.cpp: implementation of the CPai_r_data class.
//
//////////////////////////////////////////////////////////////////////
#include <stdarg.h>
#include <fcntl.h>
#include "daappconfigs.h"
#include "datools.h"
#include "sysfs.h"
//#include "tinyxml.h"
#include "pai_r_updatelog.h"

#define DBG_PAIR_LOG_INIT	1
#define DBG_PAIR_LOG_FUNC 	0 // DBG_MSG
#define DBG_PAIR_LOG_ERR  		DBG_ERR
#define DBG_PAIR_LOG_WRN		DBG_WRN

CPai_r_updatelog::CPai_r_updatelog()
{
	m_is_init = false;
	m_log_no = 0;
}

CPai_r_updatelog::~CPai_r_updatelog()
{

}


int CPai_r_updatelog::add_updatelog(PAI_R_UPDATELOG *log)
{
	u32 id = 0;
	FILE *fp;
	const char * data_path = DEF_UPDATELOG_PATH;

	if(!m_is_init){
		if(init() == 0)
			return 0;
	}
	
	fp = fopen(data_path, "rb+");

	id = (m_log_no % (UPDATELOG_MAX_COUNT));
	
	if(fp){
		fseek(fp, id  * sizeof(PAI_R_UPDATELOG), SEEK_SET);

		if(log->create_time == 0)
			log->create_time = time(0);
		
		fwrite((void *)log, 1, sizeof(PAI_R_UPDATELOG), fp);				

		fclose(fp);
		dbg_printf(DBG_PAIR_LOG_FUNC, "%s() :  %d (%s) \n", __func__, m_log_no, make_time_string(log->create_time).c_str());
		m_log_no++;
	} else {
		dbg_printf(DBG_PAIR_LOG_ERR, "%s creation failed: %d(%s)\n", data_path, errno, strerror(errno));
	}
	return m_log_no;
}

int CPai_r_updatelog::get_updatelog(u32 no, PAI_R_UPDATELOG *log, FILE *fp)
{
	u32 id = 0;
	
	if(!m_is_init){
		if(init() == 0)
			return 0;
	}

	if(no >= m_log_no){
		dbg_printf(DBG_PAIR_LOG_ERR, "%s() : id error!(%d:%d)\n", __func__, no, m_log_no);
		return 0;
	}
	
	id = (no % (UPDATELOG_MAX_COUNT));

	fseek(fp, id * sizeof(PAI_R_UPDATELOG),SEEK_SET);	
	int ret =  fread( (void *)log, 1, sizeof(PAI_R_UPDATELOG), fp );
	if (ret != sizeof(PAI_R_UPDATELOG) ) {
		dbg_printf(DBG_PAIR_LOG_ERR, "%s read failed: %d(%s) , ret = %d : %d\n", __func__, errno, strerror(errno), ret, sizeof(PAI_R_UPDATELOG));
		return 0;
	}

	dbg_printf(DBG_PAIR_LOG_FUNC, "%s get %d \n", __func__, no);
			
	return 1;
}

int CPai_r_updatelog::get_updatelog(u32 no, PAI_R_UPDATELOG *log)
{
	u32 id = 0;
	
	if(!m_is_init){
		if(init() == 0)
			return 0;
	}

	if(no >= m_log_no){
		dbg_printf(DBG_PAIR_LOG_ERR, "%s() : id error!(%d:%d)\n", __func__, no, m_log_no);
		return 0;
	}
	
	const char * data_path = DEF_UPDATELOG_PATH;
	FILE *fp = fopen(data_path, "rb");
	
	if(fp){
		get_updatelog(no, log, fp);

		fclose(fp);
	} else {
		dbg_printf(DBG_PAIR_LOG_ERR, "%s open failed: %d(%s)\n", data_path, errno, strerror(errno));
		return 0;
	}
	return 1;
}

int CPai_r_updatelog::init(void)
{
	if(m_is_init)
		return 1;
	const char * file_path = DEF_UPDATELOG_PATH;

	if ( access(file_path, R_OK ) != 0) {
		u32 i;
		FILE *fp = fopen(file_path, "wb");
		
		if(fp) {
			PAI_R_UPDATELOG log;
			memset((void *)&log, 0, sizeof(log));

			log.log_no = 1;
			log.date_type = UL_DATA_TYPE_INIT;
			log.result_type = 0;
			log.create_time = time(0);
			sprintf(log.comment, "Initialize");
			
			fwrite((void *)&log, 1, sizeof(log), fp);

			fclose(fp);
			dbg_printf(DBG_PAIR_LOG_FUNC, "%s saved %d bytes.\n", file_path, sizeof(log));
		} else {
			dbg_printf(DBG_PAIR_LOG_ERR, "%s creation failed: %d(%s)\n", file_path, errno, strerror(errno));
		}
	}

	
	FILE* file=  fopen( file_path, "r");
	int length = 0;
	
	if(!file){
		dbg_printf(DBG_PAIR_LOG_ERR," [%s] file open error!\n", file_path);
		return 0;
	}
	
	fseek( file, 0, SEEK_END );
	length = ftell( file );
	fseek( file, 0, SEEK_SET );
	
// Strange case, but good to handle up front.
	if ( length <= 0 ) {
		m_log_no = 0;
		dbg_printf(DBG_PAIR_LOG_ERR, "%s file error!\r\n", file_path);
	}
	else if(length < UPDATELOG_MAX_COUNT * sizeof(PAI_R_UPDATELOG)) {
		m_log_no = length / sizeof(PAI_R_UPDATELOG);
	}
	else {
		PAI_R_UPDATELOG log;
		int ret;
		m_log_no = 0;

		for(int i = 0; i < UPDATELOG_MAX_COUNT; i++){
			ret =  fread( (void *)&log, 1, sizeof(PAI_R_UPDATELOG), file );
			if (ret != sizeof(PAI_R_UPDATELOG) ) {
				dbg_printf(DBG_PAIR_LOG_ERR, "%s read failed: %d(%s) , ret = %d : %d\n", file_path, errno, strerror(errno), ret, sizeof(PAI_R_UPDATELOG));
			}

			if(log.log_no > m_log_no)
				m_log_no = log.log_no;
			else
				break;
		}
	}
	fclose(file);
	
	m_is_init = true;
	
	dbg_printf(DBG_PAIR_LOG_INIT, "updatelog.%s() : %d\n", __func__, m_log_no);
	return 1;
}

int CPai_r_updatelog::make(E_UL_DATA_TYPE type, u32 autoid, time_t movie_create_time)
{
	if(!m_is_init){
		if(init() == 0)
			return 0;
	}
	
	memset((void *)&m_data, 0, sizeof(m_data));
	
	m_data.log_no = m_log_no + 1;
	m_data.auto_id = autoid;
	
	m_data.create_time = time(0);
	m_data.movie_create_time = movie_create_time;
	m_data.send_time = m_data.create_time;

	m_data.date_type = (u8)type;
	dbg_printf(DBG_PAIR_LOG_FUNC, "updatelog.%s() : %d (autoid %d)\n", __func__, m_log_no, autoid);
	return m_log_no;
}

int CPai_r_updatelog::save(E_UL_RESULT_TYPE type, const char * commant, const char * last_error, const char * result_json)
{
	if(!m_is_init){
		if(init() == 0)
			return 0;
	}
	
	m_data.result_type = (u8)type;
	if(last_error && strlen(last_error))
		snprintf(m_data.comment, sizeof(m_data.comment), "%s(%s)", commant, last_error);
	else
		snprintf(m_data.comment, sizeof(m_data.comment), commant);

	if(result_json)
		snprintf(m_data.result_json, sizeof(m_data.result_json), result_json);

	add_updatelog(&m_data);
	
	dbg_printf(DBG_PAIR_LOG_FUNC, "updatelog.%s() : %d %s\n", __func__, m_data.log_no, m_data.comment);
	
	return m_log_no;
}


