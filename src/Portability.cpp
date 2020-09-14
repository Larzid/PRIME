#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __unix__
#include <stddef.h>
#include <time.h>   // nanosleep
#endif

#ifdef __MACH__
#include <stddef.h>
#include <time.h>   // nanosleep
#endif

namespace sys {

void
wait_msec (unsigned int ms)
{
#ifdef _WIN32
    Sleep (ms);
#else
    struct timespec wait;
    wait.tv_sec = ms / 1000;
    wait.tv_nsec = (ms % 1000) * 1000000;
    nanosleep (&wait, NULL);
#endif
}

}
