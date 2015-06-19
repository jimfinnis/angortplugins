/**
 * @file
 * Handles the UDP communications
 * 
 */

#include <angort/angort.h>
#include <time.h>
#include "udpclient.h"
#include "udpserver.h"

double gettime(){
    timespec ts;
    clock_gettime(CLOCK_REALTIME,&ts);
    
    double t = ts.tv_nsec;
    t *= 1e-9;
    t += ts.tv_sec;
    return t;
}

void udpwrite(const char *s,...){
    va_list args;
    va_start(args,s);
    char buf[400];
    
    sprintf(buf,"time=%f ",gettime());
    vsnprintf(buf+strlen(buf),400-strlen(buf),s,args);
    udpSend(buf);
    va_end(args);
}


extern void handleUDP();
UDPServer server;

using namespace angort;

%name udp
%shared

/// head of a linked list of UDP properties
static class UDPProperty *headUDPPropList=NULL;

class UDPProperty : public Property {
public:
    /// linkage field
    UDPProperty *next;
    
    /// actual value; gets copied to and from
    /// field v for Angort access
    float val;
    
    /// name of property
    const char *name;
    
    UDPProperty(const char *n){
        val=0;
        // add to list
        next=headUDPPropList;
        headUDPPropList=this;
        name = strdup(n);
    }
    
    ~UDPProperty(){
        free((void *)name);
    }
                   
    
    void set(float f){
        val = f;
    }
    
    virtual void postSet(){
        val = v.toFloat();
    }
    
    virtual void preGet(){
        Types::tFloat->set(&v,val);
    }
};

class MyUDPServerListener: public UDPServerListener {
    /// messages arrive as key=value pairs.
    virtual void onKeyValuePair(const char *s,float v){
        extern void setUDPProperty(const char *name,float val);
        printf("RECEIVED %s=%f\n",s,v);
        setUDPProperty(s,v);
    }
};
MyUDPServerListener listener;

void setUDPProperty(const char *name,float val){
    for(UDPProperty *p=headUDPPropList;p;p=p->next){
        if(!strcmp(name,p->name)){
            p->val = val;
            return;
        }
    }
    throw Exception().set("cannot find UDP property %s",name);
}

%word poll (--) send and receive queued UDP data
{
    server.poll(); // check for incoming
    
    // send props
    for(UDPProperty *p=headUDPPropList;p;p=p->next){
        udpwrite("%s=%f",p->name,p->val);
    }
}


%word addvar (name --) create a new global which mirrors the monitor
{
    const StringBuffer& b = a->popString();
    a->registerProperty(b.get(),new UDPProperty(b.get()));
}

%word host (name --) set host to write to (default=localhost)
{
    const StringBuffer& b = a->popString();
    hostName = strdup(b.get());
}

%word write (string --) write a string to the UDP port for the monitor (13231)
{
    const StringBuffer& b = a->popString();
    udpwrite(b.get());
}    

%word start (port --) start UDP server on port
{
    server.start(a->popInt());
    server.setListener(&listener);
}

%init
{
    printf("UDP library initialised\n");
}
