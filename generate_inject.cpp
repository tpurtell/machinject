#include <dlfcn.h>
#include <mach/mach.h>
#include <pthread.h>
#include <iostream>
#include <iomanip>
#include <vector>

#ifdef __LP64__
typedef unsigned long long              pointer_as_int;
#else
typedef unsigned int                    pointer_as_int;
#endif

extern "C" void __pthread_set_self(void*);
typedef void* (*thread_func)(void*);
#ifdef TEST_LOCALLY
#define MACH_THREAD_SELF                mach_thread_self
#define THREAD_TERMINATE                thread_terminate
#define PTHREAD_SET_SELF                __pthread_set_self
#define PTHREAD_ATTR_INIT               pthread_attr_init
#define PTHREAD_ATTR_GETSCHEDPOLICY     pthread_attr_getschedpolicy
#define PTHREAD_ATTR_SETDETACHSTATE     pthread_attr_setdetachstate
#define PTHREAD_ATTR_SETINHERITSCHED    pthread_attr_setinheritsched
#define PTHREAD_CREATE                  pthread_create
#define PTHREAD_ATTR_DESTROY            pthread_attr_destroy
#define SCHED_GET_PRIORITY_MAX          sched_get_priority_max
#define DLOPEN                          dlopen
#define PTHREAD_ATTR_SETSCHEDPARAM      pthread_attr_setschedparam
#define PTHREAD_EXIT                    pthread_exit

#define PTHREAD_STRUCT                  ((char*)pthread_self())
#define LIBRARY_STRING                  (char*)"/usr/lib/libz.dylib"    
#define LOADER_FUNCTION                 (thread_func)spawn_loader

#else
typedef thread_act_t (*pfn_mach_thread_self)();
typedef kern_return_t (*pfn_thread_terminate)(thread_act_t);
typedef void (*pfn_pthread_set_self)(void* data);
typedef int (*pfn_pthread_attr_init)(pthread_attr_t *attr);
typedef int (*pfn_pthread_attr_getschedpolicy)(const pthread_attr_t *attr, int *policy);
typedef int (*pfn_pthread_attr_setdetachstate)(pthread_attr_t *attr, int detachstate);
typedef int (*pfn_pthread_attr_setinheritsched)(pthread_attr_t *attr, int inheritsched);
typedef void* (*pfn_pthread_create)(pthread_t*, void*, thread_func, void* );
typedef int (*pfn_pthread_attr_destroy)(pthread_attr_t *attr);
typedef int (*pfn_sched_get_priority_max)(int);
typedef void* (*pfn_dlopen)(const char*, unsigned int);
typedef int (*pfn_pthread_attr_setschedparam)(pthread_attr_t *attr, const struct sched_param *param);
typedef void (*pfn_pthread_exit)(void* data);

#ifdef __LP64__
#define MACH_THREAD_SELF                ((pfn_mach_thread_self)0x0101010101010101LL)
#define THREAD_TERMINATE                ((pfn_thread_terminate)0x0202020202020202LL)
#define PTHREAD_SET_SELF                ((pfn_pthread_set_self)0x0303030303030303LL)
#define PTHREAD_ATTR_INIT               ((pfn_pthread_attr_init)0x0404040404040404LL)
#define PTHREAD_ATTR_GETSCHEDPOLICY     ((pfn_pthread_attr_getschedpolicy)0x0505050505050505LL)
#define PTHREAD_ATTR_SETDETACHSTATE     ((pfn_pthread_attr_setdetachstate)0x0606060606060606LL)
#define PTHREAD_ATTR_SETINHERITSCHED    ((pfn_pthread_attr_setinheritsched)0x0707070707070707LL)
#define PTHREAD_CREATE                  ((pfn_pthread_create)0x0808080808080808LL)
#define PTHREAD_ATTR_DESTROY            ((pfn_pthread_attr_destroy)0x0909090909090909LL)
#define SCHED_GET_PRIORITY_MAX          ((pfn_sched_get_priority_max)0x0a0a0a0a0a0a0a0aLL)
#define DLOPEN                          ((pfn_dlopen)0x0b0b0b0b0b0b0b0bLL)
#define PTHREAD_ATTR_SETSCHEDPARAM      ((pfn_pthread_attr_setschedparam)0x0c0c0c0c0c0c0c0cLL)
#define PTHREAD_EXIT                    ((pfn_pthread_exit)0x0d0d0d0d0d0d0d0dLL)

#define PTHREAD_STRUCT                  ((char*)0xf0f0f0f0f0f0f0f0LL)        
#define LIBRARY_STRING                  ((char*)0xf1f1f1f1f1f1f1f1LL)    
#define LOADER_FUNCTION                 ((thread_func)0xf2f2f2f2f2f2f2f2LL)
#else
#define MACH_THREAD_SELF                ((pfn_mach_thread_self)0x01010101)
#define THREAD_TERMINATE                ((pfn_thread_terminate)0x02020202)
#define PTHREAD_SET_SELF                ((pfn_pthread_set_self)0x03030303)
#define PTHREAD_ATTR_INIT               ((pfn_pthread_attr_init)0x04040404)
#define PTHREAD_ATTR_GETSCHEDPOLICY     ((pfn_pthread_attr_getschedpolicy)0x05050505)
#define PTHREAD_ATTR_SETDETACHSTATE     ((pfn_pthread_attr_setdetachstate)0x06060606)
#define PTHREAD_ATTR_SETINHERITSCHED    ((pfn_pthread_attr_setinheritsched)0x07070707)
#define PTHREAD_CREATE                  ((pfn_pthread_create)0x08080808)
#define PTHREAD_ATTR_DESTROY            ((pfn_pthread_attr_destroy)0x09090909)
#define SCHED_GET_PRIORITY_MAX          ((pfn_sched_get_priority_max)0x0a0a0a0a)
#define DLOPEN                          ((pfn_dlopen)0x0b0b0b0b)
#define PTHREAD_ATTR_SETSCHEDPARAM      ((pfn_pthread_attr_setschedparam)0x0c0c0c0c)
#define PTHREAD_EXIT                    ((pfn_pthread_exit)0x0d0d0d0d)

#define PTHREAD_STRUCT                  ((char*)0xf0f0f0f0)        
#define LIBRARY_STRING                  ((char*)0xf1f1f1f1)    
#define LOADER_FUNCTION                 ((thread_func)0xf2f2f2f2)
#endif

#endif

void* spawn_loader(char* library)
{
    DLOPEN(library, RTLD_NOW | RTLD_LOCAL);
    PTHREAD_EXIT(NULL);
    asm volatile (".byte 0xFF; .byte 0xFF; .byte 0xFF; .byte 0xFF");
}
void injected_thread() {
    PTHREAD_SET_SELF(PTHREAD_STRUCT);
	pthread_attr_t attr;
	PTHREAD_ATTR_INIT(&attr); 
	int policy;
	PTHREAD_ATTR_GETSCHEDPOLICY( &attr, &policy );
	PTHREAD_ATTR_SETDETACHSTATE( &attr, PTHREAD_CREATE_DETACHED );
	PTHREAD_ATTR_SETINHERITSCHED( &attr, PTHREAD_EXPLICIT_SCHED );
	
	struct sched_param sched;
	sched.sched_priority = SCHED_GET_PRIORITY_MAX( policy );
	PTHREAD_ATTR_SETSCHEDPARAM( &attr, &sched );
	
	pthread_t thread;
	    PTHREAD_CREATE( &thread, &attr, LOADER_FUNCTION, LIBRARY_STRING);
	PTHREAD_ATTR_DESTROY(&attr);
    THREAD_TERMINATE(MACH_THREAD_SELF());
    asm volatile (".byte 0xFF; .byte 0xFF; .byte 0xFF; .byte 0xFF");
}
using namespace std;
pointer_as_int find_pointer(vector<unsigned char>& data, pointer_as_int find)
{
	//assumes nothing will be at ofs 0
    if(data.size() < sizeof(pointer_as_int))
        return 0;
    for(vector<unsigned char>::iterator i = data.begin(); i != data.end(); i++)
    {
        if(*(pointer_as_int*)&*i == find)
            return i - data.begin();
    }
	return 0;
}

void print_symbol_rewrites(const char* table_name)
{
    vector< pair<void*, const char*> > patches;
    patches.push_back(make_pair((void*)MACH_THREAD_SELF, "mach_thread_self"));
    patches.push_back(make_pair((void*)THREAD_TERMINATE, "thread_terminate"));
    patches.push_back(make_pair((void*)PTHREAD_SET_SELF, "__pthread_set_self"));
    patches.push_back(make_pair((void*)PTHREAD_ATTR_INIT, "pthread_attr_init"));
    patches.push_back(make_pair((void*)PTHREAD_ATTR_GETSCHEDPOLICY, "pthread_attr_getschedpolicy"));
    patches.push_back(make_pair((void*)PTHREAD_ATTR_SETDETACHSTATE, "pthread_attr_setdetachstate"));
    patches.push_back(make_pair((void*)PTHREAD_ATTR_SETINHERITSCHED, "pthread_attr_setinheritsched"));
    patches.push_back(make_pair((void*)PTHREAD_CREATE, "pthread_create"));
    patches.push_back(make_pair((void*)PTHREAD_ATTR_DESTROY, "pthread_attr_setinheritscheddestroy"));
    patches.push_back(make_pair((void*)SCHED_GET_PRIORITY_MAX, "sched_get_priority_max"));
    patches.push_back(make_pair((void*)DLOPEN, "dlopen"));
    patches.push_back(make_pair((void*)PTHREAD_ATTR_SETSCHEDPARAM, "pthread_attr_setschedparam"));
    patches.push_back(make_pair((void*)PTHREAD_EXIT, "pthread_exit"));    

    cout << "struct symbol_rewrite " << table_name << "[] = {" << endl;
    for(vector<pair<void*, const char*> >::const_iterator i = patches.begin(); i != patches.end(); i++) {
        #ifdef __LP64__
            cout << "    " << "{ " << i->first << ", \"" << i->second << "\" }," << endl;
        #else
            cout << "    " << "{ " << i->first << "LL, \"" << i->second << "\" }," << endl;
        #endif
    }
    cout << "    " << "{ " << 0 << ", NULL }," << endl;
    cout << "};" << endl;
}

void print_array(const std::vector<unsigned char>& data, const char* name, int wrap)
{
    cout << "unsigned char " << name << "[] = {" << endl;
    cout << hex << setw(2) << setfill('0');
    for(vector<unsigned char>::const_iterator i = data.begin(); i != data.end();)
    {
        cout << "    ";
        for(int j = 0; i != data.end() && j < wrap; i++, j++)
            cout << "0x" << setw(2) << (unsigned int)*i << ",   ";
        cout << endl;
    }
    cout << dec << setfill(' ');
    cout << "};" << endl;
}
int main() {
    int injected_thread_end_offset = 0;
    unsigned char* injected_thread_bytes = (unsigned char*)injected_thread;
    while(*(unsigned int*)&injected_thread_bytes[injected_thread_end_offset] != 0xffffffff)
        injected_thread_end_offset++;

    int spawn_loader_end_offset = 0;
    unsigned char* spawn_loader_bytes = (unsigned char*)spawn_loader;
    while(*(unsigned int*)&spawn_loader_bytes[spawn_loader_end_offset] != 0xffffffff)
        spawn_loader_end_offset++;
#ifdef TEST_LOCALLY
    cout << "injected thread starts at " << (void*)injected_thread_bytes << " and is " << injected_thread_end_offset << " bytes" << endl;
    cout << "spawn loader starts at " << (void*)spawn_loader_bytes << " and is " << spawn_loader_end_offset << " bytes" << endl;
    injected_thread();
#else
    print_symbol_rewrites("symbol_map");
    cout << endl;
    vector<unsigned char> spawn_loader_data(spawn_loader_bytes, &spawn_loader_bytes[spawn_loader_end_offset]);
    print_array(spawn_loader_data, "spawn_loader", 8);
    cout << endl;
    vector<unsigned char> injected_thread_data(injected_thread_bytes, &injected_thread_bytes[injected_thread_end_offset]);
    print_array(injected_thread_data, "injected_thread", 8);
    cout << endl;
    cout << "const unsigned int INJECTED_THREAD_OFFSET_OF_PTHREAD_STRUCT = " << find_pointer(injected_thread_data, (pointer_as_int)PTHREAD_STRUCT) << ";" << endl;
    cout << "const unsigned int INJECTED_THREAD_OFFSET_OF_LIBRARY_STRING = " << find_pointer(injected_thread_data, (pointer_as_int)LIBRARY_STRING) << ";" << endl;
    cout << "const unsigned int INJECTED_THREAD_OFFSET_OF_LOADER_FUNCTION = " << find_pointer(injected_thread_data, (pointer_as_int)LOADER_FUNCTION) << ";" << endl;
    cout << endl;
#endif
    return 0;
}
