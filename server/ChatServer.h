#ifndef CHATSERVERINCLUDES
#define CHATSERVERINCLUDES
#include "Server.h"
#include "User.h"
#include <map>
#include <queue>
#include <vector>
#include "cJSON/cJSON.h"


class ChatServer: public Server{
	typedef int userID;
	typedef std::set<int>::iterator socketIterator;
	typedef std::map<std::string, userID>::iterator userIterator;
	static std::vector<std::string> commands;

protected:
	void addClient(int clientFD, sockaddr_storage clientAddr, socklen_t acceptSize);	
	void addMessage(socketIterator& sockIter, std::string sender, std::string receiver, std::string text);
	void communicate();
	void delClient(int clientFD); 
	void findOnlineUsers(socketIterator& sockIter);
	cJSON* errorObject(std::string errorMessage);
	void getMessages(socketIterator& sockIter, std::string from, std::string to);
	void parseMessage(char* msg, socketIterator& sockIter);
	int sendMessage(socketIterator& sockIter, std::string message);
	void setName(socketIterator& sockIter, std::string newName);
public:
	ChatServer(std::string port, int backlog = 10):Server(port,backlog){}
};

#endif