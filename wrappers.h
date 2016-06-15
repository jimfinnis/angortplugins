/**
 * @file wrappers.h
 * @brief  Possibly useful class for building angort GC wrappers
 * around other classes.
 *
 */

#ifndef __WRAPPERS_H
#define __WRAPPERS_H

template <class T> struct Wrapper : GarbageCollected {
    T *base;
    Wrapper(T *p){
        base = p;
    }
    virtual ~Wrapper(){
        delete base;
    }
};

template <class T>
class WrapperType : public GCType {
public:
    WrapperType(const char *nameid){
        if(strlen(nameid)!=4)throw RUNT(EX_BADPARAM,"type wrapper name length must=4");
        add(nameid,nameid);
    }
    
    T *get(Value *v){
        if(!v)
            throw RUNT(EX_TYPE,"").set("Expected %s, not a null object",name);
        if(v->t!=this)
            throw RUNT(EX_TYPE,"").set("Expected %s, not %s",name,v->t->name);
        return ((Wrapper<T> *)(v->v.gc))->base;
    }
    
    void set(Value *v,T *f){
        v->clr();
        v->t=this;
        v->v.gc = new Wrapper<T>(f);
        incRef(v);
    }
};



#endif /* __WRAPPERS_H */
