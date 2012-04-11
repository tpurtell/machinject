#include <mach/mach.h>
#include <mach/message.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <dlfcn.h>
#ifdef __LP64__
typedef unsigned long long pointer_as_int;
#else
typedef unsigned int pointer_as_int;
#endif
struct symbol_rewrite {
    pointer_as_int match;
    const char* symbol;
};

#include "inject.inc"

void patch_pointer(unsigned char* begin, unsigned char* end, pointer_as_int from, pointer_as_int to)
{
    if(end - begin < sizeof(pointer_as_int))
        return;
    for(unsigned char* i = begin; i != end; i++)
        if(*(pointer_as_int*)i == from)
            *(pointer_as_int*)i = to;
}

using namespace std;
int main(int argc, char** argv)
{
    try {
        if(argc < 3)
        {
            ostringstream o;
            o << "usage: " << argv[0] << " <pid> <dylib>";
            throw runtime_error(o.str());
        }
        pid_t pid = atoi(argv[1]);
        const char* library = argv[2];


        task_t remote_task = 0;
        thread_act_t remote_thread = 0;
        mach_msg_type_number_t state_size;
        kern_return_t r;

        r = task_for_pid(mach_task_self(), pid, &remote_task);
        if(r) throw runtime_error("failed to get task for pid");
        r = thread_create(remote_task, &remote_thread);
        if(r) throw runtime_error("failed to create remote thread");
    
        vm_address_t library_string_address = 0;
        vm_address_t pthread_struct_address = 0;
        vm_address_t spawn_loader_address = 0;
        vm_address_t injected_thread_address = 0;
        vm_address_t stack_address = 0;
        
        unsigned int library_length = strlen(library) + 1;
        r = vm_allocate(remote_task, &library_string_address, library_length, TRUE);
        if(r) throw runtime_error("failed to allocate memory for library string");
        r = vm_write(remote_task, library_string_address, (vm_offset_t)library, library_length);
        if(r) throw runtime_error("failed to write library string");

        unsigned int pthread_struct_length = 4096; //guessing
        r = vm_allocate(remote_task, &pthread_struct_address, pthread_struct_length, TRUE);
        if(r) throw runtime_error("failed to allocate memory for pthread struct");
        // r = vm_write(remote_task, pthread_struct_address, (vm_offset_t), pthread_struct_length);
        // if(r) throw runtime_error("failed to write pthread struct");

        for(int i = 0; symbol_map[i].symbol; i++)
        {
            void* address = dlsym(RTLD_DEFAULT, symbol_map[i].symbol);
            patch_pointer(spawn_loader, &spawn_loader[sizeof(spawn_loader)], symbol_map[i].match, (pointer_as_int)address);
            patch_pointer(injected_thread, &injected_thread[sizeof(injected_thread)], symbol_map[i].match, (pointer_as_int)address);
        }

        r = vm_allocate(remote_task, &spawn_loader_address, sizeof(spawn_loader), TRUE);
        if(r) throw runtime_error("failed to allocate memory for spawn loader");
        r = vm_write(remote_task, spawn_loader_address, (vm_offset_t)spawn_loader, sizeof(spawn_loader));
        if(r) throw runtime_error("failed to write spawn loader");

        *(unsigned int*)&injected_thread[INJECTED_THREAD_OFFSET_OF_PTHREAD_STRUCT] = pthread_struct_address;
        *(unsigned int*)&injected_thread[INJECTED_THREAD_OFFSET_OF_LIBRARY_STRING] = library_string_address;
        *(unsigned int*)&injected_thread[INJECTED_THREAD_OFFSET_OF_LOADER_FUNCTION] = spawn_loader_address;

        r = vm_allocate(remote_task, &injected_thread_address, sizeof(injected_thread), TRUE);
        if(r) throw runtime_error("failed to allocate memory for injected thread");
        r = vm_write(remote_task, injected_thread_address, (vm_offset_t)injected_thread, sizeof(injected_thread));
        if(r) throw runtime_error("failed to write injected thread");
        r = vm_protect(remote_task, injected_thread_address, sizeof(injected_thread), false, VM_PROT_EXECUTE | VM_PROT_READ);
        if(r) throw runtime_error("failed to fix memory protection for code");

        unsigned int stack_length = 4096; //guessing
        r = vm_allocate(remote_task, &stack_address, stack_length, TRUE);
        if(r) throw runtime_error("failed to allocate memory for stack");
        // r = vm_write(remote_task, stack_address, (vm_offset_t), stack_length);
        // if(r) throw runtime_error("failed to write stack");

        #ifdef __LP64__
        x86_thread_state64_t state;
        memset(&state, 0, sizeof(state));
        r = thread_get_state(remote_thread, x86_THREAD_STATE64, (natural_t*)&state, &state_size);
        if(r) throw runtime_error("failed to get thread state");
        state.__rip = injected_thread_address;
        state.__rsp = stack_address + stack_length;
        //state.gs = 0; //tls
        r = thread_set_state(remote_thread, x86_THREAD_STATE64, (natural_t*)&state, x86_THREAD_STATE64_COUNT);
        if(r) throw runtime_error("failed to set thread state");
        #else
        x86_thread_state32_t state;
        memset(&state, 0, sizeof(state));
        r = thread_get_state(remote_thread, x86_THREAD_STATE32, (natural_t*)&state, &state_size);
        if(r) throw runtime_error("failed to get thread state");
        state.__eip = injected_thread_address;
        state.__esp = stack_address + stack_length;
        //state.gs = 0; //tls
        r = thread_set_state(remote_thread, x86_THREAD_STATE32, (natural_t*)&state, x86_THREAD_STATE32_COUNT);
        if(r) throw runtime_error("failed to set thread state");
        #endif
    
        r = thread_resume(remote_thread);
        if(r) throw runtime_error("failed to resume thread");
        return 0;
    }
    catch(exception& e) {
        cout << "failed: " << e.what() << endl;
        return 1;
    }
}
