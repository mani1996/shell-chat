#include "ChatServer.h"
#include <sstream>
#include <algorithm>


std::vector<std::string> ChatServer::commands = {
	"psend", "gsend", "ol", "messages", "name"
};


cJSON* ChatServer::errorObject(std::string errorMessage){
	cJSON* error = cJSON_CreateObject();
	cJSON_AddItemToObject(error, "error", cJSON_CreateString(errorMessage.c_str()));
	return error;
}


void ChatServer::addClient(int clientFD, sockaddr_storage clientAddr, socklen_t acceptSize){
	(UserBuilder::getInstance())->addUser(clientFD);
	Server::addClient(clientFD, clientAddr, acceptSize);
}


void ChatServer::delClient(int clientFD){
	(UserBuilder::getInstance())->delUser(clientFD);
	Server::delClient(clientFD);	
}


void ChatServer::setName(socketIterator& sockIter, std::string newName){
	cJSON* responseObject = cJSON_CreateObject();
	UserBuilder* userBuilder = UserBuilder::getInstance();

	if(userBuilder->getID(newName) != -1) {
		cJSON_AddItemToObject(responseObject, "error", cJSON_CreateString("Username already exists!"));
	}
	else{
		User* user = userBuilder->getUser(*sockIter);

		if(user == NULL){
			cJSON* error = errorObject("Socket doesn't exist. This shouldn't be happening");
			sendMessage(sockIter, std::string(cJSON_Print(error)));
			cJSON_Delete(error);
			return ;
		}

		user->setName(newName);
		cJSON_AddItemToObject(responseObject, "name", cJSON_CreateString(newName.c_str()));
		cJSON_AddItemToObject(responseObject, "response", cJSON_CreateString("RENAME SUCCESSFUL!"));
	}

	sendMessage(sockIter, std::string(cJSON_Print(responseObject)));
	cJSON_Delete(responseObject);	
}


int ChatServer::sendMessage(socketIterator& sockIter, std::string message){
	UserBuilder* userBuilder = UserBuilder::getInstance();
	int sendStatus = send(*sockIter, message.c_str(), message.length(), 0);
	if(sendStatus == -1){
		// Send message when select() puts the receiver in writeSet
		printError("send function call");
	}
	sockIter++;
	return sendStatus;
}


void ChatServer::findOnlineUsers(socketIterator& sockIter){
	cJSON* usersObject = cJSON_CreateArray();
	cJSON* prev = NULL;
	std::vector<User*> onlineUsers = (UserBuilder::getInstance())->getUsers();

	for(User* user : onlineUsers){
		cJSON* message = cJSON_CreateString((user->getName()).c_str());
		if(prev == NULL){
			usersObject->child = message;
		}
		else{
			prev->next = message;
			message->prev = prev;
		}
		prev = message;
	}

	sendMessage(sockIter, cJSON_Print(usersObject));
	cJSON_Delete(usersObject);
}


void ChatServer::addMessage(socketIterator& sockIter, std::string sender, 
	std::string receiver, std::string text){
	UserBuilder* userBuilder = UserBuilder::getInstance();
	User* receiveUser = userBuilder->getUser(receiver);

	if(receiveUser == NULL){
		cJSON* error = errorObject("Invalid request. Receiver does not exist!");
		sendMessage(sockIter, std::string(cJSON_Print(error)));
		cJSON_Delete(error);
		return ;
	}

	Message message(userBuilder->getUser(sender),text);
	receiveUser->addMessage(message);
	cJSON* responseObject = cJSON_CreateObject();
	cJSON_AddItemToObject(responseObject, "message", cJSON_CreateString("Message sent!"));
	int response = sendMessage(sockIter, std::string(cJSON_Print(responseObject)));

	if(response == -1){
		userBuilder->getUser(*sockIter)->addPendingMessage(message);
	}

	cJSON_Delete(responseObject);
}


void ChatServer::getMessages(socketIterator& sockIter, std::string from, std::string to){
	cJSON* responseObject = cJSON_CreateObject();
	cJSON* messages = cJSON_CreateArray(), *prev = NULL;
	UserBuilder* userBuilder = UserBuilder::getInstance();
	User* fromUser = userBuilder->getUser(from);

	if(fromUser == NULL){
		cJSON* error = errorObject("Invalid request. Mentioned user does not exist!");
		sendMessage(sockIter, std::string(cJSON_Print(error)));
		cJSON_Delete(error);
		return ;
	}

	std::vector<Message> messageList = userBuilder->getUser(to)->getMessagesFrom(from);

	for(Message userMessage : messageList){
		cJSON* message = cJSON_CreateString(userMessage.format().c_str());
		if(prev == NULL){
			messages->child = message;
		}
		else{
			prev->next = message;
			message->prev = prev;
		}
		prev = message;
	}

	cJSON_AddItemToObject(responseObject, "sender", cJSON_CreateString(from.c_str()));
	cJSON_AddItemToObject(responseObject, "messages", messages);
	sendMessage(sockIter, std::string(cJSON_Print(responseObject)));
	cJSON_Delete(responseObject);
}


void ChatServer::parseMessage(char* msg, socketIterator& sockIter) {
	int clientFD = *sockIter;
	std::string sender = (UserBuilder::getInstance())->getUser(clientFD)->getName();
	cJSON* requestObject = cJSON_Parse(msg);
	std::string type = std::string(cJSON_GetObjectItem(requestObject,"type")->valuestring);

	// Command not recognised
	if( std::find(ChatServer::commands.begin(), ChatServer::commands.end(), type) == 
		ChatServer::commands.end() ){
		std::string error = "\nERROR : Unrecognised command '" + type + "'\n";
		sendMessage(sockIter, error);
		return ;
	}

	if(type == "psend"){
		std::string receiver,text;
		receiver = std::string(cJSON_GetObjectItem(requestObject,"username")->valuestring);
		text = std::string(cJSON_GetObjectItem(requestObject,"message")->valuestring);
		addMessage(sockIter, sender, receiver, text);
	}
	else if(type == "messages"){
		std::string user;
		user = std::string(cJSON_GetObjectItem(requestObject,"username")->valuestring);
		getMessages(sockIter, user, sender);
	}
	else if(type == "ol"){
		findOnlineUsers(sockIter);
	}
	else if(type == "name"){
		std::string newName;
		newName = std::string(cJSON_GetObjectItem(requestObject,"newName")->valuestring);
		setName(sockIter, newName);
	}
	else if(type == "gsend"){
		std::string gsend = "\nINFO : group chat feature isn't available yet :( \n";
		sendMessage(sockIter, gsend);
	}

	return ;
}


void ChatServer::communicate(){
	char* buf = new char[1024];
	int selectStatus,currentSocket,msgLength;
	bool incrementPtr;

	/*

	Keep polling clients for new messages. Each select() call is followed by a loop 
	through each online client. If a client has submitted a new message, store it for 
	processing.If a client having pending messages is found during looping, pending 
	messages are sent to the client, keeping the client up-to-date

	*/

	while(true){
		if(!liveSockets.empty()){
			_readSet = _writeSet = _masterSet;
			selectStatus = select(1024, &_readSet, &_writeSet, NULL, &pollWaitingTime); // non-blocking stmt

			if(selectStatus == -1){
				printError("select function call");
			}
			else if(selectStatus > 0){
				socketIterator sockIter = liveSockets.begin();

				while(sockIter != liveSockets.end()){
					currentSocket = *(sockIter);
					incrementPtr = false;

					if(FD_ISSET(currentSocket, &_readSet)){
						msgLength = recv(currentSocket, buf, 1024, 0);

						if(msgLength == -1){
							printError("recv function call");
							delClient(*(sockIter++));
						}
						else if(msgLength == 0){
							printf("Connection closed by client\n");
							delClient(*(sockIter++));
						}
						else{
							buf[msgLength] = '\0';
							parseMessage(buf,sockIter);
						}
						incrementPtr = true;
					}
					else if(FD_ISSET(currentSocket, &_writeSet)){
						User* user = (UserBuilder::getInstance())->getUser(currentSocket);

						if(user->hasPendingMessages()){
							std::string nextMessage = "";

							while(user->hasPendingMessages()){
								nextMessage = nextMessage + (user->getPendingMessage()).format() + "\n";
							}

							// sockIter is passed as reference to sendMessage() and incremented
							sendMessage(sockIter, nextMessage);
							incrementPtr = true;
						}
					}

					if(!incrementPtr)sockIter++;
					
				}
			}
		}
	}
	delete[] buf;
}


int main(){
	Server* chatServer = new ChatServer("8132");
	chatServer->listenAndAccept();
	return 0;
}