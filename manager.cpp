#include <iostream>
#include <thread>
#include <chrono>

using namespace std;

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
	//Hardcoded for testing, this will be pulled from the input file.
	int numberOfThreads = 10;

	//Start the process, I dont think anything should be below this call
	cout << "Creating: " << numberOfThreads << " threads." << endl;
	createRouterThreads(numberOfThreads);
}
