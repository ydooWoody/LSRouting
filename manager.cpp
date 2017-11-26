#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
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
#include <algorithm>
#include "project3.h"
using namespace std;

vector<Link> allLinks;
ofstream file;
int nodes;

void error(char const *msg) {
	perror(msg);
	exit(1);
}

Link::Link(int src, int dest, int cost){
	Link::src = src;
	Link::dest = dest;
	Link::cost = cost;

}

void parseFile(string filename) {
	ifstream inputFile(filename);
	if (!inputFile.is_open()) {
		cout << "Error opening the input file" << '\n';
		exit(1);
	}

	file << "Parsing a file" << '\n';

	string line = "";

	while (getline(inputFile, line)) {
		if (stoi(line) != -1) {
			nodes = stoi(line);
			file << "\n-----NODES: " << nodes << "-----\n";

			for (int i = 0; i < nodes; i++) {

				getline(inputFile, line);

				stringstream ss(line); // Turn the string into a stream.

				string token;

				vector<int> numbers;

				while (getline(ss, token, ' ')) {
					numbers.push_back(stoi(token));
				}

				allLinks.push_back(Link(numbers[0], numbers[1], numbers[2]));

				file << "Src: " << allLinks[i].src << " | ";
				file << "Dest: " << allLinks[i].dest << " | ";
				file << "Cost: " << allLinks[i].cost << '\n';
				file << '\n';
			}
		}

		else {

			vector<SrcDest> src_dest;

			int i = -1; //counter

			file << "-----PACKETS-----" << "\n";
			while (getline(inputFile, line)) {

				if (stoi(line) != -1) {
					i++;
					stringstream ss(line); // Turn the string into a stream.

					string token;

					vector<int> numbers;

					while (getline(ss, token, ' ')) {
						numbers.push_back(stoi(token));
					}

					src_dest.push_back(SrcDest { numbers[0], numbers[1] });
					file << "Source: " << src_dest[i].src << " | ";
					file << "Destination: " << src_dest[i].dest << '\n';
					file << "-----------------" << '\n';
				}

			}
			file << '\n';
		}
	}
}

/**
 * These methods are for TCP connections.
 * There are 2 methods for creating sockets: One for manager -> router and One for router -> manager
 * There are methods for sending and receiving which work via FD and work on both router and manager
 **/

//Create a manager side socket for connecting to routers.
int createTCPSocket(string destIp, string destPort) {
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
	file << "Socket created!" << endl;

	//Connect to server.
	int conErr = connect(fd, res->ai_addr, res->ai_addrlen);
	if (conErr < 0) {
		char const *p = "ERROR creating connection";
		error(p);
	}

	file << "Connection established." << endl;

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
	file << "Message sent." << endl;
}

//Try receiving from this TCP connection.
string receiveTCP(int fd) {
	char buffer[9999];
	memset(&buffer, 0, sizeof(buffer));
	//receive file size
	file << "Waiting for message." << endl;
	while (strlen(buffer) == 0) {
		recv(fd, (char*) &buffer, sizeof(buffer), 0);
	}
	file << "Received a message." << endl;
	string str(buffer);
	file << str << endl;
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
	file << "Listening on: <" << inet_ntoa(*addr_list[0]) << ", " << port << ">" << endl;

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

vector<Link> getRouterNeighbors(int routerID){
	vector<Link> neighbors;
	for(size_t i = 0; i <= allLinks.size(); i++){
		if(allLinks[i].src == routerID){
			neighbors.push_back(allLinks[i]);
		}
	}
	return neighbors;
}

//This will call a router task; ie. building a router and running router class tashs
void buildRouter(int tid) {
	file << "Router" << tid << " thread created and starting!" << '\n';

	//This is simulating a thread taking time to complete a task and to make sure everything finished before quitting
	file << "Getting neighbors for:  " << tid << '\n';
	Router router(getRouterNeighbors(tid), tid);
	file << "Router" << tid << " created with allocated neighbors." << '\n';
	//Once it reaches this point, the thread will quit.
	file << "Router" << tid << " quitting!" << '\n';
}

//THis is for running manager based tasks (ie. Sending and receiving packets)
void runManager() {
	file << "This is manager time. Even though the line below is printed late, they SHOULD all be running concurrently." << endl;
}

//Create threads, num_threads passed via input file
void createRouterThreads(int num_threads) {
	thread t[num_threads];

	//Launch threads
	for (int i = 0; i < num_threads; i++) {
		t[i] = thread(buildRouter, i);
	}

	//Do manager related tasks (not sure about the placement here, but it should be correct because once the threads are created we want to do something with them)
	runManager();

	//Join finished threads
	for (int i = 0; i < num_threads; ++i) {
		t[i].join();
	}

	file << "Manager is aware that all threads have finished completely and can now exit. Doing that." << endl;

}

//Will parse command line arguments and start processes
int main(int argc, const char * argv[]) {
	file.open("Manager.txt");
	if (argc >= 2) {
		file << "Opening the Filename: " << argv[1] << '\n';
		parseFile(argv[1]);
	}

	else
		cout << "incorrect number of arguments" << '\n';

	//Hardcoded for testing, this will be pulled from the input file.
	int numberOfThreads = nodes;

	// nodes is how it is named in header file, check for more data structure

	//Start the process, I dont think anything should be below this call
	file << "Creating: " << numberOfThreads << " threads." << '\n';
	createRouterThreads(numberOfThreads);

	return 0;
}
