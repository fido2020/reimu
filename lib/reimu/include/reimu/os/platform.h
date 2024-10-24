#pragma once

#if defined(_WIN32)

#ifndef REIMU_WIN32
#define REIMU_WIN32
#endif

#elif defined(__unix__)

#ifndef REIMU_UNIX
#define REIMU_UNIX
#endif

#else

#error "Unsupported platform"

#endif
