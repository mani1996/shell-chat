#include "ChatServer.h"
#include <sstream>
#include <algorithm>


void ChatServer::addClient(int clientFD, sockaddr_storage clientAddr, socklen_t acceptSize){
	userToID["Guest" + std::to_string(clientFD)] = clientFD;
	IDToUser[clientFD] = "Guest" + std::to_string(clientFD);
	liveSockets.insert(clientFD);
	FD_SET(clientFD, &_masterSet);
}


void ChatServer::delClient(int clientFD){
	std::string user = IDToUser[clientFD];

	// Clear-up routine
	inbox[user].clear();
	while(!pendingMessages[user].empty())pendingMessages[user].pop();
	userToID.erase(user);
	IDToUser.erase(clientFD);
	liveSockets.erase(clientFD);
	FD_CLR(clientFD, &_masterSet);
	close(clientFD);	
}


void ChatServer::setName(socketIterator& sockIter, std::string newName){

	if(userToID.find(newName) != userToID.end()){
		std::string error = "ERROR : username already exists!";
		sendMessage(sockIter, error);
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

		std::string response = "\nRename successful!\n";
		sendMessage(sockIter, response);
	}
}

void ChatServer::sendMessage(socketIterator& sockIter, std::string message){
	int sendStatus = send(*sockIter, message.c_str(), message.length(), 0);

	if(sendStatus == -1){
		printError("send function call");
		delClient(*(sockIter++));
	}
	else{
		sockIter++;
	}
}


void ChatServer::findOnlineUsers(socketIterator& sockIter){
	std::string response = "\nOnline users:\n";
	for(userIterator userIter = userToID.begin(); userIter!=userToID.end(); userIter++){
		response+=(userIter->first);
		response+="\n";
	}
	response+="\n";
	sendMessage(sockIter, response);
}


void ChatServer::addMessage(socketIterator& sockIter, std::string sender, 
	std::string receiver, std::string text){
	if(inbox.find(receiver) == inbox.end())inbox[receiver];
	if(inbox[receiver].find(sender) == inbox[receiver].end())inbox[receiver][sender];
	inbox[receiver][sender].push_back(text);
	pendingMessages[receiver].push(sender + ": " + text);
	std::string response = "\nMessage sent!\n";
	sendMessage(sockIter, response);
}


void ChatServer::getMessages(socketIterator& sockIter, std::string from, std::string to){
	std::string response = "\nMessages from user " + from + "\n";
	if(inbox.find(to) == inbox.end())inbox[to];
	if(inbox.find(from) == inbox.end())inbox[to][from];
	for(std::string userMessage : inbox[to][from]){
		response+=userMessage.c_str();
		response+="\n";
	}
	response+="\n";
	sendMessage(sockIter, response);
}


void ChatServer::parseMessage(char* msg, socketIterator& sockIter) {
	std::string message = std::string(msg);
	std::string type;
	std::stringstream messageStream(message);

	std::string commands[] = {
		"psend", "gsend", "ol", "messages", "name"
	};
	int noOfCommands = 5;

	int clientFD = *sockIter;
	std::string sender = IDToUser[clientFD];
	printf("\nMessage received from client %d : %s\n", clientFD, msg);

	getline(messageStream, type, ' ');

	// Command not recognised
	if( std::find(commands, commands+noOfCommands, type) >= commands+noOfCommands ){
		std::string error = "\nERROR : Unrecognised command '" + type + "'\n";
		sendMessage(sockIter, error);
		return ;
	}

	if(type == "psend"){
		std::string receiver,text;
		getline(messageStream, receiver, ' ');
		getline(messageStream, text);
		addMessage(sockIter, sender, receiver, text);
	}
	else if(type == "messages"){
		std::string user;
		getline(messageStream, user, ' ');
		getMessages(sockIter, user, sender);
	}
	else if(type == "ol"){
		findOnlineUsers(sockIter);
	}
	else if(type == "name"){
		std::string newName;
		getline(messageStream, newName, ' ');
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
							printf("PARSE\n");
							buf[msgLength] = '\0';
							parseMessage(buf,sockIter);
						}
						incrementPtr = true;
					}
						
					if(FD_ISSET(currentSocket, &_writeSet)){
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