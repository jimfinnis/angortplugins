/**
 * @file thread.cpp
 * @brief  Brief description of file.
 *
 */

#include <angort/angort.h>
#include <pthread.h>

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
    pthread_t thread;
    Value func;
    Runtime *runtime;
    bool running;
public:
    Thread(Angort *ang,Value *v) : GarbageCollected(){
        func.copy(v);
        runtime = new Runtime(ang,"<thread>");
        void *_threadfunc(void *);
        pthread_create(&thread,NULL,_threadfunc,this);
    }
    ~Thread(){
        // should decref func anyway.
    }
    void run(){
        running = true;
        runtime->runValue(&func);
        running = false;
        delete runtime;
    }
};

void *_threadfunc(void *p){
    Thread *t = (Thread *)p;
    t->run();
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
    void set(Value *v,Angort *ang,Value *func){
        if(!func->t->isCallable())
            throw RUNT(EX_TYPE,"not a callable");
        v->clr();
        v->t=this;
        v->v.gc = new Thread(ang,func);
        incRef(v);
    }
};

static ThreadType tThread;



%type thread tThread Thread


// words

%wordargs create c (func --) start a new thread
{
    Value v;
    v.copy(p0);
    tThread.set(a->pushval(),a->ang,&v);
}

%word lock (--) global lock
{
    hook.globalLock();
}

%word unlock (--) global unlock
{
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
    a->ang->setThreadHookObject(&hook);
}    

