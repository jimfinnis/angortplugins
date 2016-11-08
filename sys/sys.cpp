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
#include <sys/types.h>
#include <sys/wait.h>
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

%wordargs execpipe ls (arglist path -- output)
{
    static const int BUFFERSIZE=1024;
    int link[2];
    pid_t pid;
    char foo[BUFFERSIZE];
    if(pipe(link)==-1)
        throw RUNT(EX_FAILED,"cannot create pipe");
    if((pid = fork())==-1)
        throw RUNT(EX_FAILED,"cannot fork");
    if(!pid){
        dup2(link[1],STDOUT_FILENO);
        close(link[0]);
        close(link[1]);
        char **args = new char * [p0->count()+1];
        for(int i=0;i<p0->count();i++){
            args[i] = strdup(p0->get(i)->toString().get());
        }
        args[p0->count()] = NULL;
        execv(p1,args);
        exit(0);
    } else {
        char *p = NULL;
        int len=0;
        close(link[1]);
        while(1){
            int nbytes = read(link[0],foo,sizeof(foo));
            if(nbytes){
                int end=len;
                len+=nbytes;
                if(!p){
                    p=(char *)malloc(len+1);
                } else {
                    p=(char *)realloc(p,len+1);
                }
                memcpy(p+end,foo,nbytes);
            } else {
                break;
            }
        }
        wait(NULL);
        if(!p)
            a->pushString("");
        else {
            p[len]=0;
            a->pushString(p);
            free(p);
        }
    }
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

