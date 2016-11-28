#ifndef SERVERINCLUDES
#define SERVERINCLUDES

#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <cstdio>
#include <cstring>
#include <set>
#include <thread>
#include <vector>

class Server{
protected:
	std::string _port;
	addrinfo* _addrParams;
	int _socketFD;
	int _backlog;
	timeval pollWaitingTime;
	fd_set _masterSet;
	fd_set _readSet;
	fd_set _writeSet;
	std::set<int> liveSockets;

	void printError(char* target);
	void init();
	void findAndBind(addrinfo* addrNode);
	virtual void communicate() = 0;
	virtual void addClient(int clientFD, sockaddr_storage clientAddr, socklen_t acceptSize);
	virtual void delClient(int clientFD);

public:
	Server(std::string port, int backlog);
	void listenAndAccept();
	bool isSet() const;
};

#endif