/*
 Copyright 2014 Alexander Meißner
 Homepage: https://github.com/Lichtso
 License: http://opensource.org/licenses/mit
 */

#include "MacInject.h"
#include <ctype.h>

#define stringArgumentCase(name, result) \
    case name: \
        if(argumentsLen == 2 && argc > i+1) \
            result = argv[i+1], ++ i; \
        else \
            result = &argv[i][2]; \
    continue;

static bool checkStrIsUInt(const char* str) {
    for(; *str; ++ str)
        if(!isdigit(*str))
            return false;
    return true;
}

int main(int argc, const char* argv[]) {
    const char *payloadPath = NULL, *payloadEntry = "init", *targetName = NULL;
    
    // Interpret arguments
    for(size_t i = 1; i < argc; ++ i) {
        size_t argumentsLen = strlen(argv[i]);
        if(argumentsLen >= 2 && argv[i][0] == '-')
            switch(argv[i][1]) {
                case 'h':
                    fprintf(stderr, "Usage:\n");
                    fprintf(stderr, "-p [path] : path to the payload (mandatory)\n");
                    fprintf(stderr, "-e [symbol] : payload entry symbol (default is \"init\")\n");
                    fprintf(stderr, "-t [path/pid] : target name or pid (mandatory)\n");
                return 0;
                stringArgumentCase('p', payloadPath)
                stringArgumentCase('e', payloadEntry)
                stringArgumentCase('t', targetName)
            }
        fprintf(stderr, "Could not interpret argument %s\n", argv[i]);
        return 1;
    }
    
    // Check if all necessary arguments are given
    if(!payloadPath) {
        fprintf(stderr, "No path to payload given, use: -p [path]\n");
        return 1;
    }
    if(!targetName) {
        fprintf(stderr, "No target given, use: -t [name/pid]\n");
        return 1;
    }
    
    // Find target process
    struct extern_proc proc;
    if(checkStrIsUInt(targetName)) {
        sscanf(targetName, "%d", &proc.p_pid);
        proc.p_comm[0] = 0;
    }else{
        if(strlen(targetName) > 16) {
            fprintf(stderr, "Target name too long: %s\n", targetName);
            return 1;
        }
        strcpy((char*)&proc.p_comm, targetName);
    }
    if(!findProcessByNameOrPID(&proc)) {
        fprintf(stderr, "Could not find target process: %s\n", targetName);
        return 1;
    }
    
    // Acquire rights
    mach_port_t	target;
    mach_error_t err = task_for_pid(mach_task_self(), proc.p_pid, &target);
    if(err == 5)
        fprintf(stderr, "Permission denied\n");
    if(err) return err;
    
    // Do the acutal work
    const char* injectionPath = getResourcePathRelativeTo(argv[0], "MacInjection.image");
    injectPayloadIntoTarget(injectionPath, payloadPath, payloadEntry, target);
    free((void*)injectionPath);
    
    printf("DONE\n");
    return 0;
}