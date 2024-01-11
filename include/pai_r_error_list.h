#ifndef _PAI_R_ERROR_LIST_H_
#define _PAI_R_ERROR_LIST_H_


#define SUCCESS 0	//API�� ���������� �⵿�� <br> �� �� ��ȯ�� �ȿ� ������ ������ �� �ִ�.
#define ERROR_NO_NEED_ITEM  1   //�ʼ� �׸��� ���ԵǾ� ���� �ʴ�.
#define ERROR_NOT_SATISFY_DATATYPE  2   //���� ���������� ������Ű�� ���ϰ� �ִ�.
#define ERROR_UNREGISTERD   3   //DB �̵��  
#define ERROR_LOGIN_INFORMATION 4   //�� ���� �α��� ������ ������ �ִ�.
#define ERROR_CANT_USE_THIS_IPV4    6   //������ IPv4�ּҷ� �����Ѵ�
#define ERROR_DELETED_COMPANY   21  //ȸ�簡 "����"�̴�
#define ERROR_EXPIRED_COMPANY   22  //ȸ�簡 "���� ����"�� �ǰ� �ִ�
#define ERROR_DELETED_TERMINAL  23  //�ܸ��� "����"�̴�
#define ERROR_EXPIRED_TERMINAL  24  //�ܸ��� "���� ����"�� �ǰ� �ִ�
#define ERROR_DONT_MATCH_DAIRY_QR   25  //���� QR�ڵ�(��ȣ)�� ��ġ���� �� �۸� ���
#define ERROR_EXPIRED_DAIRY_QR  26  //���� QR�ڵ�(��ȣ)�� ���� �۸� ���
#define ERROR_DONT_EXIST_TEL_NUMBER 27  //��ȭ ��ȣ���ĺ��ڰ� �������� �ʴ´�
#define ERROR_DONT_EXIST_UUID   28  //UUID�� �������� �ʴ�
#define ERROR_DONT_EXIST_OPERATION_AUTOID   29  //���� �ڵ� ID�� �������� �ʴ�
#define ERROR_EXPIRED_OPERATION 30  //���� �̹� ����
#define ERROR_DONT_EXIST_OFFICE_AUTOID  33  //�������� ����� ����
#define ERROR_DONT_EXIST_EMPLOYREE_AUTOID   34  //����� ����� ����
#define ERROR_DONT_EXIST_CARTYPE_AUTOID 35  //������ ����� ����
#define ERROR_DONT_EXIST_CAR_AUTOID 36  //���� ����� ����
#define ERROR_DONT_EXIST_RECODERWIFI_MAC    37  //�� �ڽ� MAC�ּ� ����� ����
#define ERROR_DONT_EXPIRED_RECODERWIFI  38  //�� �ڽ� MAC�ּҰ� ����
#define ERROR_REGISTRATION_COUNT_OVER   61  //��� �� ����
#define ERROR_DUPLICATE_ITEM    63  //�����ϸ� �ٸ� ��� �ߺ�
#define ERROR_CANT_USING_WORD   64  //����� �� ���� ���ڰ� ���ǰ� �ִ�
#define ERROR_TOO_SMALL_WORDS   65  //�����ϴ� ���ڿ��� �ʹ� ª��
#define ERROR_BROKEN_IMAGE  81  //�ø� ȭ�� ������ �ļյǾ� �ִ�
#define ERROR_BROKEN_ZIP    82  //�ø� zip������ �ļյǾ� �ִ�
#define ERROR_BROKEN_CSV    83  //�ø� csv������ �ļյǾ� �ִ�
#define ERROR_BROKEN_MOVIE  84  //�ø� ������ ������ �ļյǾ� �ִ�
#define ERROR_DONT_HAVE_ACCESS  100 //���� ����(������ �ʿ�)
#define ERROR_PHP   700 //PHP�� ����
#define ERROR_SQL   800 //DB�� ����, SQL����
#define ERROR_UNDEFINED 999 //�Ҹ�

#endif // _PAI_R_ERROR_LIST_H_

