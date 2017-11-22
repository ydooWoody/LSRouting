/*
 * messaging.cpp
 *
 *  Created on: Nov 21, 2017
 *      Author: Chris
 */
#include <iostream>
#include <string>
#include <cstring>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <fstream>
#include <algorithm>

using namespace std;

void error(char const *msg) {
	perror(msg);
	exit(1);
}

/**
 * These methods are for TCP connections.
 * There are 2 methods for creating sockets: One for manager -> router and One for router -> manager
 * There are methods for sending and receiving which work via FD and work on both router and manager
 **/

//Create a manager side socket for connecting to routers.
int createTCPSocket(string destIp, string destPort) {
	cout << "Trying to create TCP Socket" << endl;

	//Variables for socket file descriptor and address info
	int fd;
	struct addrinfo hints, *res;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	const char* ipChar = destIp.c_str();
	const char* portChar = destPort.c_str();
	getaddrinfo(ipChar, portChar, &hints, &res);

	//Create a socket
	fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	//Avoid "address already in use" error because machine(?) still waiting to send ack and keeps getting sent into TIMEWAIT.
	int optval = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
	if (fd < 0) {
		char const *p = "ERROR creating socket";
		error(p);
	}
	cout << "Socket created!" << endl;

	//Connect to server.
	int conErr = connect(fd, res->ai_addr, res->ai_addrlen);
	if (conErr < 0) {
		char const *p = "ERROR creating connection";
		error(p);
	}

	cout << "Connection established." << endl;

	return fd;
}

//Send to a TCP connection
void sendTCP(int fd, string message) {
	//Sending
	char buffer[9999];

	//Writing a message to buffer + handling for too many characters.
	memset(&buffer, 0, sizeof(buffer));
	strcpy(buffer, message.c_str());

	//Send message
	send(fd, (char*) &buffer, strlen(buffer), 0);
	cout << "Message sent." << endl;
}

//Try receiving from this TCP connection.
string receiveTCP(int fd) {
	char buffer[9999];
	memset(&buffer, 0, sizeof(buffer));
	//receive file size
	cout << "Waiting for message." << endl;
	while (strlen(buffer) == 0) {
		recv(fd, (char*) &buffer, sizeof(buffer), 0);
	}
	cout << "Received a message." << endl;
	string str(buffer);
	cout << str << endl;
	return str;
}

//Create a router side TCP socket for connecting to the manager.
int createTCPSocket(string port) {
	//Various variables for setting up socket and getting IP from socket, address info, etc.
	int sockfd, bindErr, new_fd, lisErr;
	struct protoent *protoptr;

	//Get protocol "name"
	protoptr = getprotobyname("tcp");

	//Create a TCP socket
	sockfd = socket(AF_INET, SOCK_STREAM, protoptr->p_proto);
	//Avoid "address already in use" error because machine(?) still waiting to send ack and keeps getting sent into TIMEWAIT.
	int optval = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
	if (sockfd < 0) {
		char const *p = "ERROR creating socket";
		error(p);
	}

	//Get/set addr info
	struct addrinfo hints, *res;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	getaddrinfo(NULL, port.c_str(), &hints, &res);

	//Bind the TCP socket
	bindErr = bind(sockfd, res->ai_addr, res->ai_addrlen);
	if (bindErr < 0) {
		char const *p = "ERROR binding socket";
		error(p);
	}

	//Listen on the TCP socket
	lisErr = listen(sockfd, 25);
	if (lisErr < 0) {
		char const *p = "ERROR listening to socket";
		error(p);
	}

	//Get the server IP (this was a lot harder than you would think... used beej's guide on gethostbyname)
	char hostname[128];
	struct hostent *he;
	struct in_addr **addr_list;
	gethostname(hostname, sizeof hostname);
	he = gethostbyname(hostname);
	addr_list = (struct in_addr **) he->h_addr_list;

	//Print out important info
	cout << "Listening on: <" << inet_ntoa(*addr_list[0]) << ", " << port << ">" << endl;

	//Accept a connection to the TCP socket
	struct sockaddr_storage their_addr;
	socklen_t addr_size;
	addr_size = sizeof their_addr;
	new_fd = accept(sockfd, (struct sockaddr *) &their_addr, &addr_size);

	//Avoid "address already in use" error because machine(?) still waiting to send ack and keeps getting sent into TIMEWAIT.
	int optval1 = 1;
	setsockopt(new_fd, SOL_SOCKET, SO_REUSEADDR, &optval1, sizeof optval1);
	if (new_fd < 0) {
		char const *p = "ERROR accepting connection to socket";
		error(p);
	} else {
		char hostbuffer[256];
		inet_ntop(their_addr.ss_family, &(((struct sockaddr_in *) &their_addr)->sin_addr), hostbuffer, INET_ADDRSTRLEN);
	}
	return new_fd;
}

/**
 * These methods are for UDP connections.
 * There is one method for creating the socket for sending/receiving
 * There is a method for sending works via FD and a destination IP
 * Receive method listens on the default port for messages
 **/
int PORT = 6789;
//Create a routerA side socket for connecting to routers.
int createUDPSocket() {
	int fd;
	struct sockaddr_in myaddr;

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		char const *p = "ERROR creating socket";
		error(p);
	}
	//Get the server IP (this was a lot harder than you would think... used beej's guide on gethostbyname)
	char hostname[128];
	struct hostent *he;
	struct in_addr **addr_list;
	gethostname(hostname, sizeof hostname);
	he = gethostbyname(hostname);
	addr_list = (struct in_addr **) he->h_addr_list;
	cout << "Made socket: <" << inet_ntoa(*addr_list[0]) << ", " << PORT << ">" << endl;

	memset((char *) &myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(PORT);

	if (bind(fd, (struct sockaddr *) &myaddr, sizeof(myaddr)) < 0) {
		char const *p = "ERROR binding socket";
		error(p);
	}
	return fd;
}

//Send to a UDP connection over a fd
void sendUDP(int fd, string destIp, string message) {
	struct sockaddr_in servaddr;
	char msg[9999];

	//Writing a message to buffer
	memset(&msg, 0, sizeof(msg));
	strcpy(msg, message.c_str());

	//Set up servaddr
	memset((char*) &servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(PORT);

	//Copy the destIP to the servaddr stuct
	inet_pton(AF_INET, destIp.c_str(), &(servaddr.sin_addr));

	//Send a message
	if (sendto(fd, msg, strlen(msg), 0, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
		char const *p = "ERROR sending message";
		error(p);
	}

	cout << "Message sent!" << endl;
}

//Send back to the addr that sent to you
void sendBack(int fd, struct sockaddr_in addr, string message) {
	char msg[9999];

	//Writing a message to buffer
	memset(&msg, 0, sizeof(msg));
	strcpy(msg, message.c_str());

	sendto(fd, msg, strlen(msg), 0, (struct sockaddr *) &addr, sizeof(addr));
	cout << "Sending message back to sender!" << endl;
}

//Start receiving UDP messages, complete with ACK handling
int receivedFromFD;
struct sockaddr_in receivedFromAddr;
void receiveUDP(int port) {

	struct sockaddr_in myaddr;
	//This struct below needs to be mapped to a variable for communicating back.
	struct sockaddr_in remaddr;
	socklen_t addrlen = sizeof(remaddr);
	int recvlen;
	//Will also need to be stored for sending back
	int fd;
	char buf[9999];

	//Check if a connection is already established
	if (port != -1) {
		//Create a UDP Socket
		if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
			char const *p = "ERROR creating socket";
			error(p);
		}

		//Bind to any IP address, default port
		memset((char *) &myaddr, 0, sizeof(myaddr));
		myaddr.sin_family = AF_INET;
		myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
		myaddr.sin_port = htons(port);

		if (bind(fd, (struct sockaddr *) &myaddr, sizeof(myaddr)) < 0) {
			char const *p = "ERROR binding socket";
			error(p);
		}
	}

	cout << "Waiting on socket: Port: " << port << endl;

	while (true) {
		recvlen = recvfrom(fd, buf, 9999, 0, (struct sockaddr *) &remaddr, &addrlen);
		//These are storing the information for sending back to the "client"
		receivedFromAddr = remaddr;
		receivedFromFD = fd;
		string str;

		if (recvlen > 0) {
			//Received a message, either send an ACK or send a message depending on who is recieving.
			buf[recvlen] = 0;
			//Prints the message for now, we'll want a function call here.
			str = buf;
			cout << "Message: " << str << endl;
		}
		if (str == "ACK") {
			//Send a message back (this timing is way off, but was good for inital testing)
			sleep(10);
			cout << "Trying to send a message!" << endl;
			//This is where the router will send information to others. Will need to call some function to construct a msg
			sendBack(receivedFromFD, receivedFromAddr, "Hello back!");
			cout << "Sent a message back!" << endl;
		} else {
			cout << "HERE" << endl;
			//ACK that you received a message
			sleep(10);
			cout << "Trying to send an ACK!" << endl;
			sendBack(receivedFromFD, receivedFromAddr, "ACK");
			cout << "Sent an ACK for the message!" << endl;
		}
	}
}

//TCP works fine, UDP has issues with ports being already used; should be fixed once threading is added to this.
int main(int argc, const char * argv[]) {
	if (argc == 1) {
		int manFD = createTCPSocket("129.82.44.134", "9999");
		sendTCP(manFD, "Hello");
		receiveTCP(manFD);
		//int router1 = createUDPSocket();
		//sendUDP(router1, "129.82.44.134", "Hello");
		//receiveUDP(6789);
		//sendUDP(router1, "129.82.44.134", "ACK");
	} else {
		int routerFD = createTCPSocket("9999");
		receiveTCP(routerFD);
		sendTCP(routerFD, "Hello back!");
		//receiveUDP(6789);

	}
}
