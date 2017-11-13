#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <cstring>
#include <sstream>
#include <vector>
#include <fstream>
#include <time.h>
#include "project3.h"

using namespace std;

void parseFile (string filename) {
	ifstream inputFile(filename);
	if (!inputFile.is_open()) {
		cout << "Error opening the input file" << '\n';
		exit(1);
	}
		
	cout << "Parsing a file" << '\n';
		
	string line = "";
		
	while (getline(inputFile, line)) {
		if (stoi(line) != -1) {
			nodes = stoi(line);
			cout << "Nodes: " << nodes << '\n';
				
			vector <Router> router;
				
			for (int i = 0; i < nodes; i++) {
					
				getline(inputFile, line);
					
				stringstream ss(line); // Turn the string into a stream.
					
				string token;
					
				vector<int> numbers;

				while (getline(ss, token, ' ')) {
					numbers.push_back(stoi(token));
				}
					
				router.push_back(Router {numbers[0], numbers[1], numbers[2]} );
					
				cout << "First: " << router[i].first << " | ";
				cout << "Second: " << router[i].second << " | ";
				cout << "Cost: " << router[i].cost << '\n';
				cout << "-----------------" << '\n';
			}
		}
		
		else {
			
			vector <SrcDest> src_dest;
			
			int i = -1; //counter
			
			while (getline(inputFile, line)) {

				if (stoi(line) != -1) {
					i++;
					stringstream ss(line); // Turn the string into a stream.
						
					string token;
					
					vector<int> numbers;

					while (getline(ss, token, ' ')) {
						numbers.push_back(stoi(token));
					}
					
					src_dest.push_back(SrcDest {numbers[0], numbers[1]} );
					cout << "Source: " << src_dest[i].src << " | ";
					cout << "Destination: " << src_dest[i].dest << '\n';
					cout << "-----------------" << '\n';
				}
					
			}
		}
	}
}


int main (int argc, const char* argv[]) {
	if (argc >= 2) {
		cout << "Opening the Filename: " << argv[1] << '\n';
		parseFile(argv[1]);
	}
		
	else cout << "incorrect number of arguments" << '\n';

	return 0;
}