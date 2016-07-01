#include "bop_api.h"
#include <string.h>

void *
memmove(void *dest, const void *source, size_t num)
{
    char holder[num];

    memcpy(holder, source, num);
    memcpy(dest, holder, num);
    BOP_promise(dest, num);
    return dest;
}
