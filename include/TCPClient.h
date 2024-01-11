#if !defined(TCPCLIENT_H)

#define TCPCLIENT_H

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

using namespace std;
#define  TCPIP_SERVER_ADDRESS "192.168.35.19"

#define TCPIP_PORT_UPLOAD   15100
#define TCPIP_PORT_POLLING  17100

#ifdef DEF_MMB_VER_2
#define MMB_SERVER_HEADER0 0xB1
#define MMB_SERVER_HEADER1 0xA0

#define MMB_SERVER_REQUEST_H0       0x55
#define MMB_SERVER_REQUEST_H1       0x6A

#define MMB_SERVER_REQUEST_H1_EVENT   0x7A  // ASCII : YYYYMMDDhhmmssxxx
#define MMB_SERVER_REQUEST_H1_NOTHING 0x6A  // data 0

#define MMB_SERVER_REQUEST_FRAME_H0 0x64
#define MMB_SERVER_REQUEST_FRAME_FILELIST 0x11
#define MMB_SERVER_REQUEST_FRAME_FILE 0x12

#define MMB_CLIENT_RESPONSE_H0      0x55
#define MMB_CLIENT_RESPONSE_H1      0x6A

#define MMB_SERVER_ACK     0x06
#define MMB_SERVER_NAK     0x05

#define MMB_CLIENT_HEADER0 0xA1

#define MMB_CLIENT_HEADER1 0x11      // Event
#define MMB_CLIENT_LIST_HEADER1 0x17 // FileList
#define MMB_CLIENT_FILE_HEADER1 0x18 // File

#define MMB_CLIENT_POLLING_H0    0x64
#define MMB_CLIENT_POLLING_H1    0x44

#else
#define MMB_SERVER_HEADER0 0x45
#define MMB_SERVER_HEADER1 0x6B

#define MMB_SERVER_REQUEST_H0       0x73
#define MMB_SERVER_REQUEST_H1_EVENT   0x7A  // ASCII : YYYYMMDDhhmmssxxx
#define MMB_SERVER_REQUEST_H1_NOTHING 0x7B  // data 0

#define MMB_SERVER_ACK     0x06
#define MMB_SERVER_NAK     0x05

#define MMB_CLIENT_HEADER0 0x63
#define MMB_CLIENT_HEADER1 0x34

#define MMB_CLIENT_POLLING_H0    0x42
#define MMB_CLIENT_POLLING_H1    0x59

#endif
typedef const void *SOCK_OPT_TYPE;

#pragma pack(push, 1)

typedef struct _TagHeaderFrame {
    unsigned char header[2];
    unsigned char idLength;
    unsigned char id[20];
}HeaderFrame;

typedef struct _TagCommonHeader {
    char deviceType;
    char majorVersion[2];
    char minorVersion[2];
    char additionalInfo[10];
#ifdef DEF_MMB_VER_2
    char accOnTime[14];
#endif 
    char deviceId[20];
}CommonHeader;

typedef struct _TagEventData {
#ifndef DEF_MMB_VER_2    
    char accOnTime[14];
#endif
    char eventTime[17];

    /*
    イベント種別
    1：SOS
    2：急加速
    3：急減速
    4：急ハンドル
    5：衝?
    9：ACC ON（Event image dataは無し）
    10：ACC OFF（Event image dataは無し）
    ※イベントは可能なものだけでOK
    */
    unsigned char eventType;

    //ASCII ('A':Position fixed,'V':Position not fixed, 'P':Positioning)
    char gnssStatus; 
    int eventLongitude;
    int eventLatitude;
}EventData;

#pragma pack(pop)

enum {
	MMB_ACK = 0,
	MMB_NAK,
	MMB_ERR,

	MMB_REQUEST_FILE,
#ifdef DEF_MMB_VER_2
    MMB_REQUEST_FILELIST,
    MMB_POLLING_WAIT,
#endif  
	MMB_NOTHING,
};

typedef enum {
    UPLOAD_TYPE_EVENT = 1,
    UPLOAD_TYPE_FILELIST,
    UPLOAD_TYPE_FILE,
    UPLOAD_TYPE_MAX
}eUpLoadDataType;

typedef struct _TagImageData {
    unsigned int length;
    unsigned char *pRowData; //JPG
}ImageData;

class TCPClient {
private:
    int sock;
    string address;
    int port;
	 
    struct sockaddr_in server;

public:
	
	int frame_size;
	int mmb_ack;
#ifdef DEF_MMB_VER_2
    int retry_cnt;  // 0인경우 retry 필요없음 
#endif
	time_t request_t;
    CommonHeader header;
    EventData event;

	HeaderFrame frame_polling;
	HeaderFrame frame_upload;
#ifndef DEF_MMB_VER_2    
	HeaderFrame frame_request;
#endif	
public:
    TCPClient() {
        sock = -1;
        port = 0;
        address = "";
		mmb_ack = MMB_ERR;
		frame_size = 0;
#ifdef DEF_MMB_VER_2
        retry_cnt = 0;
#endif        
    }
    ~TCPClient() {
        exit();
    }

    // server: IP address or hostname, port: port number
    bool setup(string address, int port) {
    	if(sock != -1)
				exit();
			
        // Create socket
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == -1) {
            cerr << "Could not create socket" << endl;
            return false;
        }

        // setup server address structure
        if((signed)inet_addr(address.c_str()) == -1) {
	    		struct hostent *he;
	    		struct in_addr **addr_list;
	    		if ( (he = gethostbyname( address.c_str() ) ) == NULL)
	    		{
			      herror("gethostbyname");
	      		   cout<<"Failed to resolve hostname\n";
			      return false;
	    		}
		   	addr_list = (struct in_addr **) he->h_addr_list;
    		for(int i = 0; addr_list[i] != NULL; i++)
    		{
      		      server.sin_addr = *addr_list[i];
		      break;
    		}
	  	}
	  	else {
        	server.sin_addr.s_addr = inet_addr(address.c_str());
	  	}
			
        server.sin_family = AF_INET;
        server.sin_port = htons(port);

        // Connect to server
        if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
            cerr << "Could not connect to server.." << endl;
            return false;
        }

        return true;
    }

    // send data to the connected server
    bool sendData(string data) {
        if (sock == -1) {
            cerr << "Could not send data: No connection to server" << endl;
            return false;
        }

        if (send(sock, data.c_str(), strlen(data.c_str()), 0) < 0) {
            cerr << "Could not send data.." << endl;
            return false;
        }

        return true;
    }

    // send data to the connected server
    bool sendData(unsigned int dataLength, unsigned char* data) {
        if (sock == -1) {
            cerr << "Could not send data: No connection to server" << endl;
            return false;
        }

        if (send(sock, data, dataLength, 0) < 0) {
            cerr << "Could not send data" << endl;
            return false;
        }

        return true;
    }

    //poling 1 min
    bool sendPolingData(unsigned char idLength = 0, unsigned char* id = NULL) {
        if(idLength) {
            frame_polling.idLength = idLength;
            memcpy((void *)frame_polling.id, id, idLength);
        }
       
        return sendData(3 + frame_polling.idLength, (unsigned char *)&frame_polling);
    }

    bool sendFrameData(unsigned int dataLength) {
        int frameLength = 3 + frame_polling.idLength;
        int totalLength = frameLength + sizeof(dataLength);
        unsigned char* buffer = new unsigned char[totalLength];
        int offset = 0;
       
        memcpy(&buffer[offset], (void *)&frame_polling, frameLength);
        offset += frameLength;

        memcpy(&buffer[offset], (void *)&dataLength, sizeof(dataLength));
        offset += sizeof(dataLength);

        //Send the buffer to the server
        return sendData(totalLength, buffer);
    }

    bool sendEventData(EventData *pEvent, unsigned int image1Length = 0, unsigned char* image1Data = NULL, unsigned int image2Length = 0, unsigned char * image2Data = NULL ) {
        int totalLength = sizeof(CommonHeader) + sizeof(EventData);
        unsigned char* buffer = new unsigned char[totalLength];
        int cameras = 0;
        int offset = 0;
       
        if(pEvent == NULL)
            pEvent = &event;

        memcpy(buffer, (void *)&header, sizeof(header));
        offset += sizeof(header);

        memcpy(buffer + offset, (void *)pEvent, sizeof(event));
        offset += sizeof(event);

        if(image1Length)
            cameras++;
        if(image2Length)
            cameras++;

        memcpy(buffer + offset, (void *)&cameras, sizeof(cameras));
        offset += sizeof(cameras);

        if (sendFrameData(totalLength + image1Length + image2Length) == false) {
            cerr << "Could not send frame_polling data" << endl;
            return false;
        }

        if (send(sock, buffer, offset, 0) < 0) {
            cerr << "Could not send event data" << endl;
            return false;
        }

        if(image1Length) {
            if (send(sock, image1Data, image1Length, 0) < 0) {
                cerr << "Could not send image 1 data" << endl;
                return false;
            }
        }

        if(image2Length) {
            if (send(sock, image2Data, image2Length, 0) < 0) {
                cerr << "Could not send image 2 data" << endl;
                return false;
            }
        }

        return true;
    }

	
    int receive(unsigned char *buffer, int size, int timeout = 0) {
        int length = 0;

        if (sock == -1) {
            cerr << "Could not receive data: No connection to server" << endl;
            return false;
        }

		if(timeout) {
#if 0
			 fd_set set;
       	 struct timeval tv;
	        // set up the file descriptor set
	        FD_ZERO(&set);
	        FD_SET(sock, &set);

	        // set up the timeout value
	        tv.tv_sec = timeout;
	        tv.tv_usec = 0;

	        // wait for data to be received or timeout
	        int rv = select(sock, &set, NULL, NULL, (timeout > 0) ? &tv : NULL);
	        if (rv == -1) {
	            cerr << "Error in select()" << endl;
	            return false;
	        } else if (rv == 0) {
	            cerr << "Timeout occurred." << endl;
	            return false;
	        }
#else
		if(0 != setSockTimeout(timeout))
			cerr << "Error setSockTimeout()" << endl;			
#endif

		}

        length = recv(sock, buffer, size, 0);
        if (length <= 0) {
            cerr << "Could not receive data" << endl;
            return 0;
        }

        buffer[length] = '\0';
        return length;
    }

	int mmb_recv(unsigned char *buffer, int size, int timeout_sec)
	{
		int len;
		unsigned int start_tick = 0;
		unsigned int timeout = timeout_sec * 1000;

		 if (sock == -1) {
            cerr << "Could not receive data: No connection to server" << endl;
            return false;
        }
		 

		start_tick = (int)(systemTimeUs() / 1000);
			
		do{
			len = receive(buffer, size, timeout);
			if(len) {
				if(analyzeUploadAckData(buffer, len )) {
				}
			}
			
			if(len)
				printf("%s(): 0x%x 0x%x 0x%x\n",__func__, buffer[0], buffer[1], buffer[2]);
			
			if((systemTimeUs() / 1000) >= timeout + start_tick)
				break;

			usleep(100 * 1000);
		}while(len <= 0);

		return len;
	}	

	int setSockTimeout(int milliseconds)
	{
		int r0 = 0, r1, r2;

#ifdef _WIN32
		/* Windows specific */
		DWORD tv = (DWORD)milliseconds;
#else
		/* Linux, ... (not Windows) */

		struct timeval tv;

	/* TCP_USER_TIMEOUT/RFC5482 (http://tools.ietf.org/html/rfc5482):
	 * max. time waiting for the acknowledged of TCP data before the connection
	 * will be forcefully closed and ETIMEDOUT is returned to the application.
	 * If this option is not set, the default timeout of 20-30 minutes is used.
	*/
	/* #define TCP_USER_TIMEOUT (18) */

#if defined(TCP_USER_TIMEOUT)
		unsigned int uto = (unsigned int)milliseconds;
		r0 = setsockopt(sock, 6, TCP_USER_TIMEOUT, (const void *)&uto, sizeof(uto));
#endif

		memset(&tv, 0, sizeof(tv));
		tv.tv_sec = milliseconds / 1000;
		tv.tv_usec = (milliseconds * 1000) % 1000000;

#endif /* _WIN32 */

		r1 = setsockopt(
		    sock, SOL_SOCKET, SO_RCVTIMEO, (SOCK_OPT_TYPE)&tv, sizeof(tv));
		r2 = setsockopt(
		    sock, SOL_SOCKET, SO_SNDTIMEO, (SOCK_OPT_TYPE)&tv, sizeof(tv));

		return r0 || r1 || r2;
	}

	void exit() {
		if(sock != -1);
		  	close(sock);

		sock = -1;
	}

	short convert_short(short in)
	{
	  short out;
	  char *p_in = (char *) &in;
	  char *p_out = (char *) &out;
	  p_out[0] = p_in[1];
	  p_out[1] = p_in[0];  
	  return out;
	}

	long convert(long in)
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

	long convert(int in)
	{
	  int out;
	  char *p_in = (char *) &in;
	  char *p_out = (char *) &out;
	  p_out[0] = p_in[3];
	  p_out[1] = p_in[2];
	  p_out[2] = p_in[1];
	  p_out[3] = p_in[0];  
	  return out;
	}
	
	 time_t stringToTime(const char* time)
	{
		time_t t = 0;
		struct tm tm_t;
		int nYear, nMonth, nDay, nHour, nMinute, nSecond, nMs;

		if(sscanf(time, "%04d%02d%02d%02d%02d%02d%03d", &nYear, &nMonth, &nDay, &nHour, &nMinute, &nSecond, &nMs)==7)
		{
			tm_t.tm_year = 70+((nYear+30)%100);	// 2019 - 1900
			tm_t.tm_mon = nMonth-1;		// Month, 0 - jan
			tm_t.tm_mday = nDay;				// Day of the month
			tm_t.tm_hour = nHour;
			tm_t.tm_min = nMinute;
			tm_t.tm_sec = nSecond;

			tm_t.tm_isdst = -1; // Is DST on? 1 = yes, 0 = no, -1 = unknown

			t = mktime(&tm_t);
		}
		
		return t;
	}
		 
    int analyzeUploadAckData(unsigned char* data, int dataSize) {
		 mmb_ack = MMB_ERR;
		 
        if (data[0] != MMB_SERVER_HEADER0) {
            cerr << "Invalid data: Incorrect value at address 0" << endl;
            return -1;
        }

        if (data[1] != MMB_SERVER_HEADER1) {
            cerr << "Invalid data: Incorrect value at address 1" << endl;
            return -1;
        }

        if (data[2] == MMB_SERVER_ACK) {
            cout << "Data received: Normal" << endl;
			  mmb_ack = MMB_ACK;
            return mmb_ack;
        } else if (data[2] == MMB_SERVER_NAK) {
            cout << "Data received: Error" << endl;
			  mmb_ack = MMB_NAK;
            return mmb_ack;
        } else {
            cerr << "Invalid data: Incorrect value at address 2" << endl;
            return -1;
        }

		return mmb_ack;
    }
#ifdef DEF_MMB_VER_2
	int analyzeRequestFrameData(unsigned char* data, int dataSize) {
        mmb_ack = MMB_ERR;

        if(data[0] != MMB_SERVER_REQUEST_FRAME_H0) {
            cerr << "Invalid request data: Incorrect value at address 0" << endl;
            return -1;
        }

        if(data[1] == MMB_SERVER_REQUEST_FRAME_FILELIST) {
            mmb_ack = MMB_REQUEST_FILELIST;
        } else if(data[1] == MMB_SERVER_REQUEST_FRAME_FILE) {
            mmb_ack = MMB_REQUEST_FILE;
        }
        return mmb_ack;
    }
#endif
	int analyzePollingAckData(unsigned char* data, int dataSize) {
		unsigned int *pDataLength = 0;
		mmb_ack = MMB_ERR;
		 
        if (data[0] != MMB_SERVER_REQUEST_H0) {
            cerr << "Invalid request data: Incorrect value at address 0" << endl;
            return -1;
        }

#ifdef DEF_MMB_VER_2
        if (data[1] != MMB_SERVER_REQUEST_H1) {
            cerr << "Invalid request data: Incorrect value at address 1" << endl;
            return -1;
        }

        if (data[2] == MMB_SERVER_ACK) {
            mmb_ack = MMB_ACK;
        } else if (data[2] == MMB_SERVER_NAK) {
            mmb_ack = MMB_NAK;
        }
#else

        if (data[1] == MMB_SERVER_REQUEST_H1_EVENT) {
				mmb_ack = MMB_REQUEST_FILE;
        }
		else if(data[1] == MMB_SERVER_REQUEST_H1_NOTHING) {
		 		mmb_ack = MMB_NOTHING;
		}      
		else {
            cerr << "Invalid request data: Incorrect value at address 1" << endl;
            return -1;
        }

		pDataLength = (unsigned int*)&data[2];

		if(*pDataLength < 128)
			data[6 + *pDataLength] = 0;
		else
			data[6 + 128] = 0;
		
		printf("Data received : %s, %d (%s) \n", mmb_ack == MMB_REQUEST_FILE ? "Request Event File" : "Nothing", \
			*pDataLength, *pDataLength == 0 ? "" : (const char *)&data[6]);

		if(mmb_ack == MMB_REQUEST_FILE){
			if(*pDataLength == 0){
				cout << "Data length : Error" << endl;
			}
			else {
				cout << "Rquest Event : " << &data[6] << endl;
				request_t = stringToTime((const char *)&data[6]);
			}
		}
#endif
		return mmb_ack;
    }


    void setCommonHeader(char deviceType, const char* majorVersion, const char* minorVersion,
                                            const char* additionalInfo, char* deviceId) {
		memset((void *)&header, 0, sizeof(header));
		
		header.deviceType = deviceType;
		strncpy(header.majorVersion, majorVersion, sizeof(header.majorVersion));
        strncpy(header.minorVersion, minorVersion, sizeof(header.minorVersion));
        strncpy(header.additionalInfo, additionalInfo, sizeof(header.additionalInfo));
#ifdef DEF_MMB_VER_2
        time_t     t_rtc;
        struct tm tmThis;
        char accOnTime[15] =  { 0,};

        time(&t_rtc);
        t_rtc -= (get_tick_count() / 1000); //get acc on time
        localtime_r(&t_rtc, &tmThis);
        sprintf(accOnTime, "%04d%02d%02d%02d%02d%02d", \
        tmThis.tm_year + 1900, tmThis.tm_mon + 1, tmThis.tm_mday, \
        tmThis.tm_hour, tmThis.tm_min, tmThis.tm_sec);
        strncpy(header.accOnTime, accOnTime, sizeof(header.accOnTime));
        printf("%s Set AccOnTime = %s\n", __func__, accOnTime);
#endif
        strncpy(header.deviceId, deviceId, sizeof(header.deviceId));
    }

    //set the values of the EventData
    // Longitude, Latitude uint : 0.0001
#ifdef DEF_MMB_VER_2
    void setEventData(char* eventTime, unsigned char eventType, char gnssStatus, int eventLongitude, int eventLatitude, bool bicEndian) {
		memset((void *)&event, 0, sizeof(event));	
#else
    void setEventData(char* accOnTime, char* eventTime, unsigned char eventType, char gnssStatus, int eventLongitude, int eventLatitude, bool bicEndian) {
		memset((void *)&event, 0, sizeof(event));	
		strncpy(event.accOnTime, accOnTime, sizeof(event.accOnTime));
#endif
        strncpy(event.eventTime, eventTime, sizeof(event.eventTime));
        event.eventType = eventType;
        event.gnssStatus = gnssStatus;

		if(bicEndian){
			event.eventLongitude = convert(eventLongitude);
	       	event.eventLatitude = convert(eventLatitude);
		}
		else {
	       event.eventLongitude = eventLongitude;
	       event.eventLatitude = eventLatitude;
		}
    }

    // set th values of the HeaderFrame

    void setHeaderFramePolling(unsigned char idLength, unsigned char* deviceId) {
        frame_polling.header[0] = MMB_CLIENT_POLLING_H0;
        frame_polling.header[1] = MMB_CLIENT_POLLING_H1;

        frame_polling.idLength = idLength;
		 memset((void *)frame_polling.id, 0, sizeof(frame_polling.id));
        strncpy((char *)frame_polling.id, (char *)deviceId, sizeof(frame_polling.id));

		frame_size = 3 + idLength;
    }
		
    void setHeaderFrameUpload(unsigned char idLength, unsigned char* deviceId) {
        frame_upload.header[0] = MMB_CLIENT_HEADER0;
        frame_upload.header[1] = MMB_CLIENT_HEADER1;

        frame_upload.idLength = idLength;
		 memset((void *)frame_upload.id, 0, sizeof(frame_upload.id));
        strncpy((char *)frame_upload.id, (char *)deviceId, sizeof(frame_upload.id));

		frame_size = 3 + idLength;
    }

    void updateHeaderFrameUpload(eUpLoadDataType uploadType) {

        if(uploadType == UPLOAD_TYPE_FILELIST) {
            frame_upload.header[1] = MMB_CLIENT_LIST_HEADER1;
        }else if(uploadType == UPLOAD_TYPE_FILE) {
            frame_upload.header[1] = MMB_CLIENT_FILE_HEADER1;
        }else {
            frame_upload.header[1] = MMB_CLIENT_HEADER1;
        }
    }
#ifndef DEF_MMB_VER_2
	void setHeaderFrameRequest(unsigned char idLength, unsigned char* deviceId) {
		frame_request.header[0] = MMB_SERVER_REQUEST_H0;
		frame_request.header[1] = MMB_SERVER_REQUEST_H1_EVENT;

		frame_request.idLength = idLength;
		memset((void *)frame_request.id, 0, sizeof(frame_request.id));
		strncpy((char *)frame_request.id, (char *)deviceId, sizeof(frame_request.id));

		frame_size = 3 + idLength;
  }
#endif  
};


#endif
