#pragma once
#pragma comment (lib, "Ws2_32.lib")
#define _CRT_SECURE_NO_WARNINGS 1 
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1

#include <winsock2.h>
#include <iostream>
#include <windows.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <process.h>
#include "Thread.h"
#include "server.h"
#include <chrono>
#include <ctime>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iterator>
#include <map>
#include <list>
#include <Poco/FileStream.h>
#include <Poco/Zip/Decompress.h>
#include <Poco/Zip/Compress.h>
#include <Poco/Zip/ZipException.h>
#include <Poco/Timestamp.h>
#include <Poco/File.h>
#include <Poco/Zip/ZipException.h>
#include <filesystem>



using namespace std;
using namespace std;

TcpServer::TcpServer()
{
	WSADATA wsadata;
	if (WSAStartup(0x0202, &wsadata) != 0)
		TcpThread::err_sys("Starting WSAStartup() error\n");

	//Display name of local host
	if (gethostname(servername, HOSTNAME_LENGTH) != 0) //get the hostname
		TcpThread::err_sys("Get the host name error,exit");
	if ((serverSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		TcpThread::err_sys("Create socket error,exit");
	printf("Mail Server starting at host: %s \nWaiting to be contacted for transferring Mail...\n", servername);
	//printf("%d\n", serverSock);


	//Create the server socket


	//Fill-in Server Port and Address info.
	ServerPort = REQUEST_PORT;
	memset(&ServerAddr, 0, sizeof(ServerAddr));      /* Zero out structure */
	ServerAddr.sin_family = AF_INET;                 /* Internet address family */
	ServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);  /* Any incoming interface */
	ServerAddr.sin_port = htons(ServerPort);         /* Local port */

	//Bind the server socket
	if (bind(serverSock, (struct sockaddr*)&ServerAddr, sizeof(ServerAddr)) < 0)
		TcpThread::err_sys("Bind socket error,exit");

	//Successfull bind, now listen for Server requests.
	if (listen(serverSock, MAXPENDING) < 0)
		TcpThread::err_sys("Listen socket error,exit");
}

TcpServer::~TcpServer()
{
	WSACleanup();
}


void TcpServer::start()
{
	for (;;) /* Run forever */
	{
		/* Set the size of the result-value parameter */
		clientLen = sizeof(ClientAddr);

		/* Wait for a Server to connect */
		if ((clientSock = accept(serverSock, (struct sockaddr*)&ClientAddr,
			&clientLen)) < 0)
			TcpThread::err_sys("Accept Failed ,exit");

		/* Create a Thread for this new connection and run*/
		TcpThread* pt = new TcpThread(clientSock);
		//printf("\nReceived a request for Connection/Connected");
		pt->start();
	}
}

//////////////////////////////TcpThread Class //////////////////////////////////////////
void TcpThread::err_sys(const char* fmt, ...)
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
unsigned long TcpThread::ResolveName(char name[])
{
	struct hostent* host; // Structure containing host information* /

	if ((host = gethostbyname(name)) == NULL)
		err_sys("gethostbyname() failed");

	/* Return the binary, network byte ordered address */
	return *((unsigned long*)host->h_addr_list[0]);
}

/*
msg_recv returns the length of bytes in the msg_ptr->buffer,which have been recevied successfully.
*/
int TcpThread::msg_recv(int sock, Msg* msg_ptr)
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
int TcpThread::msg_send(int sock, Msg* msg_ptr)
{
	int n;
	if ((n = send(sock, (char*)msg_ptr, MSGHDRSIZE + msg_ptr->length, 0)) != (MSGHDRSIZE + msg_ptr->length))
		err_sys("Send MSGHDRSIZE+length Error");
	return (n - MSGHDRSIZE);

}
///
/// Recieves data in to buffer until bufferSize value is met
///
/// 
bool TcpThread::CopyFile1(const char* srcFileName, const char* destFileName)

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

int TcpThread::RecvBuffer(SOCKET s, char* buffer, int bufferSize, int chunkSize = 4 * 1024) {
	int i = 0;
	while (i < bufferSize) {
		const int l = recv(s, &buffer[i], __min(chunkSize, bufferSize - i), 0);
		if (l < 0) { return l; } // this is an error
		i += l;
	}
	return i;
}

//
// Receives a file
// returns size of file if success
// returns -1 if file couldn't be opened for output
// returns -2 if couldn't receive file length properly
// returns -3 if couldn't receive file properly
//
int64_t TcpThread::RecvFile(SOCKET s, const std::string& fileName, int chunkSize = 64 * 1024) {
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

int64_t TcpThread::GetFileSize(const std::string& fileName) {
	// no idea how to get filesizes > 2.1 GB in a C++ kind-of way.
	// I will cheat and use Microsoft's C-style file API
	FILE* f;
	if (fopen_s(&f, fileName.c_str(), "rb") != 0) {
		return -1;
	}
	_fseeki64(f, 0, SEEK_END);
	const int64_t len = _ftelli64(f);
	fclose(f);
	return len;
}
int TcpThread::SendBuffer(SOCKET s, const char* buffer, int bufferSize, int chunkSize = 4 * 1024) {

	int i = 0;
	while (i < bufferSize) {
		const int l = send(s, &buffer[i], __min(chunkSize, bufferSize - i), 0);
		if (l < 0) { return l; } // this is an error
		i += l;
	}
	return i;
}
int64_t TcpThread::SendFile(SOCKET s, const std::string& fileName, int chunkSize = 64 * 1024) {

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
std::string TcpThread::compress(std::string z_file, std::string file_path)
{
	std::ofstream fos(z_file, std::ios::binary);
	Poco::Zip::Compress c(fos, true);
	Poco::File aFile(file_path);
	if (aFile.exists()) {
		Poco::Path p(aFile.path());
		if (aFile.isDirectory()) {
			try {
				c.addRecursive(p, Poco::Zip::ZipCommon::CompressionMethod::CM_AUTO,
					Poco::Zip::ZipCommon::CL_MAXIMUM, true);
			}
			catch (const Poco::Exception e) {
				std::cout << e.what() << "/r/n";
			}
		}
	}
	else {
		std::cout << "File not found... \r\n";
	}
	// Just in case these don't get called above.
	c.close(); // MUST be done to finalize the Zip file
	fos.close();
	return z_file;
}

void TcpThread::run() //cs: Server socket
{
	Name* name;
	Resp* respp;//a pointer to response
	Rec* recp;
	Req* reqp; //a pointer to the Request Packet
	//Req* reply;
	Msg smsg, rmsg; //send_message receive_message
	struct _stat stat_buf;
	int result;
	char* decoy = new char[HOSTNAME_LENGTH];
	if (msg_recv(cs, &rmsg) != rmsg.length)
		err_sys("Receive Req error,exit");

	if (rmsg.type != REC) // for the sender
	{

		//cast it to the request packet structure
		reqp = (Req*)rmsg.buffer;
		printf("\nConnected to client:%s\n", reqp->hostname);
		char savename[80] = { 0 };
		strcpy(savename, reqp->hostname);

		// checking if the receiver exists in the server file of recipients
		string email_ID;
		string host_ID;
		string receiver;
		fstream file;
		string filename = "Client_Database.txt";
		file.open(filename.c_str());
		map<string, string> clients;
		ifstream MyReadFile("Client_Database.txt");
		while (file >> email_ID) {
			// Output the text from the file
			file >> host_ID;
			clients.insert(pair<string, string>(email_ID, host_ID));
		}
		file.close();

		//contruct the response and send it out
		//smsg.type = RESP;
		smsg.length = sizeof(Resp);
		respp = (Resp*)smsg.buffer;

		if (strstr(reqp->To, "@") != NULL && strstr(reqp->From, "@") != NULL)
		{
			host_ID = clients.find(email_ID)->second;
			memset(respp->response, 0, sizeof(Resp));
			if (clients.find(reqp->To) == clients.end())
			{
				sprintf(respp->response, "550 error"); // not in the file mapping

			}
			else
			{
				sprintf(respp->response, "250 OK");	  //  verified and in the file of recipients
			}
		}
		else
			sprintf(respp->response, "501 ERROR");	  // unverified email - not containing @
		time_t my_time = time(NULL);
		sprintf(respp->time, ctime(&my_time));
		// ctime() used to give the present time
		printf("TIME: %s", ctime(&my_time));


		if (strcmp(respp->response, "250 OK") == 0)
		{
			time_t t = time(0);   // get time now
			struct tm* now = localtime(&t);
			char buffer[80];
			strftime(buffer, 80, "%Y-%m-%d", now);;
			std::string jtp;
			std::string jtp1;
			std::string jtp2;
			std::stringstream strStream;

			strStream << reqp->To << endl;
			strStream >> jtp;
			strStream << reqp->Subject << endl;
			strStream >> jtp1;

			jtp = jtp + "/inbox/" + jtp1 + "_" + buffer + ".txt";
			std::ofstream fout(jtp);
			fout << reqp->From << std::endl;
			fout << reqp->To << std::endl;
			fout << reqp->Subject << std::endl;
			fout << reqp->Body << std::endl;

			std::string f1;
			std::string f2;
			std::stringstream stream;
			stream << reqp->From << endl;
			stream >> f1;
			stream << reqp->Subject << endl;
			stream >> f2;

			f1 = f1 + "/sent/" + f2 + "_" + buffer + ".txt";
			std::ofstream out(f1);
			out << reqp->From << std::endl;
			out << reqp->To << std::endl;
			out << reqp->Subject << std::endl;
			out << reqp->Body << std::endl;


			if (rmsg.type == yes) // receiving the attachment if there is any
			{

				if (msg_recv(cs, &rmsg) != rmsg.length)
					err_sys("Receive Req error,exit");
				//cast it to the request packet structure

				name = (Name*)rmsg.buffer;
				std::string jtp;
				std::string jtp1;
				std::stringstream strStream;

				fout << "inbox/" << name->fname << std::endl;
				strStream << reqp->To << endl;
				strStream >> jtp;
				strStream << name->fname << endl;
				strStream >> jtp1;

				out << "sent/" << name->fname << std::endl;

				jtp = jtp + "/inbox/" + jtp1;
				RecvFile(cs, jtp);

				strStream << reqp->From << endl;
				strStream >> jtp2;
				jtp2 = jtp2 + "/sent/" + jtp1;
				CopyFile1(jtp.c_str(), jtp2.c_str()); ///
			}
			fout << ctime(&t) << std::endl;
			out << ctime(&t) << std::endl;
			fout.close();
			out.close();


		}

		if (msg_send(cs, &smsg) != smsg.length)
			err_sys("send Respose failed,exit");
		printf("Response for %s has been sent out \n\n\n", savename);

	}
	else
	{
		recp = (Rec*)rmsg.buffer; // casting
		printf("\nConnected to client:%s\n", recp->hostname);
		printf("Inbox directory has been sent to user %s \n", recp->myEmail);
		std::string jtp;
		std::stringstream strStream;
		strStream << recp->myEmail << endl;
		strStream >> jtp;
		jtp = jtp + "/inbox";

		std::string s = compress("inbox.zip", jtp);
		//printf("\n%s\n", s.c_str());
		SendFile(cs, s.c_str());
		int status = remove("inbox.zip");
		if (status == 0)
		{
			//cout << "\nFile Deleted Successfully!";
		}
		else
			cout << "\nError Occurred!";


	}
	closesocket(cs);
}






////////////////////////////////////////////////////////////////////////////////////////

int main(void)
{

	TcpServer ts;
	ts.start();

	return 0;
}