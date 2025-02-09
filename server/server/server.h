#ifndef SER_TCP_H
#define SER_TCP_H

#define HOSTNAME_LENGTH 200
#define RESP_LENGTH 40
#define SUBJECT_LENGTH 200
#define BODY_LENGTH 200
#define REQUEST_PORT 5001
#define BUFFER_LENGTH 1024
#define MAXPENDING 10
#define MSGHDRSIZE 8	//Message Header Size



typedef enum {
	yes = 1, no, REC //Message type
} Type;


typedef struct
{
	char hostname[HOSTNAME_LENGTH];
	char To[HOSTNAME_LENGTH];
	char From[HOSTNAME_LENGTH];
	char Subject[SUBJECT_LENGTH];
	char Body[BODY_LENGTH];


} Req;  //request

typedef struct
{
	char hostname[HOSTNAME_LENGTH];
	char myEmail[HOSTNAME_LENGTH];


} Rec;

typedef struct
{
	char response[RESP_LENGTH];
	char time[RESP_LENGTH];
	char x[RESP_LENGTH];
} Resp; //response

typedef struct
{
	char fname[80];

} Name;  //file name

typedef struct
{

	Type type;
	int  length; //length of effective bytes in the buffer
	char buffer[BUFFER_LENGTH];
} Msg; //message format used for sending and receiving


class TcpServer
{
	int serverSock, clientSock;     /* Socket descriptor for server and client*/
	struct sockaddr_in ClientAddr; /* Client address */
	struct sockaddr_in ServerAddr; /* Server address */
	unsigned short ServerPort;     /* Server port */
	int clientLen;				   /* Length of Server address data structure */
	char servername[HOSTNAME_LENGTH];

public:
	TcpServer();
	~TcpServer();
	void start();
};

class TcpThread :public Thread
{

	int cs;
public:
	TcpThread(int clientsocket) :cs(clientsocket)
	{}
	int64_t RecvFile(SOCKET s, const std::string& fileName, int chunkSize);
	virtual void run();
	int64_t GetFileSize(const std::string& fileName);
	int SendBuffer(SOCKET s, const char* buffer, int bufferSize, int chunkSize);
	int64_t SendFile(SOCKET s, const std::string& fileName, int chunkSize);
	int msg_recv(int, Msg*);
	int msg_send(int, Msg*);
	int RecvBuffer(SOCKET s, char* buffer, int bufferSize, int chunkSize);
	std::string compress(std::string z_file, std::string file_path);
	//int recv_client_image(SOCKET sock);
	unsigned long ResolveName(char name[]);
	bool CopyFile1(const char* srcFileName, const char* destFileName);
	static void err_sys(const char* fmt, ...);
};

#endif

