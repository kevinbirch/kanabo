#include <stdio.h>

#include <check.h>

#include "options.h"
#include "test.h"


Suite *master_suite(void)
{
    TCase *options = tcase_create("options");

    Suite *master = suite_create("Master");
    suite_add_tcase(master, options);

    return master;
}
