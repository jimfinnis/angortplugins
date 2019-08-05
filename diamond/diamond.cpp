/**
 * @file diamond.cpp
 * @brief Interface with the Diamond Apparatus system
 * Will autoconnect on startup.
 */

#include "angort/angort.h"
#include "diamondapparatus/diamondapparatus.h"

static diamondapparatus::Topic topic;
#define THROWDIAMOND(e) throw RUNT("ex$diamond",e.what())
using namespace angort;

%name diamond
%shared

%wordargs subscribe s (name --) subscribe to a topic
Sends a message to subscribe to a topic - subsequent
get calls will return data on that topic.
{
    try {
        diamondapparatus::subscribe(p0);
    } catch(diamondapparatus::DiamondException d){
        THROWDIAMOND(d);
    }
}

%wordargs publish ls (list name --) publish list to a topic
Sends a message containing topic data. This should be a list of
floats or strings. Subscribers will receive a message with the
new data.
{
    using namespace diamondapparatus;
    
    try{
        ArrayListIterator<Value> iter(p0);
        Topic t;
        for(iter.first();!iter.isDone();iter.next()){
            Value *v = iter.current();
            if(v->t == Types::tFloat || v->t == Types::tInteger ||
               v->t == Types::tDouble || v->t == Types::tLong){
                t.add(Datum(v->toFloat()));
            } else {
                t.add(Datum(v->toString().get()));
            }
        }
        publish(p1,t);
    } catch(diamondapparatus::DiamondException d){
        THROWDIAMOND(d);
    }
}

inline void doGet(Runtime *a,const char *name, int flags){
    using namespace diamondapparatus;
    
    try{
        topic = get(name,flags);
        if(topic.isValid()){
            ArrayList<Value> *list = Types::tList->set(a->pushval());
            for(int i=0;i<topic.size();i++){
                switch(topic[i].t){
                case DT_FLOAT:
                    Types::tFloat->set(list->append(),topic[i].f());
                    break;
                case DT_STRING:
                    Types::tString->set(list->append(),topic[i].s());
                    break;
                default:
                    throw "unsupported type";
                }
            }
        } else {
            a->pushNone();
        }
    } catch(diamondapparatus::DiamondException d){
        THROWDIAMOND(d);
    }
    
}

%word changed (-- bool) get last topic state
Valid after a get..(), returns whether the topic has changed;  also false if no data.
{
    a->pushInt(topic.state == TOPIC_CHANGED ? 1:0);
}


%wordargs getwaitany s (name -- list) get a topic (waiting for any data)
Get data on a topic, waiting only if there is no data yet. Existing
data values will be returned if no message has been received since the
last get.
{
    doGet(a,p0,GET_WAITANY);
}

%wordargs getwaitnew s (name -- list) get a topic (waiting for new data)
Get data on a topic, waiting for new data to arrive.
{
    doGet(a,p0,GET_WAITNEW);
}

%wordargs getnowait s (name -- list/none) get a topic (no wait)
Get data on a topic, even if there is no data yet (will return
                                                   none in this case).
{
    doGet(a,p0,0);
}

%word waitany (--)
Wait for new data to be received on any topic to which I am subscribed.
{
    try {
        diamondapparatus::waitForAny();
    } catch(diamondapparatus::DiamondException d){
        THROWDIAMOND(d);
    }
}



%init
{
    try {
        if(showinit)
            fprintf(stderr,"Initialising DIAMOND plugin, %s %s\n",__DATE__,__TIME__);
        diamondapparatus::init();
    } catch(diamondapparatus::DiamondException d){
        THROWDIAMOND(d);
    }
}


%shutdown
{
    try {
        diamondapparatus::destroy();
    } catch(diamondapparatus::DiamondException d){
        THROWDIAMOND(d);
    }
}
