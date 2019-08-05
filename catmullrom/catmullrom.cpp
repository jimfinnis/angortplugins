/**
 * @file catmullrom.cpp
 * @brief  Catmull-Rom splines
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

#include <iostream>

#include <angort/angort.h>

#include "catmullrom.hpp"

using namespace angort;

%name catmullrom
%shared

class SplineWrapper : public GarbageCollected {
public:
    CatmullRom::CRSpline v;
    virtual ~SplineWrapper(){}
};


class SplineType : public GCType {
public:
    SplineType(){
        add("spline","CRSP");
    }
    
    virtual ~SplineType(){}
    
    CatmullRom::CRSpline *get(const Value *v) const{
        if(v->t != this)
            throw RUNT(EX_TYPE,"not a spline");
        SplineWrapper *w = (SplineWrapper *)(v->v.gc);
        return &w->v;
    }
    
    CatmullRom::CRSpline *set(Value *v){
        v->clr();
        v->t = this;
        SplineWrapper *w = new SplineWrapper();
        v->v.gc = w;
        incRef(v);
        return &w->v;
    }
};


static SplineType tSpline;

%type spline tSpline CatmullRom::CRSpline

%word spline (-- spline) construct spline
{
    tSpline.set(a->pushval());
}

%wordargs add ddA|spline (x y spline --) add data to spline
{
    p2->add(p0,p1);
}

%wordargs eval dA|spline (x spline -- y) evaluate spline
{
    a->pushDouble(p1->eval(p0));
}

%init
{
    if(showinit)
        fprintf(stderr,"Initialising CATMULLROM plugin, %s %s\n",__DATE__,__TIME__);
}    
