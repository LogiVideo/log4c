#ifndef __log4c_stringutil_h
#define __log4c_stringutil_h


#ifdef __cplusplus
extern "C" {
#endif


/*
 * Appends a string to a given buffer, reallocating the buffer if necessary.
 *
 * Arguments:
 *   buffer:		Address of the buffer to which the input string is to be appended.
 *   buffer_len:	Currently allocated size of the buffer.
 *                  Note that this is not necessarily equal to strlen(buffer).
 *   input:			Input string to be appended to the buffer. Does not need to be null-terminated.
 *   input_len:		Length of the input string to be appended to the buffer.
 *
 * If there is enough space to append the input string the buffer is updated accordingly and is
 * correctly null-terminated.
 * If there is not enough space the buffer is reallocated to fit the new content. As a result the
 * address in the 'buffer' pointer and the value of 'buffer_len' may be updated.
 * If reallocation fails the existing buffer is freed and the 'buffer' pointer is set to NULL.
 */
void
append_string(char **buffer, size_t *buffer_len, const char *input, size_t input_len);


/*
 * Returns a copy of the input string with variables of the format ${VAR} expanded.
 *
 * Arguments:
 *   input:			Input string in which to replace variables.
 *
 * Return value:
 *   A newly allocated, null-terminated string containing the input string with variables replaced
 *   or NULL if an error has occurred.
 *
 * Variable references in the input string need to be of the format "${VAR}" where VAR is the name
 * of the variable. For example, if 'HOME' is currently set to "/home/user" the input string
 * "${HOME}, sweet home" would result in "/home/user, sweet home" being returned.
 * Unset variables are replaced with an empty string.
 * If the input string contains invalid variable references such as a "${" without a matching "}"
 * or "${}" the behavior is undefined.
 *
 * This function replaces the following variables:
 * - LOG4C_PROCESS_ID: The ID of the current process (PID)
 * - LOG4C_PROCESS_NAME: The name of the current process (basically argv[0] without the path)
 * - Environment variables
 */
char *
expand_variables(const char *input);


#ifdef __cplusplus
}
#endif


#endif
