#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <vector>
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


//This will call a router task; ie. building a router and running router class tashs
void buildRouter(int tid) {
	cout << "Router" << tid << " created and starting!" << endl;

	//This is simulating a thread taking time to complete a task and to make sure everything finished before quitting
	cout << "Doing router " << tid << " stuff..." << endl;
	this_thread::sleep_for(chrono::seconds(tid * 4 % 3));

	//Once it reaches this point, the thread will quit.
	cout << "Router" << tid << " quitting!" << endl;
}

//THis is for running manager based tasks (ie. Sending and receiving packets)
void runManager() {
	cout << "This is manager time. Even though the line below is printed late, they SHOULD all be running concurrently." << endl;
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

	cout << "Manager is aware that all threads have finished completely and can now exit. Doing that." << endl;

}

//Will parse command line arguments and start processes
int main(int argc, const char * argv[]) {
	
	if (argc >= 2) {
		cout << "Opening the Filename: " << argv[1] << '\n';
		parseFile(argv[1]);
	}
	
	else cout << "incorrect number of arguments" << '\n';

	//Hardcoded for testing, this will be pulled from the input file.
	int numberOfThreads = nodes;
	
	// nodes is how it is named in header file, check for more data structure
	
	//Start the process, I dont think anything should be below this call
	cout << "Creating: " << numberOfThreads << " threads." << endl;
	createRouterThreads(numberOfThreads);

	return 0;
}