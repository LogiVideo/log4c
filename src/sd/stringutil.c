#ifdef __linux__
#define _GNU_SOURCE
#endif
#include <assert.h>
#include <errno.h>
#include <limits.h>		// We do this to include features.h portably
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __linux__
#include <unistd.h>
#include <sys/types.h>
#endif

#include "stringutil.h"



/*
 * TYPES
 */

/*
 * Pointer to a function that can look up the value of a given variable.
 *
 * Arguments:
 *   key:			Name of the variable to be expanded.
 *   was_allocated:	Set to a non-zero value on return if the returned string has been allocated.
 *                  The content of this variable is meaningless if NULL is returned.
 *
 * Returns:
 *   The null-terminated value of the given key or NULL if the key is unknown/invalid.
 */
typedef char * (*variable_expansion_fun_t)(const char *key, int *was_allocated);



/*
 * FUNCTIONS
 */


#if 0		// For testing the case where realloc() fails

void *
realloc(void *ptr, size_t size)
{
	(void)ptr;
	(void)size;
	return NULL;
}

#endif


static inline
size_t
max(size_t a, size_t b)
{
	return (a > b) ? a : b;
}


void
append_string(char **buffer, size_t *buffer_len, const char *input, size_t input_len)
{
	const size_t buffer_filled_len = *buffer ? strlen(*buffer) : 0;

	if(buffer_filled_len + input_len + 1 <= *buffer_len)
	{
		// Append the input to the buffer if there is enough space
		strncat(*buffer, input, input_len);
		(*buffer)[buffer_filled_len + input_len] = '\0';
	}
	else
	{
		// Otherwise reallocate the buffer and append the input
#if 0
		const size_t new_buffer_len = buffer_filled_len + input_len + 1;
#else
		// Optimization: Always increase the buffer size by at least 32 bytes.
		const size_t new_buffer_len = max(buffer_filled_len + input_len + 1, *buffer_len + 32);
#endif
		char *new_buffer = (char *)realloc(*buffer, new_buffer_len);
		if(new_buffer != NULL)
		{
			strncpy(new_buffer + buffer_filled_len, input, input_len);
			new_buffer[buffer_filled_len + input_len] = '\0';
			*buffer_len = new_buffer_len;
		}
		else
		{
			// If reallocation failed free the old buffer and set the buffer pointer to NULL
			free(*buffer);
			*buffer_len = 0;
		}
		*buffer = new_buffer;
	}

}


static
char *
expand_special_variable(const char *key, int *was_allocated)
{
	assert(key != NULL);
	assert(was_allocated != NULL);

	*was_allocated = 0;

	if(strcmp(key, "LOG4C_PROCESS_ID") == 0)
	{
#ifdef __linux__
		char *value = NULL;
		int res = asprintf(&value, "%u", getpid());
		if(res != -1)
		{
			*was_allocated = 1;
			return value;
		}
#else
#error out1
#warning LOG4C_PROCESS_ID special variable expansion not implemented.
#endif
	}
	else if(strcmp(key, "LOG4C_PROCESS_NAME") == 0)
	{
#ifdef __GLIBC__
		return program_invocation_short_name;
#else
#error out2
#warning LOG4C_PROCESS_NAME special variable expansion not implemented.
#endif
	}

	*was_allocated = 0;
	return NULL;
}


static
char *
expand_environment_variable(const char *key, int *was_allocated)
{
	assert(key != NULL);
	assert(was_allocated != NULL);

	*was_allocated = 0;
	return getenv(key);
}


static
char *
expand_variables_fun(const char *input, const variable_expansion_fun_t *expansion_functions)
{
	if(!input)
		return NULL;

	char *output = NULL;					// Output buffer (allocated when necessary)
	size_t output_len = 0;					// Output buffer length

	const char *copied_input_end = input;	// Points past the part of the input that has been copied to the output buffer

	const char * const input_end = input + strlen(input);	// Points past the end of the input buffer (to the '\0')
	assert(*input_end == '\0');
	while(input < input_end)
	{
		if(input[0] == '$' && input[1] == '{')
		{
			// Extract the variable name
			const char * const key = input + 2;				// Points to the start of the variable name
			const char *key_end = key;						// Points past the end of the variable name
			while(key_end < input_end && *key_end != '}')
				key_end++;
			if(key_end == input_end)
			{
				input = input_end;			// Encountered string end while looking for '}' => Leave the "${" as-is
				break;
			}
			if(key_end == key)
			{
				input = key_end + 1;		// Empty variable name => Leave the "${}" as-is
				continue;
			}
			assert(*key_end == '}');

			// Append everything up to the ${...} to the output string
			append_string(&output, &output_len, copied_input_end, input - copied_input_end);
			if(!output)
				return NULL;				// Bail out if reallocation has failed (append_string() has freed 'output' already)

			// Append the variable value to the output string
			{
				// Copy the key, so we have a null-terminated string for getenv()
				const size_t key_len = key_end - key;
				char name[key_len + 1];
				strncpy(name, key, key_len);
				name[key_len] = '\0';

				// Try all expansion functions until one of them returns a value
				const variable_expansion_fun_t *expansion_fun = expansion_functions;
				while(*expansion_fun)
				{
					int was_allocated = 0;
					char *value = (*expansion_fun)(name, &was_allocated);
					if(value)
					{
						append_string(&output, &output_len, value, strlen(value));
						if(was_allocated)
						{
							free(value);
							value = NULL;
						}
						if(!output)
							return NULL;			// Bail out if reallocation has failed (append_string() has freed 'output' already)

						break;
					}

					expansion_fun++;
				}
			}

			copied_input_end = key_end + 1;
			input = key_end;
		}

		input++;
	}
	assert(*input == '\0');

	// Append everything that hasn't been copied yet to the output string
	append_string(&output, &output_len, copied_input_end, input - copied_input_end);
	// If reallocation has failed 'output' has already been freed and set to NULL.

	return output;
}


char *
expand_variables(const char *input)
{
	const variable_expansion_fun_t functions[] = {
		expand_special_variable,
		expand_environment_variable,
		NULL
	};

	return expand_variables_fun(input, functions);
}
