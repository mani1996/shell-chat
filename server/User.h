#ifndef USERINCLUDES
#define USERINCLUDES
#include <map>
#include <queue>
#include <string>
#include <vector>


// A class describing each user in chatroom
class User{
	std::string username;
	std::map<std::string, std::vector<std::string> > inbox; // Map<Username, List of messages>
	std::queue<std::string> pendingMessages;

public:
	User(std::string name):username(name){}
	User():username(""){}
	void addMessage(std::string sender, std::string message);
	void addPendingMessage(std::string message);
	bool hasPendingMessages();
	std::vector<std::string> getMessagesFrom(std::string username);	
	std::string getName();
	std::string getPendingMessage();
	void setName(std::string name);
};


/* 
	Singleton class which has user <==> socket mapping.
	userID of a user is the file descriptor ID of their socket
*/

class UserBuilder{
	typedef int userID;
	typedef std::map<userID, User*>::iterator IDIterator;

	std::map<userID, User*> IDToUser;
	std::map<std::string, userID> userToID;
	static UserBuilder* userBuilder;
	UserBuilder(){}
public:
	void addUser(userID id);
	void changeUsername(std::string newName, std::string oldName);
	void delUser(userID id);
	userID getID(std::string username);
	static UserBuilder* getInstance();
	User* getUser(userID id);
	User* getUser(std::string username);
	std::vector<User*> getUsers();
};

#endif