/*
 Copyright 2014 Alexander Meißner
 Homepage: https://github.com/Lichtso
 License: http://opensource.org/licenses/mit
 */

#include "MacInject.h"
#include <mach-o/loader.h>
#include <sys/sysctl.h>
#include <dlfcn.h>

#define CONST_STR(name) #name

#define iterateSegmentsBegin(image) { \
    struct load_command* command = (struct load_command*)((uint8_t*)image->fMachOData + sizeof(struct mach_header_64)); \
    for(size_t i = 0; i < image->fMachOData->ncmds; i ++) { \
        if(command->cmd == LC_SEGMENT_64) { \
            struct segment_command_64* segment = (struct segment_command_64*)command;
#define iterateSegmentsEnd } command = (struct load_command*)((uint8_t*)command + command->cmdsize); } }

#define calculateSegmentsSize(image) \
    iterateSegmentsBegin(image) \
        if(segment->vmaddr + segment->vmsize > image##Size) \
            image##Size = (uint32_t)(segment->vmaddr + segment->vmsize); \
    iterateSegmentsEnd

#define protectSegments(image) \
    iterateSegmentsBegin(image) \
        err = vm_protect(target, info.image + segment->vmaddr, segment->vmsize, 0, segment->initprot); \
        checkError("vm_protect") \
    iterateSegmentsEnd

#define checkError(name) if(err) { mach_error(name "failed: ", err); goto CleanUp; }

#define calculateAddress(image) { \
    info.image##Entry -= (uintptr_t)image->fMachOData; \
    info.image##Entry += info.image; \
}

bool injectPayloadIntoTarget(const char* injectionPath, const char* payloadPath, const char* payloadEntry, mach_port_t target) {
    uint32_t totalSize = 0, infoSize = 4096, redzoneSize = 512, stackSize = 3584, injectionSize = 0, payloadSize = 0;
    
    // Load images
    struct ImageLoader* injection = CastToImageLoader(dlopen(injectionPath, RTLD_NOW));
    if(!injection) {
        fprintf(stderr, "Could not load injection %s\n", injectionPath);
        return false;
    }
    struct ImageLoader* payload = CastToImageLoader(dlopen(payloadPath, RTLD_NOW));
    if(!payload) {
        fprintf(stderr, "Could not load payload %s\n", payloadPath);
        return false;
    }
    
    // Initialize info
    struct InjectionInfo info;
    info.info = 0;
    info.receivePort = MACH_PORT_NULL;
    info.semaphore = MACH_PORT_NULL;
    info.keepPayload = false;
    info.injection = info.payload = (vm_address_t)NULL;
    info.injectionEntry = (uintptr_t)dlsym(injection, CONST_STR(injectionEntry));
    if(!info.injectionEntry) {
        fprintf(stderr, "Could not find injection entry point\n");
        goto CleanUp;
    }
    info.payloadEntry = (uintptr_t)dlsym(payload, payloadEntry);
    if(!info.payloadEntry) {
        fprintf(stderr, "Could not find payload entry point: %s\n", payloadEntry);
        goto CleanUp;
    }
    info.shouldSyncWithMainEventLoop = (uintptr_t)dlsym(payload, "shouldSyncWithMainEventLoop");
    
    // Calculate memory size
    calculateSegmentsSize(injection)
    calculateSegmentsSize(payload)
    totalSize = infoSize + redzoneSize + stackSize + injectionSize + payloadSize;
    
    // Create Semaphore
    mach_port_t sendPort = 0;
    semaphore_t localSemaphore = 0;
    mach_error_t err = semaphore_create(mach_task_self(), &localSemaphore, SYNC_POLICY_FIFO, 0);
    checkError("semaphore_create")
    
    // Send Semaphore
    {
        err = mach_port_allocate(target, MACH_PORT_RIGHT_RECEIVE, &info.receivePort);
        checkError("mach_port_allocate")
        
        mach_msg_type_name_t type;
        err = mach_port_extract_right(target, info.receivePort, MACH_MSG_TYPE_MAKE_SEND, &sendPort, &type);
        checkError("mach_port_extract_right")
        
        struct {
            mach_msg_base_t base;
            mach_msg_port_descriptor_t port;
        } msg;
        
        bzero(&msg, sizeof(msg));
        msg.base.body.msgh_descriptor_count = 1;
        msg.port.name = localSemaphore;
        msg.port.disposition = MACH_MSG_TYPE_COPY_SEND;
        msg.port.type = MACH_MSG_PORT_DESCRIPTOR;
        msg.base.header.msgh_remote_port = sendPort;
        msg.base.header.msgh_bits = MACH_MSGH_BITS_SET_PORTS(msg.port.disposition, 0, 0) | MACH_MSGH_BITS_COMPLEX;
        msg.base.header.msgh_size = sizeof(msg);
        
        err = mach_msg_send(&msg.base.header);
        checkError("mach_msg_send")
    }
    
    // Allocate remote memory
    err = vm_allocate(target, &info.info, totalSize, 1);
    checkError("vm_allocate")
    
    // Calculate remote memory addresses
    info.redzone = info.info + infoSize;
    info.stack = info.redzone + redzoneSize;
    info.injection = info.stack + stackSize;
    info.payload = info.injection + injectionSize;
    calculateAddress(injection)
    calculateAddress(payload)
    
    // Copy data into remote memory
    err = vm_write(target, info.info, (pointer_t)&info, sizeof(info));
    checkError("vm_write info")
    err = vm_write(target, info.injection, (pointer_t)injection->fMachOData, injectionSize);
    checkError("vm_write injection")
    err = vm_write(target, info.payload, (pointer_t)payload->fMachOData, payloadSize);
    checkError("vm_write payload")
    
    // Protect remote memory
    protectSegments(injection)
    protectSegments(payload)
    
    // Unload images
    dlclose(injection);
    dlclose(payload);
    injection = payload = NULL;
    
    // Create remote thread
    {
        x86_thread_state64_t threadState;
        bzero(&threadState, sizeof(threadState));
        threadState.__rdi = (uint64_t)info.info;
        threadState.__rip = (uint64_t)info.injectionEntry;
        threadState.__rsp = (uint64_t)info.stack;
        
        thread_act_t thread;
        err = thread_create_running(target, x86_THREAD_STATE64,
                                    (thread_state_t)&threadState, x86_THREAD_STATE64_COUNT,
                                    &thread);
        checkError("thread_create_running")
    }
    
    // Wait for remote thread
    semaphore_wait(localSemaphore);
    
    // Read data from remote memory
    {
        vm_size_t len = sizeof(info);
        err = vm_read_overwrite(target, info.info, sizeof(info), (pointer_t)&info, &len);
        checkError("vm_read_overwrite")
    }
    
    CleanUp:
    if(injection) dlclose(injection);
    if(payload) dlclose(payload);
    if(info.info) vm_deallocate(target, info.info, (info.keepPayload) ? totalSize - payloadSize : totalSize);
    if(localSemaphore) semaphore_destroy(mach_task_self(), localSemaphore);
    if(sendPort) mach_port_destroy(mach_task_self(), sendPort);
    if(info.receivePort) mach_port_destroy(target, info.receivePort);
    if(info.semaphore) mach_port_destroy(target, info.semaphore);
    
	return err == err_none;
}

bool findProcessByNameOrPID(struct extern_proc* proc) {
    size_t length = 0;
    static const int command[] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0 };
    
    if(sysctl((int*)command, 3, NULL, &length, NULL, 0))
        return false;
    
    struct kinfo_proc* result = malloc(length);
    if(sysctl((int*)command, 3, result, &length, NULL, 0))
        goto Error;
    
    size_t i = 0;
    length /= sizeof(struct kinfo_proc);
    if(proc->p_comm && strlen(proc->p_comm)) {
        for(; i < length; i++)
            if(strcmp(result[i].kp_proc.p_comm, proc->p_comm) == 0)
                goto Success;
    }else if(proc->p_pid) {
        for(; i < length; i++)
            if(result[i].kp_proc.p_pid == proc->p_pid)
                goto Success;
    }
    
    Error:
    free(result);
    return false;
    
    Success:
    memcpy(proc, &result[i].kp_proc, sizeof(struct extern_proc));
    free(result);
    return true;
}

char* getResourcePathRelativeTo(const char* base, const char* path) {
    for(size_t i = strlen(base)-1; i > 0; i --)
        if(base[i] == '/') {
            size_t pathLen = strlen(path);
            char* buffer = malloc(i+2+pathLen);
            strncpy(buffer, base, i+1);
            memcpy(&buffer[i+1], path, pathLen+1);
            return buffer;
        }
    return NULL;
}