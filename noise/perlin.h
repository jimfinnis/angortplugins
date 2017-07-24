/**
 * @file perlin.h
 * @brief  Brief description of file.
 *
 */

#ifndef __PERLIN_H
#define __PERLIN_H

class PerlinNoise
{
public:
    int mOctaves;
    float mPersistence;//!< persistence value - 0.25 is calm, 1 is spiky!
    
    PerlinNoise()
    {
        mPersistence = 0.5;
        mOctaves = 2;
    }
    
    /// get a noise value for a given x
    
    
    
    float get(float x)
    {
        float total = 0;
        
        float freq = 1;
        float amp = 1;
        for(int i=0;i<mOctaves;i++)
        {
            total += interpolatedNoise(i,x*freq) * amp;
            
            freq *= 2;
            amp *= mPersistence;
        }
        
        return total;
    }
    
private:
    
    
    /// noise functions
    
    inline float noise(int n, //!< function number
                       int x //!< argument
                       )
    {
        x = (x<<13)^x;
        x += n*31;
        return (1.0f - ((x*(x*x*15731+789221)+1376312589)&0x7fffffff)/1073741824.0f);
    }
    
    
    inline float smoothedNoise(int n,float x)
    {
        return noise(n,(int)x)/2 + noise(n,(int)x-1)/4 + noise(n,(int)x+1)/4;
    }
    
    inline float interpolatedNoise(int n,float x)
    {
        int inx = (int)x;
        float fracx = x - inx;
        float v1 = smoothedNoise(n,inx);
        float v2 = smoothedNoise(n,inx+1);
        
        return interpolate(v1,v2,fracx);
    }
    
    inline float interpolate(float a,float b,float x)
    {
        float ft = x * 3.1415927f;
        float f = (1-cosf(ft))*0.5f;
        return a*(1-f)+b*f;
    }
    
    
};


#endif /* __PERLIN_H */
