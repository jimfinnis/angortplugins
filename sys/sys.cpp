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

%word cwd (-- working directory)
{
    char buf[PATH_MAX];
    getcwd(buf,PATH_MAX);
    a->pushString(buf);
}

%wordargs chdir s (path -- success)
{
    a->pushInt((chdir(p0)<0)?0:1);
}

%word getpid (-- PID)
{
    a->pushInt(getpid());
}

%word fork (-- PID)
{
    a->pushInt(fork());
}

%wordargs exec ls (arglist path --)
{
    char **args = new char * [p0->count()+1];
    for(int i=0;i<p0->count();i++){
        args[i] = strdup(p0->get(i)->toString().get());
    }
    args[p0->count()] = NULL;
    execv(p1,args);
}

static int exitcode=0;
%wordargs system s (string --)
{
    exitcode=system(p0);
}

%word getexit (-- exitcode) get exit code of prev. system call
{
    a->pushInt(exitcode);
}



%init
{
    fprintf(stderr,"Initialising SYS plugin, %s %s\n",__DATE__,__TIME__);
}

