#include "ChatServer.h"
#include "cJSON/cJSON.h"
#include <sstream>
#include <algorithm>


void ChatServer::addClient(int clientFD, sockaddr_storage clientAddr, socklen_t acceptSize){
	userToID["Guest" + std::to_string(clientFD)] = clientFD;
	IDToUser[clientFD] = "Guest" + std::to_string(clientFD);
	liveSockets.insert(clientFD);
	FD_SET(clientFD, &_masterSet);
}


std::string ChatServer::createMessage(std::string sender, std::string text){
	return sender + ":" + text;
}


void ChatServer::delClient(int clientFD){
	std::string user = IDToUser[clientFD];

	// Clear-up routine
	while(!pendingMessages[user].empty())pendingMessages[user].pop();
	inbox.erase(user);
	userToID.erase(user);
	IDToUser.erase(clientFD);
	liveSockets.erase(clientFD);
	FD_CLR(clientFD, &_masterSet);
	close(clientFD);	
}


void ChatServer::setName(socketIterator& sockIter, std::string newName){
	cJSON* responseObject = cJSON_CreateObject();

	if(userToID.find(newName) != userToID.end()){
		cJSON_AddItemToObject(responseObject, "error", cJSON_CreateString("Username already exists!"));
	}
	else{
		std::string oldName = IDToUser[*sockIter];
		inbox[newName] = inbox[oldName];
		pendingMessages[newName] = pendingMessages[oldName];
		userToID[newName] = userToID[oldName];
		IDToUser[*sockIter] = newName;

		inbox[oldName].clear();
		while(!pendingMessages[oldName].empty())pendingMessages[oldName].pop();
		userToID.erase(oldName);

		cJSON_AddItemToObject(responseObject, "name", cJSON_CreateString(newName.c_str()));
		cJSON_AddItemToObject(responseObject, "response", cJSON_CreateString("RENAME SUCCESSFUL!"));
	}

	sendMessage(sockIter, std::string(cJSON_Print(responseObject)));
	cJSON_Delete(responseObject);	
}

void ChatServer::sendMessage(socketIterator& sockIter, std::string message){
	int sendStatus = send(*sockIter, message.c_str(), message.length(), 0);
	if(sendStatus == -1){
		// Send message when select() puts the receiver in writeSet
		printError("send function call");
		pendingMessages[IDToUser[*sockIter]].push(createMessage(IDToUser[*sockIter],message));
	}

	sockIter++;
}


void ChatServer::findOnlineUsers(socketIterator& sockIter){
	cJSON* usersObject = cJSON_CreateArray();
	cJSON* prev = NULL;

	for(userIterator userIter = userToID.begin(); userIter!=userToID.end(); userIter++){
		cJSON* message = cJSON_CreateString((userIter->first).c_str());
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
	if(inbox.find(receiver) == inbox.end())inbox[receiver];
	if(inbox[receiver].find(sender) == inbox[receiver].end())inbox[receiver][sender];
	std::string message = createMessage(sender,text);
	inbox[receiver][sender].push_back(message);
	pendingMessages[receiver].push(message);

	cJSON* responseObject = cJSON_CreateObject();
	cJSON_AddItemToObject(responseObject, "message", cJSON_CreateString("Message sent!"));
	sendMessage(sockIter, std::string(cJSON_Print(responseObject)));
	cJSON_Delete(responseObject);
}


void ChatServer::getMessages(socketIterator& sockIter, std::string from, std::string to){
	cJSON* responseObject = cJSON_CreateObject();
	cJSON* messages = cJSON_CreateArray(), *prev = NULL;

	if( (inbox.find(to) != inbox.end()) && (inbox[to].find(from) != inbox[to].end()) ){	
		for(std::string userMessage : inbox[to][from]){
			cJSON* message = cJSON_CreateString(userMessage.c_str());
			if(prev == NULL){
				messages->child = message;
			}
			else{
				prev->next = message;
				message->prev = prev;
			}
			prev = message;
		}
	}

	cJSON_AddItemToObject(responseObject, "sender", cJSON_CreateString(from.c_str()));
	cJSON_AddItemToObject(responseObject, "messages", messages);
	sendMessage(sockIter, std::string(cJSON_Print(responseObject)));
	cJSON_Delete(responseObject);
}


void ChatServer::parseMessage(char* msg, socketIterator& sockIter) {
	std::vector<std::string> commands = {
		"psend", "gsend", "ol", "messages", "name"
	};

	int clientFD = *sockIter;
	std::string sender = IDToUser[clientFD];
	cJSON* requestObject = cJSON_Parse(msg);
	std::string type = std::string(cJSON_GetObjectItem(requestObject,"type")->valuestring);

	// Command not recognised
	if( std::find(commands.begin(), commands.end(), type) == commands.end() ){
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
						std::string username = IDToUser[currentSocket];

						if(!pendingMessages[username].empty()){

							std::string nextMessage = "";

							while(!pendingMessages[username].empty()){
								nextMessage = nextMessage + pendingMessages[username].front() + "\n";
								pendingMessages[username].pop();
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
}



int main(){
	Server* chatServer = new ChatServer("8132");
	chatServer->listenAndAccept();
	return 0;
}