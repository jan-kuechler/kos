#include <string.h>

size_t strlen(const char *s)
{
    size_t c = 0;
    while (s[c]) {
        ++c;
    }
    return c;
}
