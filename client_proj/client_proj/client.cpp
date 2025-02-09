#pragma once
#pragma comment (lib, "Ws2_32.lib")
#define _CRT_SECURE_NO_WARNINGS 1 
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1

#include <winsock2.h>
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <windows.h>
#include <fstream>
#include <sstream>
#include <chrono>
#include <format>
#include <tchar.h>
#include <atlbase.h>
#include <atlconv.h>
#include <Poco/FileStream.h>
#include <Poco/Zip/Decompress.h>
#include <Poco/Zip/Compress.h>
#include <Poco/Zip/ZipException.h>
#include <Poco/Timestamp.h>
#include <Poco/File.h>
#include <Poco/Delegate.h>
#include <Poco/Zip/ZipException.h>
#include <filesystem>





#define HOSTNAME_LENGTH 200
#define FILENAME_LENGTH 200
#define RESP_LENGTH 40
#define SUBJECT_LENGTH 200
#define BODY_LENGTH 200
#define REQUEST_PORT 5001
#define BUFFER_LENGTH 1024 
#define TRACE 0
#define MSGHDRSIZE 8

typedef enum {
	yes = 1, no, REC,//Message type
} Type;


typedef struct
{
	char hostname[HOSTNAME_LENGTH];
	char To[HOSTNAME_LENGTH];
	char From[HOSTNAME_LENGTH];
	char Subject[SUBJECT_LENGTH];
	char Body[SUBJECT_LENGTH];

} Req;  //request

typedef struct
{
	char hostname[HOSTNAME_LENGTH];
	char myEmail[HOSTNAME_LENGTH];
} Rec;  //client email

typedef struct
{
	char fname[80];

} Name;  //request

typedef struct
{
	char response[RESP_LENGTH];
	char time[RESP_LENGTH];//added
} Resp; //response

typedef struct
{

	Type type;
	int  length; //length of effective bytes in the buffer
	char buffer[BUFFER_LENGTH];
} Msg; //message format used for sending and receiving


class TcpClient
{
	int sock;                    /* Socket descriptor */
	struct sockaddr_in ServAddr; /* server socket address */
	unsigned short ServPort;     /* server port */
	Rec* recp;
	Name* name;
	Req* reqp;               /* pointer to request */
	Resp* respp;          /* pointer to response*/
	Msg smsg, rmsg;               /* receive_message and send_message */
	WSADATA wsadata;
public:
	TcpClient() {}
	void run(int argc, char* argv[]);
	~TcpClient();
	int msg_recv(int, Msg*);
	int msg_send(int, Msg*);
	int64_t GetFileSize(const std::string& fileName);
	int SendBuffer(SOCKET s, const char* buffer, int bufferSize, int chunkSize);
	unsigned long ResolveName(char name[]);
	void err_sys(const char* fmt, ...);
	int64_t SendFile(SOCKET s, const std::string& fileName, int chunkSize);

	int RecvBuffer(SOCKET s, char* buffer, int bufferSize, int chunkSize);

	bool CopyFile1(const char* srcFileName, const char* destFileName);

	int64_t RecvFile(SOCKET s, const std::string& fileName, int chunkSize);

	void onDecompressError(const void* pSender, std::pair<const Poco::Zip::ZipLocalFileHeader, const std::string>& info);



};

using namespace std;
void TcpClient::run(int argc, char* argv[])
{
	int action;
	cout << "Enter 1, 2 or 3 to construct email, update local inbox or forward an email: ";
	cin >> action;
	cout << endl; // choosing what to do

	if (action == 1 || action == 3)
	{
		struct _stat stat_buf;
		int result = 0;
		//	if (argc != 4)
			//err_sys("usage: client servername filename size/time");
		char* inputTo = new char[HOSTNAME_LENGTH];
		char* decoy = new char[HOSTNAME_LENGTH];
		char* inputFrom = new char[HOSTNAME_LENGTH];
		char* inputSubject = new char[SUBJECT_LENGTH];
		char* inputBody = new char[SUBJECT_LENGTH];
		char* inputserverhostname = new char[HOSTNAME_LENGTH];// needed
		char filename[80] = { 0 };
		char forwardfilename[80] = { 0 };
		std::string literal;
		std::string base_filename;
		char* attachment = new char[3];// yes or no??
		int choice = 0;

		std::string jtp;
		std::stringstream strStream;
		reqp = (Req*)smsg.buffer;
		WSADATA w;
		int ret = WSAStartup(MAKEWORD(2, 2), &w);
		if (gethostname(reqp->hostname, HOSTNAME_LENGTH) != 0) //get the hostname
			err_sys("can not get the host name,program exit");
		printf("Sender starting at host: %s\n", reqp->hostname);

		if (action == 1)
		{
			//Creating new emailsdf
			cout << "Creating New Email.";
			cout << endl;

			cout << "Enter Destination address: ";
			cin >> inputTo;
			cout << endl;

			cout << "Enter Source address: ";
			cin >> inputFrom;
			cout << endl;


			cout << "Enter Subject: ";
			cin.ignore();
			cin.getline(inputSubject, SUBJECT_LENGTH - 1);
			cout << endl;

			cout << "Enter Body: ";
			//cin.ignore();
			cin.getline(inputBody, SUBJECT_LENGTH - 1);
			cout << endl;

			while (1)// checking for valid email addresses - TO and FROM
			{
				if (strstr(inputFrom, "@") != NULL && strstr(inputTo, "@") != NULL)
				{
					break;
				}
				else
				{
					cout << "Please provide valid email addresses" << endl;
					cout << "Enter Destination address: ";
					cin >> inputTo;
					cout << endl;
					cout << "Enter Source address: ";
					cin >> inputFrom;
					cout << endl;
				}
			}
			cout << "Add an attachment - yes/no: ";///just for checking instead of header verification in server
			cin >> attachment;
			cout << endl;


			if (strcmp(attachment, "yes") == 0)
			{

				smsg.type = yes;
				cout << "Enter file name: ";///just for checking instead of header verification in server
				cin.ignore();
				getline(cin, literal);
				//cin.getline(filename, 79);
				//printf("%s", literal.c_str());
				cout << endl;
				choice = 1;

			}
			else if (strcmp(attachment, "no") == 0)
				smsg.type = no;
			else err_sys("Wrong request type\n");
		}
		else
		{
			cout << "Enter Destination address you want to forward to: ";
			cin >> inputTo;
			cout << endl;

			cout << "Enter Source address: ";
			cin >> inputFrom;
			cout << endl;

			cout << "Please enter the name of the email you want to forward: ";
			cin.ignore();
			cin.getline(forwardfilename, 119);
			cout << endl;

			fstream file;
			file.open(forwardfilename, ios::in);
			file.getline(decoy, SUBJECT_LENGTH - 1);
			file.getline(decoy, SUBJECT_LENGTH - 1);
			file.getline(inputSubject, SUBJECT_LENGTH - 1);
			file.getline(inputBody, SUBJECT_LENGTH - 1);
			//file.getline(literal, SUBJECT_LENGTH - 1);
			getline(file, literal);
			file.getline(decoy, SUBJECT_LENGTH - 1);
			file.close();

			if (strcmp(literal.c_str(), " ") != 0 && strcmp(decoy, " ") != 0)
			{
				smsg.type = yes;
			}
			else
			{
				smsg.type = no;
			}

		}


		//Display name of local host and copy it to the req
		strcpy(reqp->To, inputTo);
		strcpy(reqp->From, inputFrom);
		strcpy(reqp->Subject, inputSubject);
		strcpy(reqp->Body, inputBody);

		//type in server name
		cout << "Type name of Mail Server: ";
		cin >> inputserverhostname;
		cout << endl;

		//initilize winsocket
		if (WSAStartup(0x0202, &wsadata) != 0)
		{
			WSACleanup();
			err_sys("Error in starting WSAStartup()\n");
		}

		//Create the socket
		if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) //create the socket 
			err_sys("Socket Creating Error");
		//printf("\nsocket of sender = %d\n", sock);

		//connect to the server
		ServPort = REQUEST_PORT;
		memset(&ServAddr, 0, sizeof(ServAddr));     /* Zero out structure */
		ServAddr.sin_family = AF_INET;             /* Internet address family */
		ServAddr.sin_addr.s_addr = ResolveName(inputserverhostname);   /* Server IP address */
		ServAddr.sin_port = htons(ServPort); /* Server port */
		if (connect(sock, (struct sockaddr*)&ServAddr, sizeof(ServAddr)) < 0)
			err_sys("Socket Creating Error");
		printf("Connected to Mail Server: %s\n", inputserverhostname);

		//send out the message
		smsg.length = sizeof(Req);

		//fprintf(stdout, "Send request to %s\n", inputserverhostname);
		if (msg_send(sock, &smsg) != sizeof(Req))
			err_sys("Sending req packet error.,exit");

		if ((result = _stat(literal.c_str(), &stat_buf)) == 0 && smsg.type == yes) {

			name = (Name*)smsg.buffer;
			base_filename = literal.substr(literal.find_last_of("/\\") + 1);
			//printf("\n%s", base_filename.c_str());
			strcpy(name->fname, base_filename.c_str());
			if (msg_send(sock, &smsg) != sizeof(Req))
				err_sys("Sending req packet error.,exit");
			SendFile(sock, literal.c_str(), 64 * 1024);
			//SendFile(sock, filename, 64 * 1024);
		}

		fprintf(stdout, "Mail Sent to Server, waiting...\n");
		time_t my_time = time(NULL);

		time_t t = time(0);   // get time now
		struct tm* now = localtime(&t);



		char buffer[80];
		strftime(buffer, 80, "%Y-%m-%d", now);;


		strStream << inputSubject << endl;
		strStream >> jtp;


		jtp = "sent/" + jtp + "_" + buffer + ".txt";

		std::ofstream fout(jtp);
		fout << reqp->From << std::endl;
		fout << reqp->To << std::endl;
		fout << reqp->Subject << std::endl;
		fout << reqp->Body << std::endl;

		strStream << base_filename.c_str() << endl;
		strStream >> jtp;
		jtp = "sent/" + jtp;
		if (smsg.type == yes)
		{
			fout << jtp << std::endl;
		}
		fout << ctime(&t) << std::endl;
		fout.close();
		CopyFile1(literal.c_str(), jtp.c_str()); //////////////// left at : copying file and take about forwarding and attaching


		//clearing buffer

		//receive the response
		if (msg_recv(sock, &rmsg) != rmsg.length)
			err_sys("recv response error,exit");

		//cast it to the response structure
		respp = (Resp*)rmsg.buffer;
		if (strcmp(respp->response, "250 OK") == 0)
			printf("%s %s", respp->time, respp->response);
		else
			printf("%s %s", respp->time, respp->response);
	}
	else if (action == 2)
	{
		struct _stat stat_buf;
		int result = 0;
		//	if (argc != 4)
			//err_sys("usage: client servername filename size/time");
		char* inputEmail = new char[HOSTNAME_LENGTH];
		char* inputserverhostname = new char[HOSTNAME_LENGTH];// neede
		char filename[80] = { 0 };
		//memset(recp, 0, sizeof(recp));
		recp = (Rec*)smsg.buffer; //recp receiver pointer
		WSADATA w;
		int ret = WSAStartup(MAKEWORD(2, 2), &w);
		if (gethostname(recp->hostname, HOSTNAME_LENGTH) != 0) //get the hostname
			err_sys("can not get the host name,program exit");
		printf("Receiver starting at host: %s\n", recp->hostname);

		cout << "Please enter your local email: ";
		cin >> inputEmail;
		cout << endl;

		strcpy(recp->myEmail, inputEmail);//local email
		cout << "Type name of Mail Server: ";
		cin >> inputserverhostname;
		cout << endl;

		//initilize winsocket
		if (WSAStartup(0x0202, &wsadata) != 0)
		{
			WSACleanup();
			err_sys("Error in starting WSAStartup()\n");
		}

		//Create the socket
		if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) //create the socket 
			err_sys("Socket Creating Error");
		//printf("\nsocket of sender = %d\n", sock);

		//connect to the server
		ServPort = REQUEST_PORT;
		memset(&ServAddr, 0, sizeof(ServAddr));     /* Zero out structure */
		ServAddr.sin_family = AF_INET;             /* Internet address family */
		ServAddr.sin_addr.s_addr = ResolveName(inputserverhostname);   /* Server IP address */
		ServAddr.sin_port = htons(ServPort); /* Server port */
		if (connect(sock, (struct sockaddr*)&ServAddr, sizeof(ServAddr)) < 0)
			err_sys("Socket Creating Error");
		printf("Connected to Mail Server: %s\n", inputserverhostname);

		//send out the message
		smsg.length = sizeof(Rec);
		smsg.type = REC;
		fprintf(stdout, "Send request to update local inbox to %s\n", inputserverhostname);
		if (msg_send(sock, &smsg) != sizeof(Rec))
			err_sys("Sending req packet error.,exit");

		RecvFile(sock, "inbox.zip", 4 * 1024);
		printf("The local inbox has been updated\n");
		//CopyFile1("linux.pdf", "inbox/linux.pdf");
		std::ifstream inp("inbox.zip", std::ios::binary);
		poco_assert(inp);
		// decompress to current working dir
		Poco::Zip::Decompress dec(inp, Poco::Path("inbox"));
		// if an error happens invoke the ZipTest::onDecompressError method
		//dec.EError += Poco::Delegate<ZipTest, std::pair<const Poco::Zip::ZipLocalFileHeader, const std::string> >(this, &ZipTest::onDecompressError);
		dec.decompressAllFiles();
		//dec.EError -= Poco::Delegate<ZipTest, std::pair<const Poco::Zip::ZipLocalFileHeader, const std::string> >(this, &ZipTest::onDecompressError);
		inp.close();
		int status = remove("inbox.zip");
		if (status == 0)
		{
			//cout << "\nFile Deleted Successfully!";
		}
		else
			cout << "\nError Occurred!";



	}
	//close the client socket
	closesocket(sock);

}
TcpClient::~TcpClient()
{
	/* When done uninstall winsock.dll (WSACleanup()) and exit */
	WSACleanup();
}


void TcpClient::err_sys(const char* fmt, ...) //from Richard Stevens's source code
{
	perror(NULL);
	va_list args;
	va_start(args, fmt);
	fprintf(stderr, "error: ");
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
	va_end(args);
	exit(1);
}

unsigned long TcpClient::ResolveName(char name[])
{
	struct hostent* host;            /* Structure containing host information */

	if ((host = gethostbyname(name)) == NULL)
		err_sys("gethostbyname() failed");

	/* Return the binary, network byte ordered address */
	return *((unsigned long*)host->h_addr_list[0]);
}

/*
msg_recv returns the length of bytes in the msg_ptr->buffer,which have been recevied successfully.
*/
int TcpClient::msg_recv(int sock, Msg* msg_ptr)
{
	int rbytes, n;

	for (rbytes = 0; rbytes < MSGHDRSIZE; rbytes += n)
		if ((n = recv(sock, (char*)msg_ptr + rbytes, MSGHDRSIZE - rbytes, 0)) <= 0)
			err_sys("Recv MSGHDR Error");

	for (rbytes = 0; rbytes < msg_ptr->length; rbytes += n)
		if ((n = recv(sock, (char*)msg_ptr->buffer + rbytes, msg_ptr->length - rbytes, 0)) <= 0)
			err_sys("Recevier Buffer Error");

	return msg_ptr->length;
}

/* msg_send returns the length of bytes in msg_ptr->buffer,which have been sent out successfully
 */
int TcpClient::msg_send(int sock, Msg* msg_ptr)
{
	int n;
	if ((n = send(sock, (char*)msg_ptr, MSGHDRSIZE + msg_ptr->length, 0)) != (MSGHDRSIZE + msg_ptr->length))
		err_sys("Send MSGHDRSIZE+length Error");
	return (n - MSGHDRSIZE);

}
int64_t TcpClient::GetFileSize(const std::string& fileName) {

	FILE* f;
	if (fopen_s(&f, fileName.c_str(), "rb") != 0) {
		return -1;
	}
	_fseeki64(f, 0, SEEK_END);
	const int64_t len = _ftelli64(f);
	fclose(f);
	return len;
}

int TcpClient::SendBuffer(SOCKET s, const char* buffer, int bufferSize, int chunkSize = 4 * 1024) {

	int i = 0;
	while (i < bufferSize) {
		const int l = send(s, &buffer[i], __min(chunkSize, bufferSize - i), 0);
		if (l < 0) { return l; } // this is an error
		i += l;
	}
	return i;
}

// Sends a file
// returns size of file if success
// returns -1 if file couldn't be opened for input
// returns -2 if couldn't send file length properly
// returns -3 if file couldn't be sent properly

int64_t TcpClient::SendFile(SOCKET s, const std::string& fileName, int chunkSize = 64 * 1024) {

	const int64_t fileSize = GetFileSize(fileName);
	if (fileSize < 0) { return -1; }

	std::ifstream file(fileName, std::ifstream::binary);
	if (file.fail()) { return -1; }

	if (SendBuffer(s, reinterpret_cast<const char*>(&fileSize),
		sizeof(fileSize)) != sizeof(fileSize)) {
		return -2;
	}

	char* buffer = new char[chunkSize];
	bool errored = false;
	int64_t i = fileSize;
	while (i != 0) {
		const int64_t ssize = __min(i, (int64_t)chunkSize);
		if (!file.read(buffer, ssize)) { errored = true; break; }
		const int l = SendBuffer(s, buffer, (int)ssize);
		if (l < 0) { errored = true; break; }
		i -= l;
	}
	delete[] buffer;

	file.close();

	return errored ? -3 : fileSize;
}

int TcpClient::RecvBuffer(SOCKET s, char* buffer, int bufferSize, int chunkSize = 4 * 1024) {
	int i = 0;
	while (i < bufferSize) {
		const int l = recv(s, &buffer[i], __min(chunkSize, bufferSize - i), 0);
		if (l < 0) { return l; } // this is an error
		i += l;
	}
	return i;
}
bool TcpClient::CopyFile1(const char* srcFileName, const char* destFileName)
{
	std::ifstream ifs(srcFileName, std::ios::binary | std::ios::in);
	std::ofstream ofs(destFileName, std::ios::binary | std::ios::out);
	if (ifs.is_open() == false || ofs.is_open() == false)
		return false;
	// Opening istream in binary mode and writing in text mode will
	// increase the file size as \r\n are treated two characters in
	// binary mode where as one character in text mode
	const int len = 4096;
	char buf[4096];
	while (1)

	{
		if (ifs.eof())
			break;
		ifs.read(buf, len);
		int nBytesRead = ifs.gcount();
		if (nBytesRead <= 0)

			break;
		ofs.write(buf, nBytesRead);
	}
	ifs.close();
	ofs.close();
	return true;
}
int64_t TcpClient::RecvFile(SOCKET s, const std::string& fileName, int chunkSize = 64 * 1024) {
	std::ofstream file(fileName, std::ofstream::binary);
	if (file.fail()) { return -1; }

	int64_t fileSize;
	if (RecvBuffer(s, reinterpret_cast<char*>(&fileSize),
		sizeof(fileSize)) != sizeof(fileSize)) {
		return -2;
	}

	char* buffer = new char[chunkSize];
	bool errored = false;
	int64_t i = fileSize;
	while (i != 0) {
		const int r = RecvBuffer(s, buffer, (int)__min(i, (int64_t)chunkSize));
		if ((r < 0) || !file.write(buffer, r)) { errored = true; break; }
		i -= r;
	}
	delete[] buffer;

	file.close();

	return errored ? -3 : fileSize;
}

//void ZipTest::onDecompressError(const void* pSender, std::pair<const Poco::Zip::ZipLocalFileHeader, const std::string>& info)
//{
//	printf("\nError in compressing\n");
//}


int main(int argc, char* argv[]) //argv[1]=servername argv[2]=filename argv[3]=time/size
{
	TcpClient* tc = new TcpClient();
	tc->run(argc, argv);
	return 0;
}
