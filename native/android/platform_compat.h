#pragma once
// Bionic does not expose ftruncate through sys/mman.h. memfd_create is API 30,
// while API 29 kernels already provide the syscall needed by the SDK's aliases.
#include <unistd.h>
#if defined(__ANDROID__) && __ANDROID_API__ < 30
#include <sys/syscall.h>
inline int memfd_create(const char* name,unsigned flags){return int(syscall(__NR_memfd_create,name,flags));}
#endif
