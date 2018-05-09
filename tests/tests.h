/**
 * @file tests.h
 * @brief libsir test suite.
 *
 * This file and accompanying source code originated from <https://github.com/ryanlederman/sir>.
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
 * 
 * ---------------------------------------------------------------------------------------------------------
 * 
 * @todo Hardening and Compatibility
 * I have only compiled and tested libsir on the following:
 * - Ubuntu 16.04 x64 (gcc)
 * - Windows 10 x64  (MinGW)
 * - macOS 10.13.4 x64 (clang)
 * #### Other platforms, such as `BSD` and `macOS` remain untested, and probably won't even compile cleanly.
 *
 * @todo Nice to have
 * 1. A plugin system or public interface for registering custom adapters, for things like:
 *   - Posting high-priority messages to a REST API endpoint.
 *   - Sending high-prirority messages via SMS or push notification.
 * 2. Compressing archived logs with zlib or similar.
 * 3. Deleting archived logs older than _n_ days.
 * 4. A project file for Visual Studio.
 * 5. A project file for Xcode.
 * 6. An accompanying C++ wrapper.
 * 7. Something I didn't think of yet.
 * 
 * ---------------------------------------------------------------------------------------------------------
 */
#ifndef _SIR_TESTS_H_INCLUDED
#define _SIR_TESTS_H_INCLUDED

#include <stdbool.h>

/** Function signature for a single test. */
typedef bool (*sir_test_fn)(void);

/** Map a test to a human-readable description. */
typedef struct {
    const char* name;
    sir_test_fn fn;
} sir_test;

/**
 * @defgroup tests Tests
 * 
 * libsir integrity tests.
 * 
 * @addtogroup tests
 * @{
 */

/**
 * @test Properly handle multiple threads competing for locked sections.
 */
bool sirtest_mthread_race();

/**
 * @test Properly handle messages that exceed internal buffer sizes.
 */
bool sirtest_exceedmaxsize();

/**
 * @test Properly handle adding and removing log files.
 */
bool sirtest_filecachesanity();

/**
 * @test Properly handle invalid text style.
 */
bool sirtest_failsetinvalidstyle();

/**
 * @test Properly handle the lack of any output destinations.
 */
bool sirtest_failnooutputdest();

/**
 * @test Properly handle invalid log file name.
 */
bool sirtest_failinvalidfilename();

/**
 * @test Properly handle log file without appropriate permissions.
 */
bool sirtest_failfilebadpermission();

/**
 * @test Properly handle null/empty input.
 */
bool sirtest_failnulls();

/**
 * @test Properly handle calls without initialization.
 */
bool sirtest_failwithoutinit();

/**
 * @test Properly handle two initialization calls without corresponding cleanup.
 */
bool sirtest_failinittwice();

/**
 * @test Properly handle calls after cleanup.
 */
bool sirtest_failaftercleanup();

/**
 * @test Properly handle initialization, cleanup, re-initialization.
 */
bool sirtest_initcleanupinit();

/**
 * @test Properly refuse to add a duplicate file.
 */
bool sirtest_faildupefile();

/**
 * @test Properly refuse to remove a file that isn't added.
 */
bool sirtest_failremovebadfile();

/**
 * @test Properly roll/archive a file when it hits max size.
 */
bool sirtest_rollandarchivefile();

/**
 * @test Properly return valid codes and messages for all possible errors.
 */
bool sirtest_allerrorsresolve();

/** @} */

bool printerror(bool pass);
void printexpectederr();

int getoserr();
unsigned int getrand();

bool rmfile(const char* filename);
bool deletefiles(const char* search, const char* filename, unsigned* data);
bool countfiles(const char* search, const char* filename, unsigned* data);

typedef bool (*fileenumproc)(const char* search, const char* filename, unsigned* data);
bool enumfiles(const char* search, fileenumproc cb, unsigned* data);

#endif /* !_SIR_TESTS_H_INCLUDED */
