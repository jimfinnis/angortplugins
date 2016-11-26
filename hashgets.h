/**
 * @file hashgets.h
 * @brief Utilities for reading values of different types from hashes
 *
 */

#ifndef __HASHGETS_H
#define __HASHGETS_H

#include <angort/hash.h>

using namespace angort;

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

inline Value *hgetsymnocheck(Hash *h,const char *s){
    Value *v = h->getSym(s);
    if(!v){
        static Value vnone;
        v = &vnone;
    }
    return v;
}



#endif /* __HASHGETS_H */
