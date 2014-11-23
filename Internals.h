/*
 Copyright 2014 Alexander Meißner
 Homepage: https://github.com/Lichtso
 License: http://opensource.org/licenses/mit
 */

#ifndef MacInject_Internals
#define MacInject_Internals

#include "MacInject.h"
#include <semaphore.h>
#include <mach/mach.h>
#include <stdlib.h>

#define CONST_STR(name) #name
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
    uintptr_t systemStack, userStack;
    uintptr_t info;
    mach_port_t receivePort;
    semaphore_t semaphore;
    bool syncWithMainEventLoop;
};

#endif