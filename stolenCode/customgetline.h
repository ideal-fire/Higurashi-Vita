#ifndef GETLINECUSTOMINCLUDED
#define GETLINECUSTOMINCLUDED
#include <goodbrew/config.h>
#include <goodbrew/base.h>
ssize_t custom_getline(char **buf, size_t *bufsiz, crossFile fp);
ssize_t custom_getdelim(char **buf, size_t *bufsiz, int delimiter, crossFile fp);
#endif