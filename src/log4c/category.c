static const char version[] = "$Id$";

/*
 * category.c
 *
 * Copyright 2001-2002, Cimai Technology SA (www.cimai.com). All rights reserved.
 * Copyright 2001-2002, Cedric Le Goater <legoater@cimai.com>. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */

#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sd/sprintf.h>
#include <sd/malloc.h>
#include <sd/factory.h>
#include <log4c/appender.h>
#include <log4c/priority.h>
#include <log4c/logging_event.h>
#include <log4c/category.h>
#include <log4c/rc.h>
#include <sd/error.h>

struct __log4c_category {
    char*			cat_name;
    int				cat_priority;
    int				cat_additive;
    const log4c_category_t*	cat_parent;
    log4c_appender_t*		cat_appender;
};

sd_factory_t* log4c_category_factory = NULL;

static const char LOG4C_CATEGORY_DEFAULT[] = "root";

/**
 * @bug the root category name should be "" not "root". *
 */

/*******************************************************************************/
extern log4c_category_t* log4c_category_get(const char* a_name)
{
    static const sd_factory_ops_t log4c_category_factory_ops = {
	fac_new:	(void*) log4c_category_new,
	fac_delete:	(void*) log4c_category_delete,
	fac_print:	(void*) log4c_category_print,
    };

    if (!log4c_category_factory) {
	log4c_category_factory = sd_factory_new("log4c_category_factory",
						&log4c_category_factory_ops);
    }

    return sd_factory_get(log4c_category_factory, a_name);
}

/*******************************************************************************/
static const char* dot_dirname(char* a_string);

extern log4c_category_t* log4c_category_new(const char* a_name)
{
    log4c_category_t* this;

    if (!a_name)
	return NULL;

    this		= sd_calloc(1, sizeof(log4c_category_t));
    this->cat_name	= sd_strdup(a_name);
    this->cat_priority	= LOG4C_PRIORITY_NOTSET;
    this->cat_additive	= 1;
    this->cat_appender	= NULL;
    this->cat_parent	= NULL;

    /* skip root category because it has a NULL parent */
    if (strcmp(LOG4C_CATEGORY_DEFAULT, a_name)) {
	char* tmp = sd_strdup(this->cat_name);

	this->cat_parent = log4c_category_get(dot_dirname(tmp));
	free(tmp);
    }
    return this;
}

/*******************************************************************************/
extern void log4c_category_delete(log4c_category_t* this)
{
    if (!this) 
	return;

    free(this->cat_name);
    free(this);
}

/*******************************************************************************/
extern const char* log4c_category_get_name(const log4c_category_t* this)
{
    return (this ? this->cat_name : "(nil)");
}

/*******************************************************************************/
extern int log4c_category_get_priority(const log4c_category_t* this)
{
    return (this ? this->cat_priority : LOG4C_PRIORITY_UNKNOWN);
}

/*******************************************************************************/
extern int log4c_category_get_chainedpriority(const log4c_category_t* this)
{
    const log4c_category_t* cat = this;

    if (!this) 
	return LOG4C_PRIORITY_UNKNOWN;

    while (cat->cat_priority == LOG4C_PRIORITY_NOTSET && cat->cat_parent)
	cat = cat->cat_parent;
	
    return cat->cat_priority;
}

/*******************************************************************************/
extern const log4c_appender_t* log4c_category_get_appender(const log4c_category_t* this)
{
    return (this ? this->cat_appender : NULL);
}

/*******************************************************************************/
extern int log4c_category_get_additivity(const log4c_category_t* this)
{
    return (this ? this->cat_additive : -1);
}

/*******************************************************************************/
extern int log4c_category_set_priority(log4c_category_t* this, int a_priority)
{
    int previous;

    if (!this) 
	return LOG4C_PRIORITY_UNKNOWN;

    previous = this->cat_priority;
    this->cat_priority = a_priority;
    return previous;
}

/**
 * @todo need multiple appenders per category
 */

/*******************************************************************************/
extern const log4c_appender_t* log4c_category_set_appender(
    log4c_category_t* this, 
    log4c_appender_t* a_appender)
{
    log4c_appender_t* previous;

    if (!this) 
	return NULL;

    previous = this->cat_appender;
    this->cat_appender = a_appender;
    return previous;
}

/*******************************************************************************/
extern int log4c_category_set_additivity(log4c_category_t* this, int a_additivity)
{
    int previous;

    if (!this) 
	return -1;

    previous = this->cat_additive;
    this->cat_additive = a_additivity;
    return previous;
}

/*******************************************************************************/
extern void log4c_category_print(const log4c_category_t* this, FILE* a_stream)
{
    if (!this) 
	return;
    
    fprintf(a_stream, "{ name:'%s' priority:%s additive:%d appender:'%s' parent:'%s' }",
	    this->cat_name,
	    log4c_priority_to_string(this->cat_priority),
	    this->cat_additive,
	    log4c_appender_get_name(this->cat_appender),
	    log4c_category_get_name(this->cat_parent)
	);
}

/*******************************************************************************/
static void call_appenders(const log4c_category_t*	this,
			   log4c_logging_event_t*	a_event)
{
    if (!this) 
	return;
    
    if (this->cat_appender)
	log4c_appender_append(this->cat_appender, a_event);

    if (this->cat_additive && this->cat_parent)
	call_appenders(this->cat_parent, a_event);
}

extern void __log4c_category_vlog(const log4c_category_t* this, 
				  const log4c_location_info_t* a_locinfo, 
				  int a_priority,
				  const char* a_format, 
				  va_list a_args)
{
    char* message;
    log4c_logging_event_t evt;

    if (!this)
	return;

    if (!log4c_rc->config.bufsize)
	message = sd_vsprintf(a_format, a_args);
    else {
	int n;

	message = alloca(log4c_rc->config.bufsize);
	if ( (n = vsnprintf(message, log4c_rc->config.bufsize, a_format, a_args))
	     >= log4c_rc->config.bufsize)
	    sd_error("truncating message of %d bytes (bufsize = %d)", n, 
			log4c_rc->config.bufsize);
	    
    }
	
    evt.evt_category	= this->cat_name;
    evt.evt_priority	= a_priority;
    evt.evt_msg	        = message;
    evt.evt_loc	        = a_locinfo;
    gettimeofday(&evt.evt_timestamp, NULL);

    call_appenders(this, &evt);
    
    if (!log4c_rc->config.bufsize) 
	free(message);
}

/*******************************************************************************/
static const char* dot_dirname(char* a_string)
{
    char* p;

    if (!a_string)
	return NULL;

    if ( (p = rindex(a_string, '.')) == NULL)
	return LOG4C_CATEGORY_DEFAULT;
    
    *p = '\0';
    return a_string;
}
