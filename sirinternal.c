/**
 * @file sirinternal.c
 * @brief Internal implementation.
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
#include "sirinternal.h"
#include "sirconsole.h"
#include "sirdefaults.h"
#include "sirfilecache.h"
#include "sirtextstyle.h"
#include "sirmutex.h"

/**
 * @addtogroup intern
 * @{
 */

static sirinit   _sir_si = {0};
static sirfcache _sir_fc = {0};

static sirmutex_t si_mutex;
static sironce_t  si_once = SIR_ONCE_INIT;

static sirmutex_t fc_mutex;
static sironce_t  fc_once = SIR_ONCE_INIT;

static sirmutex_t ts_mutex;
static sironce_t  ts_once = SIR_ONCE_INIT;

static atomic_uint_fast32_t _sir_magic;

bool _sir_sanity(void) {
    if (_SIR_MAGIC == atomic_load(&_sir_magic))
        return true;
    _sir_seterror(_SIR_E_NOTREADY);
    return false;
}

bool _sir_options_sanity(const sirinit* si) {

    if (!_sir_validptr(si))
        return false;

    bool levelcheck = true;
    levelcheck &= _sir_validlevels(si->d_stdout.levels);
    levelcheck &= _sir_validlevels(si->d_stderr.levels);

#if !defined(SIR_NO_SYSLOG)
    levelcheck &= _sir_validlevels(si->d_syslog.levels);
#endif

    bool optscheck = true;
    optscheck &= _sir_validopts(si->d_stdout.opts);
    optscheck &= _sir_validopts(si->d_stderr.opts);

    char* nullterm = strrchr(si->processName, '\0');
    return levelcheck && optscheck && _sir_validptr(nullterm);
}

bool _sir_init(sirinit* si) {

    _sir_seterror(_SIR_E_NOERROR);

    if (!_sir_validptr(si))
        return false;

    if (_SIR_MAGIC == atomic_load(&_sir_magic)) {
        _sir_seterror(_SIR_E_ALREADY);
        return false;
    }

    _sir_defaultlevels(&si->d_stdout.levels, sir_stdout_def_lvls);
    _sir_defaultopts(&si->d_stdout.opts, sir_stdout_def_opts);

    _sir_defaultlevels(&si->d_stderr.levels, sir_stderr_def_lvls);
    _sir_defaultopts(&si->d_stderr.opts, sir_stderr_def_opts);

#if !defined(SIR_NO_SYSLOG)
    _sir_defaultlevels(&si->d_syslog.levels, sir_syslog_def_lvls);
#endif

    if (!_sir_options_sanity(si))
        return false;

#if defined(_WIN32)
    if (!_sir_initialize_stdio())
        return false;
#endif

    sirinit* _si = _sir_locksection(_SIRM_INIT);
    assert(_si);

    if (_si) {
        memcpy(_si, si, sizeof(sirinit));

#if !defined(SIR_NO_SYSLOG)
        if (0 != _si->d_syslog.levels)
            openlog(_sir_validstrnofail(_si->processName) ? _si->processName : "",
                (_si->d_syslog.includePID ? LOG_PID : 0) | LOG_ODELAY, LOG_USER);
#endif

        atomic_store(&_sir_magic, _SIR_MAGIC);
        _sir_unlocksection(_SIRM_INIT);
        return true;
    }

    return false;
}

void _sir_stdoutlevels(sirinit* si, sir_update_config_data* data) {
    if (si->d_stdout.levels != *data->levels) {
        si->d_stdout.levels = *data->levels;

    }
}

void _sir_stdoutopts(sirinit* si, sir_update_config_data* data) {
    if (si->d_stdout.opts != *data->opts) {
        si->d_stdout.opts = *data->opts;

    }
}

void _sir_stderrlevels(sirinit* si, sir_update_config_data* data) {
    if (si->d_stderr.levels != *data->levels) {
        si->d_stderr.levels = *data->levels;

    }
}

void _sir_stderropts(sirinit* si, sir_update_config_data* data) {
    if (si->d_stderr.opts != *data->opts) {
        si->d_stderr.opts = *data->opts;

    }
}

void _sir_sysloglevels(sirinit* si, sir_update_config_data* data) {
    if (si->d_syslog.levels != *data->levels) {
        si->d_syslog.levels = *data->levels;

    }
}

bool _sir_writeinit(sir_update_config_data* data, sirinit_update update) {

    _sir_seterror(_SIR_E_NOERROR);

    if (_sir_sanity() && _sir_validupdatedata(data) && _sir_validaddr(update)) {
        sirinit* si = _sir_locksection(_SIRM_INIT);
        assert(si);
        if (si) {
            update(si, data);
            return _sir_unlocksection(_SIRM_INIT);
        }
    }

    return false;
}

void* _sir_locksection(sir_mutex_id mid) {

    sirmutex_t* m = NULL;
    void* sec     = NULL;

    if (_sir_mapmutexid(mid, &m, &sec)) {
        bool enter = _sirmutex_lock(m);
        assert(enter);
        return enter ? sec : NULL;
    }

    return NULL;
}

bool _sir_unlocksection(sir_mutex_id mid) {

    sirmutex_t* m = NULL;
    void* sec     = NULL;

    if (_sir_mapmutexid(mid, &m, &sec)) {
        bool leave = _sirmutex_unlock(m);
        assert(leave);
        return leave;
    }

    return false;
}

bool _sir_mapmutexid(sir_mutex_id mid, sirmutex_t** m, void** section) {

    if (!_sir_validptr(m))
        return false;

    sirmutex_t* tmpm;
    void* tmpsec;

    switch (mid) {
        case _SIRM_INIT:
            _sir_once(&si_once, _sir_initmutex_si_once);
            tmpm   = &si_mutex;
            tmpsec = &_sir_si;
            break;
        case _SIRM_FILECACHE:
            _sir_once(&fc_once, _sir_initmutex_fc_once);
            tmpm   = &fc_mutex;
            tmpsec = &_sir_fc;
            break;
        case _SIRM_TEXTSTYLE:
            _sir_once(&ts_once, _sir_initmutex_ts_once);
            tmpm   = &ts_mutex;
            tmpsec = sir_override_style_map;
            break;
        default: tmpm = NULL; tmpsec = NULL;
    }

    *m = tmpm;
    if (section)
        *section = tmpsec;

    return *m != NULL && (!section || *section != NULL);
}

bool _sir_cleanup(void) {

    if (!_sir_sanity())
        return false;

    bool cleanup   = true;
    sirfcache* sfc = _sir_locksection(_SIRM_FILECACHE);
    assert(sfc);

    cleanup &= NULL != sfc;
    if (cleanup) {
        bool destroyfc = _sir_fcache_destroy(sfc);
        assert(destroyfc);
        cleanup &= _sir_unlocksection(_SIRM_FILECACHE) && destroyfc;
    }

    sirinit* si = _sir_locksection(_SIRM_INIT);
    assert(si);

    cleanup &= NULL != si;
    if (cleanup) {
        memset(si, 0, sizeof(sirinit));
        cleanup &= _sir_unlocksection(_SIRM_INIT);
    }

    _sir_resettextstyles();
    atomic_store(&_sir_magic, 0);
    
    _sir_selflog("%s: libsir is cleaned up\n", __func__);
    return cleanup;
}

#if !defined(_WIN32)
void _sir_initmutex_si_once(void) {
    _sir_initmutex(&si_mutex);
}

void _sir_initmutex_fc_once(void) {
    _sir_initmutex(&fc_mutex);
}

void _sir_initmutex_ts_once(void) {
    _sir_initmutex(&ts_mutex);
}
#else
BOOL CALLBACK _sir_initmutex_si_once(PINIT_ONCE ponce, PVOID param, PVOID* ctx) {
    _SIR_UNUSED(ponce);
    _SIR_UNUSED(param);
    _SIR_UNUSED(ctx)
    _sir_initmutex(&si_mutex);
    return TRUE;
}

BOOL CALLBACK _sir_initmutex_fc_once(PINIT_ONCE ponce, PVOID param, PVOID* ctx) {
    _SIR_UNUSED(ponce);
    _SIR_UNUSED(param);
    _SIR_UNUSED(ctx)
    _sir_initmutex(&fc_mutex);
    return TRUE;
}

BOOL CALLBACK _sir_initmutex_ts_once(PINIT_ONCE ponce, PVOID param, PVOID* ctx) {
    _SIR_UNUSED(ponce);
    _SIR_UNUSED(param);
    _SIR_UNUSED(ctx)
    _sir_initmutex(&ts_mutex);
    return TRUE;
}
#endif

void _sir_initmutex(sirmutex_t* mutex) {
    bool init = _sirmutex_create(mutex);
    _SIR_UNUSED(init);
    assert(init);
}

bool _sir_once(sironce_t* once, sir_once_fn func) {
#if !defined(_WIN32)
    return 0 == pthread_once(once, func);
#else
    return FALSE != InitOnceExecuteOnce(once, func, NULL, NULL);
#endif
}

bool _sir_logv(sir_level level, const sirchar_t* format, va_list args) {

    _sir_seterror(_SIR_E_NOERROR);

    if (!_sir_sanity() || !_sir_validlevel(level) || !_sir_validstr(format))
        return false;

    sirinit* si = _sir_locksection(_SIRM_INIT);
    if (!si)
        return false;

    sirinit tmpsi = {0};
    memcpy(&tmpsi, si, sizeof(sirinit));

    _sir_unlocksection(_SIRM_INIT);

    static thread_local sirbuf buf = {0};
    static thread_local siroutput output = {0};
    _sir_buf2output(&buf, &output);

    bool appliedstyle = false;
    sir_textstyle style = _sir_gettextstyle(level);
    assert(SIRS_INVALID != style);

    if (SIRS_INVALID != style) {
        appliedstyle = _sir_formatstyle(style, output.style, SIR_MAXSTYLE);
        assert(appliedstyle);
    }

    time_t now;
    long nowmsec;
    bool gettime = _sir_getlocaltime(&now, &nowmsec);
    assert(gettime);

    if (gettime) {
        bool fmttime = _sir_formattime(now, output.timestamp, SIR_TIMEFORMAT);
        _SIR_UNUSED(fmttime);

        if (0 > snprintf(output.msec, SIR_MAXMSEC, SIR_MSECFORMAT, nowmsec))
            _sir_handleerr(errno);
    }

    if (0 > snprintf(output.level, SIR_MAXLEVEL, SIR_LEVELFORMAT, _sir_levelstr(level)))
        _sir_handleerr(errno);

    if (_sir_validstrnofail(tmpsi.processName) &&
        0 != strncmp(tmpsi.processName, output.name, SIR_MAXNAME))
        _sir_strncpy(output.name, SIR_MAXNAME, tmpsi.processName, SIR_MAXNAME);

    pid_t pid = _sir_getpid();
    if (0 > snprintf(output.pid, SIR_MAXPID, SIR_PIDFORMAT, pid))
        _sir_handleerr(errno);


    pid_t tid  = _sir_gettid();
    if (tid != pid) {
        if (!_sir_getthreadname(output.tid)) {
            if (0 > snprintf(output.tid, SIR_MAXPID, SIR_PIDFORMAT, tid))
                _sir_handleerr(errno);
        }
    }

    if (0 > vsnprintf(output.message, SIR_MAXMESSAGE, format, args))
        _sir_handleerr(errno);
#pragma message("TBH, if any of these formatting calls fail, the whole thing should fail. there's no point in logging something incorrect/garbled.")
    return _sir_dispatch(&tmpsi, level, &output);
}

bool _sir_dispatch(sirinit* si, sir_level level, siroutput* output) {

    bool retval       = true;
    size_t dispatched = 0;
    size_t wanted     = 0;

    if (_sir_bittest(si->d_stdout.levels, level)) {
        const sirchar_t* write = _sir_format(true, si->d_stdout.opts, output);
        bool wrote = _sir_validstrnofail(write) &&
            _sir_write_stdout(write, output->output_len);
        retval &= wrote;

        if (wrote)
            dispatched++;
        wanted++;
    }

    if (_sir_bittest(si->d_stderr.levels, level)) {
        const sirchar_t* write = _sir_format(true, si->d_stderr.opts, output);
        bool wrote = _sir_validstrnofail(write) &&
            _sir_write_stderr(write, output->output_len);
        retval &= wrote;

        if (wrote)
            dispatched++;
        wanted++;
    }

#if !defined(SIR_NO_SYSLOG)
    if (_sir_bittest(si->d_syslog.levels, level)) {
        syslog(_sir_syslog_maplevel(level), "%s", output->message);
        dispatched++;
        wanted++;
    }
#endif

    sirfcache* sfc = _sir_locksection(_SIRM_FILECACHE);
    if (sfc) {
        size_t fdispatched = 0;
        size_t fwanted = 0;
        retval &= _sir_fcache_dispatch(sfc, level, output, &fdispatched, &fwanted);
        retval &= _sir_unlocksection(_SIRM_FILECACHE);
        dispatched += fdispatched;
        wanted += fwanted;
    }

    if (0 == wanted) {
        _sir_seterror(_SIR_E_NODEST);
        return false;
    }

    return retval && (dispatched == wanted);
}

const sirchar_t* _sir_format(bool styling, sir_options opts, siroutput* output) {

    if (_sir_validopts(opts) && _sir_validptr(output) && _sir_validptr(output->output)) {
        bool first = true;

        _sir_resetstr(output->output);

        if (styling)
            _sir_strncat(output->output, SIR_MAXOUTPUT, output->style, SIR_MAXSTYLE);

        if (!_sir_bittest(opts, SIRO_NOTIME)) {
            _sir_strncat(output->output, SIR_MAXOUTPUT, output->timestamp, SIR_MAXTIME);
            first = false;

#if defined(SIR_MSEC_TIMER)
            if (!_sir_bittest(opts, SIRO_NOMSEC))
                _sir_strncat(output->output, SIR_MAXOUTPUT, output->msec, SIR_MAXMSEC);
#endif
        }

        if (!_sir_bittest(opts, SIRO_NOLEVEL)) {
            if (!first)
                _sir_strncat(output->output, SIR_MAXOUTPUT, " ", 1);
            _sir_strncat(output->output, SIR_MAXOUTPUT, output->level, SIR_MAXLEVEL);
            first = false;
        }

        bool name = false;
        if (!_sir_bittest(opts, SIRO_NONAME) && _sir_validstrnofail(output->name)) {
            if (!first)
                _sir_strncat(output->output, SIR_MAXOUTPUT, " ", 1);
            _sir_strncat(output->output, SIR_MAXOUTPUT, output->name, SIR_MAXNAME);
            first = false;
            name  = true;
        }

        bool wantpid = !_sir_bittest(opts, SIRO_NOPID) && _sir_validstrnofail(output->pid);
        bool wanttid = !_sir_bittest(opts, SIRO_NOTID) && _sir_validstrnofail(output->tid);

        if (wantpid || wanttid) {
            if (name)
                _sir_strncat(output->output, SIR_MAXOUTPUT, "(", 1);
            else if (!first)
                _sir_strncat(output->output, SIR_MAXOUTPUT, " ", 1);

            if (wantpid)
                _sir_strncat(output->output, SIR_MAXOUTPUT, output->pid, SIR_MAXPID);

            if (wanttid) {
                if (wantpid)
                    _sir_strncat(output->output, SIR_MAXOUTPUT, SIR_PIDSEPARATOR, 1);
                _sir_strncat(output->output, SIR_MAXOUTPUT, output->tid, SIR_MAXPID);
            }

            if (name)
                _sir_strncat(output->output, SIR_MAXOUTPUT, ")", 1);
        }

        if (!first)
            _sir_strncat(output->output, SIR_MAXOUTPUT, ": ", 2);

        _sir_strncat(output->output, SIR_MAXOUTPUT, output->message, SIR_MAXMESSAGE);

        if (styling)
            _sir_strncat(output->output, SIR_MAXOUTPUT, SIR_ENDSTYLE, SIR_MAXSTYLE);

        _sir_strncat(output->output, SIR_MAXOUTPUT, "\n", 1);

        output->output_len = strnlen(output->output, SIR_MAXOUTPUT);

        return output->output;
    }

    return NULL;
}

#if !defined(SIR_NO_SYSLOG)
int _sir_syslog_maplevel(sir_level level) {
    switch (level) {
        case SIRL_EMERG:  return LOG_EMERG;
        case SIRL_ALERT:  return LOG_ALERT;
        case SIRL_CRIT:   return LOG_CRIT;
        case SIRL_ERROR:  return LOG_ERR;
        case SIRL_WARN:   return LOG_WARNING;
        case SIRL_NOTICE: return LOG_NOTICE;
        case SIRL_INFO:   return LOG_INFO;
        case SIRL_DEBUG:  return LOG_DEBUG;
        default:
            assert(!"invalid sir_level");
            return LOG_DEBUG;
    }
}
#endif

void _sir_buf2output(sirbuf* buf, siroutput* output) {
    if (_sir_validptr(buf) && _sir_validptr(output)) {
        output->style       = buf->style;
        output->timestamp   = buf->timestamp;
        output->msec        = buf->msec;
        output->level       = buf->level;
        output->name        = buf->name;
        output->pid         = buf->pid;
        output->tid         = buf->tid;
        output->message     = buf->message;
        output->output      = buf->output;
        output->output_len  = 0;
        output->state.level = SIRL_NONE;
        output->state.pid   = 0;
        output->state.tid   = 0;
    }
}

const sirchar_t* _sir_levelstr(sir_level level) {
        
    if (_sir_validlevel(level)) {
        _SIR_DECLARE_BIN_SEARCH(0, _sir_countof(sir_level_str_map));
        _SIR_BEGIN_BIN_SEARCH();

        if (sir_level_str_map[_mid].level == level)
            return sir_level_str_map[_mid].str;

        int comparison = sir_level_str_map[_mid].level < level ? 1 : - 1;

        _SIR_ITERATE_BIN_SEARCH(comparison);
        _SIR_END_BIN_SEARCH();
    }

    return SIR_UNKNOWN;
}

bool _sir_formattime(time_t now, sirchar_t* buffer, const sirchar_t* format) {

    if (0 != now && _sir_validptr(buffer) && _sir_validstr(format)) {
        struct tm timebuf = {0};
        size_t fmttime = strftime(buffer, SIR_MAXTIME, format, _sir_localtime(&now, &timebuf));
        assert(0 != fmttime);

        if (0 == fmttime)
            _sir_selflog("%s: strftime returned 0; format string: '%s'", __func__, format);

        return 0 != fmttime;
    }

    return false;
}

bool _sir_getlocaltime(time_t* tbuf, long* nsecbuf) {
    if (tbuf) {
        time(tbuf);
#if defined(SIR_MSEC_POSIX)
        struct timespec ts = {0};
        int clock          = clock_gettime(SIR_MSECCLOCK, &ts);
        assert(0 == clock);

        if (0 == clock) {
            if (nsecbuf) {
                *nsecbuf = (ts.tv_nsec / 1e6);
                assert(*nsecbuf < 1000);
            }
        } else {
            *nsecbuf = 0;
            _sir_selflog("%s: clock_gettime failed; errno: %d\n", __func__, errno);
        }
#elif defined(SIR_MSEC_WIN32)
        static const ULONGLONG uepoch = (ULONGLONG)116444736e9;

        FILETIME ftutc = {0};
        GetSystemTimePreciseAsFileTime(&ftutc);

        ULARGE_INTEGER ftnow;
        ftnow.HighPart = ftutc.dwHighDateTime;
        ftnow.LowPart  = ftutc.dwLowDateTime;
        ftnow.QuadPart = (ULONGLONG)((ftnow.QuadPart - uepoch) / 1e7);

        *tbuf = (time_t)ftnow.QuadPart;

        if (nsecbuf) {
            SYSTEMTIME st = {0};
            FileTimeToSystemTime(&ftutc, &st);
            *nsecbuf = st.wMilliseconds;
        }
#else
        time(tbuf);
        if (nsecbuf)
            *nsecbuf = 0;
#endif
        return true;
    }

    return false;
}

pid_t _sir_getpid(void) {
#if !defined(_WIN32)
    return getpid();
#else
    return (pid_t)GetProcessId(GetCurrentProcess());
#endif
}

pid_t _sir_gettid(void) {
    pid_t tid = 0;
#if defined(__MACOS__)
    uint64_t tid64 = 0;
    int gettid = pthread_threadid_np(NULL, &tid64);
    if (0 != gettid)
        _sir_handleerr(gettid);
    tid = (pid_t)tid64;
#elif defined(__BSD__)
    tid = (pid_t)pthread_getthreadid_np();    
#elif defined(_DEFAULT_SOURCE)
    tid = syscall(SYS_gettid);
#elif defined(_WIN32)
    tid = (pid_t)GetCurrentThreadId();
#else
#   error "cannot determine how to get thread id; please contact the author"
#endif
    return tid;
}

bool _sir_getthreadname(char name[SIR_MAXPID]) {
#if defined(__BSD__) || defined(_GNU_SOURCE)
    return 0 == pthread_getname_np(pthread_self(), name, SIR_MAXPID);
#else
    _SIR_UNUSED(name);
    return false;
#endif
}

/** @} */
