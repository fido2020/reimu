#pragma once

#include <reimu/os/platform.h>

#if defined(REIMU_WIN32)

#include <windows.h>

typedef HANDLE os_handle_t;

#elif defined(REIMU_UNIX)

#include <sys/epoll.h>

typedef int os_handle_t;

#endif
