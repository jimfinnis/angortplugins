/**
 * @file regex.cpp
 * @brief  Brief description of file.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <regex.h>

#include <angort/angort.h>
#include <angort/hash.h>

using namespace angort;

%name regex
%shared

class Regex : public GarbageCollected {
public:
    regex_t *r;
    int errorcode; //zero or error code
    Regex(const char *s){
        r = new regex_t;
        errorcode = regcomp(r,s,0); // no flags
    }
    virtual ~Regex(){
        regfree(r);
    }
};

class RegexType : public GCType {
public:
    RegexType(){
        add("regex","REGX");
    }
    virtual ~RegexType(){};
    
    Regex *get(const Value *v) const {
        if(v->t != this)
            throw RUNT("not a regex");
        return (Regex *)(v->v.gc);
    }
    
    void set(Value *v,const char *s){
        v->clr();
        v->t=this;
        v->v.gc = new Regex(s);
        incRef(v);
    }
};

static RegexType tR;

%type regex tR Regex

%wordargs compile s (string -- compiled regex) compile a regex
{
    Value v;
    v.copy(a->popval());  // copy to avoid pushval() overwrite
    tR.set(a->pushval(),p0);
}

%wordargs chk A|regex (regex -- none or string) error check
{
    Value *v = a->pushval();
    if(p0->errorcode){
        int size = regerror(p0->errorcode,p0->r,NULL,0)+1;
        char *s = Types::tString->allocate(v,size,Types::tString);
        regerror(p0->errorcode,p0->r,s,1024);
    } else {
        a->pushNone();
    }
}

%wordargs match sA|regex (str regex -- match list or none) match
{
    regmatch_t matches[128];
    int rv = regexec(p1->r,p0,128,matches,0); // no flags
    if(rv == REG_NOMATCH){
        a->pushNone();
    } else {
        ArrayList<Value> *list = Types::tList->set(a->pushval());
        for(int i=0;i<128;i++){
            if(matches[i].rm_so<0)break;
            Value *v = list->append();
            ArrayList<Value> *pair = Types::tList->set(v);
            Types::tInteger->set(pair->append(),matches[i].rm_so);
            Types::tInteger->set(pair->append(),matches[i].rm_eo);
        }
    }
}

%init
{
    fprintf(stderr,"Initialising REGEX plugin, %s %s\n",__DATE__,__TIME__);
}
