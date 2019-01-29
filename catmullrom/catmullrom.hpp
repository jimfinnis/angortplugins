/**
 * @file catmullrom.hpp
 * @brief  Brief description of file.
 *
 */

#ifndef __CATMULLROM_HPP
#define __CATMULLROM_HPP

#include <cmath>
#include <vector>


namespace CatmullRom {

struct CubicPoly
{
    double c0, c1, c2, c3;
    
    /*
     * Compute coefficients for a cubic polynomial
     *   p(s) = c0 + c1*s + c2*s^2 + c3*s^3
     * such that
     *   p(0) = x0, p(1) = x1
     *  and
     *   p'(0) = t0, p'(1) = t1.
     */
    void init(double x0, double x1, double t0, double t1)
    {
        c0 = x0;
        c1 = t0;
        c2 = -3*x0 + 3*x1 - 2*t0 - t1;
        c3 = 2*x0 - 2*x1 + t0 + t1;
    }
    
    double eval(double t)
    {
        double t2 = t*t;
        double t3 = t2 * t;
        return c0 + c1*t + c2*t2 + c3*t3;
    }
};

struct Vec2D
{
    Vec2D(double _x, double _y) : x(_x), y(_y) {}
    double x, y;
};

double vecDistSquared(const Vec2D& p, const Vec2D& q)
{
    double dx = q.x - p.x;
    double dy = q.y - p.y;
    return dx*dx + dy*dy;
}


class Segment {
    
    CubicPoly px,py;
public:
    double px1,px2;
private:    
    // compute coefficients for a nonuniform Catmull-Rom spline
    void initPoly(double x0,double x1,double x2,double x3,
                  double dt0, double dt1, double dt2,CubicPoly &p)
    {
        // compute tangents when parameterized in [t1,t2]
        double t1 = (x1 - x0) / dt0 - (x2 - x0) / (dt0 + dt1) + (x2 - x1) / dt1;
        double t2 = (x2 - x1) / dt1 - (x3 - x1) / (dt1 + dt2) + (x3 - x2) / dt2;
        
        // rescale tangents for parametrization in [0,1]
        t1 *= dt1;
        t2 *= dt1;
        p.init(x1, x2, t1, t2);
    }
public:    
    Segment(const Vec2D& p0, const Vec2D& p1, const Vec2D& p2, const Vec2D& p3) {
        double dt0 = powf(vecDistSquared(p0, p1), 0.25f);
        double dt1 = powf(vecDistSquared(p1, p2), 0.25f);
        double dt2 = powf(vecDistSquared(p2, p3), 0.25f);
        
        // store the edges of the range of this pair
        px1 = p1.x;
        px2 = p2.x;
    
        // safety check for repeated points
        if (dt1 < 1e-4f)    dt1 = 1.0f;
        if (dt0 < 1e-4f)    dt0 = dt1;
        if (dt2 < 1e-4f)    dt2 = dt1;
    
        initPoly(p0.x, p1.x, p2.x, p3.x, dt0, dt1, dt2, px);
        initPoly(p0.y, p1.y, p2.y, p3.y, dt0, dt1, dt2, py);
    }
    
    // return true if the given X is in the valid range for this pair
    inline bool isIn(double x){
        return x >= px1 && x <= px2;
    }
    
    // evaluate X at a T
    double evalX(double t){
        return px.eval(t);
    }
    
    // evaluate X at a T
    double evalY(double t){
        return py.eval(t);
    }

    // evaluate Y at an X
    double eval(double x){
        // calculate parameter T at x.
        // Must be between p1.x and p2.x.
        x -= px1;
        if(x<0)
            throw "oops1";
        x /= px2-px1;
        if(x>1)
            throw "oops2";
        // x is now 0-1, i.e. =t. Now we can calculate Y
        return py.eval(x);
    }
    
};

class CRSpline
{
    std::vector<Vec2D> points;
    std::vector<Segment> segments;
    bool calculated;

public:
    CRSpline(){
        calculated= false;
    }
    
    // add a control point
    void add(double x,double y){
        points.push_back(Vec2D(x,y));
    }
    
    void prt(int a,int b,int c,int d){
        std::cout << a << " " << b << " " << c << " " << d << std::endl;
    }
    
    inline int clip(int i,int sz){
        if(i<0)return 0;
        if(i>=sz)return sz-1;
        return i;
    }
    
    // calculate all segments, including extra points at the start and end.
    // Done when all points have been added.
    void calculate(){
        int sz = points.size();
        
        // calculate extra end points. This is done by extending the line from
        // p1-p0 to an extra "pre" point, and the line from p[sz-2] to p[sz-1] to
        // an extra "post" point. 
        
        double m,dx,dy,px,py;
        dx = points[1].x-points[0].x;
        dy = points[1].y-points[0].y;
        
        if(dx*dx+dy*dy < 1e-4)
            throw "first two points the same!";
        
        m = dx/dy;
        px = points[0].x-dx;
        px = points[0].y-dy*dx;
        
        // first point
        segments.push_back(Segment(Vec2D(px,py),points[0],points[1],points[2]));
        
        
        // last point will be generated using this
        
        dx = points[sz-1].x-points[sz-2].x;
        dy = points[sz-1].y-points[sz-2].y;
        if(dx*dx+dy*dy < 1e-4)
            throw "last two points the same!";
        
        m = dx/dy;
        px = points[sz-1].x+dx;
        px = points[sz-1].y+dy*dx;
        points.push_back(Vec2D(px,py));
        sz++;
        
        // intermediate and final segments
        for(int i=1;i<sz-2;i++){
            int p1 = clip(i-1,sz);
            int p2 = clip(i,sz);
            int p3 = clip(i+1,sz);
            int p4 = clip(i+2,sz);
            segments.push_back(Segment(points[p1],points[p2],points[p3],points[p4]));
        }
        calculated = true;
    }
    
    // evaluate the spline at X.
    double eval(double x){
        if(!calculated)calculate();
        
        if(x<segments[0].px1)
            return segments[0].eval(segments[0].px1);
        else if(x>segments.back().px2)
            return segments.back().eval(segments.back().px2);
        
        // first step - find the pair. We'll do this the dumb way.
        for(std::vector<Segment>::iterator p=segments.begin();p!=segments.end();p++){
            if((*p).isIn(x)){
                return (*p).eval(x);
            }
        }
        // this should probably be an exception.
        return -100;
    }
};    



}


#endif // __CATMULLROM_HPP

