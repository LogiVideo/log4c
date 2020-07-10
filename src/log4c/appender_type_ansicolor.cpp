/*
 * ANSI color terminal appender
 *
 * This appender colors logging output according to category. Colors are given
 * to categories in the order they are used. If too many categories are used
 * simultaneously (more than 11 at this point) the extra categories aren't
 * colored. (This is because there aren't enough well-readable colors in the
 * average ANSI color terminal.)
 *
 * A 'stream' argument can be provided to the appender to choose between
 * stdout and stderr for the output.
 *
 * The following is an example of how the appender could be used in the
 * configuration file:
 *   <appender name="stdoutc" type="ansicolor" stream="stdout" layout="dated" />
 *   <appender name="stderrc" type="ansicolor" stream="stderr" layout="dated" />
*/

#include <algorithm>
#include <iterator>
#include <unordered_map>
#include <utility>
#include <vector>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
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


namespace
{

	// ANSI console colors
	enum Color
	{
		Default,
		Black,
		Blue,
		Green,
		Cyan,
		Red,
		Purple,
		Brown,
		LightGray,
		DarkGray,
		LightBlue,
		LightGreen,
		LightCyan,
		LightRed,
		LightPurple,
		Yellow,
		White,
	};

	// Escape sequences for console colors
	using AnsiColorStringMapType = vector<pair<Color, const char *>>;
	const auto AnsiColorStrings = AnsiColorStringMapType{
		// Good colors
		{ Color::Yellow,		"\033[1;33m" },
		{ Color::LightCyan,		"\033[1;36m" },
		{ Color::LightRed,		"\033[1;31m" },
		{ Color::LightPurple,	"\033[1;35m" },
		{ Color::LightGreen,	"\033[1;32m" },
		{ Color::LightBlue,		"\033[1;34m" },
		{ Color::Green,			"\033[0;32m" },
		{ Color::Cyan,			"\033[0;36m" },
		{ Color::Red,			"\033[0;31m" },
		{ Color::Purple,		"\033[0;35m" },
		{ Color::Brown,			"\033[0;33m" },
		// Bad colors
		{ Color::LightGray,		"\033[0;37m" },
		{ Color::DarkGray,		"\033[1;30m" },
		{ Color::Blue,			"\033[0;34m" },
		{ Color::White,			"\033[1;37m" },
		// Unusable colors
		{ Color::Black,			"\033[0;30m" },
		{ Color::Default,		"\033[0m" },
	};

	AnsiColorStringMapType::const_iterator
	FindAnsiColorString(Color color)
	{
		return find_if(begin(AnsiColorStrings), end(AnsiColorStrings),
			[color] (const AnsiColorStringMapType::value_type& p) { return p.first == color; }
		);
	}

	const auto FirstGoodColor	= FindAnsiColorString(Color::Yellow);
	const auto LastGoodColorP1	= next(FindAnsiColorString(Color::Brown));	// Last good color plus one

	const auto ResetColorString	= FindAnsiColorString(Color::Default)->second;

}


namespace
{

	// Helper class to hold category color state.
	// This is a separate class from ansicolor_udata_t to ensure that the destructor of this class'
	// members is called, something that is not the case for ansicolor_udata_t.
	class ColorState
	{
		unordered_map<string, const char *>
					categoryColorStrings_{
						{ "root",	FindAnsiColorString(Color::Default)->second },
						{ "global",	FindAnsiColorString(Color::Default)->second },
					};
		AnsiColorStringMapType::const_iterator
					nextGoodColor_{ FirstGoodColor };

	public:
		const char *
		GetNextGoodColorString()
		{
			if(nextGoodColor_ != LastGoodColorP1)
			{
				return (nextGoodColor_++)->second;
			}
			return FindAnsiColorString(Color::Default)->second;
		}

		const char *
		GetCategoryAnsiColorString(const string& category)
		{
			const auto it = categoryColorStrings_.find(category);
			if(it != end(categoryColorStrings_))
			{
				return it->second;
			}
			else
			{
				return categoryColorStrings_.insert(
					make_pair(category, GetNextGoodColorString())
				).first->second;
			}
		}
	};

	struct ansicolor_udata_t
	{
		// Init state (valid after init)
		FILE *				fh_;			// Handle to the console (always stdout or stderr)
		// Open state (valid between open and close)
		ColorState *		colorState_;
	};


	// Note: Do not access directly. Use acquire_lock() and release_lock() instead.
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


	// Allocates a context structure for use by the 'ansicolor' appender
	ansicolor_udata_t *
	make_udata()
	{
		const auto udata = static_cast<ansicolor_udata_t *>(malloc(sizeof(ansicolor_udata_t)));
		if(udata != NULL)
		{
			memset(udata, 0, sizeof(*udata));
		}
		return udata;
	}


	int
	acquire_lock()
	{
		int res = pthread_mutex_lock(&mutex);
		assert(res == 0);
		if(res != 0)
		{
			sd_error("[ansicolor.acquire_lock] Unable to acquire lock.");
		}

		return res;
	}


	int
	release_lock()
	{
		int res = pthread_mutex_unlock(&mutex);
		assert(res == 0);
		if(res != 0)
		{
			sd_error("[ansicolor.release_lock] Unable to release lock.");
		}

		return res;
	}


	extern "C"
	int
	ansicolor_init(log4c_appender_t *ctx, const log4c_appender_init_data_t *init_data)
	{
		sd_debug("[ansicolor_init] init_data=%p", (void *)init_data);

		// Check if we got a DOM node. Without a DOM node we can't configure ourselves.
		const auto node = static_cast<sd_domnode_t *>(init_data->dom_node);
		if(node == NULL)
			return -1;

		// Check if we have a stream name and validate it
		const auto stream = sd_domnode_attrs_get_expanded(node, "stream");
		FILE *fh = nullptr;
		const char *streamname = nullptr;
		if(!stream || !stream->value || strlen(stream->value) == 0)
		{
			fh = stderr;	// No stream specified => Use a sensible default
			streamname = "stderr";
		}
		else if(strcasecmp(stream->value, "stdout") == 0)
		{
			fh = stdout;
			streamname = "stdout";
		}
		else if(strcasecmp(stream->value, "stderr") == 0)
		{
			fh = stderr;
			streamname = "stderr";
		}
		else
		{
			sd_error("[ansicolor_init] Invalid stream name '%s' specified.", stream->value);
			return -1;
		}

		// Create a context and associate it with the appender. It is now owned by log4c.
		const auto udata = make_udata();
		if(udata == NULL)
			return -1;
		sd_debug("[ansicolor_init]   created udata=%p", udata);
		log4c_appender_set_udata(ctx, udata);

		// Store the configuration data
		(void)streamname;	// Avoid warning in release builds where the usage below is removed
		sd_debug("[ansicolor_init]   stream='%s'", streamname);
		udata->fh_ = fh;

		return 0;
	}


	extern "C"
	int
	ansicolor_open(log4c_appender_t *ctx)
	{
		int ret = 0;

		acquire_lock();

		// Retrieve the context
		const auto udata = static_cast<ansicolor_udata_t *>(log4c_appender_get_udata(ctx));
		sd_debug("[ansicolor_open] udata=%p", udata);
		assert(udata != NULL);
		if(udata == NULL)
		{
			ret = -1;
			goto done;
		}

		// Create the "open state" of the context
		assert(udata->colorState_ == NULL);
		udata->colorState_ = new ColorState();

	done:
		release_lock();
		return ret;
	}


	extern "C"
	int
	ansicolor_close(log4c_appender_t *ctx)
	{
		int ret = 0;

		acquire_lock();

		const auto udata = static_cast<ansicolor_udata_t *>(log4c_appender_get_udata(ctx));
		sd_debug("[ansicolor_close] udata=%p", udata);
		if(udata == NULL)
			goto done;		// This is not considered an error

		// Destroy the "open state" of the context
		if(udata->colorState_)
		{
			delete udata->colorState_;
			udata->colorState_ = nullptr;
		}

	done:
		release_lock();
		return ret;
	}


	extern "C"
	int
	ansicolor_append(log4c_appender_t *ctx, const log4c_logging_event_t *a_event)
	{
		int ret = 0;

		// Note: This lock isn't strictly necessary because liblog currently synchronizes calls to
		//       log4c_category_log() internally. However, this may change in the future, so just to
		//       be on the safe side we acquire the lock anyway. This also makes sure that append
		//       operations are synchronized against open/close operations, which is not guaranteed
		//       by log4c at this point.
		acquire_lock();

		const auto udata = static_cast<ansicolor_udata_t *>(log4c_appender_get_udata(ctx));
		sd_debug("[ansicolor_append] udata=%p, category='%s'", udata, a_event->evt_category);
		//sd_debug("[ansicolor_append] udata=%p, category='%s', msg='%s', rmsg='%s'", udata, a_event->evt_category, a_event->evt_msg, a_event->evt_rendered_msg);
		assert(udata != NULL);
		if(!udata)
		{
			ret = -1;
			goto done;
		}

		// Print the message using the following order:
		//   1. Color code
		//   2. Rendered message with "\n" removed
		//   3. Color reset code
		//   4. "\n"
		// Printing everything in a single call and printing the color reset code before the newline
		// increases resilience against accidentally coloring output that is not from log4c,
		// especially in the case where stderr and stdout are both being output to.
		// Note that, by convention, the rendered message ends with "\n".
		{
			const auto msgLen = static_cast<int>(strlen(a_event->evt_rendered_msg));
			const auto truncLen = msgLen >= 1 ? 1 : 0;
			ret = fprintf(udata->fh_, "%s%.*s%s\n",
				udata->colorState_->GetCategoryAnsiColorString(a_event->evt_category),
				msgLen - truncLen, a_event->evt_rendered_msg,
				ResetColorString
			);
		}

	done:
		release_lock();
		return ret;
	}

}


extern "C"
const log4c_appender_type_t log4c_appender_type_ansicolor = {
	"ansicolor",
	ansicolor_open,
	ansicolor_append,
	ansicolor_close,
	ansicolor_init,
};
