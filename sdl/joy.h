/**
 * @file joy.h
 * @brief  Brief description of file.
 *
 */

#ifndef __JOY_H
#define __JOY_H

class Joystick : public GarbageCollected {
public:
    SDL_Joystick *j;
    Joystick(SDL_Joystick *_j){
        j=_j;
    }
    virtual ~Joystick(){
        SDL_JoystickClose(j);
    }
};

class JoystickType : public GCType {
public:
    JoystickType(){
        add("Joystick","SDLJ");
    }
    
    Joystick *get(Value *v){
        if(v->t != this)
            throw RUNT(EX_TYPE,"not a joystick");
        return (Joystick *)(v->v.gc);
    }
    
    void set(Value *v, SDL_Joystick *s){
        v->clr();
        v->t = this;
        v->v.gc = new Joystick(s);
        incRef(v);
    }
};
static JoystickType tJoystick;

#endif /* __JOY_H */
