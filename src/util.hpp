#ifndef __UTIL_H__
#define __UTIL_H__
#include <vector>
#include <string>
#include <string.h>

std::vector<std::string> split(const char *str, const char *delim)
{
    std::vector<std::string> ret;
    char *tmp = strdup(str);
    char *p = strtok(tmp, delim);
    while (p)
    {
        ret.push_back(p);
        p = strtok(NULL, delim);
    }
    free(tmp);
    return ret;
}
#endif