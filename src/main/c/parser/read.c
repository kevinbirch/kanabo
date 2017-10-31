#include "jsonpath.h"

MaybeJsonPath read_path(const char *expression __attribute__((unused)))
{
    return (MaybeJsonPath){.tag=NOTHING};
}
