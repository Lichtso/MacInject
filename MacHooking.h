/*
 Copyright 2014 Alexander Meißner
 Homepage: https://github.com/Lichtso
 License: http://opensource.org/licenses/mit
 */

#ifndef MacHooking
#define MacHooking

#if !defined(__x86_64__)
#error This architecture is not supported
#endif

#include "MacInject.h"

__BEGIN_DECLS

/*! Checks if proc points to a procedure header
 @param proc Pointer to the procedure
 */
bool isProcedureHeader(void* proc);

/*! Checks if proc points to a already manipulated procedure
 @param proc Pointer to the procedure
 */
bool checkProcedureJump(void* proc);

/*! Returnes the injected jump address
 @param proc Pointer to the procedure
 @return Injected jump address
 */
void* getProcedureJump(void* proc);

/*! Replaces a procedure at dst with a jump to src
 @param proc Pointer to the procedure
 @param addr Jump address to inject
 */
void injectProcedureJump(void* proc, void* addr);

__END_DECLS

#endif