/**
 * @file surface.h
 * @brief  Brief description of file.
 *
 */

#ifndef __SURFACE_H
#define __SURFACE_H

class Surface : public GarbageCollected {
public:
    SDL_Surface *s;
    Surface(SDL_Surface *_s){ 
        s=_s;
    }
    virtual ~Surface(){
//        printf("Destroying surface\n");
        SDL_FreeSurface(s);
    }
};

class SurfaceType : public GCType {
public:
    SurfaceType(){
        add("surface","SDLS");
    }
    
    Surface *get(Value *v){
        if(v->t != this)
            throw RUNT(EX_TYPE,"not a surface");
        return (Surface *)(v->v.gc);
    }
    
    void set(Value *v, Surface *s){
        v->clr();
        v->t = this;
        v->v.gc = s;
        incRef(v);
    }
};

static SurfaceType tSurface;


#endif /* __SURFACE_H */
