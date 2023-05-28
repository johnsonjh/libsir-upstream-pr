/**
 * @file tests.c
 * @brief libsir test suite
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
#include "tests.h"

static const sir_test sir_tests[] = {
    {"performance",             sirtest_perf},
    {"multi-thread race",       sirtest_mthread_race},
    {"exceed max buffer size",  sirtest_exceedmaxsize},
    {"file cache sanity",       sirtest_filecachesanity},
    {"set invalid text style",  sirtest_failsetinvalidstyle},
    {"no output destination",   sirtest_failnooutputdest},
    {"invalid file name",       sirtest_failinvalidfilename},
    {"bad file permissions",    sirtest_failfilebadpermission},
    {"null pointers",           sirtest_failnulls},
    {"output without init",     sirtest_failwithoutinit},
    {"superfluous init",        sirtest_failinittwice},
    {"output after cleanup",    sirtest_failaftercleanup},
    {"re-initialize",           sirtest_initcleanupinit},
    {"duplicate file name",     sirtest_faildupefile},
    {"remove nonexistent file", sirtest_failremovebadfile},
    {"roll/archive large file", sirtest_rollandarchivefile},
    {"error handling sanity",   sirtest_errorsanity},
    {"text style sanity",       sirtest_textstylesanity},
    {"update levels/options",   sirtest_updatesanity},
};

static const char* arg_wait = "--wait"; /* wait for key press before exiting. */
static const char* arg_perf = "--perf"; /* run performance test instead of standard tests. */

int main(int argc, char** argv) {

#if !defined(_WIN32)
    /* Disallow execution by root / sudo; some of the tests rely on lack of permissions. */
    if (geteuid() == 0) {
        fprintf(stderr, "Sorry, but this program may not be executed by root.\n");
        return EXIT_FAILURE;
    }
#else
#if defined(_DEBUG)
    /* Prevents assert() from calling abort() before the user is able to:
     * a.) break into the code and debug (Retry button)
     * b.) ignore the assert() and continue. */
    _set_error_mode(_OUT_TO_MSGBOX);
#endif
#endif

    bool wait = false;
    bool perf = false;

    for (int n = 1; n < argc; n++) {
        if (0 == strncmp(argv[n], arg_wait, strlen(arg_wait)))
            wait = true;
        else if (0 == strncmp(argv[n], arg_perf, strlen(arg_perf)))
            perf = true;
    }

    bool       allpass = true;
    int        tests   = (perf ? 1 : (sizeof(sir_tests) / sizeof(sir_test)) - 1);
    int        first   = (perf ? 0 : 1);
    int        passed  = first;
    sirtimer_t timer = {0};

    printf(WHITE("running %d libsir %s...\n"), tests, TEST_S(tests));

    if (!startsirtimer(&timer))
        printf(RED("failed to start timer; elapsed time won't be measured correctly!") "\n");

    for (int n = first; n < tests; n++) {
        printf(WHITE("\t'%s'...") "\n", sir_tests[n].name);
        bool thispass = sir_tests[n].fn();
        allpass &= thispass;
        passed += (thispass ? 1 : 0);
        printf(WHITE("\t'%s' finished; result: ") "%s\n", sir_tests[n].name,
            thispass ? GREEN("PASS") : RED("FAIL"));
    }

    float elapsed = sirtimerelapsed(&timer);
    
    if (allpass)
        printf(WHITE("done; ") GREEN("%s%d libsir %s passed in %.02fsec") "\n",
            tests > 1 ? "all " : "", tests, TEST_S(tests), elapsed / 1e3);
    else
        printf(WHITE("done; ") RED("%d of %d libsir %s failed in %.02fsec") "\n",
            tests - passed, tests, TEST_S(tests), elapsed / 1e3);

    if (wait) {
        printf(WHITE("press any key to exit...") "\n");
        int ch = _sir_getchar();
        _SIR_UNUSED(ch);
    }

    return allpass ? EXIT_SUCCESS : EXIT_FAILURE;
}

bool sirtest_exceedmaxsize(void) {
    INIT(si, SIRL_ALL, 0, 0, 0);
    bool pass = si_init;

    sirchar_t toobig[SIR_MAXMESSAGE + 100] = {0};
    memset(toobig, 'a', SIR_MAXMESSAGE - 99);

    pass &= sir_info(toobig);

    sir_cleanup();
    return print_result_and_return(pass);
}

bool sirtest_filecachesanity(void) {
    INIT(si, SIRL_ALL, 0, 0, 0);
    bool pass = si_init;

    size_t numfiles               = SIR_MAXFILES + 1;
    sirfileid_t ids[SIR_MAXFILES] = {0};

    sir_options even = SIRO_MSGONLY;
    sir_options odd  = SIRO_ALL;

    for (size_t n = 0; n < numfiles - 1; n++) {
        sirchar_t path[SIR_MAXPATH] = {0};
        snprintf(path, SIR_MAXPATH, "test-%zu.log", n);
        rmfile(path);
        ids[n] = sir_addfile(path, SIRL_ALL, (n % 2) ? odd : even);
        pass &= NULL != ids[n] && sir_info("test %u", n);
    }

    pass &= sir_info("test test test");

    /* this one should fail; max files already added. */
    pass &= NULL == sir_addfile("should-fail.log", SIRL_ALL, SIRO_MSGONLY);

    sir_info("test test test");

    /* now remove previously added files in a different order. */
    size_t removeorder[SIR_MAXFILES];
    memset(removeorder, -1, sizeof(removeorder));

    long processed = 0;
    printf("\tcreating random file ID order...\n");

    do {
        size_t rnd = (size_t)(getrand() % SIR_MAXFILES);
        bool skip  = false;

        for (size_t n = 0; n < SIR_MAXFILES; n++)
            if (removeorder[n] == rnd) {
                skip = true;
                break;
            }

        if (skip)
            continue;

        removeorder[processed++] = rnd;

        if (processed == SIR_MAXFILES)
            break;
    } while (true);

    printf("\tremove order: {");
    for (size_t n = 0; n < SIR_MAXFILES; n++) printf(" %zu%s", removeorder[n], (n < SIR_MAXFILES - 1) ? "," : "");
    printf(" }...\n");

    for (size_t n = 0; n < SIR_MAXFILES; n++) {
        pass &= sir_remfile(ids[removeorder[n]]);

        sirchar_t path[SIR_MAXPATH] = {0};
        snprintf(path, SIR_MAXPATH, "test-%zu.log", removeorder[n]);
        rmfile(path);
    }

    pass &= sir_info("test test test");

    sir_cleanup();
    return print_result_and_return(pass);
}

bool sirtest_failsetinvalidstyle(void) {
    INIT(si, SIRL_ALL, 0, 0, 0);
    bool pass = si_init;

    pass &= !sir_settextstyle(SIRL_INFO, 0xfefe);
    pass &= sir_info("hello there, I set an invalid style.");
    //pass &= !sir_settextstyle(SIRL_ALL, SIRS_FG_RED | SIRS_FG_DEFAULT);
    //pass &= sir_info("oops, did it again...");
#pragma message("TODO: uncomment the above when the TODO at sirtextstyle.c:50 is resolved")
    pass &= !sir_settextstyle(SIRL_ALERT, SIRS_FG_BLACK | SIRS_BG_BLACK);
    pass &= sir_info("and again.");

    sir_cleanup();
    return print_test_error(pass, true);
}

bool sirtest_failnooutputdest(void) {
    INIT(si, 0, 0, 0, 0);
    bool pass = si_init;

    static const char* logfilename = "nodestination.log";

    pass &= !sir_info("this goes nowhere!");

    if (pass) {
        print_expected_error();

        pass &= sir_stdoutlevels(SIRL_INFO);
        pass &= sir_info("this goes to stdout");
        pass &= sir_stdoutlevels(SIRL_NONE);

        sirfileid_t fid = sir_addfile(logfilename, SIRL_INFO, SIRO_DEFAULT);
        pass &= NULL != fid;
        pass &= sir_info("this goes to %s", logfilename);
        pass &= sir_filelevels(fid, SIRL_NONE);
        pass &= !sir_info("this goes nowhere!");

        if (NULL != fid)
            pass &= sir_remfile(fid);

        rmfile(logfilename);
    }

    sir_cleanup();
    return print_result_and_return(pass);
}

bool sirtest_failinvalidfilename(void) {
    INIT(si, SIRL_ALL, 0, 0, 0);
    bool pass = si_init;

    pass &= NULL == sir_addfile("bad file!/name", SIRL_ALL, SIRO_MSGONLY);

    if (pass)
        print_expected_error();

    sir_cleanup();
    return print_result_and_return(pass);
}

bool sirtest_failfilebadpermission(void) {
    INIT(si, SIRL_ALL, 0, 0, 0);
    bool pass = si_init;

#if !defined(_WIN32)
    static const char* path = "/noperms";
#else
    static const char* path = "C:\\Windows\\System32\\noperms";
#endif    

    pass &= NULL == sir_addfile(path, SIRL_ALL, SIRO_MSGONLY);

    if (pass)
        print_expected_error();

    sir_cleanup();
    return print_result_and_return(pass);
}

bool sirtest_failnulls(void) {
    INIT(si, SIRL_ALL, 0, 0, 0);
    bool pass = si_init;

    pass &= !sir_info(NULL);
    pass &= NULL == sir_addfile(NULL, SIRL_ALL, SIRO_MSGONLY);

    if (pass)
        print_expected_error();

    sir_cleanup();
    return print_result_and_return(pass);
}

bool sirtest_failwithoutinit(void) {
    bool pass = !sir_info("sir isn't initialized; this needs to fail");

    if (pass)
        print_expected_error();

    return print_result_and_return(pass);
}

bool sirtest_failinittwice(void) {
    INIT(si, SIRL_ALL, 0, 0, 0);
    bool pass = si_init;

    INIT(si2, SIRL_ALL, 0, 0, 0);
    pass &= !si2_init;

    if (pass)
        print_expected_error();

    sir_cleanup();
    return print_result_and_return(pass);
}

bool sirtest_failaftercleanup(void) {
    INIT(si, SIRL_ALL, 0, 0, 0);
    bool pass = si_init;

    sir_cleanup();
    pass &= !sir_info("already cleaned up; this needs to fail");

    if (pass)
        print_expected_error();

    return print_result_and_return(pass);
}

bool sirtest_initcleanupinit(void) {
    INIT(si1, SIRL_ALL, 0, 0, 0);
    bool pass = si1_init;

    pass &= sir_info("init called once; testing output...");
    sir_cleanup();

    INIT(si2, SIRL_ALL, 0, 0, 0);
    pass &= si2_init;

    pass &= sir_info("init called again after re-init; testing output...");
    sir_cleanup();

    return print_result_and_return(pass);
}

bool sirtest_faildupefile(void) {
    INIT(si, SIRL_ALL, 0, 0, 0);
    bool pass = si_init;

    const sirchar_t* filename = "faildupefile.log";
    sirfileid_t fid = sir_addfile(filename, SIRL_ALL, SIRO_DEFAULT);

    pass &= NULL != fid;
    pass &= NULL == sir_addfile(filename, SIRL_ALL, SIRO_DEFAULT);
    pass &= sir_remfile(fid);

    rmfile(filename);
    sir_cleanup();
    return print_result_and_return(pass);
}

bool sirtest_failremovebadfile(void) {
    INIT(si, SIRL_ALL, 0, 0, 0);
    bool pass = si_init;

    int invalidid = 9999999;
    pass &= !sir_remfile(&invalidid);

    sir_cleanup();
    return print_result_and_return(pass);
}

bool sirtest_rollandarchivefile(void) {

    /* roll size minus 1KB so we can write until it maxes. */
    static const long       deltasize   = 1024L;
    const long              fillsize    = SIR_FROLLSIZE - deltasize;
    static const sirchar_t* logbasename = "rollandarchive";
    static const sirchar_t* logext      = ".log";
    static const sirchar_t* line        = "hello, i am some data. nice to meet you.";

    sirchar_t logfilename[SIR_MAXPATH] = { 0 };
    snprintf(logfilename, SIR_MAXPATH, "%s%s", logbasename, logext);

    unsigned delcount = 0;
    if (!enumfiles(logbasename, deletefiles, &delcount)) {
        handle_os_error(false, "failed to enumerate log files with base name: %s!", logbasename);
        print_os_error();
        return false;
    }

    if (delcount > 0)
        printf("\tfound and removed %u log file(s)\n", delcount);

    FILE* f  = NULL;
    _sir_fopen(&f, logfilename, "w");

    if (!f) {
        print_os_error();
        return false;
    }

    if (0 != fseek(f, fillsize, SEEK_SET)) {
        handle_os_error(true, "fseek in file %s failed!", logfilename);
        print_os_error();
        fclose(f);
        return false;
    }

    if (EOF == fputc('\0', f)) {
        handle_os_error(true, "fputc in file %s failed!", logfilename);
        print_os_error();
        fclose(f);
        return false;
    }

    fclose(f);

    INIT(si, 0, 0, 0, 0);
    bool pass = si_init;

    sirfileid_t fileid = sir_addfile(logfilename, SIRL_DEBUG, SIRO_MSGONLY | SIRO_NOHDR);
    pass &= NULL != fileid;

    if (pass) {
        /* write an (approximately) known quantity until we should have rolled */
        size_t written  = 0;
        size_t linesize = strlen(line);

        do {
            pass &= sir_debug("%s", line);
            if (!pass)
                break;

            written += linesize;
        } while (written < deltasize + (linesize * 50));

        /* Look for files matching the original name. */
        unsigned foundlogs = 0;
        if (!enumfiles(logbasename, countfiles, &foundlogs)) {
            handle_os_error(false, "failed to enumerate log files with base name: %s!", logbasename);
            print_os_error();
            pass = false;
        }

        /* If two are present, the test is a pass. */
        pass &= foundlogs == 2;
    }

    if (NULL != fileid)
        pass &= sir_remfile(fileid);

    delcount = 0;
    if (!enumfiles(logbasename, deletefiles, &delcount)) {
        handle_os_error(false, "failed to enumerate log files with base name: %s!", logbasename);
        print_os_error();
        return false;
    }

    if (delcount > 0)
        printf("\tfound and removed %u log file(s)\n", delcount);

    sir_cleanup();
    return print_result_and_return(pass);
}

bool sirtest_errorsanity(void) {

    INIT(si, SIRL_ALL, 0, 0, 0);
    bool pass = si_init;

    struct {
        uint16_t    code;
        const char* name;
    } errors[] = {
        {SIR_E_NOERROR,   "SIR_E_NOERROR"},   /**< The operation completed successfully (0) */
        {SIR_E_NOTREADY,  "SIR_E_NOTREADY"},  /**< libsir has not been initialized (1) */
        {SIR_E_ALREADY,   "SIR_E_ALREADY"},   /**< libsir is already initialized (2) */
        {SIR_E_DUPFILE,   "SIR_E_DUPFILE"},   /**< File already managed by libsir (3) */
        {SIR_E_NOFILE,    "SIR_E_NOFILE"},    /**< File not managed by libsir (4) */
        {SIR_E_FCFULL,    "SIR_E_FCFULL"},    /**< Maximum number of files already managed (5) */
        {SIR_E_OPTIONS,   "SIR_E_OPTIONS"},   /**< Option flags are invalid (6) */
        {SIR_E_LEVELS,    "SIR_E_LEVELS"},    /**< Level flags are invalid (7) */
        {SIR_E_TEXTSTYLE, "SIR_E_TEXTSTYLE"}, /**< Text style is invalid (8) */
        {SIR_E_STRING,    "SIR_E_STRING"},    /**< Invalid string argument (9) */
        {SIR_E_NULLPTR,   "SIR_E_NULLPTR"},   /**< NULL pointer argument (10) */
        {SIR_E_INVALID,   "SIR_E_INVALID"},   /**< Invalid argument (11) */
        {SIR_E_NODEST,    "SIR_E_NODEST"},    /**< No destinations registered for level (11) */
        {SIR_E_PLATFORM,  "SIR_E_PLATFORM"},  /**< Platform error code %d: %s (12) */
        {SIR_E_UNKNOWN,   "SIR_E_UNKNOWN"},   /**< Error is not known (4095) */
    };

    sirchar_t message[SIR_MAXERROR] = {0};
    for (size_t n = 0; n < (sizeof(errors) / sizeof(errors[0])); n++) {
        _sir_seterror(_sir_mkerror(errors[n].code));
        memset(message, 0, SIR_MAXERROR);
        uint16_t err = err = sir_geterror(message);
        pass &= errors[n].code == err && *message != '\0';
        printf("\t%s = %s\n", errors[n].name, message);
    }

    sir_cleanup();
    return print_result_and_return(pass);
}

bool sirtest_textstylesanity(void) {

    INIT(si, SIRL_ALL, 0, 0, 0);
    bool pass = si_init;

    if (pass) {
        pass &= sir_debug("default style");
        pass &= sir_settextstyle(SIRL_DEBUG, SIRS_FG_YELLOW | SIRS_BG_DGRAY);
        pass &= sir_debug("override style");

        pass &= sir_info("default style");
        pass &= sir_settextstyle(SIRL_INFO, SIRS_FG_GREEN | SIRS_BG_MAGENTA);
        pass &= sir_info("override style");

        pass &= sir_notice("default style");
        pass &= sir_settextstyle(SIRL_NOTICE, SIRS_FG_BLACK | SIRS_BG_LYELLOW);
        pass &= sir_notice("override style");

        pass &= sir_warn("default style");
        pass &= sir_settextstyle(SIRL_WARN, SIRS_FG_BLACK | SIRS_BG_WHITE);
        pass &= sir_warn("override style");

        pass &= sir_error("default style");
        pass &= sir_settextstyle(SIRL_ERROR, SIRS_FG_WHITE | SIRS_BG_BLUE);
        pass &= sir_error("override style");

        pass &= sir_crit("default style");
        pass &= sir_settextstyle(SIRL_CRIT, SIRS_FG_DGRAY | SIRS_BG_LGREEN);
        pass &= sir_crit("override style");

        pass &= sir_alert("default style");
        pass &= sir_settextstyle(SIRL_ALERT, SIRS_BRIGHT | SIRS_FG_LBLUE);
        pass &= sir_alert("override style");

        pass &= sir_emerg("default style");
        pass &= sir_settextstyle(SIRL_EMERG, SIRS_BRIGHT | SIRS_FG_DGRAY);
        pass &= sir_emerg("override style");
    }

    printf("\tcleanup to reset styles...\n");
    sir_cleanup();

    INIT(si2, SIRL_ALL, 0, 0, 0);
    pass &= si2_init;

    pass &= sir_debug("default style");
    pass &= sir_info("default style");
    pass &= sir_notice("default style");
    pass &= sir_warn("default style");
    pass &= sir_error("default style");
    pass &= sir_crit("default style");
    pass &= sir_alert("default style");
    pass &= sir_emerg("default style");

    sir_cleanup();

    return print_result_and_return(pass);
}

bool sirtest_perf(void) {

    static const sirchar_t* logbasename = "libsir-perf";
    static const sirchar_t* logext      = ".log";

#if !defined(_WIN32)
    static const size_t perflines       = 1000000;
#else
    static const size_t perflines       = 100000;
#endif

    INIT_N(si, SIRL_ALL, SIRO_NOMSEC, 0, 0, "perf");
    bool pass = si_init;

    if (pass) {
        float printfelapsed = 0.0f;
        float stdioelapsed  = 0.0f;
        float fileelapsed   = 0.0f;

        printf("\t" BLUE("%zu lines printf...") "\n", perflines);

        sirtimer_t printftimer = {0};
        startsirtimer(&printftimer);

        for (size_t n = 0; n < perflines; n++)
            printf("\x1b[97m%.2f: lorem ipsum foo bar %s: %zu\x1b[0m\n", sirtimerelapsed(&printftimer), "baz", 1234 + n);

        printfelapsed = sirtimerelapsed(&printftimer);

        printf("\t" BLUE("%zu lines libsir(stdout)...") "\n", perflines);

        sirtimer_t stdiotimer = {0};
        startsirtimer(&stdiotimer);

        for (size_t n = 0; n < perflines; n++)
            sir_debug("%.2f: lorem ipsum foo bar %s: %zu", sirtimerelapsed(&stdiotimer), "baz", 1234 + n);

        stdioelapsed = sirtimerelapsed(&stdiotimer);

        sir_cleanup();

        INIT(si2, 0, 0, 0, 0);
        pass &= si2_init;

        sirchar_t logfilename[SIR_MAXPATH] = {0};
        snprintf(logfilename, SIR_MAXPATH, logbasename, logext);

        sirfileid_t logid = sir_addfile(logfilename, SIRL_ALL, SIRO_NONAME | SIRO_NOPID);
        pass &= NULL != logid;

        if (pass) {
            printf("\t" BLUE("%zu lines libsir(log file)...") "\n", perflines);

            sirtimer_t filetimer = {0};
            startsirtimer(&filetimer);

            for (size_t n = 0; n < perflines; n++)
                sir_debug("lorem ipsum foo bar %s: %zu", "baz", 1234 + n);

            fileelapsed = sirtimerelapsed(&filetimer);

            pass &= sir_remfile(logid);
        }

        if (pass) {
            printf("\t" WHITE("printf: ") CYAN("%zu lines in %.2fsec (%.1f lines/sec)") "\n",
                perflines, printfelapsed / 1e3, perflines / (printfelapsed / 1e3));
            printf("\t" WHITE("libsir(stdout): ") CYAN("%zu lines in %.2fsec (%.1f lines/sec)") "\n",
                perflines, stdioelapsed / 1e3, perflines / (stdioelapsed / 1e3));
            printf("\t" WHITE("libsir(log file): ") CYAN("%zu lines in %.2fsec (%.1f lines/sec)") "\n",
                perflines, fileelapsed / 1e3, perflines / (fileelapsed / 1e3));
        }
    }

    unsigned deleted = 0;
    enumfiles(logbasename, deletefiles, &deleted);

    if (deleted > 0)
        printf("\tdeleted %d log file(s)\n", deleted);

    sir_cleanup();
    return print_result_and_return(pass);
}

bool sirtest_updatesanity(void) {

    INIT_N(si, SIRL_DEFAULT, 0, SIRL_DEFAULT, 0, "update_sanity");
    bool pass = si_init;

    static const char* logfile = "update-sanity.log";

    rmfile(logfile);
    sirfileid_t id1 = sir_addfile(logfile, SIRL_DEFAULT, SIRO_DEFAULT);

    pass &= NULL != id1;

    if (pass) {
        pass &= sir_debug("default config");
        pass &= sir_info("default config");
        pass &= sir_notice("default config");
        pass &= sir_warn("default config");
        pass &= sir_error("default config");
        pass &= sir_crit("default config");
        pass &= sir_alert("default config");
        pass &= sir_emerg("default config");

        pass &= sir_stdoutlevels(SIRL_DEBUG);
        pass &= sir_stdoutopts(SIRO_NOTIME);
        pass &= sir_stderrlevels(SIRL_ALL);
        pass &= sir_stderropts(SIRO_NONAME);

        pass &= sir_filelevels(id1, SIRL_DEBUG);
        pass &= sir_fileopts(id1, SIRO_MSGONLY);

        pass &= sir_debug("modified config");
        pass &= sir_info("modified config");
        pass &= sir_notice("modified config");
        pass &= sir_warn("modified config");
        pass &= sir_error("modified config");
        pass &= sir_crit("modified config");
        pass &= sir_alert("modified config");
        pass &= sir_emerg("modified config");
        pass &= sir_remfile(id1);
    }

    rmfile(logfile);
    sir_cleanup();

    return print_result_and_return(pass);
}

#if !defined(_WIN32)
static void* sirtest_thread(void* arg);
#else
static unsigned sirtest_thread(void* arg);
#endif

#define NUM_THREADS 4

bool sirtest_mthread_race(void) {
#if !defined(_WIN32)
    pthread_t thrds[NUM_THREADS] = {0};
#else
    uintptr_t thrds[NUM_THREADS] = {0};
#endif

    INIT_N(si, SIRL_ALL, SIRO_NOPID, 0, 0, "multi-thread race");
    bool pass           = si_init;
    bool any_created    = false;
    size_t last_created = 0;

    for (size_t n = 0; n < NUM_THREADS; n++) {
        char* path = (char*)calloc(SIR_MAXPATH, sizeof(char));
        snprintf(path, SIR_MAXPATH, "multi-thread-race-%zu.log", n);

#if !defined(_WIN32)
        int create = pthread_create(&thrds[n], NULL, sirtest_thread, (void*)path);
        if (0 != create) {
            errno = create;
            handle_os_error(true, "pthread_create() for thread #%zu failed!", n + 1);
#else
        thrds[n] = _beginthreadex(NULL, 0, sirtest_thread, (void*)path, 0, NULL);
        if (0 == thrds[n]) {
            handle_os_error(true, "_beginthreadex() for thread #%zu failed!", n + 1);
#endif
            free(path);
            pass = false;
            break;
        }

        last_created = n;
        any_created = true;
    }

    if (any_created) {
        for (size_t j = 0; j < last_created + 1; j++) {
            bool joined = true;
            printf("\twaiting for thread %zu/%zu...\n", j + 1, last_created + 1);
#if !defined(_WIN32)
            int join = pthread_join(thrds[j], NULL);
            if (0 != join) {
                joined = false;
                errno = join;
                handle_os_error(true, "pthread_join() for thread #%zu (%p) failed!", j + 1, (void*)thrds[j]);
            }
#else
            DWORD wait = WaitForSingleObject((HANDLE)thrds[j], INFINITE);
            if (WAIT_OBJECT_0 != wait) {
                joined = false;
                handle_os_error(false, "WaitForSingleObject() for thread #%zu (%p) failed!", j + 1, (HANDLE)thrds[j]);
            }
#endif
            if (joined)
                printf("\tthread %zu/%zu joined.\n", j + 1, last_created + 1);
        }
    }

    sir_cleanup();
    return print_result_and_return(pass);
}

#if !defined(_WIN32)
static void* sirtest_thread(void* arg) {
#else
unsigned sirtest_thread(void* arg) {
#endif
    pid_t threadid = _sir_gettid();

    sirchar_t mypath[SIR_MAXPATH] = {0};
    _sir_strncpy(mypath, SIR_MAXPATH, (const char*)arg, SIR_MAXPATH);
    free(arg);

    rmfile(mypath);
    sirfileid_t id = sir_addfile(mypath, SIRL_ALL, SIRO_MSGONLY);

    if (NULL == id) {
        fprintf(stderr, "\t" RED("failed to add file %s!") "\n", mypath);
#if !defined(_WIN32)
        return NULL;
#else
        return 0;
#endif
    }

    printf("\thi, i'm thread #%d, log file: '%s'\n", threadid, mypath);

    for (size_t n = 0; n < 100; n++) {
        for (size_t i = 0; i < 10; i++) {
            sir_debug("this is random gibberish %zu", (n * i) + i + n);

            int r = getrand() % 15;
            if (r % 2 == 0) {
                if (!sir_remfile(id))
                    print_test_error(false, false);

                id = sir_addfile(mypath, SIRL_ALL, SIRO_MSGONLY);

                if (NULL == id)
                    print_test_error(false, false);
                if (!sir_settextstyle(SIRL_DEBUG, SIRS_FG_RED | SIRS_BG_DEFAULT))
                    print_test_error(false, false);
            } else {
                if (!sir_settextstyle(SIRL_DEBUG, SIRS_FG_CYAN | SIRS_BG_YELLOW))
                    print_test_error(false, false);
            }
        }
    }

    sir_remfile(id);
    rmfile(mypath);

#if !defined(_WIN32)
    return NULL;
#else
    return 0;
#endif
}

/*
bool sirtest_XXX(void) {

    INIT(si, SIRL_ALL, 0, 0, 0);
    bool pass = si_init;


    sir_cleanup();
    return printerror(pass);
}
*/

bool print_test_error(bool result, bool expected) {
    sirchar_t message[SIR_MAXERROR] = { 0 };
    uint16_t code = sir_geterror(message);

    if (!expected && !result) {
        printf("\t" RED("!! Unexpected (%hu, %s)") "\n", code, message);
    } else if (expected) {
        printf("\t" GREEN("Expected (%hu, %s)") "\n", code, message);
    }
    return result;
}

void print_os_error(void) {
    sirchar_t message[SIR_MAXERROR] = { 0 };
    uint16_t  code = sir_geterror(message);
    fprintf(stderr, "\t" RED("OS error: (%hu, %s)") "\n", code, message);
}

unsigned int getrand(void) {
    static unsigned int seed = 0;
#if !defined(_WIN32)
    return (unsigned int)rand_r(&seed);
#else
    if (0 == rand_s(&seed)) {
        return seed;
    } else {
        srand(seed);
        return (unsigned int)rand();
    }
#endif
}

bool rmfile(const char* filename) {
    bool removed = false;

    /* just return true if the file doesn't exist. */
    struct stat st;
    if (0 != stat(filename, &st)) {
        if (ENOENT == errno)
            return true;

        handle_os_error(true, "failed to stat %s!", filename);
        print_os_error();
        return false;
    }

#if !defined(_WIN32)
    removed = 0 == remove(filename);
#else
    removed = FALSE != DeleteFile(filename);
#endif

    if (!removed) {
        handle_os_error(false, "failed to delete %s!", filename);
        print_os_error();
    } else {
        printf("\tsuccessfully deleted %s (%ld bytes)...\n", filename, (long)st.st_size);
    }

    return removed;
}

bool deletefiles(const char* search, const char* filename, unsigned* data) {
    if (strstr(filename, search)) {
        rmfile(filename);
        (*data)++;
    }
    return true;
}

bool countfiles(const char* search, const char* filename, unsigned* data) {
    if (strstr(filename, search))
        (*data)++;
    return true;
}

bool enumfiles(const char* search, fileenumproc cb, unsigned* data) {

#if !defined(_WIN32)
    DIR* d = opendir(".");
    if (!d)
        return false;

    rewinddir(d);
    struct dirent* di = readdir(d);
    if (!di)
        return false;

    while (NULL != di) {
        if (!cb(search, di->d_name, data))
            break;
        di = readdir(d);
    };

    closedir(d);
    d = NULL;
#else
    WIN32_FIND_DATA finddata   = {0};
    HANDLE          enumerator = FindFirstFile("./*", &finddata);

    if (INVALID_HANDLE_VALUE == enumerator)
        return false;

    do {
        if (!cb(search, finddata.cFileName, data))
            break;
    } while (FindNextFile(enumerator, &finddata) > 0);

    FindClose(enumerator);
    enumerator = NULL;
#endif

    return true;
}

bool startsirtimer(sirtimer_t* timer) {
#if !defined(_WIN32)
    int gettime = clock_gettime(CLOCK_MONOTONIC, &timer->ts);
    if (0 != gettime) {
        handle_os_error(true, "clock_gettime(%s) failed!", "CLOCK_MONOTONIC");
        print_os_error();
    }

    return 0 == gettime;
#else
    GetSystemTimePreciseAsFileTime(&timer->ft);
    return true;
#endif
}

float sirtimerelapsed(const sirtimer_t* timer) {
#if !defined(_WIN32)
    struct timespec now;
    if (0 == clock_gettime(CLOCK_MONOTONIC, &now)) {
        return (float)((now.tv_sec * 1e3) + (now.tv_nsec / 1e6) - (timer->ts.tv_sec * 1e3) +
                       (timer->ts.tv_nsec / 1e6));
    } else {
        handle_os_error(true, "clock_gettime(%s) failed!", "CLOCK_MONOTONIC");
        print_os_error();
    }
    return 0.0f;
#else
    FILETIME now;
    GetSystemTimePreciseAsFileTime(&now);
    ULARGE_INTEGER start = {0};
    start.LowPart = timer->ft.dwLowDateTime;
    start.HighPart = timer->ft.dwHighDateTime;
    ULARGE_INTEGER n100sec = {0};
    n100sec.LowPart = now.dwLowDateTime;
    n100sec.HighPart = now.dwHighDateTime;
    return (float)((n100sec.QuadPart - start.QuadPart) / 1e4);
#endif
}
