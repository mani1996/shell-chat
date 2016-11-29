#ifndef USERINCLUDES
#define USERINCLUDES
#include <ctime>
#include <map>
#include <queue>
#include <string>
#include <vector>


class Message;

// A class describing each user in chatroom
class User{
	std::string username;
	std::map<int, std::vector<Message> > inbox; // Map<Socket FD, List of messages>
	std::queue<Message> pendingMessages;

public:
	User(std::string name):username(name){}
	User():username(""){}
	void addMessage(Message message);
	void addPendingMessage(Message message);
	bool hasPendingMessages();
	std::vector<Message> getMessagesFrom(std::string username);	
	std::string getName();
	Message getPendingMessage();
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
	~UserBuilder();
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


class Message{
	User* sender;
	std::string text;
	std::tm sentTime;
	static std::tm timeStamp();
	static Message* emptyMessage;
public:
	Message(User* sender, std::string text);
	std::string format();
	User* getSender();
	static Message getEmptyMessage();
};
#endif