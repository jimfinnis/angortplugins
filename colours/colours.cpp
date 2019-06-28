#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <angort/angort.h>

using namespace angort;

// handling colours as [r,g,b,a] tuples


%name colours
%shared

%init
{
    fprintf(stderr,"Initialising COLOURS plugin, %s %s\n",__DATE__,__TIME__);
}    

%wordargs hsva2rgba nnnn (h s v a -- r g b a) hsv floats in 0-1 to rgba colour
{
    float h = p0;
    float s = p1;
    float v = p2;
    float alpha = p3;
    float r,g,b;
    
    h = fmodf(h,1);
    if(h<0)h=1+h;
    if(h<0.0001f)h=0.0001f;
    if(h>0.9999f)h=0.9999f;
    if(s>1)s=1;
    if(v>1)v=1;
    
    h = 6*h;
    
    float i = floorf(h);
    float f = h-i;
    
    float m = v*(1-s);
    float n = v*(1-(s*f));
    float k = v*(1-(s*(1-f)));
    
    switch((int)i)
    {
    default:
    case 0:
        r=v;g=k;b=m;break;
    case 1:
        r=n;g=v;b=m;break;
    case 2:
        r=m;g=v;b=k;break;
    case 3:
        r=m;g=n;b=v;break;
    case 4:
        r=k;g=m;b=v;break;
    case 5:
        r=v;g=m;b=n;break;
    }
    
    a->pushFloat(r);
    a->pushFloat(g);
    a->pushFloat(b);
    a->pushFloat(alpha);
}

%wordargs rgba2hsva nnnn (r g b a -- h s v a)
{
    float r = p0;
    float g = p1;
    float b = p2;
    float alpha = p3;
    float h,s,v;
    
    float max = r;
    if(g>max)max=g;
    if(b>max)max=b;
    
    v = max;
    
    if(!max){
        h = s = 0;
    } else {
        float min = r;
        if(g<min)min=g;
        if(b<min)min=b;
        
        float delta = max-min;
        s = delta / max;
        
        if(!s){
            h=0;
        } else {
            if(r==max)
                h = (g-b)/delta;
            else if(g==max)
                h = 2.0f+(b-r)/delta;
            else
                h = 4.0f+(r-g)/delta;
            
            h *= 60.0f;
            if(h<0)h+=360.0f;
            h /= 360.0f;
        }
    }
    
    a->pushFloat(h);
    a->pushFloat(s);
    a->pushFloat(v/255.0f);
    a->pushFloat(alpha);
}    
