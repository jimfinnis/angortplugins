/**
 * @file curl.cpp
 * @brief  Brief description of file.
 *
 */

#include <stdio.h>
#include <angort/angort.h>
#include <angort/hash.h>
#include <curl/curl.h>

using namespace angort;

struct CurlWrapper : GarbageCollected {
    char buf[CURL_ERROR_SIZE];
    CURL *c;
    char *data;
    size_t len,maxsize;
    
    CurlWrapper(){
        size_t writefunc(void *buffer, size_t size, size_t nmemb, void *userp);
        c = curl_easy_init();
        maxsize=1024;
        data=(char *)malloc(maxsize);
        *data=0;
        len = 0;
        curl_easy_setopt(c,CURLOPT_ERRORBUFFER,buf);
        curl_easy_setopt(c,CURLOPT_WRITEFUNCTION,writefunc);
        curl_easy_setopt(c,CURLOPT_WRITEDATA,this);
    }
    ~CurlWrapper(){
        curl_easy_cleanup(c);
        free(data);
    }
};

class CurlType : public GCType {
public:
    CurlType(){
        add("CURL","CURL");
    }
    
    CurlWrapper *get(Value *v){
        if(v->t != this)
            throw RUNT(EX_TYPE,"").set("Expected CURL, not %s",v->t->name);
        return (CurlWrapper*)(v->v.gc);
    }
    
    void set(Value *v){
        v->clr();
        v->t = this;
        v->v.gc = new CurlWrapper();
        incRef(v);
    }
};

size_t writefunc(void *buffer, size_t size, size_t nmemb, void *userp){
    CurlWrapper *c = (CurlWrapper *)userp;
    
    size_t s = size*nmemb;
    while(c->len+s+1>c->maxsize){
        c->maxsize+=1024;
        c->data=(char *)realloc(c->data,c->maxsize);
    }
    memcpy(c->data+c->len,buffer,s);
    c->len+=s;
    c->data[c->len]=0;
    return s;
}


static CurlType tCurl;
    

%name curl
%shared

%type curl tCurl CurlWrapper


inline float hgetfloatdef(Hash *h,const char *s,float def=0){
    Value *v = h->getSym(s);
    if(v)
        return v->toFloat();
    else
        return def;
}


inline int hgetintdef(Hash *h,const char *s,int def=0){
    Value *v = h->getSym(s);
    if(v)
        return v->toInt();
    else
        return def;
}
inline const char *hgetstrdef(Hash *h,const char *s,const char *def=NULL){
    Value *v = h->getSym(s);
    if(v)
        return v->toString().get();
    else
        return def;
}

inline float hgetfloat(Hash *h,const char *s){
    Value *v = h->getSym(s);
    if(!v)throw RUNT(EX_NOTFOUND,"").set("required key '%s' not found in hash",s);
    return v->toFloat();
}

inline int hgetint(Hash *h,const char *s){
    Value *v = h->getSym(s);
    if(!v)throw RUNT(EX_NOTFOUND,"").set("required key '%s' not found in hash",s);
    return v->toInt();
}
inline const char *hgetstr(Hash *h,const char *s){
    Value *v = h->getSym(s);
    if(!v)throw RUNT(EX_NOTFOUND,"").set("required key '%s' not found in hash",s);
    return v->toString().get();
}
inline Value *hgetsym(Hash *h,const char *s){
    Value *v = h->getSym(s);
    if(!v)throw RUNT(EX_NOTFOUND,"").set("required key '%s' not found in hash",s);
    return v;
}

%init
{
    fprintf(stderr,"Initialising CURL plugin, %s %s\n",__DATE__,__TIME__);
    curl_global_init(CURL_GLOBAL_ALL);
}

%shutdown
{
    curl_global_cleanup();
}

%word make (-- curl) make a curl object, need this to do anything. Then setopt and perform with it.
{
    tCurl.set(a->pushval());
}

%word setopt (curl hash -- curl) set opts in curl (supported: `url, `post)
{
    Hash *p1 = Types::tHash->get(a->popval());
    CurlWrapper *p0 = tCurl.get(a->stack.peekptr());
    CURL *c = p0->c;
    
    if(const char *s = hgetstrdef(p1,"url",NULL))
        curl_easy_setopt(c,CURLOPT_URL,s);
    if(const char *s = hgetstrdef(p1,"postfields",NULL))
        curl_easy_setopt(c,CURLOPT_COPYPOSTFIELDS,s);
    if(const char *s = hgetstrdef(p1,"agent",NULL))
        curl_easy_setopt(c,CURLOPT_USERAGENT,s);
    if(const char *s = hgetstrdef(p1,"referer",NULL))
        curl_easy_setopt(c,CURLOPT_REFERER,s);
}

%wordargs args h (hash -- string) turn a hash into an arg string
{
    int maxsize = 256;
    int curlen=0;
    char *buf = (char *)malloc(maxsize);
    *buf=0;
    
    Iterator<Value *> *iter = p0->createIterator(true);
    
    
    for(iter->first();!iter->isDone();iter->next()){
        Value *v = iter->current();
        const StringBuffer &b = v->toString();
        const char *s = b.get();
        int klen = strlen(s);
        if(p0->find(v)){
            const StringBuffer &bv = p0->getval()->toString();
            const char *sv = bv.get();
            int vlen = strlen(sv);
            while(klen+vlen+curlen+4>maxsize){
                maxsize+=256;
                buf = (char *)realloc(buf,maxsize);
            }
            sprintf(buf+curlen,"%s=%s&",s,sv);
            curlen+=klen+vlen+2;
        }
    }
    buf[strlen(buf)-1]=0; // remove last &
    a->pushString(buf);
    free(buf);
    
    delete iter;
}

%wordargs perform A|curl (curl -- error/none) perform the transfer, use curl$data to get the data
{
    CURLcode rv = curl_easy_perform(p0->c);
    if(rv == CURLE_OK)
        a->pushNone();
    else {
        a->pushString(p0->buf);
    }
}

%wordargs data A|curl (curl -- data) get downloaded data (as string)
{
    Value v;
    Types::tString->set(&v,p0->data);
    a->pushval()->copy(&v);
}
