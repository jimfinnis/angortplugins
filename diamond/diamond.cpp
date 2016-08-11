/**
 * @file diamond.cpp
 * @brief Interface with the Diamond Apparatus system
 * Will autoconnect on startup.
 */

#include "angort/angort.h"
#include "diamondapparatus/diamondapparatus.h"

using namespace angort;

%name diamond
%shared

%wordargs subscribe s (name --) subscribe to a topic
{
    diamondapparatus::subscribe(p0);
}

%wordargs publish ls (list name --) publish list to a topic
{
    using namespace diamondapparatus;
    
    ArrayListIterator<Value> iter(p0);
    Topic t;
    for(iter.first();!iter.isDone();iter.next()){
        Value *v = iter.current();
        if(v->t == Types::tFloat || v->t == Types::tInteger){
            t.add(Datum(v->toFloat()));
        } else {
            t.add(Datum(v->toString().get()));
        }
    }
    publish(p1,t);
}

inline void doGet(Angort *a,const char *name, int flags){
    using namespace diamondapparatus;
    
    Topic t = get(name,flags);
    
    if(t.isValid()){
        ArrayList<Value> *list = Types::tList->set(a->pushval());
        for(int i=0;i<t.size();i++){
            switch(t[i].t){
            case DT_FLOAT:
                Types::tFloat->set(list->append(),t[i].f());
                break;
            case DT_STRING:
                Types::tString->set(list->append(),t[i].s());
                break;
            default:
                throw "unsupported type";
            }
        }
    } else {
        a->pushNone();
    }
}

%wordargs getwaitany s (name -- list) get a topic (waiting for any data)
{
    doGet(a,p0,GET_WAITANY);
}

%wordargs getwaitnew s (name -- list) get a topic (waiting for new data)
{
    doGet(a,p0,GET_WAITNEW);
}

%wordargs getnowait s (name -- list/none) get a topic (no wait)
{
    doGet(a,p0,0);
}

%word waitany (--)
{
    diamondapparatus::waitForAny();
}



%init
{
    fprintf(stderr,"Initialising DIAMOND plugin, %s %s\n",__DATE__,__TIME__);
    diamondapparatus::init();
}


%shutdown
{
    diamondapparatus::destroy();
}
