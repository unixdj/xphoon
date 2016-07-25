/* dtime.c - extracted from the phoon/libtws package
**
** Copyright (C) 1988,1991 by Jef Poskanzer <jef@acme.com>.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
** OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
** HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
** OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
** SUCH DAMAGE.
*/

#include "tws.h"
#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#ifdef SYS5
extern int daylight;
extern long timezone;
#else /*SYS5*/
#include <sys/timeb.h>
#endif /*SYS5*/

struct tws*
dtwstime( void )
    {
    long clock;

    (void) time( &clock );
    return dlocaltime( &clock );
    }

struct tws*
dlocaltime( long* clock )
    {
    register struct tm* tm;
#ifndef SYS5
    struct timeb tb;
#endif /* !SYS5 */
    static struct tws tw;

    if ( clock == (long*) 0 )
	return (struct tws*) 0;
    tw.tw_flags = TW_NULL;

    tm = localtime( clock );
    tw.tw_sec = tm->tm_sec;
    tw.tw_min = tm->tm_min;
    tw.tw_hour = tm->tm_hour;
    tw.tw_mday = tm->tm_mday;
    tw.tw_mon = tm->tm_mon;
    tw.tw_year = tm->tm_year;
    tw.tw_wday = tm->tm_wday;
    tw.tw_yday = tm->tm_yday;
    if ( tm->tm_isdst )
	tw.tw_flags |= TW_DST;
#ifndef SYS5
    ftime( &tb );
    tw.tw_zone = -tb.timezone;
#else /* SYS5 */
    tzset();
    tw.tw_zone = -( timezone / 60 );
#endif /* SYS5 */
    tw.tw_flags &= ~TW_SDAY;
    tw.tw_flags |= TW_SEXP;
    tw.tw_clock = *clock;

    return &tw;
    }
