#pragma once

#include <reimu/os/platform.h>

#if defined(REIMU_WIN32)

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

typedef HANDLE os_handle_t;

#elif defined(REIMU_UNIX)

#include <sys/epoll.h>

typedef int os_handle_t;

#endif
