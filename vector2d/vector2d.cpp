/**
 * @file vector2d.cpp
 * @brief  Brief description of file.
 *
 */

#include <angort/angort.h>
#include <math.h>
using namespace angort;

%name vector2d
%shared

class Vector2D : public GarbageCollected {
public:
    double x,y;
    
    Vector2D(double _x,double _y){
        x=_x;
        y=_y;
    }
    
    Vector2D(){
        x=0;
        y=0;
    }
    
    double magsq(){
        return x*x+y*y;
    }
    
    double mag(){
        return sqrt(magsq());
    }
        
    
    Vector2D& operator+=(const Vector2D& a) {
        x+=a.x;
        y+=a.y;
        return *this;
    }
    Vector2D& operator-=(const Vector2D& a) {
        x-=a.x;
        y-=a.y;
        return *this;
    }
    Vector2D& operator*=(double s){
        x*=s;
        y*=s;
        return *this;
    }
    
    Vector2D operator-(){
        return Vector2D(-x,-y);
    }
    
    Vector2D operator+(const Vector2D& a) const {
        return Vector2D(*this)+=a;
    }
    Vector2D operator-(const Vector2D& a) const {
        return Vector2D(*this)-=a;
    }
    Vector2D operator*(double s) const {
        return Vector2D(*this)*=s;
    }
    
    double dot(const Vector2D & a){
        return x*a.x + y*a.y;
    }
    
    /// angle between this vector and another, taking account of
    /// the side we're on
    double angleBetween(const Vector2D &a){
        double a1 = atan2(x,y);
        double a2 = atan2(a.x,a.y);
        return a1-a2;
    }
    
    double angle(){
        return atan2(x,y);
    }
    
};

class Matrix3x3 : public GarbageCollected {
public:
    double m00,m01,m02;
    double m10,m11,m12;
    double m20,m21,m22;
    
    Matrix3x3(){
        reset();
    }
    
    Matrix3x3(double a, double b, double c,
              double d, double e, double f,
              double g, double h, double i){
        m00=a;m01=b;m02=c;
        m10=d;m11=e;m12=f;
        m20=g;m21=h;m22=i;
    }
    
    void set(double a, double b, double c,
             double d, double e, double f,
             double g, double h, double i){
        m00=a;m01=b;m02=c;
        m10=d;m11=e;m12=f;
        m20=g;m21=h;m22=i;
    }
    
    void reset(){
        m00=1;m01=0;m02=0;
        m10=0;m11=1;m12=0;
        m20=0;m21=0;m22=1;
    }
    
    void makeTranslate(double x,double y){
        m00=1;m01=0;m02=0;
        m10=0;m11=1;m12=0;
        m20=x;m21=y;m22=1;
    }
    
    void makeScale(double x, double y){
        m00=x;m01=0;m02=0;
        m10=0;m11=y;m12=0;
        m20=0;m21=0;m22=1;
    }
    
    void makeRotate(double a){
        double c = cos(a);
        double s = sin(a);
        
        m00=c;m01=-s;m02=0;
        m10=s;m11=c;m12=0;
        m20=0;m21=0;m22=1;
    }
    
    Vector2D transform(Vector2D v){
        double x = v.x*m00 + v.y*m10 + m20;
        double y = v.x*m01 + v.y*m11 + m21;
        return Vector2D(x,y);
    }
    
    
    /// set this matrix to be the product of two others; generally
    /// used in operator calls from within this class. Copies the
    /// inputs, so a bit slow, but safe.
    void mul(const Matrix3x3 a, const Matrix3x3 b){
        m00 = a.m00*b.m00 + a.m01*b.m10 + a.m02*b.m20;
        m01 = a.m00*b.m01 + a.m01*b.m11 + a.m02*b.m21;
        m02 = a.m00*b.m02 + a.m01*b.m12 + a.m02*b.m22;
        
        m10 = a.m10*b.m00 + a.m11*b.m10 + a.m12*b.m20;
        m11 = a.m10*b.m01 + a.m11*b.m11 + a.m12*b.m21;
        m12 = a.m10*b.m02 + a.m11*b.m12 + a.m12*b.m22;
        
        m20 = a.m20*b.m00 + a.m21*b.m10 + a.m22*b.m20;
        m21 = a.m20*b.m01 + a.m21*b.m11 + a.m22*b.m21;
        m22 = a.m20*b.m02 + a.m21*b.m12 + a.m22*b.m22;
    }
    
    Matrix3x3 operator*(const Matrix3x3 &a) const {
        Matrix3x3 r;
        r.mul(*this,a);
        return r;
    }
    
    Matrix3x3& operator*=(const Matrix3x3 &a) {
        mul(*this,a);
        return *this;
    }
    
    void scale(double x, double y){
        Matrix3x3 s;
        s.makeScale(x,y);
        mul(*this,s);
    }
    void translate(double x, double y){
        Matrix3x3 s;
        s.makeTranslate(x,y);
        mul(*this,s);
    }
    void rotate(double a){
        Matrix3x3 s;
        s.makeRotate(a);
        mul(*this,s);
    }
    
    double det(){
        return m00*m11*m22 + 
              m01*m12*m20+
              m02*m10*m21-
              m02*m11*m20-
              m01*m10*m22-
              m00*m12*m21;
    }
    
    Matrix3x3 transpose(){
        return Matrix3x3(m00,m10,m20,
                         m01,m11,m21,
                         m02,m12,m22);
    }
    
    Matrix3x3 inverse(){
        double d = det();
        if(d!=0.0){
            double r = 1.0/d;
            
            double i00 = m11*m22-m21*m12;
            double i01 = m21*m02-m01*m22;
            double i02 = m01*m12-m11*m02;
            double i10 = m20*m12-m10*m22;
            double i11 = m00*m22-m20*m02;
            double i12 = m10*m02-m00*m12;
            double i20 = m10*m21-m20*m11;
            double i21 = m20*m01-m00*m21;
            double i22 = m00*m11-m10*m01;
            
            // create inverse
            return Matrix3x3(i00*r,i01*r,i02*r,
                             i10*r,i11*r,i12*r,
                             i20*r,i21*r,i22*r);
            
        }
    }   
          
    
    void dump(){
        printf("[[ %3.3g, %3.3g %3.3g]\n", m00,m01,m02);
        printf(" [ %3.3g, %3.3g %3.3g]\n", m10,m11,m12);
        printf(" [ %3.3g, %3.3g %3.3g]]\n",m20,m21,m22);
    }
    
};


class Vector2DType : public GCType {
public:
    Vector2DType(){
        add("vector2D","VC2D");
    }
    
    Vector2D *get(const Value *v) const {
        if(v->t!=this)
            throw RUNT("not a Vector2D");
        return (Vector2D*)v->v.gc;
    }
    
    void set(Value *v,Vector2D a){
        v->clr();
        v->t=this;
        v->v.gc = new Vector2D(a.x,a.y);
        incRef(v);
    }
    
    void set(Value *v,double x,double y){
        v->clr();
        v->t=this;
        v->v.gc = new Vector2D(x,y);
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
    Matrix3x3Type(){
        add("Matrix3x3","MT33");
    }
    
    Matrix3x3 *get(Value *v){
        if(v->t!=this)
            throw RUNT("not a Matrix3x3");
        return (Matrix3x3*)v->v.gc;
    }
    
    void set(Value *v){
        v->clr();
        v->t=this;
        v->v.gc = new Matrix3x3();
        incRef(v);
    }
};


static Matrix3x3Type tM33;
static Vector2DType tV2;

%word vec (x y -- v)
{
    Value *p[2];
    a->popParams(p,"nn");
    
    tV2.set(a->pushval(),p[0]->toFloat(),p[1]->toFloat());
}

%word add (v v -- v)
{
    Value *p[2];
    a->popParams(p,"AA",&tV2);
    Vector2D *m = tV2.get(p[0]);
    Vector2D *n = tV2.get(p[1]);
    tV2.set(a->pushval(),*m + *n);
}

%word dot (v v -- v)
{
    Value *p[2];
    a->popParams(p,"AA",&tV2);
    Vector2D *m = tV2.get(p[0]);
    Vector2D *n = tV2.get(p[1]);
    Types::tFloat->set(a->pushval(),m->dot(*n));
}

%word mag (v -- n)
{
    Value *p;
    a->popParams(&p,"A",&tV2);
    Vector2D *v = tV2.get(p);
    Types::tFloat->set(a->pushval(),v->mag());
}
    


%word identity (-- m)
{
    tM33.set(a->pushval());
}

