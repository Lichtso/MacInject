/*
 Copyright 2014 Alexander Meißner
 Homepage: https://github.com/Lichtso
 License: http://opensource.org/licenses/mit
 */

#ifndef MacInject
#define MacInject

#if !defined(__x86_64__)
#error This architecture is not supported
#endif

#include <semaphore.h>
#include <mach/mach.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#define CastToImageLoader(library) ((struct ImageLoader*)((uintptr_t)library & (-4)))

struct ImageLoader {
    const char *fPath, *fRealPath;
    uint8_t whatever[56];
    struct mach_header_64* fMachOData;
    struct segment_command_64* fLinkEditBase;
    uintptr_t fSlide;
    uint32_t fEHFrameSectionOffset, fUnwindInfoSectionOffset, fDylibIDOffset;
    uint32_t fSegmentsCount : 8,
    fIsSplitSeg : 1,
    fInSharedCache : 1,
    fHasSubLibraries : 1,
    fHasSubUmbrella : 1,
    fInUmbrella : 1,
    fHasDOFSections : 1,
    fHasDashInit : 1,
    fHasInitializers : 1,
    fHasTerminators : 1,
    fRegisteredAsRequiresCoalescing : 1;
    union {
        struct {
            const char* fStrings;
            const struct macho_nlist* fSymbolTable;
            const struct dysymtab_command* fDynamicInfo;
        };
        const struct dyld_info_command* fDyldInfo;
    };
};

struct InjectionInfo {
    uintptr_t injection, injectionEntry;
    uintptr_t payload, payloadEntry;
    uintptr_t redzone, stack;
    uintptr_t info, shouldSyncWithMainEventLoop;
    mach_port_t receivePort;
    semaphore_t semaphore;
    bool keepPayload;
};

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
bool injectPayloadIntoTarget(const char* injectionPath, const char* payloadPath, const char* payloadEntry, mach_port_t target);

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