/*Manager.cpp
 * Created by Chris Marques, Ryan Cox, and Nikolay Radaev
 */
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
vector<SrcDest> packets;

ofstream file;
int nodes;
string thisIP;
int thisPort = 7000;
vector<int> fd_vector;
vector<int> ac_vector;

void error(char const *msg) {
	perror(msg);
	exit(1);
}

string getTime() {
	time_t now = time(NULL);
	tm * ptm = localtime(&now);
	char buffer[32];
	strftime(buffer, 32, "%a, %d.%m.%Y %H:%M:%S", ptm);
	return buffer;
}

Link::Link(int src, int dest, int cost) {
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
	cout << "Parsing File..." << endl;
	file << getTime() << ": " << "Parsing a file" << '\n';

	string line = "";
	file << "\n-----PARSING FINISHED-----\n" << endl;
	file << "\n-----LINKS PARSED-----\n" << endl;
	int i = 0;
	while (getline(inputFile, line)) {
		if (i == 0) {
			nodes = stoi(line);
			file << getTime() << ": " << "\nNODES: " << nodes << endl;
			getline(inputFile, line);
		}
		if (line.at(0) != '-') {

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
			i++;

		} else {

			int i = -1; //counter

			file << "\n-----PACKETS PARSED-----\n" << endl;
			while (getline(inputFile, line)) {

				if (stoi(line) != -1) {
					i++;
					stringstream ss(line); // Turn the string into a stream.

					string token;

					vector<int> numbers;

					while (getline(ss, token, ' ')) {
						numbers.push_back(stoi(token));
					}

					packets.push_back(SrcDest { numbers[0], numbers[1] });
					file << "Source: " << packets[i].src << " | ";
					file << "Destination: " << packets[i].dest << '\n';
				}

			}
			file << getTime() << ": " << "-----------------\n";
		}
	}
}

int acceptAny(int fds[], unsigned int count, struct sockaddr *addr, socklen_t *addrlen) {
	fd_set readfds;
	int maxfd, fd;
	unsigned int i;
	int status;

	FD_ZERO(&readfds);
	maxfd = -1;
	for (i = 0; i < count; i++) {
		FD_SET(fds[i], &readfds);
		if (fds[i] > maxfd)
			maxfd = fds[i];
	}
	status = select(maxfd + 1, &readfds, NULL, NULL, NULL);
	if (status < 0)
		return -1;
	fd = -1;
	for (i = 0; i < count; i++)
		if (FD_ISSET(fds[i], &readfds)) {
			fd = fds[i];
			break;
		}
	if (fd == -1)
		return -1;
	else
		return accept(fd, addr, addrlen);
}

/**
 * These methods are for TCP connections.
 * There are 2 methods for creating sockets: One for manager -> router and One for router -> manager
 * There are methods for sending and receiving which work via FD and work on both router and manager
 **/
//Create a router side TCP socket for connecting to the manager.
int createTCPSocket(string port) {
	//Various variables for setting up socket and getting IP from socket, address info, etc.
	int sockfd, bindErr, lisErr;
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
	thisIP = inet_ntoa(*addr_list[0]);
	file << getTime() << ": " << "Listening on: <" << thisIP << ", " << port << ">" << "\n\n";
	return sockfd;
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
	file << getTime() << ": " << "Message sent." << endl;
}

//Try receiving from this TCP connection.
string receiveTCP(int fd) {
	char buffer[9999];
	memset(&buffer, 0, sizeof(buffer));
	//receive file size
	file << getTime() << ": " << "Waiting for message." << endl;
	while (strlen(buffer) == 0) {
		recv(fd, (char*) &buffer, sizeof(buffer), 0);
	}
	file << getTime() << ": " << "Received a message." << endl;
	string str(buffer);
	file << getTime() << ": " << str << endl;
	return str;
}

string getRouterNeighbors(int routerID) {
	string nd = to_string(nodes);
	string ret = "*" + nd + "*";
	vector<Link> neighbors;
	for (size_t i = 0; i < allLinks.size(); i++) {
		if ((allLinks[i].src == routerID) || allLinks[i].dest == routerID) {
			neighbors.push_back(allLinks[i]);
		}
	}
	if (neighbors.size() == 0) {
		return "*None";
	}
	for (size_t i = 0; i < neighbors.size(); i++) {
		ret = ret + to_string(neighbors[i].src) + "," + to_string(neighbors[i].dest) + "," + to_string(neighbors[i].cost) + ":";
	}
	ret = ret.substr(0, ret.length() - 1);
	file << getTime() << ": " << "MESSAGE TO SEND TO ROUTER:" << ret << "" << endl;
	return ret;
}

//This will call a router task; ie. building a router and running router class tashs
void buildRouter(int tid) {
	//Create the router object
	file << getTime() << ": " << "Router" << tid << " thread created and starting!" << '\n';
	Router router(tid);
	//Create the socket for the manager talking to a router
	string routerPort = "" + (thisPort + tid);
	router.setIP(thisIP);
	router.setPort(thisPort + tid);
	router.run(thisPort + tid - 1000);
	file << getTime() << ": " << "Router" << tid << " created and given TCP info." << '\n';
	//Once it reaches this point, the thread will quit.
	file << getTime() << ": " << "Router" << tid << " quitting!" << '\n';
}

//THis is for running manager based tasks (ie. Sending and receiving packets)
void runManager(int num_threads) {
	bool linksSent = false, tablesDone = false, broadcast = false, table = false, sent = false, quitting = false;
	int waitingOn = num_threads, waitingOnFWD = num_threads, waitingOnBroad = num_threads, waitingOnTable = num_threads, waitingOnSent = packets.size(), waitingOnQuit = num_threads;
	cout << "Creating Routers..." << endl;

	for (int i = 0; i < num_threads; i++) {
		file << "\n-----ROUTER " << i << " SOCKET CREATION-----\n";
		file << getTime() << ": " << "Creating socket on port: " << (thisPort + i) << '\n';
		fd_vector.push_back(createTCPSocket(to_string(thisPort + i)));
	}
	int accepted;
	int* fds = &fd_vector[0];
	struct sockaddr_storage their_addr;
	socklen_t addr_size;
	addr_size = sizeof their_addr;
	//Send and receive link information
	cout << "Linking TCP To Routers..." << endl;
	while (!linksSent) {
		//Accept initial connections
		accepted = acceptAny(fds, num_threads, (struct sockaddr *) &their_addr, &addr_size);
		//Solidify connections for later
		ac_vector.push_back(accepted);
		//Assuming all is well, start sending and receiving
		if (accepted != -1) {
			//Get port number from accepted
			struct sockaddr_in sin;
			socklen_t addrlen = sizeof(sin);
			int local_port;
			if (getsockname(accepted, (struct sockaddr *) &sin, &addrlen) == 0 && sin.sin_family == AF_INET && addrlen == sizeof(sin)) {
				local_port = ntohs(sin.sin_port);
			}
			file << "\n-----ROUTER " << to_string(local_port - 7000) << " TCP LINK MESSAGING-----\n";
			file << getTime() << ": " << "Accepted a connection on port: " << local_port << endl;
			sendTCP(accepted, getRouterNeighbors(local_port - 7000));
			string newMessage = receiveTCP(accepted);
			if (newMessage == "Ready!") {
				waitingOn--;
				if (waitingOn <= 0) {
					linksSent = true;
				}
			} else {
				newMessage == "";
			}
		}
	}
	file << "\n-----SETUP COMPLETED, STARTING LINK ESTABLISHMENT-----\n";
	cout << "Linking UDP Routers..." << endl;
	while (!tablesDone) {
		file << getTime() << ": " << "Sending Start Messages" << endl;
		for (size_t i = 0; i < ac_vector.size(); i++) {
			//sleep(1);
			sendTCP(ac_vector[i], "$START");
			if (receiveTCP(ac_vector[i]) == "DONE") {
				waitingOnFWD--;
				if (waitingOnFWD == 0) {
					tablesDone = true;
					break;
				}
			}
		}
	}
	file << "\n-----LINK ESTABLISHMENT COMPLETED, STARTING BROADCAST-----\n";
	cout << "Broadcasting LSPs..." << endl;
	while (!broadcast) {
		file << getTime() << ": " << "Sending Broadcast Messages" << endl;
		for (size_t i = 0; i < ac_vector.size(); i++) {
			//sleep(1);
			sendTCP(ac_vector[i], "#BROADCAST");
			if (receiveTCP(ac_vector[i]) == "READY") {
				waitingOnBroad--;
				if (waitingOnBroad == 0) {
					broadcast = true;
					break;
				}
			}
		}
	}
	cout << "Creating Routing Tables..." << endl;
	while (!table) {
		file << "\n-----BROADCASTING COMPLETED, STARTING ROUTING TABLE CREATION-----\n" << endl;
		for (size_t i = 0; i < ac_vector.size(); i++) {
			//sleep(1);
			sendTCP(ac_vector[i], "!Table");
			if (receiveTCP(ac_vector[i]) == "RTDone") {
				waitingOnTable--;
				if (waitingOnTable == 0) {
					table = true;
					break;
				}
			}
		}
	}
	//sleep(5);
	cout << "Sending Packets..." << endl;
	while (!sent) {
		file << "\n-----ROUTING TABLES CREATED, SENDING PACKETS-----\n" << endl;
		for (size_t i = 0; i < packets.size(); i++) {
			sleep(1);
			string mess = "~" + to_string(packets[i].dest);
			sendTCP(ac_vector[packets[i].src], mess);
			file << getTime() << ": " << "Sent Packet: " << i+1 << endl;
			if (receiveTCP(ac_vector[packets[i].src]) == "PackSent") {
				waitingOnSent--;
				if (waitingOnSent == 0) {
					sent = true;
					break;
				}
			}
		}
	}
	//sleep(5);
	cout << "Quitting..." << endl;
	while (!quitting) {
		file << "\n-----PACKETS HAVE BEEN SENT, SENDING QUITs TO ROUTERS-----\n" << endl;
		for (size_t i = 0; i < ac_vector.size(); i++) {
			sendTCP(ac_vector[i], "^quit");
			if (receiveTCP(ac_vector[i]) == "QUITACK") {
				waitingOnQuit--;
				if (waitingOnQuit == 0) {
					quitting = true;
					break;
				}
			}
		}
	}
	for (size_t i = 0; i < ac_vector.size(); i++) {
		close(ac_vector[i]);
	}
}

//Create threads, num_threads passed via input file
void createRouterThreads(int num_threads) {
	thread t[num_threads];

//Launch threads
	for (int i = 0; i < num_threads; i++) {
		t[i] = thread(buildRouter, i);
	}

//Do manager related tasks (not sure about the placement here, but it should be correct because once the threads are created we want to do something with them)
	runManager(num_threads);
	file << "\n-----ROUTERS HAVE QUIT, QUITTING-----\n" << endl;
	file << getTime() << ": " << "DONE";

//Join finished threads
	for (int i = 0; i < num_threads; ++i) {
		t[i].join();
	}

	exit(0);
}

//Will parse command line arguments and start processes
int main(int argc, const char * argv[]) {
	file.open("Manager.log");
	if (argc >= 2) {
		file << "\n-----STARTING-----\n" << endl;
		file << getTime() << ": " << "Opening the Filename: " << argv[1] << '\n';
		parseFile(argv[1]);
	}

	else
		cout << "incorrect number of arguments" << '\n';

//Hardcoded for testing, this will be pulled from the input file.
	int numberOfThreads = nodes;

// nodes is how it is named in header file, check for more data structure

//Start the process, I dont think anything should be below this call
	file << "\n-----CREATING " << numberOfThreads << " THREADS-----\n" << endl;
	createRouterThreads(numberOfThreads);

	return 0;
}
