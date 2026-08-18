#pragma once

#if !defined(_WIN32)
    #include_next <unistd.h>
#else
    #include <BaseTsd.h>
    using ssize_t = SSIZE_T;
#endif

#define STDOUT_FILENO 1

inline ssize_t write(int fd, const void* buf, size_t count)
{
   // return _write(fd, buf, static_cast<unsigned int>(count));
   return count; // pretend we wrote everything
}
