#include "Server.h"
#include <iostream>

Server::Server(std::string port, int backlog = 10){
	_port = port;
	_addrParams = NULL;
	_backlog = backlog;
	init();
}


void Server::printError(char* target){
	strcat(target," - Error");
	perror(target);
}


/* 
	Traverse the list of addresses and bind with the first address 
	that allows socket creation + binding
*/
void Server::findAndBind(addrinfo* addrNode){
	int yes = 1;

	while(addrNode!=NULL){
		if((_socketFD = socket(addrNode->ai_family, addrNode->ai_socktype, addrNode->ai_protocol))
				== -1){
			printError("Socket Creation");
			addrNode = addrNode->ai_next;
			continue;
		}

		if(setsockopt(_socketFD, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
			printError("setsockopt function call");
			return ;
		}

		if(bind(_socketFD, addrNode->ai_addr, addrNode->ai_addrlen) == -1){
			printError("bind function call");
			close(_socketFD);
			addrNode = addrNode->ai_next;
			continue;
		}

		_addrParams = addrNode;
		printf("Server started at port %s\n", _port.c_str());
		return ; 
	}

	return ;
}


void Server::listenAndAccept(){
	if(!isSet()){
		printError("can't listen due to bind");
		return ;
	}

	if(listen(_socketFD, _backlog) == -1){
		printError("listen function call");
		return ;
	}

	sockaddr_storage clientAddr;
	socklen_t acceptSize;
	int clientFD;

	std::thread communicatorThread(&Server::communicate, this);

	while(true){
		acceptSize = sizeof(sockaddr_storage);
		clientFD = accept(_socketFD, (sockaddr*) &clientAddr, &acceptSize);

		if(clientFD != -1){
			printf("New client - FD : %d\n", clientFD);
			addClient(clientFD, clientAddr, acceptSize);
		}
		else{
			printError("accept function call");
		}	
	}

	communicatorThread.join();
}


void Server::init(){
	int status;
	addrinfo hints, *servInfo;

	FD_ZERO(&_readSet);
	FD_ZERO(&_writeSet);
	FD_ZERO(&_masterSet);

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	pollWaitingTime.tv_sec = pollWaitingTime.tv_usec = 0; // No blocking on select() call

	if((status = getaddrinfo(NULL, _port.c_str(), &hints, &servInfo))!=0){
		printError("getaddrinfo function call");
		return ;
	}

	findAndBind(servInfo);
	freeaddrinfo(servInfo);
}


bool Server::isSet() const{
	return _addrParams!=NULL;
}


void Server::delClient(int clientFD){
	FD_CLR(clientFD, &_masterSet);
	close(clientFD);
	liveSockets.erase(clientFD);
}