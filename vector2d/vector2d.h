/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#ifndef __VECTOR2D_H
#define __VECTOR2D_H

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#define PI 3.1415927
#define TWOPI (2.0*PI)
#define RADIANS_TO_DEGREES (180.0/PI)
#define DEGREES_TO_RADIANS (PI/180.0)

struct Vector2D {
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

    Vector2D normalize(){
        double m = 1.0/mag();
        return Vector2D(x*m,y*m);
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

    /// faster vector rotation than creating a matrix, this rotates in place
    void rotateInPlace(double r){
        double xx = x;
        double yy = y;
        x = xx*cos(r)-yy*sin(r);
        y = xx*sin(r)+yy*cos(r);
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
    
    void dump(){
        printf("[ %.3g, %.3g]\n",x,y);
    }
    
};

struct Matrix3x3 {
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
    
    /// construct a matrix will convert to a coordinate
    /// system with the given origin and the given Y axis.
    /// The resulting matrix will subtract the origin,
    /// then rotate the remaining vector.
    Matrix3x3(Vector2D origin, Vector2D yaxis){
        makeTranslate(-origin.x,-origin.y);
        rotate(-yaxis.angle());
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
        else
            return Matrix3x3(); // if not invertible, return identity
    }   
    
    void dump(){
        printf("[[ %3.3g, %3.3g %3.3g]\n", m00,m01,m02);
        printf(" [ %3.3g, %3.3g %3.3g]\n", m10,m11,m12);
        printf(" [ %3.3g, %3.3g %3.3g]]\n",m20,m21,m22);
    }
    
};

#endif /* __VECTOR2D_H */
