
#if !defined(PAI_R_UPDATELOG_H)
#define PAI_R_UPDATELOG_H

#include <string>
#include <list>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include "datypes.h"
#include "daversion.h"
#include "pai_r_error_list.h"

#define DEF_SYSTEM_DIR 					"/mnt/extsd/System"
#define DEF_UPDATELOG_PATH 		"/mnt/extsd/System/updatelog.dat"
//#define DEF_UPDATELOG_NO_PATH 			"/mnt/extsd/System/updatelog_no.bin"


#define UPDATELOG_MAX_COUNT		4096
typedef enum {
	UL_DATA_TYPE_AUTHENTICATION = 0,
	UL_DATA_TYPE_LOCATION,
	UL_DATA_TYPE_MOVIE,

	UL_DATA_TYPE_INFORMATION,
	UL_DATA_TYPE_POLLING, //for mmb server
	
	UL_DATA_TYPE_INIT = 99,

	UL_DATA_TYPE_ALL = 256,
	
	UL_DATA_TYPE_END
}E_UL_DATA_TYPE;

typedef enum {
	UL_RESULT_TYPE_SUCCESS = 0,
	UL_RESULT_TYPE_CONNECTION_FAILURE,	//Error: Connection failure
	UL_RESULT_TYPE_API_ERROR,	//Error: Successful connection / API error
	
	UL_RESULT_TYPE_END
}E_UL_RESULT_TYPE;

typedef struct {
	u32		log_no;
	u32 		auto_id;					//동영상 자동 아이디
	time_t	create_time;			//로그 작성 일시
	time_t	movie_create_time;
	time_t	send_time;				//전송 시작 일시
	u8		date_type;				//E_UL_DATA_TYPE
	u8		result_type;				//E_UL_RESULT_TYPE
	char		comment[64];			// Timeout ...
//----------------------------------------------------
//	88 Byte
//-----------------------------------------------------	
	char 	result_json[1024 - 88];
}PAI_R_UPDATELOG;

class CPai_r_updatelog 
{
public:
	CPai_r_updatelog();
	virtual ~CPai_r_updatelog();

public:
	int add_updatelog(PAI_R_UPDATELOG *log);
	int get_updatelog(u32 no, PAI_R_UPDATELOG *log);
	int get_updatelog(u32 no, PAI_R_UPDATELOG *log, FILE *fp);
	
	int make(E_UL_DATA_TYPE type, u32 autoid, time_t movie_create_time = 0);
	int save(E_UL_RESULT_TYPE type, const char * commant, const char * last_error, const char * result_json = NULL);
	
	PAI_R_UPDATELOG m_data;
	u32  m_log_no;

protected:
	bool m_is_init;
	int init(void);
};
#endif // !defined(PAI_R_UPDATELOG_H)

