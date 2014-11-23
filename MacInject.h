/*
 Copyright 2014 Alexander Meißner
 Homepage: https://github.com/Lichtso
 License: http://opensource.org/licenses/mit
 */

#ifndef MacInject_Externals
#define MacInject_Externals

#if !defined(__x86_64__)
#error This architecture is not supported
#endif

#include <stdbool.h>
#include <stdio.h>

__BEGIN_DECLS

/*! Injects a piece of programm code into another process and starts a thread running it
 @param injectionPath Path to the MacInjection.image
 @param payloadPath Path to a dynamically linked image which should be injected and contains payloadEntry
 @param payloadEntry The name of the function to be called when the code has been injected
 @param target Port to the target process
 @param syncWithMainEventLoop If true, payloadEntry will be called in the next iteration of the targets MainEventLoop else it will be called immediately
 @return True on success or false if errors occured
 @warning This function is blocking and waits for the injected thread to end to free all resources
 */
bool injectPayloadIntoTarget(const char* injectionPath, const char* payloadPath, const char* payloadEntry, mach_port_t target, bool syncWithMainEventLoop);

/*! Finds a process by the given name or pid
 @param proc On input p_pid or p_comm has to be filled, on output the entire struct will be filled
 @return True if the process exists or false if not
 */
bool findProcessByNameOrPID(struct extern_proc* proc);

/*! Calculates a absolute path by stripping everything behind the last / from base and adding path
 @param base "/A/B/C/def.ghi"
 @param path "J/K/lmn.opq"
 @return "/A/B/C/J/K/lmn.opq"
 */
char* getResourcePathRelativeTo(const char* base, const char* path);

__END_DECLS

#endif