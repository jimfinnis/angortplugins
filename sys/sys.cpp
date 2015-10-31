/**
 * @file
 * Assorted system functions
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <angort/angort.h>

using namespace angort;

%name sys
%shared

%word sleep (time --) sleep for some time
{
    Value *p;
    a->popParams(&p,"n");
    usleep((int)(p->toFloat()*1.0e6f));
}

%wordargs getenv s (name -- str or none)
{
    const char *s = getenv(p0);
    if(s)
        a->pushString(s);
    else
        a->pushNone();
}

%word cwd (-- working directory)
{
    char buf[PATH_MAX];
    getcwd(buf,PATH_MAX);
    a->pushString(buf);
}

%init
{
    fprintf(stderr,"Initialising SYS plugin, %s %s\n",__DATE__,__TIME__);
}

