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
#include "../wrappers.h"

using namespace angort;


struct Array {
    int ndims; // how many dimensions has the array?
    int *dims; // size of each dimension
    int size; // number of elements
    Value *data; // the actual values
    Value none;
    
    Array(int nd,int *d){
        ndims = nd;
        size = 1;
        dims = new int[nd];
        for(int i=0;i<nd;i++){
            dims[i]=d[i];
            size *= dims[i];
        }
        data = new Value[size];
    }
    
    ~Array(){
        delete [] dims;
        delete [] data;
    }
    
    Value *get(int *idxs){
        int idx=0;
        int mul=1;
        for(int i=0;i<ndims;i++){
            if(idxs[i]<0 || idxs[i]>=dims[i])return NULL;
            idx += idxs[i]*mul;
            mul*=dims[i];
        }
//        printf("Index is %d\n",idx);
        if(idx>=size)
            return NULL;
        else
            return data+idx;
    }
};

static WrapperType<Array> tArray("ARRM");

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

%wordargs set A|array (val idx...idx array -- val) get data from array
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

