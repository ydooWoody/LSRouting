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
#include "project3.h"

using namespace std;

Router::Router(vector<Link> neighbors, int id){
	filename = "Router" + to_string(id) + ".txt";
	file.open(filename);
	file << "Router " << id << "\nNeighbor list size: " << neighbors.size() << "\n";
}
int Router::test(){
	cout << "TESTTTTTTTTT" << endl;
	return 1;
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
