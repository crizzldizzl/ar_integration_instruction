#ifndef GRPC_INCLUDE_BEGIN
#define GRPC_INCLUDE_BEGIN

#if (defined(PLATFORM_HOLOLENS) || defined(PLATFORM_WINDOWS))
#include "Windows/WindowsHWrapper.h"
#include "Windows/AllowWindowsPlatformAtomics.h"
#include <winnt.h>

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif

#ifndef _INC_WINDOWS
// define only if not already present
#ifndef UE_MEMORY_BARRIER
#define UE_MEMORY_BARRIER
static void UnrealMemoryBarrier() {}
#endif
#endif

THIRD_PARTY_INCLUDES_START
#pragma warning(push)
#pragma warning (disable : 4800 4125 4647 4668 4582 4583 4946 4996)
#endif

#endif // GRPC_INCLUDE_BEGIN