/*Router.cpp
 * Created by Chris Marques, Ryan Cox, and Nikolay Radaev
 */
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
#include <bits/stdc++.h>
#include <algorithm>
#include "project3.h"

using namespace std;

# define INF 0x3f3f3f3f

Router::Router(int id) {
	filename = "Router" + to_string(id) + ".log";
	file.open(filename);
}

int Router::run(int port) {

	file << getTime() << ": " << "Router starting up!" << endl;
	UDPPort = port;
	createUDPSocket(UDPPort);
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

	string nodes = message.substr(1, message.length() - 1);
	replace(nodes.begin(), nodes.end(), '*', ' ');
	vector<string> nds;
	stringstream ds(nodes);
	string tms;
	while (ds >> tms) {
		nds.push_back(tms);
	}
	total = stoi(nds[0]);
	plain.push_back(nds[1]);
	string trucMessage = nds[1];

	replace(trucMessage.begin(), trucMessage.end(), ':', ' ');
	vector < string > links;
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

//Dijkstras algoritm and shortest hop building. Modified from the implementation of Dijkstra's from http://www.geeksforgeeks.org/dijkstras-shortest-path-algorithm-using-set-in-stl/
Graph::Graph(int V) {
	this->V = V;
	adj = new list<pair<int, int> > [V];
}

void Graph::addEdge(int u, int v, int w) {
	adj[u].push_back(make_pair(v, w));
	adj[v].push_back(make_pair(u, w));
}

vector<int> Graph::shortestPath(int src) {
	vector<int> nextHop;

	int tempArray[500];
	fill_n(tempArray, 500, -1);
	set<pair<int, int> > setds;
	vector<int> dist(V, INF);

	setds.insert(make_pair(0, src));
	dist[src] = 0;
	while (!setds.empty()) {
		pair<int, int> tmp = *(setds.begin());
		setds.erase(setds.begin());
		int u = tmp.second;

		list<pair<int, int> >::iterator i;
		for (i = adj[u].begin(); i != adj[u].end(); ++i) {
			int v = (*i).first;
			int weight = (*i).second;

			if (dist[v] > dist[u] + weight) {
				tempArray[v] = u;
				if (dist[v] != INF)
					setds.erase(setds.find(make_pair(dist[v], v)));

				dist[v] = dist[u] + weight;
				setds.insert(make_pair(dist[v], v));
			}
		}
	}
	tempArray[src] = 0;
	for (int i = 0; i < 500; i++) {
		if (tempArray[i] != -1) {
			nextHop.push_back(tempArray[i]);
		} else {
			break;
		}
	}
	return nextHop;
}

int recursionMF(vector<int> links, int start, int src, int dest) {
	size_t size = links.size();
	vector<string> linkStrings;
	if (dest == start) {
		return src;
	} else {
		for (size_t i = 0; i < size; i++) {
			if (static_cast<int>(i) != start) {
				string temp = to_string(links[i]) + " " + to_string(static_cast<int>(i));
				linkStrings.push_back(temp);
			}
		}
		for (size_t i = 0; i < linkStrings.size(); i++) {
			stringstream ss(linkStrings[i]);
			string a, b;
			ss >> a;
			ss >> b;
			if (atoi(b.c_str()) == dest) {
				return recursionMF(links, start, atoi(b.c_str()), atoi(a.c_str()));
			}
		}
	}
	return -1;
}

vector<int> algorithm(vector<Link> forwardTable, int numRouters, int thisRouter) {
	vector<int> array(numRouters);

	Graph g(numRouters);
	for (size_t i = 0; i < forwardTable.size(); i++) {
		g.addEdge(forwardTable[i].src, forwardTable[i].dest, forwardTable[i].cost);
	}
	vector<int> routerPaths = g.shortestPath(thisRouter);
	for (int i = 0; i < numRouters; i++) {
		int number = recursionMF(routerPaths, thisRouter, thisRouter, i);
		array[i] = number;
	}
	return array;
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
	file << getTime() << ": " << getTime() << ": " << "Message sent." << endl;
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
	while (strlen(buffer) == 0) {
		recv(fd, (char*) &buffer, sizeof(buffer), 0);
	}
	string str(buffer);
	file << getTime() << ": " << "Received a message: " << str << endl;
	return str;
}
bool Router::dupe(Link link) {
	int linkSrc = link.src;
	int linkDest = link.dest;
	int linkCost = link.cost;

	for (size_t i = 0; i < forwardTable.size(); i++) {
		int fSrc = forwardTable[i].src;
		int fDest = forwardTable[i].dest;
		int fCost = forwardTable[i].cost;
		if ((linkSrc == fSrc) && (linkDest == fDest) && (linkCost == fCost)) {
			return true;
		}
	}

	return false;
}

void Router::buildForward() {
	for (size_t i = 0; i < plain.size(); i++) {
		string test = plain[i];
		replace(test.begin(), test.end(), ':', ' ');
		vector < string > links;
		stringstream ss(test);
		string tmp;
		while (ss >> tmp) {
			links.push_back(tmp);
		}
		for (size_t j = 0; j < links.size(); j++) {
			replace(links[j].begin(), links[j].end(), ',', ' ');
			vector<string> LttP;
			stringstream ss2(links[j]);
			string tmp2;
			while (ss2 >> tmp2) {
				LttP.push_back(tmp2);
			}
			Link link = Link(atoi(LttP[0].c_str()), atoi(LttP[1].c_str()), atoi(LttP[2].c_str()));
			if (!dupe(link)) {
				forwardTable.push_back(link);
			}
		}

	}
}
bool Router::sendPacket(string message) {
	//Parse message
	string newMsg = message.substr(1, message.length());
	int dest = atoi(newMsg.c_str());

	//For when you are the destination
	if (dest == nodeNum) {
		file << endl;
		file << getTime() << ": " << "****A Packet Destined for this Router has been received!****" << endl;
		file << endl;
		return false;
	} else {

		//Get the hop from RT
		int hop = routTable[dest];

		//Build the sockets
		int fd;
		int fds = 10000 + nodeNum;
		if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
			char const *p = "ERROR creating socket";
			error(p);
		}
		struct sockaddr_in myaddr;
		memset((char *) &myaddr, 0, sizeof(myaddr));
		myaddr.sin_family = AF_INET;
		myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
		myaddr.sin_port = htons(fds);
		while (bind(fd, (struct sockaddr *) &myaddr, sizeof(myaddr)) < 0) {
			fds++;
			myaddr.sin_port = htons(fds);
		}

		//Send the packet
		sendUDP(fd, (hop + 6000), TCPIP, (">" + newMsg));
		file << getTime() << ": " << "Packet Sent to Router: " << hop << endl;
		//Receiving an ACK
		int recvlen;
		struct sockaddr_in remaddr;
		socklen_t addrlen = sizeof(remaddr);
		char buf[9999];
		memset(&buf, 0, sizeof(buf));
		file << getTime() << ": " << "Waiting for Packet ACK from: " << hop << endl;
		recvlen = recvfrom(fd, buf, 9999, 0, (struct sockaddr *) &remaddr, &addrlen);
		string str;
		buf[recvlen] = 0;
		str = buf;
		file << getTime() << ": " << "Received ACK for Sent Packet to: " << hop << "\n" << endl;
		close(fd);
		return true;
	}
}

//Create a manager side socket for connecting to routers.
int Router::createTCPSocket(string destIp, string destPort) {
	file << "\n-----TCP STARTUP-----" << endl;
	file << getTime() << ": " << "Trying to create TCP Socket" << endl;

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
	file << getTime() << ": " << "TCP Socket Created!" << endl;
	//Connect to server.
	int conErr = connect(fd, res->ai_addr, res->ai_addrlen);
	if (conErr < 0) {
		char const *p = "ERROR creating connection";
		error(p);
	}

	file << getTime() << ": " << "Connection established with server. Port: " << destPort << "\n" << endl;
	nodeNum = (atoi(destPort.c_str()) - 7000);
	bool finished = false;
	while (!finished) {
		file << getTime() << ": " << "Waiting on TCP" << endl;
		string message = receiveTCP(fd);
		if (message.at(0) == '*') {
			if (message != "*None") {
				neighbors = messageToLinks(message);
				file << "\n-----RECEIVED NEIGHBORING LINKS OVER TCP-----" << endl;
				for (size_t i = 0; i < neighbors.size(); i++) {
					file << getTime() << ": " << "SRC: " << neighbors[i].src << " Dest: " << neighbors[i].dest << " Cost: " << neighbors[i].cost << endl;
				}
			}
			file << getTime() << ": " << "This router is node: " << nodeNum << endl;
			struct sockaddr_in sin;
			file << getTime() << ": " << "Sending ACK back to manager." << endl;
			sendBack(fd, sin, "Ready!");
			file << getTime() << ": " << "ACK sent to manager." << endl;
			file << "\n-----AWAITING START SIGNAL-----\n" << endl;
		} else if (message.at(0) == '$') {
			struct sockaddr_in sin;
			file << getTime() << ": " << "Received start" << endl;

			file << "\n-----SENDING NEIGHBORING LINKS TO NEIGHBORS-----\n" << endl;
			linkRequest();
			sendBack(fd, sin, "DONE");
			routers.push_back(nodeNum);
			for (size_t i = 0; i < neighbors.size(); i++) {
				forwardTable.push_back(neighbors[i]);
			}
			file << getTime() << ": " << "Finished linking to neighbors." << endl;
			file << "\n-----Awaiting BROADCAST signal-----\n" << endl;
		} else if (message.at(0) == '#') {
			struct sockaddr_in sin;

			file << getTime() << ": " << "Received broadcast signal." << endl;
			file << "\n-----BROADCASTING-----\n" << endl;
			broadcast();
			sendBack(fd, sin, "READY");
			while (routers.size() < 10) {
				usleep(400000);
			}
			buildForward();
			file << "\n-----ALL LINKS RECEIVED-----\n" << endl;
			for (size_t i = 0; i < routers.size(); i++) {
				file << getTime() << ": " << "Router number: " << routers[i] << " --- " << plain[i] << endl;
			}
			file << "\n-----TRANSLATED FORWARDING TABLE-----\n" << endl;
			for (size_t i = 0; i < forwardTable.size(); i++) {
				file << getTime() << ": " << "src: " << forwardTable[i].src << "  dest: " << forwardTable[i].dest << "  cost: " << forwardTable[i].cost << endl;
			}
			file << "\n-----Awaiting ROUTING TABLE signal-----\n" << endl;
		} else if (message.at(0) == '!') {
			struct sockaddr_in sin;

			file << getTime() << ": " << "Building Routing Table" << endl;

			routTable = algorithm(forwardTable, total, nodeNum);
			file << getTime() << ": " << "Routing Table Created" << endl;

			file << "\n-----TRANSLATED ROUTING TABLE-----\n" << endl;
			for (size_t i = 0; i < routTable.size(); i++) {
				file << getTime() << ": " << "Destination: " << i << ", Next Hop: " << routTable[i] << endl;
			}
			sendBack(fd, sin, "RTDone");
			file << getTime() << ": " << "Finished Routing Table." << endl;
			file << "\n-----Awaiting PACKET signal and/or QUIT SIGNAL-----\n" << endl;
		} else if (message.at(0) == '~') {
			file << getTime() << ": " << "Received a Packet Signal" << endl;
			struct sockaddr_in sin;
			file << getTime() << ": " << "Trying to Send Packet onwards.." << endl;
			sendPacket(message);
			sendBack(fd, sin, "PackSent");
			file << getTime() << ": " << "Sent a TCP ACK on Packet Sent." << endl;
			file << getTime() << ": " << "Packet Sending Complete.\n" << endl;
			file << "-------------------------" << endl;
		} else if (message.at(0) == '^') {
			struct sockaddr_in sin;
			file << "\n-----QUITTING-----\n" << endl;
			file << getTime() << ": " << "Received QUIT Signal." << endl;
			sendBack(fd, sin, "QUITACK");
			file << getTime() << ": " << "Exiting.";
			//This sleep is needed, timing; unsure
			sleep(1);
			close(fd);
			exit(0);
		} else {

		}
	}
	return fd;
}

int receivedFromFD2;
struct sockaddr_in receivedFromAddr2;
void Router::linkRequest() {
	for (size_t i = 0; i < neighbors.size(); i++) {
		int linkport;
		if (neighbors[i].src == nodeNum) {
			linkport = neighbors[i].dest + 6000;
		} else {
			linkport = neighbors[i].src + 6000;
		}
		file << getTime() << ": " << "Sending Link to:" << (linkport - 6000) << endl;
		string s = to_string(nodeNum);
		string linkmsg = "%" + s;
		int fd;
		int fds = 8000 + nodeNum;
		if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
			char const *p = "ERROR creating socket";
			error(p);
		}
		struct sockaddr_in myaddr;
		memset((char *) &myaddr, 0, sizeof(myaddr));
		myaddr.sin_family = AF_INET;
		myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
		myaddr.sin_port = htons(fds);

		if (bind(fd, (struct sockaddr *) &myaddr, sizeof(myaddr)) < 0) {
			char const *p = "ERROR binding socket";
			error(p);
		}
		sendUDP(fd, linkport, TCPIP, linkmsg);
		int recvlen;
		struct sockaddr_in remaddr;
		socklen_t addrlen = sizeof(remaddr);
		char buf[9999];
		memset(&buf, 0, sizeof(buf));
		//receive file size
		recvlen = recvfrom(fd, buf, 9999, 0, (struct sockaddr *) &remaddr, &addrlen);
		//These are storing the information for sending back to the "client"
		receivedFromAddr2 = remaddr;
		receivedFromFD2 = fd;
		string str;
		buf[recvlen] = 0;
		str = buf;
		file << getTime() << ": " << "Linked with: " << (linkport - 6000) << endl;
		close(fd);
	}
}

int receivedFromFD3;
struct sockaddr_in receivedFromAddr3;
void Router::broadcast() {
	for (size_t i = 0; i < neighbors.size(); i++) {
		int linkport;
		if (neighbors[i].src == nodeNum) {
			linkport = neighbors[i].dest + 6000;
		} else {
			linkport = neighbors[i].src + 6000;
		}
		file << getTime() << ": " << "Sending Broadcast to:" << (linkport - 6000) << endl;
		string s = to_string(nodeNum);
		string linkmsg = "+" + s + "!" + s + "?";
		for (size_t j = 0; j < neighbors.size(); j++) {
			string src = to_string(neighbors[j].src);
			string dest = to_string(neighbors[j].dest);
			string cost = to_string(neighbors[j].cost);
			linkmsg += src + "," + dest + "," + cost + ":";
		}
		linkmsg = linkmsg.substr(0, linkmsg.length() - 1);
		int fd;
		int fds = 8000 + nodeNum;
		if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
			char const *p = "ERROR creating socket";
			error(p);
		}
		struct sockaddr_in myaddr;
		memset((char *) &myaddr, 0, sizeof(myaddr));
		myaddr.sin_family = AF_INET;
		myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
		myaddr.sin_port = htons(fds);

		if (bind(fd, (struct sockaddr *) &myaddr, sizeof(myaddr)) < 0) {
			char const *p = "ERROR binding socket";
			error(p);
		}
		sendUDP(fd, linkport, TCPIP, linkmsg);
		int recvlen;
		struct sockaddr_in remaddr;
		socklen_t addrlen = sizeof(remaddr);
		char buf[9999];
		memset(&buf, 0, sizeof(buf));
		//receive file size
		recvlen = recvfrom(fd, buf, 9999, 0, (struct sockaddr *) &remaddr, &addrlen);
		//These are storing the information for sending back to the "client"
		receivedFromAddr3 = remaddr;
		receivedFromFD3 = fd;
		string str;
		buf[recvlen] = 0;
		str = buf;
		file << getTime() << ": " << "Ack broadcast from: " << (linkport - 6000) << endl;
		close(fd);
	}

}

void Router::reBroadcast(string str, int current) {
	vector<Link> temp;
	int sender;
	str = str.substr(1, str.length());
	replace(str.begin(), str.end(), '!', ' ');
	vector < string > links;
	stringstream ss(str);
	string tmp;
	while (ss >> tmp) {
		links.push_back(tmp);
	}
	sender = stoi(links[0]);
	string node = to_string(nodeNum);
	string resend = "+" + node + "!" + links[1];
	str = links[1];
	replace(str.begin(), str.end(), '?', ' ');
	vector<string> others;
	stringstream sothers(str);
	string tmpothers;
	while (sothers >> tmpothers) {
		others.push_back(tmpothers);
	}
	int owner = stoi(others[0]);
	if (find(routers.begin(), routers.end(), owner) != routers.end()) {
		;
	} else {
		//cout << nodeNum << " " << others[1] << " " << owner << endl;
		routers.push_back(owner);
		plain.push_back(others[1]);
		reBroadcast2(sender, resend, current);
	}

}

int receivedFromFD4;
struct sockaddr_in receivedFromAddr4;
void Router::reBroadcast2(int sender, string msg, int current) {
	for (size_t i = 0; i < neighbors.size(); i++) {
		if ((neighbors[i].src != sender) || (neighbors[i].dest != sender)) {
			int linkport;
			if (neighbors[i].src == nodeNum) {
				linkport = neighbors[i].dest + 6000;
			} else {
				linkport = neighbors[i].src + 6000;
			}
			int fd;
			int fds = 9000 + current;
			if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
				char const *p = "ERROR creating socket";
				error(p);
			}
			struct sockaddr_in myaddr;
			memset((char *) &myaddr, 0, sizeof(myaddr));
			myaddr.sin_family = AF_INET;
			myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
			myaddr.sin_port = htons(fds);
			while (bind(fd, (struct sockaddr *) &myaddr, sizeof(myaddr)) < 0) {
				fds++;
				myaddr.sin_port = htons(fds);
			}
			sendUDP(fd, linkport, TCPIP, msg);
			int recvlen;
			struct sockaddr_in remaddr;
			socklen_t addrlen = sizeof(remaddr);
			char buf[9999];
			memset(&buf, 0, sizeof(buf));
			//receive file size
			recvlen = recvfrom(fd, buf, 9999, 0, (struct sockaddr *) &remaddr, &addrlen);
			//These are storing the information for sending back to the "client"
			receivedFromAddr4 = remaddr;
			receivedFromFD4 = fd;
			string str;
			buf[recvlen] = 0;
			str = buf;
			file << getTime() << ": " << "2: Ack broadcast from: " << (linkport - 6000) << endl;
			close(fd);
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
	file << getTime() << ": " << "Trying to create UDP Socket" << endl;
	int fd;

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		char const *p = "ERROR creating socket";
		error(p);
	}
	UDPfd = fd;
	thread up(&Router::receiveUDP, this, fd, port);
	createTCPSocket(TCPIP, to_string(TCPPort));
	up.join();
	return (fd);
}

//Send to a UDP connection over a fd
void Router::sendUDP(int fd, int port, string destIp, string message) {
	struct sockaddr_in servaddr;
	char msg[9999];

	//Writing a message to buffer
	memset(&msg, 0, sizeof(msg));
	strcpy(msg, message.c_str());

	//Set up servaddr
	memset((char*) &servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);

	//Copy the destIP to the servaddr stuct
	inet_pton(AF_INET, destIp.c_str(), &(servaddr.sin_addr));

	//Send a message
	if (sendto(fd, msg, strlen(msg), 0, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
		char const *p = "ERROR sending message";
		error(p);
	}

	file << getTime() << ": " << "Message sent!" << endl;
}

//Start receiving UDP messages, complete with ACK handling
int receivedFromFD;
struct sockaddr_in receivedFromAddr;
void Router::receiveUDP(int fd, int port) {

	char hostname[128];
	struct sockaddr_in myaddr;
	int current = 0;
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
	file << getTime() << ": " << "UDP socket created: <" << inet_ntoa(*addr_list[0]) << ", " << port << ">" << endl;

	memset((char *) &myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(port);

	if (bind(fd, (struct sockaddr *) &myaddr, sizeof(myaddr)) < 0) {
		char const *p = "ERROR binding socket";
		error(p);
	}
	file << getTime() << ": " << "UDP Listening." << endl;

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
		}
		if (str.at(0) == '%') {
			//This is where the router will send information to others. Will need to call some function to construct a msg
			sendBack(receivedFromFD, receivedFromAddr, "AWKLINK");
		}
		if (str.at(0) == '+') {
			//This is where the router will send information to others. Will need to call some function to construct a msg
			sendBack(receivedFromFD, receivedFromAddr, "AWKBROADCAST");
			current += 5;
			usleep(200000);
			thread up(&Router::reBroadcast, this, str, current);

			up.detach();
		}
		if (str.at(0) == '>') {
			sendBack(receivedFromFD, receivedFromAddr, "AWKPACK");
			sendPacket(str);
		}
	}
}
