/**
 * @file array.cpp
 * @brief  Multidimensional arrays
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

#include <angort/angort.h>

using namespace angort;

enum CoordMode {
    COORD_ERROR,
    COORD_CLIP,
    COORD_WRAP
};


struct Array : public GarbageCollected {
    int ndims; // how many dimensions has the array?
    int *dims; // size of each dimension
    int size; // number of elements
    Value *data; // the actual values
    Value none;
    CoordMode cm; // how to deal with out-of-range
    
    Array(int nd,int *d){
        ndims = nd;
        cm = COORD_ERROR;
        size = 1;
        dims = new int[nd];
        for(int i=0;i<nd;i++){
            dims[i]=d[i];
            size *= dims[i];
        }
        data = new Value[size];
    }
    
    virtual void wipeContents(){
        for(int i=0;i<size;i++){
            data[i].wipeIfInGCCycle();
        }
    }
    
    ~Array(){
        delete [] dims;
        delete [] data;
    }
    
    Value *get(int *idxs){
        int idx=0;
        int mul=1;
        for(int i=0;i<ndims;i++){
            int d=dims[i];
            int j=idxs[i];
            if(j<0 || j>=d){
                switch(cm){
                case COORD_ERROR:
                    return NULL;
                case COORD_WRAP:
                    j=((j%d)+d)%d;
                    break;
                case COORD_CLIP:
                    if(j<0)j=0;
                    else if(j>=d)j=d-1;
                    break;
                }
            }
            idx += j*mul;
            mul*=dims[i];
        }
//        printf("Index is %d\n",idx);
        if(idx>=size)
            return NULL;
        else
            return data+idx;
    }
};

class ArrayType : public GCType {
public:
    ArrayType(){
        add("array","ARRM");
    }
    
    Array *get(Value *v){
        if(!v)
            throw RUNT(EX_TYPE,"").set("Expected %s, not a null object",name);
        if(v->t!=this)
            throw RUNT(EX_TYPE,"").set("Expected %s, not %s",name,v->t->name);
        return (Array *)(v->v.gc);
    }
    
    void set(Value *v,Array *a){
        v->clr();
        v->t=this;
        v->v.gc = a;
        incRef(v);
    }
    virtual void clone(Value *out,const Value *in,bool deep=false)const;
};

static ArrayType tArray;

void ArrayType::clone(Value *out,const Value *in,bool deep)const{
    const Array *old = tArray.get((Value *)in);
    Array *na = new Array(old->ndims,old->dims);
    for(int i=0;i<na->size;i++){
        Value *v = old->data+i;
        Value *nv = na->data+i;
        if(deep)
            v->t->clone(nv,v,true);
        else
            nv->copy(v);
    }
    tArray.set(out,na);
}


%type array tArray Array

%name array
%shared


%wordargs make l (list -- array) make a new array given the dimensions as a list
{
    int n = p0->count();
    if(n>=100)
        throw RUNT(EX_OUTOFRANGE,"array has too many dimensions");
    int dims[100];
    
    // we build this in reverse, because the dimensions are popped off in reverse
    // in set and get.
    for(int i=0;i<n;i++){
        dims[(n-1)-i] = p0->get(i)->toInt();
    }
    Array *arr = new Array(n,dims);
    tArray.set(a->pushval(),arr);
}

%wordargs setclip A|array (array --) make this array clip out of range coords
{
    p0->cm = COORD_CLIP;
}

%wordargs setwrap A|array (array --) make this array wrap out of range coords toroidally
{
    p0->cm = COORD_WRAP;
}

%wordargs get A|array (idx...idx array -- val) get data from array
{
    // get extra args
    int idxs[100];
    for(int i=0;i<p0->ndims;i++)
        idxs[i]=a->popInt();
    
    Value *ptr = p0->get(idxs);
    if(!ptr)
        throw RUNT(EX_OUTOFRANGE,"array index out of range");
    a->pushval()->copy(ptr);
}

%wordargs set A|array (val idx...idx array --) get data from array
{
    // get extra args
    int idxs[100];
    for(int i=0;i<p0->ndims;i++)
        idxs[i]=a->popInt();
    // get ptr and write data
    Value *ptr = p0->get(idxs);
    if(!ptr){
        throw RUNT(EX_OUTOFRANGE,"array index out of range");
    }
    ptr->copy(a->popval());
}

%wordargs dims A|array (array -- list) get dimensions
{
    ArrayList<Value> *l = Types::tList->set(a->pushval());
    
    for(int i=p0->ndims-1;i>=0;i--){
        Types::tInteger->set(l->append(),p0->dims[i]);
    }
}

%wordargs clear vA|array (v array --) clear all members to v
{
    for(int i=0;i<p1->size;i++){
        p1->data[i].copy(p0);
    }
}

%word map (array func -- array) map all array values through a function
{
    Value func;
    func.copy(a->popval()); // need a local copy
    
    Value arrv;
    arrv.copy(a->popval()); // ditto
    
    Array *inarr = tArray.get(&arrv);
    
    Array *outarr = new Array(inarr->ndims,inarr->dims);
    for(int i=0;i<inarr->size;i++){
        a->pushval()->copy(inarr->data+i);
        a->runValue(&func);
        outarr->data[i].copy(a->popval());
    }
    
    tArray.set(a->pushval(),outarr);
}

%init
{
    fprintf(stderr,"Initialising ARRAY plugin, %s %s\n",__DATE__,__TIME__);
}    
