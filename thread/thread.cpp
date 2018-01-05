/**
 * @file thread.cpp
 * @brief  Brief description of file.
 *
 */

#include <angort/angort.h>
#include <pthread.h>
#include <unistd.h>
#include "../wrappers.h"

using namespace angort;

%name thread
%shared

// thread hook object Angort uses to do... stuff.

class MyThreadHookObject : public ThreadHookObject {
    // a big global lock
    pthread_mutex_t mutex;
public:
    MyThreadHookObject(){
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&mutex,&attr);
    }
    ~MyThreadHookObject(){
        pthread_mutex_destroy(&mutex);
    }
        
    // single global mutex, nestable
    virtual void globalLock(){
        pthread_mutex_lock(&mutex);
    }
    virtual void globalUnlock(){
        pthread_mutex_unlock(&mutex);
    }
};

static MyThreadHookObject hook;


class Thread : public GarbageCollected {
    Runtime *runtime;
public:
    Value func;
    Value retval; // the returned value
    pthread_t thread;
    
    bool isRunning(){
        return runtime != NULL;
    }
    Thread(Angort *ang,Value *v,Value *arg) : GarbageCollected(){
        incRefCt(); // make sure we don't get deleted until complete
        func.copy(v);
        runtime = new Runtime(ang,"<thread>");
        runtime->pushval()->copy(arg);
        void *_threadfunc(void *);
//        printf("Creating thread at %p/%s\n",this,func.t->name);
        pthread_create(&thread,NULL,_threadfunc,this);
    }
    ~Thread(){
//        printf("Thread destroyed at %p\n",this);
    }
    void run(){
        const StringBuffer& buf = func.toString();
        runtime->runValue(&func);
        hook.globalLock();
        // pop the last value off the thread runtime
        // and copy it into the return value, if there is one.
        if(!runtime->stack.isempty()){
            retval.copy(runtime->stack.popptr());
        }
        delete runtime;
        runtime = NULL;
        // decrement refct, and delete this if it's zero. This is kind
        // of OK, here - it's the last thing that happens.
        if(decRefCt())delete this; 
        hook.globalUnlock();
    }
};

void *_threadfunc(void *p){
    Thread *t = (Thread *)p;
//    printf("starting thread at %p/%s\n",t,t->func.t->name);
    t->run();
//    printf("Thread func terminated OK?\n");
    return NULL;
}


class ThreadType : public GCType {
public:
    ThreadType(){
        add("thread","THRD");
    }
    
    Thread *get(Value *v){
        if(v->t!=this)
            throw RUNT(EX_TYPE,"not a thread");
        return (Thread *)(v->v.gc);
    }
    
    // create a new thread!
    void set(Value *v,Angort *ang,Value *func,Value *pass){
        if(func->t != Types::tCode)
            throw RUNT(EX_TYPE,"not a codeblock (can't be a closure)");
        hook.globalLock();
        v->clr();
        v->t=this;
        v->v.gc = new Thread(ang,func,pass);
        incRef(v);
        hook.globalUnlock();
    }
};

static ThreadType tThread;

// mutex wrapper class
struct Mutex {
    pthread_mutex_t m;
    Mutex(){
        // make the mutex recursive
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&m,&attr);
    }
    ~Mutex(){
        pthread_mutex_destroy(&m);
    }
};
    
    
// the wrapper has a wrapper type..
static WrapperType<pthread_mutex_t> tMutex("MUTX");

%type mutex tMutex pthread_mutex_t
%type thread tThread Thread


// words

%wordargs create vc (arg func --) start a new thread
{
    Value v,p;
    p.copy(p0);
    v.copy(p1);
    tThread.set(a->pushval(),a->ang,&v,&p);
}

%word glock (--) global lock
{
    hook.globalLock();
}

%word unglock (--) global unlock
{
    hook.globalUnlock();
}

%wordargs join l (threadlist --) wait for threads to complete
{
    ArrayListIterator<Value> iter(p0);
    
    // check types first
    for(iter.first();!iter.isDone();iter.next()){
        Value *p = iter.current();
        if(p->t != &tThread)
            throw RUNT(EX_TYPE,"expected threads only in thread list");
    }
    
    // then join each in turn.
    for(iter.first();!iter.isDone();iter.next()){
        Value *p = iter.current();
        pthread_join(tThread.get(p)->thread,NULL);
    }
}

%word mutex (-- mutex) create a recursive mutex
{
    pthread_mutex_t *v = new pthread_mutex_t();
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(v,&attr);
    
    tMutex.set(a->pushval(),v);
}

%wordargs lock A|mutex (mutex -- ) lock a mutex
{
    pthread_mutex_lock(p0);
}
%wordargs unlock A|mutex (mutex -- ) unlock a mutex
{
    pthread_mutex_unlock(p0);
}

%wordargs retval A|thread (thread --) get return of a finished thread
{
    hook.globalLock();
    if(p0->isRunning()){
        hook.globalUnlock();
        throw RUNT("ex$threadrunning","thread is still running");
    }
    
    a->pushval()->copy(&p0->retval);
    hook.globalUnlock();
}


%init
{
    fprintf(stderr,"Initialising THREAD plugin, %s %s\n",__DATE__,__TIME__);
    
    fprintf(stderr,
            "Todo: \n"
            "-make sure threads die correctly on end of function,\n"
            "-signal handling in both Angort and here\n"
            "-thread kill and join (list of threads)\n"
            "-thread data stored in Runtime so we can get thread\n"
            "-finer grained lock object\n"
            );
    Angort::setThreadHookObject(&hook);
}    

