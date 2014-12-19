/*
 Copyright 2014 Alexander Meißner
 Homepage: https://github.com/Lichtso
 License: http://opensource.org/licenses/mit
 */

#include "MacHooking.h"
#include <sys/mman.h>

#if 0

asm(
    ".intel_syntax noprefix;"
    "_procedureJumpIntel:"
    "push rbp;"
    "mov rbp, rsp;"
    "mov dword ptr [rsp-0x4], 0x55667788;"
    "mov dword ptr [rsp-0x8], 0x11223344;"
    "jmp [rsp-0x8];"
);

asm(
    ".att_syntax;"
    "_procedureJumpATT:"
    "pushq %rbp;"
    "movq %rsp, %rbp;"
    "movl $0x55667788, -0x4(%rsp);"
    "movl $0x11223344, -0x8(%rsp);"
    "jmpq *-0x8(%rsp);"
);

#endif

static uint8_t procedureJump[] = {
    0x55, 0x48, 0x89, 0xe5,
    0xc7, 0x44, 0x24, 0xfc,
    0xc7, 0x44, 0x24, 0xf8,
    0xFF, 0x64, 0x24, 0xf8
};

static bool compareBytes(uint8_t* a, uint8_t* b, size_t n) {
    for(size_t i = 0; i < n; i ++)
        if(a[i] != b[i])
            return false;
    return true;
}

bool isProcedureHeader(void* proc) {
    return compareBytes(proc, procedureJump, 4);
}

bool checkProcedureJump(void* proc) {
    return compareBytes(proc, procedureJump+4, 4) &&
    compareBytes(proc+8, procedureJump+8, 4) &&
    compareBytes(proc+16, procedureJump+12, 4);
}

void* getProcedureJump(void* proc) {
    uint64_t addr;
    uint32_t* ptr = (uint32_t*)&addr;
    memcpy(&ptr[1], proc+4, 4);
    memcpy(&ptr[0], proc+12, 4);
    return (void*)addr;
}

void injectProcedureJump(void* proc, void* addr) {
    uint32_t* ptr = (uint32_t*)&addr;
    uint64_t pageSize = sysconf(_SC_PAGESIZE),
    pageAddr = (uint64_t)proc / pageSize * pageSize;
    
    mprotect((void*)pageAddr, pageSize*2, PROT_READ | PROT_WRITE);
    memcpy(proc, procedureJump+4, 4);
    memcpy(proc+4, &ptr[1], 4);
    memcpy(proc+8, procedureJump+8, 4);
    memcpy(proc+12, &ptr[0], 4);
    memcpy(proc+16, procedureJump+12, 4);
    mprotect((void*)pageAddr, pageSize*2, PROT_READ | PROT_EXEC);
}