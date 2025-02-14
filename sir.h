/**
 * @file sir.h
 * @brief Public interface to libsir
 *
 * The functions and types defined here comprise the entire set intended for
 * use by an implementor of the library-unless modification is desired.
 *
 * @author    Ryan M. Lederman \<lederman@gmail.com\>
 * @date      2018-2023
 * @version   2.2.0
 * @copyright The MIT License (MIT)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef _SIR_H_INCLUDED
# define _SIR_H_INCLUDED

# include "sirplatform.h"
# include "sirtypes.h"

# if defined(__cplusplus)
extern "C" {
# endif

/**
 * @defgroup public Public Interface
 *
 * Functions and types that comprise the public interface to libsir.
 *
 * @addtogroup public
 * @{
 *
 * @defgroup publicfuncs Functions
 * @{
 */

/**
 * @brief Fills out a ::sirinit structure with default values.
 *
 * Creates an initialization configuration for libsir essentially using all of
 * the default values (i.e., levels, options, text styling).
 *
 * @note Does not fill in string fields, such as ::sirinit.name.
 *
 * @param   si   Pointer to a ::sirinit structure to receive default values.
 * @returns bool `true` if `si` is not `NULL`, `false` otherwise.
 */
bool sir_makeinit(sirinit* si);

/**
 * @brief Initializes libsir for use.
 *
 * Must be called before making any other calls into libsir (with the exception
 * of ::sir_makeinit).
 *
 * For every call to ::sir_init, there must be a corresponding call to
 * ::sir_cleanup. All exported libsir functions are thread-safe, so you may
 * initialize and cleanup on whichever thread you wish.
 *
 * @see ::sir_makeinit
 * @see ::sir_cleanup
 *
 * @param si Pointer to a ::sirinit structure containing the desired settings
 * and configuration. libsir makes a copy of this structure, so its lifetime
 * is not a concern.
 *
 * @returns bool `true` if initialization was successful, `false` otherwise. Call
 * ::sir_geterror to obtain information about any error that may have occurred.
 */
bool sir_init(sirinit* si);

/**
 * @brief Tears down and cleans up libsir after use.
 *
 * Deallocates resources such as memory buffers, file descriptors, etc. and
 * resets the internal state. No calls into libsir will succeed after calling
 * ::sir_cleanup (with the exception of ::sir_makeinit, ::sir_init, and
 * ::sir_geterror).
 *
 * May be called from any thread. If you wish to utilize libsir again during the
 * same process' lifetime, simply call ::sir_init again.
 *
 * @returns bool `true` if cleanup was successful, `false otherwise`. Call
 * ::sir_geterror to obtain information about any error that may have occurred.
 */
bool sir_cleanup(void);

/**
 * @brief Retrieves information about the last error that occurred.
 *
 * libsir maintains errors on a per-thread basis, so it's important that the
 * same thread that encountered a failed library call be the one to get the
 * error information.
 *
 * @param   message  A buffer of ::SIR_MAXERROR chars to receive an error message.
 * @returns uint16_t The error code. Possible error codes are listed in
 *                   ::sir_errorcode.
 */
uint16_t sir_geterror(char message[SIR_MAXERROR]);

/**
 * @brief Dispatches a ::SIRL_DEBUG level message.
 *
 * The message will be delivered to all destinations registered to receive
 * debug-level messages, each with their own formatting and styling options.
 *
 * @remark To change options or level registrations for `stdout`/`stderr`, call
 * ::sir_stdoutopts/::sir_stdoutlevels and ::sir_stderropts/::sir_stderrlevels,
 * respectively. To change the text styling on a per-level basis, call
 * ::sir_settextstyle.
 *
 * @see ::sir_level
 * @see ::sir_option
 * @see ::default
 *
 * @param   format A printf-style format string, representing the template for
 *                 the message to dispatch.
 * @param   ...    Arguments whose type and position align with the format
 *                 specifiers in `format`.
 * @returns bool   `true` if the message was dispatched succcessfully to all
 *                 registered destinations, `false` otherwise. Call ::sir_geterror
 *                 to obtain information about any error that may have occurred.
 */
bool sir_debug(const char* format, ...);

/**
 * @brief Dispatches a ::SIRL_INFO level message.
 *
 * The message will be delivered to all destinations registered to receive
 * information-level messages, each with their own formatting and styling options.
 *
 * @remark To change options or level registrations for `stdout`/`stderr`, call
 * ::sir_stdoutopts/::sir_stdoutlevels and ::sir_stderropts/::sir_stderrlevels,
 * respectively. To change the text styling on a per-level basis, call
 * ::sir_settextstyle.
 *
 * @see ::sir_level
 * @see ::sir_option
 * @see ::default
 *
 * @param   format A printf-style format string, representing the template for
 *                 the message to dispatch.
 * @param   ...    Arguments whose type and position align with the format
 *                 specifiers in `format`.
 * @returns bool   `true` if the message was dispatched succcessfully to all
 *                 registered destinations, `false` otherwise. Call ::sir_geterror
 *                 to obtain information about any error that may have occurred.
 */
bool sir_info(const char* format, ...);

/**
 * @brief Dispatches a ::SIRL_NOTICE level message.
 *
 * The message will be delivered to all destinations registered to receive
 * notice-level messages, each with their own formatting and styling options.
 *
 * @remark To change options or level registrations for `stdout`/`stderr`, call
 * ::sir_stdoutopts/::sir_stdoutlevels and ::sir_stderropts/::sir_stderrlevels,
 * respectively. To change the text styling on a per-level basis, call
 * ::sir_settextstyle.
 *
 * @see ::sir_level
 * @see ::sir_option
 * @see ::default
 *
 * @param   format A printf-style format string, representing the template for
 *                 the message to dispatch.
 * @param   ...    Arguments whose type and position align with the format
 *                 specifiers in `format`.
 * @returns bool   `true` if the message was dispatched succcessfully to all
 *                 registered destinations, `false` otherwise. Call ::sir_geterror
 *                 to obtain information about any error that may have occurred.
 */
bool sir_notice(const char* format, ...);

/**
 * @brief Dispatches a ::SIRL_WARN level message.
 *
 * The message will be delivered to all destinations registered to receive
 * warning-level messages, each with their own formatting and styling options.
 *
 * @remark To change options or level registrations for `stdout`/`stderr`, call
 * ::sir_stdoutopts/::sir_stdoutlevels and ::sir_stderropts/::sir_stderrlevels,
 * respectively. To change the text styling on a per-level basis, call
 * ::sir_settextstyle.
 *
 * @see ::sir_level
 * @see ::sir_option
 * @see ::default
 *
 * @param   format A printf-style format string, representing the template for
 *                 the message to dispatch.
 * @param   ...    Arguments whose type and position align with the format
 *                 specifiers in `format`.
 * @returns bool   `true` if the message was dispatched succcessfully to all
 *                 registered destinations, `false` otherwise. Call ::sir_geterror
 *                 to obtain information about any error that may have occurred.
 */
bool sir_warn(const char* format, ...);

/**
 * @brief Dispatches a ::SIRL_ERROR level message.
 *
 * The message will be delivered to all destinations registered to receive
 * error-level messages, each with their own formatting and styling options.
 *
 * @remark To change options or level registrations for `stdout`/`stderr`, call
 * ::sir_stdoutopts/::sir_stdoutlevels and ::sir_stderropts/::sir_stderrlevels,
 * respectively. To change the text styling on a per-level basis, call
 * ::sir_settextstyle.
 *
 * @see ::sir_level
 * @see ::sir_option
 * @see ::default
 *
 * @param   format A printf-style format string, representing the template for
 *                 the message to dispatch.
 * @param   ...    Arguments whose type and position align with the format
 *                 specifiers in `format`.
 * @returns bool   `true` if the message was dispatched succcessfully to all
 *                 registered destinations, `false` otherwise. Call ::sir_geterror
 *                 to obtain information about any error that may have occurred.
 */
bool sir_error(const char* format, ...);

/**
 * @brief Dispatches a ::SIRL_CRIT level message.
 *
 * The message will be delivered to all destinations registered to receive
 * crtical-level messages, each with their own formatting and styling options.
 *
 * @remark To change options or level registrations for `stdout`/`stderr`, call
 * ::sir_stdoutopts/::sir_stdoutlevels and ::sir_stderropts/::sir_stderrlevels,
 * respectively. To change the text styling on a per-level basis, call
 * ::sir_settextstyle.
 *
 * @see ::sir_level
 * @see ::sir_option
 * @see ::default
 *
 * @param   format A printf-style format string, representing the template for
 *                 the message to dispatch.
 * @param   ...    Arguments whose type and position align with the format
 *                 specifiers in `format`.
 * @returns bool   `true` if the message was dispatched succcessfully to all
 *                 registered destinations, `false` otherwise. Call ::sir_geterror
 *                 to obtain information about any error that may have occurred.
 */
bool sir_crit(const char* format, ...);

/**
 * @brief Dispatches a ::SIRL_ALERT level message.
 *
 * The message will be delivered to all destinations registered to receive
 * alert-level messages, each with their own formatting and styling options.
 *
 * @remark To change options or level registrations for `stdout`/`stderr`, call
 * ::sir_stdoutopts/::sir_stdoutlevels and ::sir_stderropts/::sir_stderrlevels,
 * respectively. To change the text styling on a per-level basis, call
 * ::sir_settextstyle.
 *
 * @see ::sir_level
 * @see ::sir_option
 * @see ::default
 *
 * @param   format A printf-style format string, representing the template for
 *                 the message to dispatch.
 * @param   ...    Arguments whose type and position align with the format
 *                 specifiers in `format`.
 * @returns bool   `true` if the message was dispatched succcessfully to all
 *                 registered destinations, `false` otherwise. Call ::sir_geterror
 *                 to obtain information about any error that may have occurred.
 */
bool sir_alert(const char* format, ...);

/**
 * @brief Dispatches a ::SIRL_EMERG level message.
 *
 * The message will be delivered to all destinations registered to receive
 * emergency-level messages, each with their own formatting and styling options.
 *
 * @remark To change options or level registrations for `stdout`/`stderr`, call
 * ::sir_stdoutopts/::sir_stdoutlevels and ::sir_stderropts/::sir_stderrlevels,
 * respectively. To change the text styling on a per-level basis, call
 * ::sir_settextstyle.
 *
 * @see ::sir_level
 * @see ::sir_option
 * @see ::default
 *
 * @param   format A printf-style format string, representing the template for
 *                 the message to dispatch.
 * @param   ...    Arguments whose type and position align with the format
 *                 specifiers in `format`.
 * @returns bool   `true` if the message was dispatched succcessfully to all
 *                 registered destinations, `false` otherwise. Call ::sir_geterror
 *                 to obtain information about any error that may have occurred.
 */
bool sir_emerg(const char* format, ...);

/**
 * @brief Adds a log file and registeres it to receive log output.
 *
 * The file at `path` will be created if it does not exist, otherwise it
 * be appended to.
 *
 * The levels for which the file will receive output are specified in the
 * `levels` bitmask. If you wish to use the default levels, pass
 * ::SIRL_DEFAULT.
 *
 * The formatting options for the log output sent to the file are specified in
 * the `opts` bitmask. If you wish to use the default options, pass
 * ::SIRO_DEFAULT. By default, log files receive all available information.
 *
 * @remark Take note of the ::SIR_FROLLSIZE compile-time constant. When any log
 * file reaches that size in bytes, it will be archived to a date-stamped file
 * in the same directory, and logging will resume at the path of the original file.
 *
 * @remark If `path` is a relative path, it shall be treated as relative
 * to the _current working directory_. This is not necessarily the same path
 * that your process' binary file resides in.
 *
 * @remark The return value from this function is only valid for the lifetime of
 * the process. If a crash or restart occurs, you will no longer be able to refer
 * to the file by that identifier, and will have to add the file again (libsir
 * does not persist its list of log files).
 *
 * @remark To change the file's level registrations or options after adding it,
 * call ::sir_filelevels and ::sir_fileopts, respectively.
 *
 * @see ::sir_remfile
 *
 * @param path        Either a relative or absolute path to a file name which
 *                    shall become a log file managed by libsir.
 * @param levels      Levels of output to register the file for.
 * @param opts        Formatting options for the output sent to the file.
 * @returns sirfileid If successful, a unique identifier that can later be used
 *                    to modify level registrations, options, or remove the file
 *                    from libsir. Upon failure, returns `NULL`. Use ::sir_geterror
 *                    to obtain information about any error that may have occurred.
 */
sirfileid sir_addfile(const char* path, sir_levels levels, sir_options opts);

/**
 * @brief Removes a file previously added to libsir.
 *
 * Immediately causes the file represented by `id` (the value returned from
 * ::sir_addfile) to be removed from libsir, and its file descriptor closed.
 *
 * Its contents will remain intact, and any pending writes will be flushed to
 * the file before it is closed.
 *
 * @see ::sir_filelevels
 * @see ::sir_fileopts
 *
 * @param   id   The ::sirfileid obtained when the file was added to libsir.
 * @returns bool `true` if the file is known to libsir, and was successfully
 *               removed, `false` otherwise. Use ::sir_geterror to obtain
 *               information about any error that may have occurred.
 */
bool sir_remfile(sirfileid id);

/**
 * @brief Set new level registrations for a log file already managed by libsir.
 *
 * By default, log files are registered for the following levels:
 *
 * - all levels (SIRL_ALL)
 *
 * @see ::sir_fileopts
 *
 * @param   id     The ::sirfileid obtained when the file was added to libsir.
 * @param   levels New bitmask of ::sir_level to register for. If you wish to use
 *                 the default levels, pass ::SIRL_DEFAULT.
 * @returns bool   `true` if the file is known to libsir and was succcessfully
 *                 updated, `false` otherwise. Use ::sir_geterror to obtain
 *                 information about any error that may have occurred.
 */
bool sir_filelevels(sirfileid id, sir_levels levels);

/**
 * @brief Set new formatting options for a log file already managed by libsir.
 *
 * By default, log files have the following formatting options:
 *
 * - ::SIRO_ALL
 * - ::SIRO_NOHOST
 *
 * @see ::sir_filelevels
 *
 * @param   id    The ::sirfileid obtained when the file was added to libsir.
 * @param   opts  New bitmask of ::sir_option for the file. If you wish to use
 *                the default options, pass ::SIRO_DEFAULT.
 * @returns bool  `true` if the file is known to libsir and was succcessfully
 *                updated, `false` otherwise. Use ::sir_geterror to obtain
 *                information about any error that may have occurred.
 */
bool sir_fileopts(sirfileid id, sir_options opts);

/**
 * @brief Set new text styling for stdio (stdout/stderr) destinations on a
 * per-level basis.
 *
 * @see ::sir_resettextstyles
 * @see ::default
 *
 * @param   level The ::sir_level for which to set the text styling.
 * @param   style New bitmask of ::sir_textstyle for the level.
 * @returns bool  `true` if succcessfully updated, `false` otherwise. Use
 *                ::sir_geterror to obtain information about any error that may
 *                have occurred.
 */
bool sir_settextstyle(sir_level level, sir_textstyle style);

/**
 * @brief Reset text styling for stdio (stdout/stderr) destinations to their
 * default values.
 *
 * @see ::sir_settextstyle
 * @see ::default
 *
 * @returns `true` if succcessfully reset, `false` otherwise. Use ::sir_geterror
 *          to obtain information about any error that may have occurred.
 */
bool sir_resettextstyles(void);

/**
 * @brief Set new level registrations for `stdout`.
 *
 * By default, `stdout` is registered for the following levels:
 *
 * - debug   (SIRL_DEBUG)
 * - info    (SIRL_INFO)
 * - notice  (SIRL_NOTICE)
 * - warning (SIRL_WARN)
 *
 * To modify formatting options for `stdout`, use ::sir_stdoutopts.
 *
 * @see ::sir_stdoutopts
 *
 * @param   levels New bitmask of ::sir_level to register for. If you wish to use
 *                 the default levels, pass ::SIRL_DEFAULT.
 * @returns bool   `true` if succcessfully updated, `false` otherwise. Use
 *                 ::sir_geterror to obtain information about any error that may
 *                 have occurred.
 */
bool sir_stdoutlevels(sir_levels levels);

/**
 * @brief Set new formatting options for `stdout`.
 *
 * By default, `stdout` has the following formatting options:
 *
 * - ::SIRO_NOTIME
 * - ::SIRO_NOHOST
 * - ::SIRO_NOPID
 * - ::SIRO_NOTID
 *
 * To modify level registrations for `stdout`, use ::sir_stdoutlevels.
 *
 * @see ::sir_stdoutlevels
 *
 * @param   opts New bitmask of ::sir_option for `stdout`. If you wish to use the
 *               default values, pass ::SIRL_DEFAULT.
 * @returns bool `true` if succcessfully updated, `false` otherwise. Use
 *               ::sir_geterror to obtain information about any error that may
 *               have occurred.
 */
bool sir_stdoutopts(sir_options opts);

/**
 * @brief Set new level registrations for `stderr`.
 *
 * By default, `stderr` is registered for the following levels:
 *
 * - error     (SIRL_ERROR)
 * - critical  (SIRL_CRIT)
 * - alert     (SIRL_ALERT)
 * - emergency (SIRL_EMERG)
 *
 * To modify formatting options for `stderr`, use ::sir_stderropts.
 *
 * @see ::sir_stderropts
 *
 * @param   levels New bitmask of ::sir_level to register for. If you wish to use
 *                 the default levels, pass ::SIRL_DEFAULT.
 * @returns bool   `true` if succcessfully updated, `false` otherwise. Use
 *                 ::sir_geterror to obtain information about any error that may
 *                 have occurred.
 */
bool sir_stderrlevels(sir_levels levels);

/**
 * @brief Set new formatting options for `stderr`.
 *
 * By default, `stderr` has the following formatting options:
 *
 * - ::SIRO_NOTIME
 * - ::SIRO_NOHOST
 * - ::SIRO_NOPID
 * - ::SIRO_NOTID
 *
 * To modify level registrations for `stderr`, use ::sir_stderrlevels.
 *
 * @see ::sir_stderrlevels
 *
 * @param   opts New bitmask of ::sir_option for `stderr`. If you wish to use the
 *               default values, pass ::SIRL_DEFAULT.
 * @returns bool `true` if succcessfully updated, `false` otherwise. Use
 *               ::sir_geterror to obtain information about any error that may
 *               have occurred.
 */
bool sir_stderropts(sir_options opts);

/**
 * @brief Set new level registrations for the system logger destination.
 *
 * By default, the system logger is registered for the following levels:
 *
 * - notice    (SIRL_NOTICE)
 * - warning   (SIRL_WARNING)
 * - error     (SIRL_ERROR)
 * - critical  (SIRL_CRIT)
 * - emergency (SIRL_EMERG)
 *
 * To modify formatting options for the system logger, use ::sir_syslogopts.
 *
 * @see ::sir_syslogopts
 *
 * @param   levels New bitmask of ::sir_level to register for. If you wish to use
 *                 the default levels, pass ::SIRL_DEFAULT.
 * @returns bool   `true` if succcessfully updated, `false` otherwise. Use
 *                 ::sir_geterror to obtain information about any error that may
 *                 have occurred.
 */
bool sir_sysloglevels(sir_levels levels);

/**
 * @brief Set new formatting options for the system logger destination.
 *
 * By default, the system logger has the following formatting options:
 *
 * - ::SIRO_MSGONLY
 *
 * To modify level registrations for the system logger, use ::sir_sysloglevels.
 *
 * @see ::sir_sysloglevels
 *
 * @param   opts New bitmask of ::sir_option for the system logger. If you wish
 *               to use the default values, pass ::SIRL_DEFAULT.
 * @returns bool `true` if succcessfully updated, `false` otherwise. Use
 *               ::sir_geterror to obtain information about any error that may
 *               have occurred.
 */
bool sir_syslogopts(sir_options opts);

/**
 * @brief Set new system logger identity.
 *
 * In the context of the system logger (i.e., `syslog`/`os_log`/etc.), identity
 * refers to the `name` that appears in the log for the current process.
 *
 * Upon library initialization, the system logger identity is resolved as follows:
 *
 * 1. If the @ref sir_syslog_dest.identity "sirinit.d_syslog.identity" string is set,
 *    it will be used.
 * 2. If the ::sirinit.name string is set, it will be used.
 * 3. If the name of the current process is available, it will be used.
 * 4. If none of these are available, the string ::SIR_FALLBACK_SYSLOG_ID will
 *    be used.
 *
 * @remark If `SIR_NO_SYSTEM_LOGGERS` is defined when compiling libsir, this
 * function will immediately return false, and set the last error to
 * ::SIR_E_UNAVAIL.
 *
 * @param   identity The string to use as the system logger identity.
 * @returns bool     `true` if succcessfully updated, `false` otherwise. Use
 *                   ::sir_geterror to obtain information about any error that
 *                   may have occurred.
 */
bool sir_syslogid(const char* identity);

/**
 * @brief Set new system logger category.
 *
 * Some system logger facilities (e.g. `os_log`, the system logger on macOS), ask
 * for a category string when logging messages–to more easily group messages
 * together (which, as a byproduct, enhances searching).
 *
 * If you are not utilizing a system logger that requires a category string, you
 * do not need to call this function (or set @ref sir_syslog_dest.category
 * "sirinit.d_syslog.category").
 *
 * Upon library initialization, the system logger category is resolved as follows:
 *
 * 1. If the @ref sir_syslog_dest.category "sirinit.d_syslog.category" string is set,
 *    it will be used.
 * 2. The string ::SIR_FALLBACK_SYSLOG_CAT will be used.
 *
 * @remark If `SIR_NO_SYSTEM_LOGGERS` is defined when compiling libsir, this
 * function will immediately return false, and set the last error to
 * ::SIR_E_UNAVAIL.
 *
 * @param category The string to use as the system logger category.
 * @returns bool   `true` if succcessfully updated, `false` otherwise. Use
 *                 ::sir_geterror to obtain information about any error that
 *                 may have occurred.
 */
bool sir_syslogcat(const char* category);

/**
 * @}
 * @}
 */

# if defined(__cplusplus)
}
# endif

#endif /* !_SIR_H_INCLUDED */
