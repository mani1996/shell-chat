#include "User.h"

void User::setName(std::string name){
	UserBuilder* userBuilder = UserBuilder::getInstance();
	userBuilder->changeUsername(name, username);
	username = name;	
}


std::string User::getName(){
	return username;
}


void User::addMessage(Message message){
	UserBuilder* userBuilder = UserBuilder::getInstance();
	int id = userBuilder->getID((message.getSender())->getName());
	if(inbox.find(id) == inbox.end())inbox[id];
	inbox[id].push_back(message);
	pendingMessages.push(message);
}


void User::addPendingMessage(Message message){
	pendingMessages.push(message);
}


Message User::getPendingMessage(){
	if(pendingMessages.empty()){
		printf("No pending messages");
		return Message::getEmptyMessage();
	}

	Message nextMessage = pendingMessages.front();
	pendingMessages.pop();
	return nextMessage; 
}


bool User::hasPendingMessages(){
	return pendingMessages.size() > 0;
}


std::vector<Message> User::getMessagesFrom(std::string username){
	return inbox[(UserBuilder::getInstance())->getID(username)];
}


UserBuilder* UserBuilder::userBuilder = NULL;


void UserBuilder::addUser(userID id){
	if(IDToUser.find(id) == IDToUser.end()){
		std::string name = "Guest" + std::to_string(id);
		IDToUser[id] = new User(name);
		userToID[name] = id;
	}
	else{
		printf("Non unique ID. Can't happen since clientID is always unique\n");
	}
}


void UserBuilder::changeUsername(std::string newName, std::string oldName){
	userToID[newName] = userToID[oldName];
	userToID.erase(oldName);
}


void UserBuilder::delUser(userID id){
	if(IDToUser.find(id) != IDToUser.end()){
		User* temp =  IDToUser[id];
		std::string username = temp->getName();
		IDToUser.erase(id);
		userToID.erase(username);
		delete temp;
	}
}


User* UserBuilder::getUser(userID id){
	if(IDToUser.find(id) == IDToUser.end())return NULL;
	return IDToUser[id];
}


User* UserBuilder::getUser(std::string username){
	if(userToID.find(username) == userToID.end())return NULL;
	return IDToUser[getID(username)];
}


UserBuilder::userID UserBuilder::getID(std::string username){
	if(userToID.find(username) == userToID.end())return -1;
	return userToID[username];
}


UserBuilder* UserBuilder::getInstance(){
	if(UserBuilder::userBuilder == NULL)UserBuilder::userBuilder = new UserBuilder;
	return UserBuilder::userBuilder;
}


UserBuilder::~UserBuilder(){
	for(IDIterator it = IDToUser.begin(); it != IDToUser.end(); it++){
		delete it->second;
	}
}


std::vector<User*> UserBuilder::getUsers(){
	std::vector<User*> users;
	for(IDIterator it = IDToUser.begin(); it != IDToUser.end(); it++){
		users.push_back(it->second);
	}
	return users;
}


Message::Message(User* sender, std::string text):sender(sender),text(text){
	sentTime = Message::timeStamp();
}


Message* Message::emptyMessage = new Message(NULL, "");


Message Message::getEmptyMessage(){
	return *(Message::emptyMessage);
}

std::tm Message::timeStamp(){
	std::time_t t = time(0);
	return *(std::localtime(&t));
}


std::string Message::format(){
	std::string msgFormat = "";
	std::string dateFormat = "(" + std::to_string(sentTime.tm_mday) + "/" + std::to_string(
		sentTime.tm_mon+1) + "/" + std::to_string(sentTime.tm_year+1900) + " - " + std::to_string(
		sentTime.tm_hour) + ":" + std::to_string(sentTime.tm_min) + ") ";

	msgFormat+=dateFormat;
	msgFormat+=((sender->getName()) + ": " + text);
	return msgFormat;
}


User* Message::getSender(){
	return sender;
}