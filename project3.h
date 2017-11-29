#ifndef project3
#define project3
#include <vector>
#include <iostream>
#include <fstream>
using namespace std;

void error(char const *msg);

class Link {
public:
	int src;
	int dest;
	int cost;
	Link(int src, int dest, int cost);
};

class Router {
public:
	int nodeNum;
	string TCPIP;
	int TCPPort;
	int UDPPort;
	int UDPfd;
	int links;
	bool linkawk;
	string filename;
	ofstream file;
	vector<Link> neighbors;
	vector<Link> forwardTable;
	vector<int> routers;
	Router(int id);
	int run(int port);
	void setIP(string ip);
	void setPort(int port);
	int createTCPSocket(string destIp, string destPort);
	void sendTCP(int fd, string message);
	string receiveTCP(int fd);
	vector<Link> messageToLinks(string message);
	int createUDPSocket(int port);
	void sendUDP(int fd, int port, string destIp, string message);
	void receiveUDP(int fd, int port);
	void linkRequest();
	void broadcast();
	void reBroadcast(string str);

};

class SrcDest {
public:
	int src;
	int dest;
};

#endif
