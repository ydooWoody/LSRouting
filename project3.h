#ifndef project3
#define project3
#include <vector>
#include <iostream>
#include <fstream>
#include <list>
using namespace std;

void error(char const *msg);
string getTime();

class Link {
public:
	int src;
	int dest;
	int cost;
	Link(int src, int dest, int cost);
};

class Graph {
	int V;    // No. of vertices

	// In a weighted graph, we need to store vertex
	// and weight pair for every edge
	list<pair<int, int> > *adj;

public:
	Graph(int V);  // Constructor

	// function to add an edge to graph
	void addEdge(int u, int v, int w);

	// prints shortest path from s
	vector<int> shortestPath(int s);
};

class Router {
public:
	vector<int> routTable;
	int nodeNum;
	int total;
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
	vector<string> plain;
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
	void reBroadcast(string str, int current);
	void reBroadcast2(int sender, string msg, int current);
	void buildForward();
	bool dupe(Link link);
	bool sendPacket(string message);

};

class SrcDest {
public:
	int src;
	int dest;
};

#endif
