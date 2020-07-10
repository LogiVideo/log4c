#include <string>
#include <cassert>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include <linux/limits.h>

#include <log4c.h>

extern "C"
{
// domnode.h has the unfortunate convention of using the identifier 'this' for its context
// variables. We therefore rename those temporarily.
#define this thiz
#include <sd/domnode.h>
#undef this
#ifndef NDEBUG
#include <sd/error.h>
#else
// Disable log4c debugging for our module in release builds
#define sd_error(...) do {} while(false)
#define sd_debug(...) do {} while(false)
#endif
}


using namespace std;


typedef struct file_udata_t_
{
	FILE *		fh;					// Handle to the log file
	char		path[PATH_MAX];		// Path to the log file
} file_udata_t;



namespace
{


	// Note: Do not access directly. Use acquire_lock() and release_lock() instead.
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;



	// Allocates a context structure for use by the 'file' appender
	file_udata_t *
	make_udata(void)
	{
		file_udata_t *udata = (file_udata_t *)malloc(sizeof(file_udata_t));
		if(udata != NULL)
		{
			memset(udata, 0, sizeof(*udata));
		}
		return udata;
	}


	int
	acquire_lock(void)
	{
		int res = pthread_mutex_lock(&mutex);
		assert(res == 0);
		if(res != 0)
		{
			sd_error("[file.acquire_lock] Unable to acquire lock.");
		}

		return res;
	}


	int
	release_lock(void)
	{
		int res = pthread_mutex_unlock(&mutex);
		assert(res == 0);
		if(res != 0)
		{
			sd_error("[file.release_lock] Unable to release lock.");
		}

		return res;
	}


	extern "C"
	int
	file_init(log4c_appender_t *ctx, const log4c_appender_init_data_t *init_data)
	{
		sd_debug("[file_init] init_data=%p", (void *)init_data);

		// Check if we got a DOM node. Without a DOM node we can't configure ourselves.
		sd_domnode_t * const node = (sd_domnode_t *)init_data->dom_node;
		if(node == NULL)
			return -1;

		// Check if we have a path
		sd_domnode_t * const path = sd_domnode_attrs_get_expanded(node, "path");
		if(!path || !path->value)
			return -1;
		if(strlen(path->value) >= sizeof(file_udata_t::path))
			return -1;

		// Create a context and associate it with the appender. It is now owned by log4c.
		file_udata_t * const udata = make_udata();
		if(udata == NULL)
			return -1;
		sd_debug("[file_init]   created udata=%p", udata);
		log4c_appender_set_udata(ctx, udata);

		// Store the configuration data
		sd_debug("[file_init]   path='%s'", path->value);
		strcpy(udata->path, path->value);

		return 0;
	}


	extern "C"
	int
	file_open(log4c_appender_t *ctx)
	{
		int ret = 0;

		acquire_lock();

		// Retrieve the context
		file_udata_t *udata = (file_udata_t *)log4c_appender_get_udata(ctx);
		sd_debug("[file_open] udata=%p", udata);
		assert(udata != NULL);
		if(udata == NULL)
		{
			ret = -1;
			goto done;
		}

		// Open the file
		if(udata->fh == NULL)
		{
			udata->fh = fopen(udata->path, "w");
			assert(udata->fh != NULL);
			if(udata->fh == NULL)
			{
				ret = -1;
				goto done;
			}

			// Disable file buffering
			setbuf(udata->fh, NULL);
		}
		else
		{
			sd_debug("[file_open] Warning: File is already open.");
		}

	done:
		release_lock();
		return ret;
	}


	extern "C"
	int
	file_close(log4c_appender_t *ctx)
	{
		int ret = 0;

		acquire_lock();

		file_udata_t * const udata = (file_udata_t *)log4c_appender_get_udata(ctx);
		sd_debug("[file_close] udata=%p", udata);
		if(udata == NULL)
			goto done;		// This is not considered an error

		// Close the file handle
		if(udata->fh != NULL)
		{
			int res = fclose(udata->fh);
			if(res != 0)
			{
				ret = -errno;
				goto done;
			}
			udata->fh = NULL;
		}

	done:
		release_lock();
		return ret;
	}


	extern "C"
	int
	file_append(log4c_appender_t *ctx, const log4c_logging_event_t *a_event)
	{
		int ret = 0;

		// Note: This lock isn't strictly necessary because liblog currently synchronizes calls to
		//       log4c_category_log() internally. However, this may change in the future, so just to
		//       be on the safe side we acquire the lock anyway. This also makes sure that append
		//       operations are synchronized against open/close operations, which is not guaranteed
		//       by log4c at this point.
		acquire_lock();

		file_udata_t * const udata = (file_udata_t *)log4c_appender_get_udata(ctx);
		sd_debug("[file_append] udata=%p", udata);
		//sd_debug("[file_append] udata=%p, msg='%s', rmsg='%s'", udata, a_event->evt_msg, a_event->evt_rendered_msg);
		assert(udata != NULL);
		if(!udata)
		{
			ret = -1;
			goto done;
		}

		ret = fputs(a_event->evt_rendered_msg, udata->fh);

	done:
		release_lock();
		return ret;
	}


}


extern "C"
const log4c_appender_type_t log4c_appender_type_file = {
	"file",
	file_open,
	file_append,
	file_close,
	file_init,
};
