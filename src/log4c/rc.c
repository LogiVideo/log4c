static const char version[] = "$Id$";

/*
* rc.c
*
* Copyright 2001-2003, Meiosys (www.meiosys.com). All rights reserved.
*
* See the COPYING file for the terms of usage and distribution.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <log4c/rc.h>
#include <log4c/category.h>
#include <log4c/appender.h>
#include <log4c/layout.h>
#include <log4c/appender_type_rollingfile.h>
#include <log4c/rollingpolicy.h>
#include <log4c/rollingpolicy_type_sizewin.h>
#include <log4c/appender_type_socket.h>
#include <log4c/version.h>
#include <sd/error.h>
#include <sd/domnode.h>
#include <sd/malloc.h>
#include <sd/sd_xplatform.h>
#include <sd/factory.h>
#include <stdlib.h>
#include <string.h>
#if !(defined(_WIN32) || defined(_WIN64)) || (_MSC_VER >= 1600)
	#include <errno.h>
#else
	static int errno = 0;
#endif


static log4c_rc_t __log4c_rc = { { 0, 0, 0, 0 } };

log4c_rc_t* const log4c_rc = &__log4c_rc;

const char * NOT_SET = "(not set)";

/******************************************************************************/
static long parse_byte_size (const char *astring)
{
	/* Parse size in bytes depending on the suffix.   Valid suffixes are KB, MB and GB */
	size_t sz = strlen (astring);
	long res = strtol(astring, (char **) NULL, 10);

	if (res <= 0)
		return 0;

	if (astring[ sz - 1 ] == 'B') {
		switch (astring[ sz - 2 ]) {
		case 'K':
			res *= 1024;
			break;
		case 'M':
			res *= 1024 * 1024;
			break;
		case 'G':
			res *= 1024 * 1024 * 1024;
			break;
		default:
			sd_debug("Wrong suffix parsing size in bytes for string %s, ignoring suffix", 
				astring);
		}
	}
	sd_debug("Parsed size parameter %s to value %ld",astring, res);
	return (res);
}

/******************************************************************************/
static int config_load(log4c_rc_t* this, sd_domnode_t* anode)
{
	sd_list_iter_t* i = NULL;

	for (i = sd_list_begin(anode->children); i != sd_list_end(anode->children); 
		i = sd_list_iter_next(i)) 
	{
		sd_domnode_t* node = i->data;

		if (!strcmp(node->name, "nocleanup")) {
			this->config.nocleanup = atoi(node->value);
			if (this->config.nocleanup)
				sd_debug("deactivating log4c cleanup");		
		}

		if (!strcmp(node->name, "bufsize")) {	    
			this->config.bufsize = parse_byte_size(node->value);

			if (this->config.bufsize)
				sd_debug("using fixed buffer size of %d bytes", 
				this->config.bufsize);
			else
				sd_debug("using dynamic allocated buffer");
		}

		if (!strcmp(node->name, "debug")) {
			sd_domnode_t* level = sd_domnode_attrs_get(node, "level");

			if (level) {
				this->config.debug = atoi(level->value);
				sd_debug("activating log4c debugging. level = %d", this->config.debug);
			}
		}
		if (!strcmp(node->name, "reread")) {
			this->config.reread = atoi(node->value);
			sd_debug("log4crc reread is %d",this->config.reread);
			if (0 == this->config.reread)
				sd_debug("deactivating log4crc reread");
		}
	}

	return 0;
}

/******************************************************************************/
static int category_load(log4c_rc_t* this, sd_domnode_t* anode)
{
	sd_domnode_t*     name     = sd_domnode_attrs_get(anode, "name");
	sd_domnode_t*     priority = sd_domnode_attrs_get(anode, "priority");
	sd_domnode_t*     additivity = sd_domnode_attrs_get(anode, "additivity");
	sd_domnode_t*     appender = sd_domnode_attrs_get(anode, "appender");
	log4c_category_t* cat      = NULL;

	if (!name) {
		sd_error("attribute \"name\" is missing");
		return -1;
	}

	cat = log4c_category_get(name->value);

	if (priority)
		log4c_category_set_priority(
		cat, log4c_priority_to_int(priority->value));

	if (additivity) {
		if (!strcasecmp(additivity->value, "false")) {
			log4c_category_set_additivity(cat, 0);
		} else if (!strcasecmp(additivity->value, "true")) {
			log4c_category_set_additivity(cat, 1);
		} else {
			sd_error("additivity value is invalid : %s", additivity->value);
		}
	}

	if (appender)
		log4c_category_set_appender(
		cat, log4c_appender_get(appender->value));

	return 0;
}

/******************************************************************************/
/* JAN: edited this method to include options to get our new "socket" appender
type. */
static int appender_load(log4c_rc_t* this, sd_domnode_t* anode)
{
	sd_domnode_t*     name   = sd_domnode_attrs_get(anode, "name");
	sd_domnode_t*     type   = sd_domnode_attrs_get(anode, "type");
	sd_domnode_t*     layout = sd_domnode_attrs_get(anode, "layout");
	log4c_appender_t* app    = NULL;
	socket_udata_t* sock = NULL;

	char oldpath[256] = {0};
	char newpath[256] = {0};
	const char* homePath = NULL;
	int chdirResult = 0;

	if (!name) {
		sd_error("attribute \"name\" is missing");
		return -1;
	}

	sd_debug("appender_load[name='%s'", 
		(name->value ? name->value : NOT_SET ));

	app = log4c_appender_get(name->value);

	if (type){
		sd_debug("appender type is '%s'",
			(type->value ? type->value: NOT_SET ));
		log4c_appender_set_type(app, log4c_appender_type_get(type->value));

		/* JAN: code to fetch additional parameters for socket appender */
		if( type->value != NULL )
		{
			/* MR: Pass some initialization data to the appender's init function, so that it can
			 * read its own attributes. That way, when adding a new appender, log4c itself does
			 * not need to be modified. */
			{
				log4c_appender_init_data_t id;
				id.dom_node = (void *)anode;
				int res = log4c_appender_init(app, &id);
				if (res) {
					sd_error("failed to initialize appender of type '%s': error %d", type->value, res);
					return res;
				}
			}

			if (!strcasecmp(type->value, "socket")) 
			{
				sd_domnode_t*  destport = sd_domnode_attrs_get(anode,"destport");
				sd_domnode_t*  dest = sd_domnode_attrs_get(anode,"dest");

				sd_debug("destport='%s', dest='%s'",
					(destport && destport->value ? destport->value : NOT_SET ),
					(dest && dest->value ? dest->value : NOT_SET ));

				sock = socket_make_udata();

				socket_udata_set_destport( sock, (destport && destport->value ? (char*)destport->value : DEFAULT_DESTINATION_PORT ));
				socket_udata_set_dest( sock, (dest && dest->value ? (char*)dest->value : DEFAULT_DESTINATION));
				log4c_appender_set_udata(app, sock);
			}

	#ifdef WITH_ROLLINGFILE
			if ( !strcasecmp(type->value, "rollingfile")) {
				rollingfile_udata_t *rfup = NULL;
				log4c_rollingpolicy_t *rollingpolicyp = NULL;
				sd_domnode_t*  logdir = sd_domnode_attrs_get_expanded(anode,
					"logdir");
				sd_domnode_t*  logprefix = sd_domnode_attrs_get_expanded(anode,
					"prefix");
				sd_domnode_t*  rollingpolicy_name = sd_domnode_attrs_get(anode,
					"rollingpolicy");

				sd_debug("logdir='%s', prefix='%s', rollingpolicy='%s'",
					(logdir && logdir->value ? logdir->value : NOT_SET),
					(logprefix && logprefix->value ? logprefix->value :NOT_SET),
					(rollingpolicy_name && rollingpolicy_name->value ?
					rollingpolicy_name->value : NOT_SET));

				if(logdir && logdir->value)
				{
					getcwd(oldpath, sizeof(oldpath));
					if((logdir->value[0] == '~') && 
				   	   (logdir->value[1] == '/' || logdir->value[1] == '\\'))
					{
						// The path starts with something like ~/
						// Treat that as if they want ~ to expand to the home directory.
						sd_debug("logdir starts with ~. Try $HOME environment variable.");
						homePath = getenv("HOME");
						if(homePath == NULL)
						{
							// No $HOME. Just go for it.
							chdirResult = chdir((char *)logdir->value);
						}
						else
						{
							chdir(homePath);
							chdirResult = chdir((char*)&(logdir->value[2]));
						}
					}
					else
					{
						// Use the logdir as-is
						chdirResult = chdir((char *)logdir->value);
					}
					sd_debug("chdir to logdir result: %d, errno: %d", chdirResult, errno);
					getcwd(newpath, sizeof(newpath));
					chdir(oldpath);
				}

				rfup = rollingfile_make_udata();              
				if (newpath[0] == 0)
				{
					sd_debug("newpath for logdir is empty. Using %s", logdir && logdir->value ? logdir->value : NOT_SET);
					rollingfile_udata_set_logdir(rfup, (char *)logdir->value);
				}
				else
				{
					sd_debug("newpath for logdir is %s", newpath);
					rollingfile_udata_set_logdir(rfup, newpath);
				}
				rollingfile_udata_set_files_prefix(rfup, (char *)logprefix->value);

				if (rollingpolicy_name){
					/* recover a rollingpolicy instance with this name */
					rollingpolicyp = log4c_rollingpolicy_get(rollingpolicy_name->value);

					/* connect that policy to this rollingfile appender conf */
					rollingfile_udata_set_policy(rfup, rollingpolicyp);
					log4c_appender_set_udata(app, rfup);

					/* allow the policy to initialize itself */
					log4c_rollingpolicy_init(rollingpolicyp, rfup);
				} else {
					/* no rollingpolicy specified, default to default sizewin */
					sd_debug("no rollingpolicy name specified--will default");
				}
			}
		}
#endif
	}

	if (layout)
		log4c_appender_set_layout(app, log4c_layout_get(layout->value));

	sd_debug("]");

	return 0;
}

/******************************************************************************/
static int layout_load(log4c_rc_t* this, sd_domnode_t* anode)
{
	sd_domnode_t*   name   = sd_domnode_attrs_get(anode, "name");
	sd_domnode_t*   type   = sd_domnode_attrs_get(anode, "type");
	log4c_layout_t* layout = NULL;

	if (!name) {
		sd_error("attribute \"name\" is missing");
		return -1;
	}

	layout = log4c_layout_get(name->value);

	if (type)
		log4c_layout_set_type(layout, log4c_layout_type_get(type->value));

	return 0;
}

#ifdef WITH_ROLLINGFILE
/******************************************************************************/
static int rollingpolicy_load(log4c_rc_t* this, sd_domnode_t* anode)
{
	sd_domnode_t*   name   = sd_domnode_attrs_get(anode, "name");
	sd_domnode_t*   type   = sd_domnode_attrs_get(anode, "type");
	log4c_rollingpolicy_t* rpolicyp = NULL;
	long a_maxsize;

	sd_debug("rollingpolicy_load[");
	if (!name) {
		sd_error("attribute \"name\" is missing");
		return -1;
	}

	if( name )
		rpolicyp = log4c_rollingpolicy_get(name->value);    

	if (type){
		log4c_rollingpolicy_set_type(rpolicyp,
			log4c_rollingpolicy_type_get(type->value));

		if (!strcasecmp(type->value, "sizewin")){
			sd_domnode_t*   maxsize   = sd_domnode_attrs_get(anode, "maxsize");
			sd_domnode_t*   maxnum  = sd_domnode_attrs_get(anode, "maxnum");
			rollingpolicy_sizewin_udata_t *sizewin_udatap = NULL;

			sd_debug("type='sizewin', maxsize='%s', maxnum='%s', "
				"rpolicyname='%s'",
				(maxsize && maxsize->value ? maxsize->value :NOT_SET),
				(maxnum && maxnum->value ? maxnum->value :NOT_SET),
				(name && name->value ? name->value :NOT_SET));
			/*
			* Get a new sizewin policy type and configure it.
			* Then attach it to the policy object.
			* Check to see if this policy already has a
			sw udata object.  If so, leave as is except update
			the params
			*/
			if ( !(sizewin_udatap = log4c_rollingpolicy_get_udata(rpolicyp))){ 
				sd_debug("creating new sizewin udata for this policy");
				sizewin_udatap = sizewin_make_udata();
				log4c_rollingpolicy_set_udata(rpolicyp,sizewin_udatap);   
				a_maxsize = parse_byte_size(maxsize->value);
				if (a_maxsize)
					sizewin_udata_set_file_maxsize(sizewin_udatap, a_maxsize);
				else{
					sd_debug("When parsing %s a size of 0 was returned. Default size %d will be used",
						maxsize->value, ROLLINGPOLICY_SIZE_DEFAULT_MAX_FILE_SIZE);
					sizewin_udata_set_file_maxsize(sizewin_udatap, ROLLINGPOLICY_SIZE_DEFAULT_MAX_FILE_SIZE); 
				}

				sizewin_udata_set_max_num_files(sizewin_udatap, atoi(maxnum->value));
			}else{
				sd_debug("policy already has a sizewin udata--just updating params");
				sizewin_udata_set_file_maxsize(sizewin_udatap, parse_byte_size(maxsize->value));
				sizewin_udata_set_max_num_files(sizewin_udatap, atoi(maxnum->value));
				/* allow the policy to initialize itself */
				log4c_rollingpolicy_init(rpolicyp, 
					log4c_rollingpolicy_get_rfudata(rpolicyp));
			}

		}

	}
	sd_debug("]");

	return 0;
}
#endif

/******************************************************************************/
extern int log4c_rc_load(log4c_rc_t* this, const char* a_filename)
{    
	sd_list_iter_t* i = NULL;
	sd_domnode_t*   node = NULL;        
	sd_domnode_t*   root_node = NULL;

	sd_debug("parsing file '%s'\n", a_filename);

	if (!this)
		return -1;

	root_node = sd_domnode_new(NULL, NULL);

	if (sd_domnode_load(root_node, a_filename) == -1) {
		sd_domnode_delete(root_node);
		return -1;
	}

	/* Check configuration file root node */
	if (strcmp(root_node->name, "log4c")) {
		sd_error("invalid root name %s", root_node->name);
		sd_domnode_delete(root_node);
		return -1;
	}

	/* Check configuration file revision */
	if ( (node = sd_domnode_attrs_get(root_node, "version")) != NULL)
		if (strcmp(log4c_version(), node->value)) {
			sd_error("version mismatch: %s != %s", log4c_version(), node->value);
			sd_domnode_delete(root_node);
			return -1;
		}

		/* backward compatibility. */
		if ( (node = sd_domnode_attrs_get(root_node, "cleanup")) != NULL) {
			sd_debug("attribute \"cleanup\" is deprecated");
			this->config.nocleanup = !atoi(node->value);
		}

		/* load configuration elements */
		for (i = sd_list_begin(root_node->children);
			i != sd_list_end(root_node->children); 
			i = sd_list_iter_next(i)) 
		{
			node = i->data;

			if (!strcmp(node->name, "category")) category_load(this, node);
			if (!strcmp(node->name, "appender")) appender_load(this, node);
#ifdef WITH_ROLLINGFILE
			if (!strcmp(node->name, "rollingpolicy"))rollingpolicy_load(this, node);
#endif
			if (!strcmp(node->name, "layout"))   layout_load(this, node);
			if (!strcmp(node->name, "config"))   config_load(this, node);
		}

		sd_domnode_delete(root_node);

		return 0;
}

/******************************************************************************/
extern int log4c_load(const char* a_filename)
{
	return log4c_rc_load(&__log4c_rc, a_filename);
}
