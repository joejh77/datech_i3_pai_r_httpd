#ifndef _PAI_R_ERROR_LIST_H_
#define _PAI_R_ERROR_LIST_H_


#define SUCCESS 0	//API가 정상적으로 기동한 <br> ※ 단 반환값 안에 에러를 포함할 수 있다.
#define ERROR_NO_NEED_ITEM  1   //필수 항목이 포함되어 있지 않다.
#define ERROR_NOT_SATISFY_DATATYPE  2   //값이 데이터형을 충족시키지 못하고 있다.
#define ERROR_UNREGISTERD   3   //DB 미등록  
#define ERROR_LOGIN_INFORMATION 4   //몇 개의 로그인 정보에 오류가 있다.
#define ERROR_CANT_USE_THIS_IPV4    6   //부정한 IPv4주소로 접속한다
#define ERROR_DELETED_COMPANY   21  //회사가 "삭제"이다
#define ERROR_EXPIRED_COMPANY   22  //회사가 "지원 종료"가 되고 있다
#define ERROR_DELETED_TERMINAL  23  //단말이 "삭제"이다
#define ERROR_EXPIRED_TERMINAL  24  //단말이 "지원 종료"가 되고 있다
#define ERROR_DONT_MATCH_DAIRY_QR   25  //일일 QR코드(암호)가 일치하지 않 앱만 사용
#define ERROR_EXPIRED_DAIRY_QR  26  //일일 QR코드(암호)이 시한 앱만 사용
#define ERROR_DONT_EXIST_TEL_NUMBER 27  //전화 번호·식별자가 존재하지 않는다
#define ERROR_DONT_EXIST_UUID   28  //UUID가 존재하지 않는
#define ERROR_DONT_EXIST_OPERATION_AUTOID   29  //업무 자동 ID가 존재하지 않는
#define ERROR_EXPIRED_OPERATION 30  //들은 이미 끝난
#define ERROR_DONT_EXIST_OFFICE_AUTOID  33  //영업소의 등록이 없다
#define ERROR_DONT_EXIST_EMPLOYREE_AUTOID   34  //사원의 등록이 없다
#define ERROR_DONT_EXIST_CARTYPE_AUTOID 35  //차종의 등록이 없다
#define ERROR_DONT_EXIST_CAR_AUTOID 36  //차량 등록이 없다
#define ERROR_DONT_EXIST_RECODERWIFI_MAC    37  //블랙 박스 MAC주소 등록이 없다
#define ERROR_DONT_EXPIRED_RECODERWIFI  38  //블랙 박스 MAC주소가 만료
#define ERROR_REGISTRATION_COUNT_OVER   61  //등록 수 오버
#define ERROR_DUPLICATE_ITEM    63  //변경하면 다른 행과 중복
#define ERROR_CANT_USING_WORD   64  //사용할 수 없는 문자가 사용되고 있다
#define ERROR_TOO_SMALL_WORDS   65  //설정하는 문자열이 너무 짧다
#define ERROR_BROKEN_IMAGE  81  //올린 화상 파일이 파손되어 있다
#define ERROR_BROKEN_ZIP    82  //올린 zip파일이 파손되어 있다
#define ERROR_BROKEN_CSV    83  //올린 csv파일이 파손되어 있다
#define ERROR_BROKEN_MOVIE  84  //올린 동영상 파일이 파손되어 있다
#define ERROR_DONT_HAVE_ACCESS  100 //접속 금지(권한이 필요)
#define ERROR_PHP   700 //PHP의 부진
#define ERROR_SQL   800 //DB의 부진, SQL오류
#define ERROR_UNDEFINED 999 //불명

#endif // _PAI_R_ERROR_LIST_H_

