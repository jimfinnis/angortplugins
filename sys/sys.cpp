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
#include <sys/errno.h>
#include <angort/angort.h>

using namespace angort;

%name sys
%shared

%word sleep (time --) sleep for some time. Identical to time$delay.
{
    Value *p;
    a->popParams(&p,"n");
    usleep((int)(p->toFloat()*1.0e6f));
}

%word cwd (-- working directory) get current directory
{
    char buf[PATH_MAX];
    getcwd(buf,PATH_MAX);
    a->pushString(buf);
}

%wordargs chdir s (path -- success) change current directory
{
    a->pushInt((chdir(p0)<0)?0:1);
}

%word getpid (-- PID) get process id
{
    a->pushInt(getpid());
}

%word fork (-- PID) fork, returning 0 for child and PID for parent, like fork(2).
{
    a->pushInt(fork());
}

%wordargs exec ls (arglist path --) perform an execv(2).
Perform execv(2) with a list of arguments, which are converted to strings.
{
    char **args = new char * [p0->count()+1];
    for(int i=0;i<p0->count();i++){
        args[i] = strdup(p0->get(i)->toString().get());
    }
    args[p0->count()] = NULL;
    execv(p1,args);
}

static bool execpipedebug=false;

%wordargs debug i (bool --) enable/disable execpipe debug
{
    execpipedebug=(p0!=0);
}

static void epdprintf(const char *s,...){
    if(execpipedebug){
        char buf[256];
        va_list args;
        va_start(args,s);
        vsnprintf(buf,256,s,args);
        printf("execpipe debug:  %s\n",buf);
    }
}

%wordargs execpipe sls (input arglist path -- output)
Pipe a string (possibly consisting of multiple lines) through a 
program. Does this with two pipes - one into and one out of the process,
connected to the stdin and stdout of that process.
{
    static const int BUFFERSIZE=1024;
    int linkfromcmd[2];
    int linktocmd[2];
    pid_t pid;
    char foo[BUFFERSIZE];
    if(pipe(linkfromcmd)==-1)
        throw RUNT(EX_FAILED,"cannot create pipe");
    if(pipe(linktocmd)==-1)
        throw RUNT(EX_FAILED,"cannot create pipe");
    if((pid = fork())==-1)
        throw RUNT(EX_FAILED,"cannot fork");
    if(!pid){
        epdprintf("CHILD started");
        // this is the CHILD
        dup2(linkfromcmd[1],STDOUT_FILENO);
        dup2(linktocmd[0],STDIN_FILENO);
        close(linkfromcmd[0]);
        close(linktocmd[1]);
        char **args = new char * [p1->count()+2];
        args[0]=(char *)p2;
        for(int i=0;i<p1->count();i++){
            args[i+1] = strdup(p1->get(i)->toString().get());
        }
        args[p1->count()+1] = NULL;
        epdprintf("CHILD executing execv");
        execv(p2,args);
        epdprintf("CHILD returned!");
        exit(1);
        
    } else {
        // this is the PARENT
        char *p = NULL;
        int len=0;
        epdprintf("PARENT running");
        close(linkfromcmd[1]);
        close(linktocmd[0]);
        write(linktocmd[1],p0,strlen(p0)+1);
        close(linktocmd[1]);
        while(1){
            int nbytes = read(linkfromcmd[0],foo,sizeof(foo));
            epdprintf("PARENT read %d",nbytes);
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
        epdprintf("PARENT done");
        // reap
        epdprintf("PARENT waiting");
        wait(NULL);
        if(!p)
            a->pushString("");
        else {
            p[len]=0;
            a->pushString(p);
            free(p);
        }
    }
    close(linkfromcmd[0]);
    close(linktocmd[0]);
    close(linkfromcmd[1]);
    close(linktocmd[1]);
}
        

static int exitcode=0;
%wordargs system s (string --) Perform system().
{
    exitcode=system(p0);
}

%word getexit (-- exitcode) get exit code of previous system call
{
    a->pushInt(exitcode);
}



%init
{
    if(showinit)
        fprintf(stderr,"Initialising SYS plugin, %s %s\n",__DATE__,__TIME__);
}

