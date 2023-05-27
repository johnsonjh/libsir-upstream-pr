/**
 * @file sirhelpers.h
 * @brief Internal macros and inline functions.
 *
 * This file and accompanying source code originated from <https://github.com/aremmell/libsir>.
 * If you obtained it elsewhere, all bets are off.
 *
 * @author Ryan M. Lederman <lederman@gmail.com>
 * @copyright
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Ryan M. Lederman
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
#ifndef _SIR_HELPERS_H_INCLUDED
#define _SIR_HELPERS_H_INCLUDED

#include "sirtypes.h"

/**
 * @addtogroup intern
 * @{
 */

/** Computes the size of an array. */
#define _sir_countof(arr) (sizeof(arr) / sizeof(arr[0]))

/**
 * Creates an error code that (hopefully) doesn't conflict
 * with any of those defined by the platform.
 */
#define _sir_mkerror(code) \
    (((uint32_t)(code & 0x7fff) << 16) | 0x80000000)

/** Validates an internal error. */
static inline
bool _sir_validerror(sirerror_t err) {
    sirerror_t masked = err & 0x8fffffff;
    return masked >= 0x80000000 && masked <= 0x8fff0000;
}

/** Extracts just the code from an internal error. */
static inline
uint16_t _sir_geterrcode(sirerror_t err) {
    return (err >> 16) & 0x7fff;
}

/** Evil macro used for _sir_lv wrappers. */
#define _SIR_L_START(format) \
    bool r = false;          \
    va_list args;            \
    va_start(args, format);

/** Evil macro used for _sir_lv wrappers. */
#define _SIR_L_END(args) va_end(args);

/** Squelches warnings about unreferenced parameters. */
#define _SIR_UNUSED(param) (void)param;

/** Even more evil macros used for binary searching arrays. */
#define _SIR_DECLARE_BIN_SEARCH(low, high) \
    bool _matched = false; \
    size_t _low   = low;   \
    size_t _high  = high;  \
    size_t _mid   = (_low + _high) / 2;

#define _SIR_BEGIN_BIN_SEARCH() do { \

#define _SIR_ITERATE_BIN_SEARCH(comparison) \
    if (0 == comparison) { \
        _matched = true; \
        break; \
    } \
    \
    if (_low == _high) \
        break; \
    \
    if (0 > comparison) { \
        _high = _mid - 1; \
    } else { \
        _low = _mid + 1; \
    } \
    \
    _mid = (_low + _high) / 2; \

#define _SIR_END_BIN_SEARCH() \
    } while(true); \
    _SIR_UNUSED(_matched);

/** Validates a pointer-to-pointer, pointer,
 * pointer to function, etc. but ignores whether it's invalid. */
#define _sir_validaddr(addr) (NULL != addr)

/** Checks a bitfield for a specific set of bits. */
static inline
bool _sir_bittest(uint32_t flags, uint32_t test) {
    return (flags & test) == test;
}

/** Wraps \a free. */
static inline
void __sir_safefree(void** p) {
    if (!p || (p && !*p))
        return;
    free(*p);
    *p = NULL;
}

/** Wraps \a free. */
static inline
void _sir_safefree(void* p) {
    __sir_safefree(&p);
}

/** Validates a log file identifier. */
bool _sir_validfid(int id);

/** Validates a set of ::sir_level flags. */
bool _sir_validlevels(sir_levels levels);

/** Validates a single ::sir_level. */
bool _sir_validlevel(sir_level level);

/** Applies default ::sir_level flags if applicable. */
static inline
void _sir_defaultlevels(sir_levels* levels, sir_levels def) {
    if (levels && SIRL_DEFAULT == *levels) *levels = def;
}

/** Applies default ::sir_options flags if applicable. */
static inline
void _sir_defaultopts(sir_options* opts, sir_options def) {
    if (opts && SIRO_DEFAULT == *opts) *opts = def;
}

/** Validates a set of ::sir_option flags. */
bool _sir_validopts(sir_options opts);

/** Validates a string pointer and optionally fails if it's invalid. */
bool __sir_validstr(const sirchar_t* str, bool fail);

/** Validates a string pointer and fails if it's invalid. */
static inline
bool _sir_validstr(const sirchar_t* str) {
    return __sir_validstr(str, true);
}

/** Validates a string pointer but ignores whether it's invalid. */
static inline
bool _sir_validstrnofail(const sirchar_t* str) {
    return __sir_validstr(str, false);
}

/** Validates a pointer and optionally fails if it's invalid. */
bool __sir_validptr(const void* restrict p, bool fail);

/** Validates a pointer and fails if it's invalid. */
static inline
bool _sir_validptr(const void* restrict p) {
    return __sir_validptr(p, true);
}

/** Validates a pointer but ignores whether it's invalid. */
static inline
bool _sir_validptrnofail(const void* restrict p) {
    return __sir_validptr(p, false);
}

/** Validates a sir_update_data structure. */
static inline
bool _sir_validupdatedata(sir_update_data* data) {
    return NULL != data && ((NULL == data->levels || _sir_validlevels(*data->levels)) &&
        (NULL == data->opts || _sir_validopts(*data->opts)));
}

/** Places a null terminator at the first index in a string buffer. */
static inline
void _sir_resetstr(sirchar_t* str) {
    str[0] = (sirchar_t)'\0';
}

/** 
 * Wrapper for strncpy/strncpy_s. Determines which one to use
 * based on preprocessor macros.
 */
int _sir_strncpy(sirchar_t* restrict dest, size_t destsz, const sirchar_t* restrict src, size_t count);

/**
  * Wrapper for strncat/strncat_s. Determines which one to use
  * based on preprocessor macros.
  */
int _sir_strncat(sirchar_t* restrict dest, size_t destsz, const sirchar_t* restrict src, size_t count);

/**
  * Wrapper for fopen/fopen_s. Determines which one to use
  * based on preprocessor macros.
  */
int _sir_fopen(FILE* restrict *restrict streamptr, const sirchar_t* restrict filename, const sirchar_t* restrict mode);

/**
  * Wrapper for localtime/localtime_s. Determines which one to use
  * based on preprocessor macros.
  */
struct tm* _sir_localtime(const time_t* restrict timer, struct tm* restrict buf);

/**
 * A portable "press any key to continue" implementation; On Windows, uses _getch().
 * otherwise, uses tcgetattr()/tcsetattr() and getchar().
 */
int _sir_getchar(void);

/** @} */

#endif /* !_SIR_HELPERS_H_INCLUDED */
