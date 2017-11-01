#include "jsonpath.h"

MaybeJsonPath read_path(const char *expression)
{
    return (MaybeJsonPath){.tag=NOTHING};
}
