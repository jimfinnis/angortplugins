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
        errorcode = regcomp(r,s,REG_EXTENDED); // no flags
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
    char *s = strdup(p0); // copy to avoid pushval overwrite
    tR.set(a->pushval(),s);
    free(s);
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

%wordargs match sA|regex (str regex -- matchlist) match
{
    regmatch_t match;
    Value v;
    ArrayList<Value> *list = Types::tList->set(&v);
    int off=0;
    int flags = 0;
    for(;;){
        int rv = regexec(p1->r,p0+off,1,&match,flags);
        if(rv == REG_NOMATCH)
            break;
        flags=REG_NOTBOL;
        Value *v = list->append();
        ArrayList<Value> *pair = Types::tList->set(v);
        Types::tInteger->set(pair->append(),match.rm_so+off);
        Types::tInteger->set(pair->append(),match.rm_eo+off);
        off += match.rm_eo;
    }
    a->pushval()->copy(&v);
}

%wordargs repl ssA|regex (subject repl regex -- result)
{
    regmatch_t match;
    int subjlen = strlen(p0);
    int repllen = strlen(p1);
    
    // first pass to calculate length
    bool hadMatches;
    int newlen = subjlen;
    int off=0;
    int flags = 0;
    for(;;){
        int rv = regexec(p2->r,p0+off,1,&match,flags);
        if(rv==REG_NOMATCH)
            break;
        flags=REG_NOTBOL;
        hadMatches = true;
        int start = match.rm_so;
        int end = match.rm_eo;
        if(start<0)break;
        int len = end-start;
        newlen+=(repllen-len);
        off += end;
    }
    
    // then do the replacements
    Value v;
    char *s = Types::tString->allocate(&v,newlen+1,Types::tString);
    
    int inoffset=0;
    int outoffset=0;
    flags = 0;
    if(hadMatches){ // skip if no matches in first pass
        for(;;){
            int rv = regexec(p2->r,p0+inoffset,1,&match,flags);
            if(rv==REG_NOMATCH)
                break;
            flags=REG_NOTBOL;
            int start = match.rm_so;
            int end = match.rm_eo;
            int len = end-start;
            
            // copy from current position in original string to
            // new string, the length of the next unreplaced section.
            memcpy(s+outoffset,p0+inoffset,start);
            
            // increment the offset into the output
            outoffset+=start;
            
            // copy in the replacement
            memcpy(s+outoffset,p1,repllen);
            
            // increment the offset into the original string, and
            // the offset into the output string
            inoffset+=start+len;
            outoffset+=repllen;
        }
    }
    // copy the remainder of the output string
    strcpy(s+outoffset,p0+inoffset);
    a->pushval()->copy(&v);
}

%wordargs split sA|regex (string regex -- list)
{
    regmatch_t match;
    Value v,*v2;
    char *s;
    ArrayList<Value> *list = Types::tList->set(&v);
    int off=0;
    int flags = 0;
    for(;;){
        int rv = regexec(p1->r,p0+off,1,&match,flags);
        if(rv == REG_NOMATCH)
            break;
        flags=REG_NOTBOL;
        v2 = list->append();
        s = Types::tString->allocate(v2,match.rm_so+1,Types::tString);
        memcpy(s,p0+off,match.rm_so);
        s[match.rm_so]=0;
        off += match.rm_eo;
    }
    v2 = list->append();
    Types::tString->set(v2,p0+off);
    
    a->pushval()->copy(&v);
}

%init
{
    fprintf(stderr,"Initialising REGEX plugin, %s %s\n",__DATE__,__TIME__);
}
