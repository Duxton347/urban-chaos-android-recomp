#pragma once
// The SDK dispatcher includes this on non-Windows systems but calls no x86 intrinsics.
#if !defined(__aarch64__)
#error This compatibility header is only for the Android ARM64 host
#endif
