/**
 * @file osc.cpp
 * @brief  Brief description of file.
 *
 */

#include "angort/angort.h"
#include "../wrappers.h"
#include <lo/lo.h>
#define THROWOSC(e) throw RUNT("ex$diamond",e.what())

using namespace angort;

static BasicWrapperType<lo_address> tLoAddr("LOAD");

%type LoAddr tLoAddr lo_address

%name osc
%shared



%wordargs makeport s (s ---) make a new OSC port on localhost, given the port name
{
    lo_address lo = lo_address_new(NULL,p0);
    if(!lo)
        a->pushNone();
    else
        tLoAddr.set(a->pushval(),lo);
}

%wordargs send lsA|LoAddr (floatlist path port -- ret) send to path from a port, returns none on fail.
{
    lo_message msg = lo_message_new();
    ArrayListIterator<Value> iter(p0);
    int i=0;
    for(iter.first();!iter.isDone();iter.next(),i++){
        lo_message_add_float(msg,iter.current()->toFloat());
    }
    int ret = lo_send_message(*p2,p1,msg);
    if(ret<0)
        a->pushNone();
    else
        a->pushInt(0);
    lo_message_free(msg);
}   
        
    

%init
{
    fprintf(stderr,"Initialising OSC plugin (send only), %s %s\n",__DATE__,__TIME__);
}


%shutdown
{
}
