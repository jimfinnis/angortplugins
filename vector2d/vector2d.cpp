/**
 * @file vector2d.cpp
 * @brief  Brief description of file.
 *
 */

#include <angort/angort.h>
#include <math.h>
#include "vector2d.h"

using namespace angort;

%name vector2d
%shared

class VectorWrapper : public GarbageCollected {
public:
    Vector2D v;
    virtual ~VectorWrapper(){}
    VectorWrapper(Vector2D _v){
        v = _v;
    }
};

class MatrixWrapper : public GarbageCollected {
public:
    Matrix3x3 m;
    virtual ~MatrixWrapper(){}
    MatrixWrapper(Matrix3x3 &_v){
        m = _v;
    }
};


class Vector2DType : public GCType {
public:
    Vector2DType(){
        add("vec2d","VC2D");
    }
    virtual ~Vector2DType(){};
    
    Vector2D *get(const Value *v) const {
        if(v->t!=this)
            throw RUNT(EX_TYPE,"not a Vector2D");
        VectorWrapper *w = (VectorWrapper *)(v->v.gc);
        return &w->v;
    }
    
    void set(Value *v,Vector2D a){
        v->clr();
        v->t=this;
        v->v.gc = new VectorWrapper(Vector2D(a.x,a.y));
        incRef(v);
    }
    
    void set(Value *v,double x,double y){
        v->clr();
        v->t=this;
        v->v.gc = new VectorWrapper(Vector2D(x,y));
        incRef(v);
    }
    virtual const char *toString(bool *allocated,const Value *v) const {
        char buf[128];
        Vector2D *w = get(v);
        snprintf(buf,128,"(%f,%f)",w->x,w->y);
        *allocated=true;
        return strdup(buf);
    }
};

class Matrix3x3Type : public GCType {
public:
    virtual ~Matrix3x3Type(){};
    Matrix3x3Type(){
        add("mat3x3","MT33");
    }
    
    Matrix3x3 *get(Value *v){
        if(v->t!=this)
            throw RUNT(EX_TYPE,"not a Matrix3x3");
        MatrixWrapper *w = (MatrixWrapper *)v->v.gc;
        return &w->m;
    }
    
    // will copy the given matrix into a new matrix
    void set(Value *v,Matrix3x3 *m){
        v->clr();
        v->t=this;
        v->v.gc = new MatrixWrapper(*m);
        incRef(v);
    }
};


static Matrix3x3Type tM33;
static Vector2DType tV2;

// use these types because the get() methods automatically unwrap

%type vec tV2 Vector2D
%type mat tM33 Matrix3x3

%word vec (x y -- v) construct vector
{
    Value *p[2];
    a->popParams(p,"nn");
    Vector2D v(p[0]->toFloat(),p[1]->toFloat());
    tV2.set(a->pushval(),v);
}

%word vneg (v -- v) vector negation
{
    Value *p;
    a->popParams(&p,"A",&tV2);
    Vector2D *v = tV2.get(p);
    tV2.set(a->pushval(),- *v);
}

%word dot (v v -- v) dot product
{
    Value *p[2];
    a->popParams(p,"AA",&tV2);
    Vector2D *m = tV2.get(p[0]);
    Vector2D *n = tV2.get(p[1]);
    Types::tFloat->set(a->pushval(),m->dot(*n));
}

%word mag (v -- n) get vector mag
{
    Value *p;
    a->popParams(&p,"A",&tV2);
    Vector2D *v = tV2.get(p);
    Types::tFloat->set(a->pushval(),v->mag());
}

%word xy (v -- x y) get xy coords onto stack
{
    Value *p;
    a->popParams(&p,"A",&tV2);
    Vector2D *v = tV2.get(p);
    Types::tFloat->set(a->pushval(),v->x);
    Types::tFloat->set(a->pushval(),v->y);
}

%word x (v -- x) get x onto stack
{
    Value *p;
    a->popParams(&p,"A",&tV2);
    Vector2D *v = tV2.get(p);
    Types::tFloat->set(a->pushval(),v->x);
}
%word y (v -- y) get y onto stack
{
    Value *p;
    a->popParams(&p,"A",&tV2);
    Vector2D *v = tV2.get(p);
    Types::tFloat->set(a->pushval(),v->y);
}

%wordargs angle A|vec (v -- n) get angle from Y axis
{
    a->pushFloat(p0->angle());
}    

%wordargs anglebetween AA|vec (v v -- angle) angle between two vectors, clockwise
{
    a->pushFloat(p0->angleBetween(*p1));
}

%word identity (-- m) identity matrix
{
    Matrix3x3 m;
    tM33.set(a->pushval(),&m);
}

%wordargs makebasis AA|vec (origin rot -- m) construct matrix to rotate to new coord system
{
    Matrix3x3 m(*p0,*p1);
    tM33.set(a->pushval(),&m);
}

%wordargs mktranslate A|vec (v -- m) construct translation matrix
{
    Matrix3x3 m;
    m.makeTranslate(p0->x,p0->y);
    tM33.set(a->pushval(),&m);
}

%wordargs mkscale nn (x y -- m) construct scaling matrix
{
    Matrix3x3 m;
    m.makeScale(p0,p1);
    tM33.set(a->pushval(),&m);
}    

%wordargs mkrotate n (angle -- m) construct rotation matrix
{
    Matrix3x3 m;
    m.makeRotate(p0);
    tM33.set(a->pushval(),&m);
}

%binop mat3x3 mul mat3x3
{
    Matrix3x3 *p = tM33.get(lhs);
    Matrix3x3 *q = tM33.get(rhs);
    Matrix3x3 m;
    
    m.mul(*p,*q);
    tM33.set(a->pushval(),&m);
}

%wordargs xform AB|vec,mat (vec mat -- vec) matrix transform
{
    tV2.set(a->pushval(),p1->transform(*p0));
}

%binop vec2d mul mat3x3
{
    Vector2D *v = tV2.get(lhs);
    Matrix3x3 *m = tM33.get(rhs);
    tV2.set(a->pushval(),m->transform(*v));
}

%wordargs translate AB|mat,vec (m v -- m) matrix translate
{
    Matrix3x3 m = *p0;
    m.translate(p1->x,p1->y);
    tM33.set(a->pushval(),&m);
}

%wordargs mget nnA|mat (r c m --) get element of matrix
{
    double *p = &(p2->m00);
    int idx = p1+3*p0;
    if(idx<0 || idx>8)
        a->pushNone();
    else
        a->pushFloat(p[idx]);
}

%wordargs mdump A|mat (m --) dump matrix to stdout
{
    p0->dump();
}

%wordargs zerotranslate A|mat (m -- m) zero the translate elements of a matrix
{
    Matrix3x3 m = *p0;
    m.m20=0;m.m21=0;m.m22=1;
    tM33.set(a->pushval(),&m);
}


%binop vec2d add vec2d
{
    Vector2D *m = tV2.get(lhs);
    Vector2D *n = tV2.get(rhs);
    tV2.set(a->pushval(),*m + *n);
}

%binop vec2d sub vec2d
{
    Vector2D *m = tV2.get(lhs);
    Vector2D *n = tV2.get(rhs);
    tV2.set(a->pushval(),*m - *n);
}

%binop vec2d mul number
{
    Vector2D *m = tV2.get(lhs);
    double n = rhs->toFloat();
    
    tV2.set(a->pushval(),*m * n);
}

%binop number mul vec2d
{
    Vector2D *m = tV2.get(rhs);
    double n = lhs->toFloat();
    
    tV2.set(a->pushval(),*m * n);
}

/// collide with a circle
inline bool collide(Vector2D &w1, Vector2D& w2,double xc,double yc,double r){
    float a = w2.x - w1.x;
    float b = w2.y - w1.y;
    float c = xc - w1.x;
    float d = yc - w1.y;
    if ((d*a - c*b)*(d*a - c*b) <= r*r*(a*a + b*b)) {
        // Collision is possible
        if (c*c + d*d <= r*r) {
            // Line segment start point is inside the circle
            return true;
            
        }
        else if ((a-c)*(a-c) + (b-d)*(b-d) <= r*r) {
            // Line segment end point is inside the circle
            return true;
        }
        else if (c*a + d*b >= 0 && c*a + d*b <= a*a + b*b) {
            // Middle section only
            return true;
        }
    }
    return false;
}


%wordargs circlesegcollide AAAn|vec (end1 end2 centre rad --) circle/line segment colliding?
{
    bool rv = collide(*p0,*p1,p2->x,p2->y,p3);
    a->pushInt(rv?1:0);
}


%init
{
    fprintf(stderr,"Initialising VECTOR2D plugin, %s %s\n",__DATE__,__TIME__);
}
