#include <iostream>
#include <thread>
#include <string>
#include <sstream>
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

Router::Router(int id) {
	filename = "Router" + to_string(id) + ".log";
	file.open(filename);
}
int Router::run(int port) {
	file << "Router starting up!" << endl;
	UDPPort = port;
	UDPfd = createUDPSocket(UDPPort);
	createTCPSocket(TCPIP, to_string(TCPPort));
	return 1;
}

void Router::setIP(string ip) {
	TCPIP = ip;
}
void Router::setPort(int port) {
	TCPPort = port;
}
vector<Link> Router::messageToLinks(string message) {
	vector < Link > neighbors;
	string trucMessage = message.substr(1, message.length() - 1);
	replace(trucMessage.begin(), trucMessage.end(), ':', ' ');
	vector<string> links;
	stringstream ss(trucMessage);
	string temp;
	while (ss >> temp) {
		links.push_back(temp);
	}
	for (size_t i = 0; i < links.size(); i++) {
		replace(links[i].begin(), links[i].end(), ',', ' ');
		vector<string> vals;
		stringstream ss1(links[i]);
		string temp1;
		while (ss1 >> temp1) {
			vals.push_back(temp1);
		}
		neighbors.push_back(Link(atoi(vals[0].c_str()), atoi(vals[1].c_str()), atoi(vals[2].c_str())));
	}
	return neighbors;
}
//Send to a TCP connection
void Router::sendTCP(int fd, string message) {
	//Sending
	char buffer[9999];

	//Writing a message to buffer + handling for too many characters.
	memset(&buffer, 0, sizeof(buffer));
	strcpy(buffer, message.c_str());

	//Send message
	send(fd, (char*) &buffer, strlen(buffer), 0);
	file << "Message sent." << endl;
}

//Send back to the addr that sent to you
void sendBack(int fd, struct sockaddr_in addr, string message) {
	char msg[9999];

	//Writing a message to buffer
	memset(&msg, 0, sizeof(msg));
	strcpy(msg, message.c_str());

	sendto(fd, msg, strlen(msg), 0, (struct sockaddr *) &addr, sizeof(addr));
}

//Try receiving from this TCP connection.
string Router::receiveTCP(int fd) {
	char buffer[9999];
	memset(&buffer, 0, sizeof(buffer));
	//receive file size
	file << "READY! Waiting for message.\n" << endl;
	while (strlen(buffer) == 0) {
		recv(fd, (char*) &buffer, sizeof(buffer), 0);
	}
	file << "Received a message." << endl;
	string str(buffer);
	file << str << endl;
	return str;
}

//Create a manager side socket for connecting to routers.
int Router::createTCPSocket(string destIp, string destPort) {
	file << "\n-----TCP STARTUP-----" << endl;
	file << "Trying to create TCP Socket" << endl;

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
	file << "TCP Socket Created!" << endl;
	//Connect to server.
	int conErr = connect(fd, res->ai_addr, res->ai_addrlen);
	if (conErr < 0) {
		char const *p = "ERROR creating connection";
		error(p);
	}

	file << "Connection established with server. Port: " << destPort << "\n" << endl;
	nodeNum = (atoi(destPort.c_str()) - 7000);
	bool finished = false;
	while (!finished) {
		file << "Waiting" << endl;
		string message = receiveTCP(fd);
		if (message.at(0) == '*') {
			if (message != "*None") {
				neighbors = messageToLinks(message);
				file << "-----RECEIVED LINKS OVER TCP-----" << endl;
				for (size_t i = 0; i < neighbors.size(); i++) {
					file << "SRC: " << neighbors[i].src << " Dest: " << neighbors[i].dest << " Cost: " << neighbors[i].cost << endl;
				}
			}
			file << "This router is node: " << nodeNum << endl;
			struct sockaddr_in sin;
			file << "Sending ACK back to manager." << endl;
			sendBack(fd, sin, "Ready!");
			file << "ACK sent to manager." << endl;
			file << "\nAwaiting START signal...\n" << endl;
		} else if (message.at(0) == '$') {
			struct sockaddr_in sin;
			file << "Received start" << endl;
			//... THIS IS WHERE A CALL TO START LS ROUTING BEGINS
			linkRequest();
			file << "Finished linking to neighbors." << endl;
			sendBack(fd, sin, "DONE");
			file << "\nAwaiting PACKET signal...\n" << endl;
		} else if (message.at(0) == '#') {
			struct sockaddr_in sin;
			file << "Received a packet." << endl;
			//...THIS SHOULD BE A CALL TO SEND TO ANOTHER ROUTER OVER UDP OR THE MANAGER OVER TCP
			file << "Sending it off to another router or sending a received to manager." << endl;
			sendBack(fd, sin, "RECV");
		} else if(message.at(0) == '!'){
			exit(1);
		} else {
			close(fd);
		}
	}
	return fd;
}

void Router::linkRequest(){
	
	for(size_t i = 0; i < neighbors.size(); i++){
		if(neighbors[i].src == nodeNum){
			int linkport = neighbors[i].dest + 6000;
			cout << linkport << " " << nodeNum << endl;
			sendUDP(UDPfd, TCPIP, "LINK");
		}else{
			int linkport = neighbors[i].src + 6000;
			cout << linkport << " " << nodeNum << endl;
			sendUDP(UDPfd, TCPIP, "LINK");
		}
	}
}

/**
 * These methods are for UDP connections.
 * There is one method for creating the socket for sending/receiving
 * There is a method for sending works via FD and a destination IP
 * Receive method listens on the default port for messages
 **/

//Create a routerA side socket for connecting to routers.
int Router::createUDPSocket(int port) {
	file << "\n-----UDP STARTUP-----" << endl;
	file << "Trying to create UDP Socket" << endl;
	int fd;

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		char const *p = "ERROR creating socket";
		error(p);
	}
	thread up (&Router::receiveUDP,this, fd, port);
	//Get the server IP (this was a lot harder than you would think... used beej's guide on gethostbyname)
	cout << "Hello!?!?" << endl;
	
	return (fd);
}

//Send to a UDP connection over a fd
void Router::sendUDP(int fd, string destIp, string message) {
	struct sockaddr_in servaddr;
	char msg[9999];

	//Writing a message to buffer
	memset(&msg, 0, sizeof(msg));
	strcpy(msg, message.c_str());

	//Set up servaddr
	memset((char*) &servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(UDPPort);

	//Copy the destIP to the servaddr stuct
	inet_pton(AF_INET, destIp.c_str(), &(servaddr.sin_addr));

	//Send a message
	if (sendto(fd, msg, strlen(msg), 0, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
		char const *p = "ERROR sending message";
		error(p);
	}

	file << "Message sent!" << endl;
}

//Start receiving UDP messages, complete with ACK handling
int receivedFromFD;
struct sockaddr_in receivedFromAddr;
void Router::receiveUDP(int fd, int port) {
	
	char hostname[128];
	struct sockaddr_in myaddr;
		//This struct below needs to be mapped to a variable for communicating back.
		struct sockaddr_in remaddr;
		socklen_t addrlen = sizeof(remaddr);
		int recvlen;
		char buf[9999];
		struct hostent *he;
		struct in_addr **addr_list;
		gethostname(hostname, sizeof hostname);
		he = gethostbyname(hostname);
		addr_list = (struct in_addr **) he->h_addr_list;
		cout << fd << " " << port << endl;
		file << "UDP socket created: <" << inet_ntoa(*addr_list[0]) << ", " << port << ">" << endl;

		memset((char *) &myaddr, 0, sizeof(myaddr));
		myaddr.sin_family = AF_INET;
		myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
		myaddr.sin_port = htons(port);

		if (bind(fd, (struct sockaddr *) &myaddr, sizeof(myaddr)) < 0) {
			char const *p = "ERROR binding socket";
			error(p);
		}
		file << "UDP Listening." << endl;	

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
