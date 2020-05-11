#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <vector>
#include <iomanip>
#include <getopt.h>
#include <ctime>
#include <mutex>
#include <condition_variable>

/*	In a just world, we could simply  use posix semaphores like  most of the known
	universe. But... Apple. Apple did away with posix semaphore because... Well...
	Apple.

	Therefore, to avoid c++ 20, we will build our own semaphore out of a mutex and
	a condition variable.
*/

using namespace std;

int32_t maximum_number_of_resources = 4;
int32_t currently_allocated_resources = 0;
int32_t number_of_threads = 8;
bool keep_going = true;
mutex myM;

/* Something goes here in order to improve VeryBrokenWorker()
*/

/* Then there's this:
*/

class Semaphore {
	private:
		mutex m;
		condition_variable c;
		int32_t count = 0;

	public:
		Semaphore(int32_t initial_value) {
			count = initial_value;
		}

		void Post() {
			m.lock();
			++count;
			c.notify_one();
			m.unlock();
		}

		int32_t Wait() {
			{
				unique_lock<mutex> lock(m);
				while(count <= 0){
					c.wait(lock);
				}
				--count;
			}
			return count;
		}
}; 

Semaphore sem(maximum_number_of_resources);

void FixedWorker(int32_t tid) {
	while(keep_going){
		int32_t my_resource = sem.Wait();
		myM.lock();
		cout << "Thread: " << setw(3) << tid << " get resources";
		if(my_resource < 0){
			cout << " TOO SMALL\n";
		} else if(my_resource > maximum_number_of_resources){
			cout << " TOO LARGE\n";
		} else{
			cout << endl;
		}
		myM.unlock();
		this_thread::sleep_for(chrono::milliseconds(rand() % 100 + 10));
		sem.Post();
		myM.lock();
		cout << "Thread: " << setw(3) << tid << " releasing resource\n";
		myM.unlock();
		this_thread::sleep_for(chrono::milliseconds(rand() % 100 + 10));
	}
}


void BrokenWorker(int32_t tid) {
	while(keep_going){
		if(currently_allocated_resources < maximum_number_of_resources){
			int32_t my_resource = ++ currently_allocated_resources;
			myM.lock();
			cout << "Thread: " << setw(3) << tid << " got resource ";
			if(my_resource < 1){
				cout << " TOO SMALL\n";
			} else if(my_resource > maximum_number_of_resources){
				cout << " TOO LARGE\n";
			} else {
				cout << endl;
			}
			myM.unlock();
			this_thread::sleep_for(chrono::milliseconds(rand() % 100 + 10));
			currently_allocated_resources--;
			myM.lock();
			cout << "Thread: " << setw(3) << tid << " releasing resource\n";
			myM.unlock();
			this_thread::sleep_for(chrono::milliseconds(rand() % 100 + 10));
		}
	}
}

void VeryBrokenWorker(int32_t tid) {
	while (keep_going) {
		if (currently_allocated_resources < maximum_number_of_resources) {
			int32_t my_resource = ++currently_allocated_resources;
			cout << "Thread: " << setw(3) << tid << " got resource ";
			if (my_resource < 1)
				cout << " TOO SMALL\n";
			else if (my_resource > maximum_number_of_resources)
				cout << " TOO LARGE\n";
			else
				cout << endl;
			this_thread::sleep_for(chrono::milliseconds(rand() % 100 + 10));
			currently_allocated_resources--;
			cout << "Thread: " << setw(3) << tid << " releasing resource\n";
			this_thread::sleep_for(chrono::milliseconds(rand() % 100 + 10));
		}
	}
}

int main(int argc, char ** argv) {
	vector<thread *> threads;
	srand(int32_t(time(nullptr)));
	int c;
	void (* foo)(int32_t) = nullptr;

	while ((c = getopt(argc, argv, "wbv")) != -1) {
		switch (c) {
			case 'w':
				foo = FixedWorker;
				break;

			case 'b':
				foo = BrokenWorker;
				break;

			case 'v':
				foo = VeryBrokenWorker;
				break;

			default:
				cerr << "Bad command line option\n";
				return 1;
		}
	}
	if (foo == nullptr) {
		cerr << "Must choose on of -w, -b or -v\n";
		return 1;
	}

	for (int32_t i = 0; i < number_of_threads; i++) {
		threads.push_back(new thread(foo, i));
	}
	this_thread::sleep_for(chrono::seconds(3));
	keep_going = false;
	for (auto & t : threads) {
		t->join();
	}
	return 0;
}