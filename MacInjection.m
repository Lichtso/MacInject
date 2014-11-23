/*
 Copyright 2014 Alexander Meißner
 Homepage: https://github.com/Lichtso
 License: http://opensource.org/licenses/mit
 */

#include "Internals.h"
#include <Carbon/Carbon.h>
#include <pthread.h>
#include <syslog.h>

static void EventLoopTimerEntry(EventLoopTimerRef inTimer, struct InjectionInfo* info) {
    struct {
        mach_msg_base_t base;
        mach_msg_port_descriptor_t port;
        mach_msg_trailer_t trailer;
    } msg;
    
    msg.base.header.msgh_size = sizeof(msg);
    msg.base.header.msgh_local_port = info->receivePort;
    
    mach_error_t err = mach_msg_receive(&msg.base.header);
    if(err)
        syslog(LOG_WARNING, "MacInject could not receive semaphore: %s\n", mach_error_string(err));
    else{
        info->semaphore = msg.port.name;
        
        ((void(*)(struct InjectionInfo* info))info->payloadEntry)(info);
        
        semaphore_signal(info->semaphore);
    }
}

static void* pthread_entry(struct InjectionInfo* info) {
    if(info->syncWithMainEventLoop)
        InstallEventLoopTimer(GetMainEventLoop(), 0, 0, (EventLoopTimerUPP)EventLoopTimerEntry, (void*)info, NULL);
    else
        EventLoopTimerEntry(NULL, info);
    
    return NULL;
}

void injectionEntry(struct InjectionInfo* info) {
    extern void __pthread_set_self(void*);
    __pthread_set_self((void*)info->systemStack);
    
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    
    int policy;
    struct sched_param sched;
    pthread_attr_getschedpolicy(&attr, &policy);
	sched.sched_priority = sched_get_priority_max(policy);
	pthread_attr_setschedparam(&attr, &sched);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
	
	pthread_t thread;
	pthread_create(&thread, &attr, (void*(*)(void*))pthread_entry, info);
	pthread_attr_destroy(&attr);
    
    thread_terminate(mach_thread_self());
}