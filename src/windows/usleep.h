#ifndef USLEEP_H
#define USLEEP_H

#ifdef __unix__
# include <unistd.h>
#elif defined _WIN32
# include <windows.h>
#define usleep(x) sleep(x)
#endif

#endif /* USLEEP_H */
