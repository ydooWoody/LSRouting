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
	string filename;
	ofstream file;
	vector<Link> neighbors;
	Router(vector<Link> neighbors, int id);
	int test();
};

class SrcDest {
public:
	int src;
	int dest;
};

#endif
