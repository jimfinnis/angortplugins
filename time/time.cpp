/**
 * @file time.cpp
 * @brief  Brief description of file.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <angort/angort.h>
#include "../hashgets.h"
#include "../wrappers.h"

using namespace angort;

typedef struct tm Timestruct;

static BasicWrapperType<Timestruct> tTM("TMTS");

%type tm tTM Timestruct

%name time
%shared

static timespec progstart;

inline double time_diff(timespec start, timespec end)
{
    timespec temp;
    if ((end.tv_nsec-start.tv_nsec)<0) {
        temp.tv_sec = end.tv_sec-start.tv_sec-1;
        temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
    } else {
        temp.tv_sec = end.tv_sec-start.tv_sec;
        temp.tv_nsec = end.tv_nsec-start.tv_nsec;
    }
    
    double t = temp.tv_sec;
    double ns = temp.tv_nsec;
    t += ns*1e-9;
    return t;
}

%word now (-- double) get current time since start of program
{
    struct timespec t;
    extern struct timespec progstart;
    
    clock_gettime(CLOCK_MONOTONIC,&t);
    double diff=time_diff(progstart,t);
    a->pushDouble(diff);
}

%word absnow (-- double) -- get absolute time as double (NOT for use with format)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC,&ts);
    double t = ts.tv_nsec;
    t *= 1e-9;
    t += (double)ts.tv_sec;
    a->pushDouble(t);
}

%word delay (float --) wait for a number of seconds
{
    float f = a->popFloat();
    f *= 1e6f;
    usleep((int)f);
}

%word localtime (-- tm) get the current local time as a TM
{
    time_t t;
    Timestruct tm;
    time(&t);
    localtime_r(&t,&tm);
    tTM.set(a->pushval(),tm);
}

%word gmtime (-- tm) get the current GMT time as a TM
{
    time_t t;
    Timestruct tm;
    time(&t);
    localtime_r(&t,&tm);
    tTM.set(a->pushval(),tm);
}

%wordargs tolong A|tm (tm -- long) convert time to long integer
{
    a->pushLong(mktime(p0));
}

%wordargs longtotimelocal L (long -- tm) convert time from long integer (local time)
{
    time_t t = (time_t)p0;
    Timestruct tm;
    localtime_r(&t,&tm);
    tTM.set(a->pushval(),tm);
}

%wordargs longtotimegmt L (long -- tm) convert time from long integer (GMT)
{
    time_t t = (time_t)p0;
    Timestruct tm;
    gmtime_r(&t,&tm);
    tTM.set(a->pushval(),tm);
}

// because wrappers use the ID as the internal code (see wrappers.h constructors)

%binop TMTS sub TMTS
{
    time_t p = mktime(tTM.get(lhs));
    time_t q = mktime(tTM.get(rhs));
    a->pushDouble(difftime(q,p));
}

%wordargs format As|tm (TM string --) format according to strftime() rules
{
    char buf[1024];
    strftime(buf,1024,p1,p0);
    a->pushString(buf);
}

%wordargs timetohash A|tm (TM -- hash) convert time to a hash
{
    Timestruct tm = *p0;
    Hash *h = Types::tHash->set(a->pushval());
    h->setSymInt("sec",tm.tm_sec);
    h->setSymInt("min",tm.tm_min);
    h->setSymInt("hour",tm.tm_hour);
    h->setSymInt("mday",tm.tm_mday);
    h->setSymInt("mon",tm.tm_mon);
    h->setSymInt("year",tm.tm_year+1900);
    h->setSymInt("wday",tm.tm_wday);
    h->setSymInt("yday",tm.tm_yday);
    h->setSymInt("isdst",tm.tm_isdst);
}

%wordargs hashtotime h (hash -- TM) convert hash to a time
{
    Timestruct tm;
    tm.tm_sec = hgetintdef(p0,"sec",0);
    tm.tm_min = hgetintdef(p0,"min",0);
    tm.tm_hour = hgetintdef(p0,"hour",0);
    tm.tm_mday = hgetintdef(p0,"mday",0);
    tm.tm_mon = hgetintdef(p0,"mon",0);
    tm.tm_year = hgetintdef(p0,"year",0)-1900;
    tm.tm_wday = hgetintdef(p0,"wday",0);
    tm.tm_yday = hgetintdef(p0,"yday",0);
    tm.tm_isdst = hgetintdef(p0,"isdst",0);
    // there will be warnings for GNU stuff (gmtoff and zone)
#ifdef __GNUC__
    tm.tm_zone = "";
    tm.tm_gmtoff = 0;
#endif
    tTM.set(a->pushval(),tm);
}
    


%init
{
    clock_gettime(CLOCK_MONOTONIC,&progstart);
    fprintf(stderr,"Initialising Time plugin, %s %s\n",__DATE__,__TIME__);
}

