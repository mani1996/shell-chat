#ifndef CHATSERVERINCLUDES
#define CHATSERVERINCLUDES
#include "Server.h"
#include <map>
#include <queue>
#include <vector>

class ChatServer: public Server{
	typedef int userID;
	typedef std::set<int>::iterator socketIterator;
	typedef std::map<std::string, userID>::iterator userIterator;

	std::map<std::string, userID> userToID;
	std::map<userID, std::string> IDToUser;
	std::map<std::string, std::map<std::string, std::vector<std::string> > > inbox;
	std::map<std::string, std::queue<std::string>> pendingMessages;
protected:
	void addClient(int clientFD, sockaddr_storage clientAddr, socklen_t acceptSize);	
	void addMessage(socketIterator& sockIter, std::string sender, std::string receiver, std::string text);
	void communicate();
	std::string createMessage(std::string sender, std::string text);
	void delClient(int clientFD); 
	void findOnlineUsers(socketIterator& sockIter);
	void getMessages(socketIterator& sockIter, std::string from, std::string to);
	void parseMessage(char* msg, socketIterator& sockIter);
	void sendMessage(socketIterator& sockIter, std::string message);
	void setName(socketIterator& sockIter, std::string newName);
public:
	ChatServer(std::string port, int backlog = 10):Server(port,backlog){}
};

#endif