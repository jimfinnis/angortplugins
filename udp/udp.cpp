/**
 * @file
 * Handles key/value pairs over UDP. Values are received with k/v
 * and also can be sent with k/v. 
 * 
 * UDP properties are special variables to do this.
 * These variables can be set by k=v messages coming in from outside,
 * and when set by this side are sent to the client.
 * 
 */

#include <angort/angort.h>
#include <time.h>
#include "udpclient.h"
#include "udpserver.h"

bool udpDebugging=false;

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

/// thing to call when change on UDPvar received
static Value onChange;

class UDPProperty : public Property {
public:
    /// linkage field
    UDPProperty *next;
    
    /// actual value; gets copied to and from
    /// field v for Angort access
    float val;
    /// has been written locally, needs to be sent
    bool changed;
    
    /// call this when change received from remote
    Value onChange;
    
    /// name of property
    const char *name;
    
    UDPProperty(const char *n){
        if(!strcmp(n,"time"))
            throw Exception("time cannot be a UDP var");
        val=0;
        changed=true;
        // add to list
        next=headUDPPropList;
        headUDPPropList=this;
        name = strdup(n);
    }
    
    ~UDPProperty(){
        free((void *)name);
    }
    
    static UDPProperty *getByName(const char *n){
        for(UDPProperty *p=headUDPPropList;p;p=p->next){
            if(!strcmp(p->name,n))
                return p;
        }
        return NULL;
    }
                   
    
    void set(float f){
        val = f;
        changed=true;
    }
    
    virtual void postSet(){
        val = v.toFloat();
        changed=true;
    }
    
    virtual void preGet(){
        Types::tFloat->set(&v,val);
    }
};

class MyUDPServerListener: public UDPServerListener {
    /// messages arrive as key=value pairs.
    virtual void onKeyValuePair(const char *s,float v){
        extern void setUDPProperty(const char *name,float val);
        if(udpDebugging)printf("RECEIVED %s=%f\n",s,v);
        setUDPProperty(s,v);
    }
};
MyUDPServerListener listener;

void setUDPProperty(const char *name,float val){
    UDPProperty *p = UDPProperty::getByName(name);
    if(p){
        p->val = val;
    } else if(strcmp(name,"time"))
        throw Exception().set("cannot find UDP property in set from remote %s",name);
}

%word poll (--) send and receive queued UDP data
{
    server.poll(); // check for incoming
    
    // send props
    for(UDPProperty *p=headUDPPropList;p;p=p->next){
        if(p->changed){
            udpwrite("%s=%f",p->name,p->val);
            p->changed=false;
        }
    }
}


%word addvar (name --) create a new global which mirrors the monitor
{
    const StringBuffer& b = a->popString();
    a->registerProperty(b.get(),new UDPProperty(b.get()));
}

%word write (string --) write a string to the UDP port
{
    const StringBuffer& b = a->popString();
    udpwrite(b.get());
}    

%wordargs start isi (sport chost cport --) start UDP server and client
{
    initClient(p1,p2);
    server.start(p0);
    server.setListener(&listener);
}

%wordargs onchange cs (callback(v k --) name --) set callback for change received on UDP var
{
    UDPProperty *p = UDPProperty::getByName(p1);
    if(p)
        p->onChange.copy(p0);
    else
        throw Exception().set("cannot find UDP property in onchange %s",p1);
}

%wordargs debug i (bool --) set debugging on messages
{
    udpDebugging = p0;
}

%init
{
    printf("UDP library initialised\n");
}
