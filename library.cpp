#include <iostream>
#include <unistd.h>

using namespace std;

struct do_on_load {
    do_on_load() {
        cout << "from " << getpid() << ": i have been injected with a poison!" << endl;
        cout.flush();
	    exit(1);
    }
} on_load;
