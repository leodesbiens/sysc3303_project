/*
B3G1 Project Iteration 1

Members:
Leo Desbiens
Jackson Fry
Ahmad Mohamad
Betselot Semaw
Ava Wiebe

Purpose: 
Iteration 1 demonstrates the following:
- 3 Threads: Elevator, Floor, and Scheduler.
- One complete cycle of passing data between threads:
	-> Floor receives input data from file.
	-> Floor passes data to Scheduler.
	-> Elevator receives data from Scheduler.
	-> Elevator passes data to Scheduler.
	-> Floor receives data from Scheduler.
	-> Process terminates.
- Importantly, the order of the passing is kept rigid. Elevator cannot grab Elevator data from the Scheduler -> Instead, Elevator must grab Floor data, and vice versa.

Note: when checking for info source / destination
	0: Floor
	1: Elevator
*/
//include dependencies
#include <chrono>
#include <thread>
#include <iostream>
#include<fstream>	
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mutex>
#include <condition_variable>

using namespace std;

//create a struct to hold all data 
typedef struct{
	int time = 0;
	int elevNum = -1;
	int floorNum = -1;
	int button = 0;
	int source = -1; // 0: Floor	1: Elevator
} data_t;


// Class Scheduler: Main purpose has not yet been implemented in this iteration. Currently serves as a location to store data that is being transferred between Floor and Elevator.
class Scheduler{
	private:
		data_t heldData; // Data held by this Scheduler
		bool empty = true;	// Stores if this Scheduler currently holds data
		mutex mtx;				
		condition_variable cv;
		
	public:
		Scheduler() : mtx(), empty(true), cv(), heldData() {}	//constructor for scheduler
		
		// Main function of this thread - not needed for this iteration
		void run()
		{
			cerr<<"Entered Scheduler."<<endl;
			while(1){};
		}
		
		// Stores data in the scheduler and outputs the source to the console
		void put(data_t newData)
		{
			//lock mutex
			unique_lock<mutex> lock(mtx);
			
			while(!empty) cv.wait(lock); // Wait if scheduler is full.
			
			heldData = newData; // Update currently held data

			// Output source of data message
			switch (heldData.source)
			{
				case 0: 
					cerr<<"Obtained data from floor"<<endl;
					break;
				case 1:
					cerr<<"Obtained data from elevator"<<endl;
					break;
				default: 
					cerr<<"Invalid source Exiting"<<endl;
					exit(1);
					break;
			}
			
			empty = false;	// Scheduler is now full
			cv.notify_all(); //notify waiting threads
			return;
		}
		
		// Returns data to a specified destination. Only allows data to be obtained if the source and destination are different (i.e, Floor cannot receive Floor data).
		data_t get(int destination)
		{
			
			//lock mutex
			unique_lock<mutex> lock(mtx);
			
			// Wait until scheduler has data that can be obtained by the calling thread (elevator cannot fetch elevator data, floor cannot fetch floor data)
			while(empty || (destination == heldData.source)) cv.wait(lock);
			
			// Temporary storage for data to be returned
			data_t dataOut;
			
			// In future iterations, heldData will become a list (or similar) of Data entries and must be popped to a temporary variable
			dataOut = heldData;
			
			// In future empty will not be set to true unless list is completely empty
			empty = true;
			
			cv.notify_all(); //notify waiting threads
			return dataOut; 
		}
};

// Class Floor receives input data from a file and passes it to the Scheduler.
class Floor{
	private:
		data_t floorData; // Data for this Floor
		int floorID; // ID of this Elevator - IDs start at 0
		Scheduler& scheduler; // Reference to the scheduler this floor interfaces with
		bool empty; // Stores if this floor is currently holding data -> in the future, this will be accomplished using a list or queue's "empty()" function.
		
	public:
		// Floor Constructor
		Floor(Scheduler& schedulerRef, int id) : scheduler(schedulerRef), floorID(id), floorData(), empty(1) {}
		
		void run(string inputFileName)
		{
			ifstream inputData(inputFileName);
			// Create input data
			// NOTE: this iteration will ONLY HAVE 1 input data in the test file as we only need to show data 
			// 		 being sent from the Floor to the Scheduler to the Elevator and back.
			string line;
			while(getline(inputData, line, ','))
			{
				floorData.time = stoi(line);
				getline(inputData, line, ',');
				floorData.elevNum = stoi(line);
				getline(inputData, line, ',');
				floorData.floorNum = stoi(line);
				getline(inputData, line);
				floorData.button = stoi(line);
				floorData.source = 0;
			}
			
			// Data has been fetched, Floor is now full.
			empty = false;
			
			while(1){
				if (!empty) {// Cannot put data in Scheduler if Floor has no data
					scheduler.put(floorData);
					empty = true;
					this_thread::sleep_for(chrono::milliseconds(500));	
				}
				
				if (empty) // Cannot grab data from Scheduler if Floor already has data
				{	
					floorData = scheduler.get(0);
					floorData.source=0;
					empty = false;
					cerr<<"Floor received data from scheduler"<<endl; 
					this_thread::sleep_for(chrono::milliseconds(500));
					
					cerr<<"Data has successfully been transferred to and from elevator. Exiting."<<endl; 
					exit(0); // This is for testing purposes, for this iteration we only need to show one pass to the elevator and back, so we exit the demo here.
				}
			}
		}
};

// Class Elevator receives data from the Scheduler and returns it, having modified the source to be from Elevator.
class Elevator{
	private:
		data_t elevData; // Data for this Elevator
		int elevID; // ID of this Elevator - IDs start at 100
		Scheduler& scheduler; // Reference to the scheduler this elevator interfaces with
		bool empty; // Stores if this elevator is currently holding data -> in the future, this will be accomplished using a list or queue's "empty()" function.
		
	public:
		// Elevator Constructor
		Elevator(Scheduler& schedulerRef, int id) : scheduler(schedulerRef), elevID(id), elevData(), empty(1) {}
		
		void run()
		{	
			while(1){
				
				if (!empty) // Cannot put data in Scheduler if Elevator has no data
				{
					cerr<<"Elevator sending to scheduler"<<endl;
					scheduler.put(elevData);
					empty = true;	
					this_thread::sleep_for(chrono::milliseconds(500));
				}
				if (empty) // Cannot grab data from Scheduler if Elevator already has data
				{	
					elevData = scheduler.get(1);
					elevData.source=1;
					empty = false;
					this_thread::sleep_for(chrono::milliseconds(500));
					cerr<<"Elevator received data from scheduler"<<endl; 
				}
			}
		}
};

int main(int argc, char *argv[]){	
	
	// Creating objects
	Scheduler scheduler;
	Floor floor(scheduler, 0);
	Elevator elev(scheduler, 100);
	
	// Creating threads
	cerr << "Creating Scheduler thread" << endl;
	thread schedulerThread(&Scheduler::run, &scheduler);
	cerr << "Created: " << schedulerThread.get_id()<<endl;
	
	cerr<< "Creating Floor thread"<<endl;
	// Set input file name to argument in run line (argv[1])
	thread floorThread(&Floor::run, &floor, argv[1]);
	cerr << "Created: " << floorThread.get_id()<<endl;
	
	cerr<< "Creating Elevator thread"<<endl;
	thread elevThread(&Elevator::run, &elev);
	cerr << "Created: " << elevThread.get_id()<<endl;
	
	// Joining threads
	schedulerThread.join();
	floorThread.join();
	elevThread.join();
	
	return 0;
}

