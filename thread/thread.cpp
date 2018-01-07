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

class MsgBuffer {
    static const int size=8;
    Value msgs[size];
    int wr,rd;
    int length;
    pthread_mutex_t mutex,mutex2;
    pthread_cond_t cond,cond2;
    
    bool isEmpty(){
        return length==0;
    }
    
    bool isFull(){
        return length==size;
    }
    
    void _read(Value *v){
        if(isEmpty())printf("NO MESSAGE\n");
        v->copy(msgs+rd);
        rd++;
        rd %= size;
        length--;
    }
    
    bool _write(Value *v){
        if(isFull())return false;
        wr++;
        wr %= size;
        msgs[wr].copy(v);
        length++;
        return true;
    }
public:
    MsgBuffer(){
        wr=size-1;
        rd=0;
        length=0;
        pthread_mutex_init(&mutex,NULL);
        pthread_cond_init(&cond,NULL);
        pthread_mutex_init(&mutex2,NULL);
        pthread_cond_init(&cond2,NULL);
    }
    ~MsgBuffer(){
        pthread_mutex_destroy(&mutex);
        pthread_cond_destroy(&cond);
        pthread_mutex_destroy(&mutex2);
        pthread_cond_destroy(&cond2);
    }
    
    // wait for a message, blocking if we have to
    void waitAndRead(Value *dest){
        pthread_mutex_lock(&mutex);
        while(isEmpty())
            pthread_cond_wait(&cond,&mutex);
        _read(dest);
        pthread_cond_signal(&cond2);
//        printf("%d SIGNALLING.\n",snark);
        pthread_mutex_unlock(&mutex);
//        printf("%d read ok\n",snark);
    }
    
    // write a message, return false if no room
    bool writeNoWait(Value *v){
        bool ok;
        pthread_mutex_lock(&mutex);
        ok = _write(v);
        if(ok)pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
        return ok;
    }
    
    // write a message, blocking if we're full.
    void writeWait(Value *v){
//        printf("%d write \n",snark);
        pthread_mutex_lock(&mutex2);
        while(isFull()){
//            printf("%d FULL.\n",snark);
            pthread_cond_wait(&cond2,&mutex2);
        }
        pthread_mutex_lock(&mutex);
        _write(v);
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
//        printf("%d write ok\n",snark);
        pthread_mutex_unlock(&mutex2);
    }
    
    
    
};

void *_threadfunc(void *);

namespace angort {
class Thread : public GarbageCollected {
    // our run environment
    Runtime *runtime;
    
public:
    Value func;
    Value retval; // the returned value
    pthread_t thread;
    MsgBuffer msgs;
    int id;
    
    bool isRunning(){
        return runtime != NULL;
    }
    Thread(Angort *ang,Value *v,Value *arg) : GarbageCollected(){
        incRefCt(); // make sure we don't get deleted until complete
        func.copy(v);
        runtime = new Runtime(ang,"<thread>");
        id = runtime->id;
        runtime->thread = this;
        runtime->pushval()->copy(arg);
        //        printf("Creating thread at %p/%s\n",this,func.t->name);
        pthread_create(&thread,NULL,_threadfunc,this);
    }
    ~Thread(){
        //        printf("Thread destroyed at %p\n",this);
    }
    void run(){
        const StringBuffer& buf = func.toString();
        try {
            runtime->runValue(&func);
        } catch(Exception e){
            hook.globalLock();
            printf("Exception in thread %d\n",runtime->id);
            hook.globalUnlock();
        }
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
}

void *_threadfunc(void *p){
    Thread *t = (Thread *)p;
    //    printf("starting thread at %p/%s\n",t,t->func.t->name);
    t->run();
    //    printf("Thread func terminated OK?\n");
    return NULL;
}






%name thread
%shared

// crude hack - this is the message buffer for the default thread.
static MsgBuffer defaultMsgs;

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
    
    // set a value to a thread
    void set(Value *v, Thread *t){
        hook.globalLock();
        v->clr();
        v->t = this;
        v->v.gc = t;
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

%wordargs retval A|thread (thread -- val) get return of a finished thread
One way of communicating with a thread is to send data into it with
the function parameter, and receive the result (after the thread has
completed) using thread$retval. If the thread is still running an
exception will be thrown. Use thread$join to wait for thread completion.
{
    hook.globalLock();
    if(p0->isRunning()){
        hook.globalUnlock();
        throw RUNT("ex$threadrunning","thread is still running");
    }
    
    a->pushval()->copy(&p0->retval);
    hook.globalUnlock();
}

%wordargs sendnoblock vv|thread (msg thread|none -- bool) send a message value to a thread
Send a message to a thread. The default thread is indicated by "none".
Will return a boolean indicating the send was successful. 
{
    MsgBuffer *b;
    if(p1->isNone())
        b = &defaultMsgs;
    else {
        Thread *t = tThread.get(p1);
        b = &t->msgs;
    }
    a->pushInt(b->writeNoWait(p0)?1:0);
}

%wordargs send vv|thread (msg thread|none --) send a message value to a thread
Send a message to a thread. The default thread is indicated by "none".
Will wait if the buffer is full.
{
    MsgBuffer *b;
    if(p1->isNone())
        b = &defaultMsgs;
    else {
        Thread *t = tThread.get(p1);
        b = &t->msgs;
    }
    b->writeWait(p0);
}

%word waitrecv (--- msg) blocking message read
Wait for a message to arrive on this thread and return it.
{
    MsgBuffer *b;
    if(a->thread)
        b = &a->thread->msgs;
    else
        b = &defaultMsgs;
    Value v;
    b->waitAndRead(&v);
    a->pushval()->copy(&v);
}

%word cur (-- thread) return current thread
{
    tThread.set(a->pushval(),a->thread);
}

%wordargs id A|thread (thread -- id) get ID from thread
{
    a->pushInt(p0->id);
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

