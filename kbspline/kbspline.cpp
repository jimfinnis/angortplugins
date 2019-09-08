/**
 * @file kbspline.cpp
 * @brief  Kochanek Bartels splines
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

#include "kbspline.hpp"

using namespace angort;

%name kbspline
%shared

class SplineWrapper : public GarbageCollected {
public:
    KBSpline v;
    SplineWrapper(double tension, double bias, double continuity) :
        v(tension,bias,continuity){}
    virtual ~SplineWrapper(){}
};


class SplineType : public GCType {
public:
    SplineType(){
        add("spline","CRSP");
    }
    
    virtual ~SplineType(){}
    
    KBSpline *get(const Value *v) const{
        if(v->t != this)
            throw RUNT(EX_TYPE,"not a spline");
        SplineWrapper *w = (SplineWrapper *)(v->v.gc);
        return &w->v;
    }
    
    KBSpline *set(Value *v,double t,double b,double c){
        v->clr();
        v->t = this;
        SplineWrapper *w = new SplineWrapper(t,b,c);
        v->v.gc = w;
        incRef(v);
        return &w->v;
    }
};


static SplineType tSpline;

%type spline tSpline KBSpline

%wordargs spline ddd (tension bias continuity -- spline) construct spline
{
    tSpline.set(a->pushval(),p0,p1,p2);
}

%wordargs add dA|spline (x spline --) add data to spline
{
    p1->add(p0);
}

%wordargs eval dA|spline (x spline -- y) evaluate spline
{
    a->pushDouble(p1->eval(p0));
}

%init
{
    if(showinit)
        fprintf(stderr,"Initialising KBSPLINE plugin, %s %s\n",__DATE__,__TIME__);
}    
