/**
 * @file catmullrom.hpp
 * @brief  Brief description of file.
 *
 */

#ifndef __KBSPLINE_HPP
#define __KBSPLINE_HPP

#include <cmath>
#include <vector>
#include <stdexcept>


class KBSpline 
{
private:    
    // matrix elements
    double a,b,c,d,e,f,g,h,i,j,k,l;
    // actual points
    std::vector<double> points;
    
public:
    /// initialisation. 0,0,0 gives Catmull-Rom, 1,0,0 = simple cubic, 0,0,1 = linear interpolation
    KBSpline(
             double tension,	//!< +1 = tight, -1 = round
             double bias,		//!< +1 = post shoot, -1 = pre shoot
             double continuity //!< +1 = inverted corners, -1 = box corners
             )
    {
        double FT = tension;
        double FB = bias;
        double FC = continuity;
    
        double FFA=(1-FT)*(1+FC)*(1+FB);
        double FFB=(1-FT)*(1-FC)*(1-FB);
        double FFC=(1-FT)*(1-FC)*(1+FB);
        double FFD=(1-FT)*(1+FC)*(1-FB);
        
        // now get the matrix coefficients
        

        a=-FFA;
        b=(4+FFA-FFB-FFC);
        c=(-4+FFB+FFC-FFD);
        d=FFD;

        e=2*FFA;
        f=(-6-2*FFA+2*FFB+FFC);
        g=(6-2*FFB-FFC+FFD);
        h=-FFD;

        i=-FFA;
        j=(FFA-FFB);
        k=FFB;
        l=0;

    }
private:
    /// 1D spline function
    
    inline double baseSpline1D(
                      double p,			//!< control points
                      double q,
                      double r,
                      double s,
                      double t				//!< position of desired point between points[1] and points[2] - i.e. between 2nd and 3rd points
                      )
    {
        double t2 = t*t;
        double t3 = t2*t;
        
        return 0.5 * (
                       (a*p + b*q + c*r + d*s) * t3 + 
                       (e*p + f*q + g*r + h*s) * t2 +
                       (i*p + j*q + k*r + l*s) * t +
                       2*q);
        
        
    }
    
    inline int correctIndex(int i,int n){
        while(i<0)i+=n;
        while(i>=n)i-=n;
        return i;
    }
public:
    /// get spline in set of points
    double eval(double t) // parameter between 0 and 1
    {
        int n = points.size();
        t*=(n-1); 
        // [floor(t),floor(t)+1] are the segment we want. They might be out of range;
        // we'll deal with that.
        
        double fl = floor(t);
        int i1 = correctIndex((int)fl,n);
        int i0 = correctIndex(i1-1,n);
        int i2 = correctIndex(i1+1,n);
        int i3 = correctIndex(i1+2,n);
        
        
        return baseSpline1D(points[i0],points[i1],points[i2],points[i3],
                            t-fl);
    }
    
    void add(double d){
        points.push_back(d);
    }
    
};


#endif // __KBSPLINE_HPP

