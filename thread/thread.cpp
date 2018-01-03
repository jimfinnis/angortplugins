/**
 * @file thread.cpp
 * @brief  Brief description of file.
 *
 */

#include <angort/angort.h>
#include <pthread.h>
#include <unistd.h>

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
    Runtime *runtime;
    bool running;
public:
    Value func;
    pthread_t thread;
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
        running = true;
        const StringBuffer& buf = func.toString();
        runtime->runValue(&func);
        running = false;
        delete runtime;
        runtime = NULL;
        // decrement refct, and delete this if it's zero. This is kind
        // of OK, here - it's the last thing that happens.
        if(decRefCt())delete this; 
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
        v->clr();
        v->t=this;
        v->v.gc = new Thread(ang,func,pass);
        incRef(v);
    }
};

static ThreadType tThread;



%type thread tThread Thread


// words

%wordargs create vc (arg func --) start a new thread
{
    Value v,p;
    p.copy(p0);
    v.copy(p1);
    tThread.set(a->pushval(),a->ang,&v,&p);
}

%word lock (--) global lock
{
    hook.globalLock();
}

%word unlock (--) global unlock
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

