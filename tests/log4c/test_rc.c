static const char version[] = "$Id$";

/*
 * test_rc.c
 *
 * Author: Cedric Le Goater <legoater@cimai.com>, (c) Cimai 2001
 */

#include <log4c/rc.h>
#include <log4c/category.h>
#include <sd/test.h>
#include <sd/factory.h>
#include <stdio.h>


/******************************************************************************/
static void log4c_print(FILE* a_fp)
{   
    extern sd_factory_t* log4c_category_factory;
    extern sd_factory_t* log4c_appender_factory;
    extern sd_factory_t* log4c_layout_factory;

    sd_factory_print(log4c_category_factory, a_fp);	fprintf(a_fp, "\n");
    sd_factory_print(log4c_appender_factory, a_fp);	fprintf(a_fp, "\n");
    sd_factory_print(log4c_layout_factory, a_fp);	fprintf(a_fp, "\n");
}

/******************************************************************************/
static int test0(sd_test_t* a_test, int argc, char* argv[])
{    
    log4c_print(sd_test_out(a_test));
    return 1;
}

/******************************************************************************/
static int test1(sd_test_t* a_test, int argc, char* argv[])
{    
    log4c_rc_t* rc = log4c_rc_new();

    if (log4c_rc_load(rc, SRCDIR "/test_rc.in") == -1)
	return 0;

    log4c_rc_delete(rc);

    log4c_print(sd_test_out(a_test));
    return 1;
}

/******************************************************************************/
int main(int argc, char* argv[])
{    
    sd_test_t* t = sd_test_new(argc, argv);

    sd_test_add(t, test0);
    sd_test_add(t, test1);

    return ! sd_test_run(t, argc, argv);
}