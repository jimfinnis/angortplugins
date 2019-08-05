/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#include <SDL2/SDL.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <angort/angort.h>

using namespace angort;

#include "texture.h"
#include "surface.h"
#include "font.h"
#include "joy.h"

%name sdl
%shared

%type joystick tJoystick Joystick
%type tex tTexture Texture
%type surf tSurface Surface

static SDL_Window *screen = NULL;
static SDL_Renderer *renderer = NULL;
static bool inited=false;


static bool done = false; // set when we want to quit


class ColProperty : public Property {
public:
    SDL_Colour col;
    ColProperty(uint8_t r,uint8_t g,uint8_t b,uint8_t a){
        col.r = r;
        col.g = g;
        col.b = b;
        col.a = a;
    }
    
    virtual void postSet(){
        ArrayList<Value> *list = Types::tList->get(&v);
        col.r = list->get(0)->toInt();
        col.g = list->get(1)->toInt();
        col.b = list->get(2)->toInt();
        col.a = list->get(3)->toInt();
    }
    
    virtual void preGet(){
        ArrayList<Value> *list = Types::tList->set(&v);
        Types::tInteger->set(list->append(),col.r);
        Types::tInteger->set(list->append(),col.g);
        Types::tInteger->set(list->append(),col.b);
        Types::tInteger->set(list->append(),col.a);
    }
    
    void set(){
        SDL_SetRenderDrawColor(renderer,col.r,col.g,col.b,col.a);
    }
    
};

ColProperty forecol(255,255,255,255);
ColProperty backcol(0,0,0,255);


static void chkscr(){
    if(!inited)
        throw RUNT(EX_NOTREADY,"SDL not initialised");
    if(!screen)
        throw RUNT(EX_NOTREADY,"SDL screen not open");
}

static void initsdl(){
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_InitSubSystem(SDL_INIT_JOYSTICK);
    IMG_Init(IMG_INIT_JPG|IMG_INIT_PNG);
    TTF_Init();
    SDL_JoystickEventState(SDL_ENABLE);
    fprintf(stderr,"SDL initialised\n");
    inited=true;
}

%word close (--) close SDL window
{
    if(screen)
        SDL_Quit();
    screen=NULL;
}

static void openwindow(const char *title, int w,int h,int flags){
    // must be 24-bit
    initsdl();
    screen = SDL_CreateWindow(title,
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              w,h,flags);
    if(!screen)
        throw RUNT(EX_FAILED,"cannot open screen");
    renderer = SDL_CreateRenderer(screen,-1,0);
    
    backcol.set();
    SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);
    SDL_RenderFillRect(renderer,NULL);
    SDL_RenderPresent(renderer);
    SDL_RenderFillRect(renderer,NULL);
    SDL_RenderPresent(renderer);
    forecol.set();
}

%wordargs showmouse n (boolean --) show/hide the mouse cursor
{
    SDL_ShowCursor(p0);
}

%word fullscreenopen (w/none h/none --) init SDL and open a fullscreen hw window
{
    Value *p[2];
    a->popParams(p,"NN");
    
    initsdl();
    int w;
    int h;
    if(p[0]->isNone()){
        SDL_DisplayMode disp;
        int ret = SDL_GetCurrentDisplayMode(0,&disp);
        if(ret)
            throw RUNT(EX_FAILED,"").set("could not get video mode: %s",SDL_GetError());
        
        w = disp.w;
        h = disp.h;
    } else {
        w = p[0]->toInt();
        h = p[0]->toInt();
    }
    
    openwindow("",w,h,SDL_WINDOW_FULLSCREEN|SDL_WINDOW_SHOWN);
}

%word open (title w h -- ) init SDL and open a window
{
    Value *p[3];
    a->popParams(p,"snn");
    
    int w = p[1]->toInt();
    int h = p[2]->toInt();
    
    openwindow(p[0]->toString().get(),w,h,SDL_WINDOW_SHOWN);
}

%word scrsize (-- height width) get screen dimensions
{
    int w,h;
    SDL_GetWindowSize(screen,&w,&h);
    a->pushInt(h);
    a->pushInt(w);
}


%word texsize (t -- height width) get texture dimensions
{
    Value *p;
    a->popParams(&p,"a",&tTexture);
    
    int w,h;
    SDL_QueryTexture(tTexture.get(p)->t,NULL,NULL,&w,&h);
    a->pushInt(h);
    a->pushInt(w);
}

%word load (file -- texture/none) load an image into a texture
{
    Value *p;
    a->popParams(&p,"s");
    
    chkscr(); // need a screen open to do format conversion
    
    //    printf("attempting load: %s\n",p->toString().get());
    SDL_Surface *tmp = IMG_Load(p->toString().get());
    if(!tmp)
        printf("Failed to load %s\n",p->toString().get());
    
    p = a->pushval();
    
    if(!tmp)
        p->setNone();
    else {
        SDL_Texture *t = SDL_CreateTextureFromSurface(renderer,tmp);
        if(!t){
            printf("Failed to create texture %s: %s\n",p->toString().get(),
                   SDL_GetError());
            p->setNone();
            SDL_FreeSurface(tmp);
            return;
        }
        if(SDL_SetTextureBlendMode(t,SDL_BLENDMODE_BLEND))
            printf("blend mode not supported\n");
        Texture *tt = new Texture(t);
        tTexture.set(p,tt);
        SDL_FreeSurface(tmp);
    }
}

%wordargs loadsurf s (file -- surf/none) load an image into a surface (NOT a texture)
{
    chkscr(); // need a screen open to do format conversion
    
    //    printf("attempting load: %s\n",p->toString().get());
    SDL_Surface *tmp = IMG_Load(p0);
    if(!tmp)
        printf("Failed to load %s\n",p0);
    
    Value *p = a->pushval();
    if(!tmp)
        p->setNone();
    else{
        Surface *s = new Surface(tmp);
        tSurface.set(p,s);
    }
}

%wordargs surf2tex A|surf (surf -- tex) create a texture from a surface
{
    chkscr();
    Value *p = a->pushval();
    SDL_Texture *t = SDL_CreateTextureFromSurface(renderer,p0->s);
    if(!t){
        printf("Failed to create texture %s: %s\n",p->toString().get(),
               SDL_GetError());
        p->setNone();
        return;
    }
    if(SDL_SetTextureBlendMode(t,SDL_BLENDMODE_BLEND))
        printf("blend mode not supported\n");
    Texture *tt = new Texture(t);
    tTexture.set(p,tt);
}

%wordargs getpix iiA|surf (x y surf -- none|long) get pixel from surface
{
    SDL_Surface *s = p2->s;
    
    if(p0<0 || p1<0 || p0>=s->w || p1>=s->h){
        a->pushNone();
        return;
    }
    int bpp = s->format->BytesPerPixel;
    uint8_t *p = (uint8_t *)s->pixels + p1*s->pitch + p0*bpp;
    uint32_t v;
    
    switch(bpp){
    case 1:
        v = *p;
        break;
    case 2:
        v = *(uint16_t*)p;
        break;
    case 3:
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
            v= (p[0] << 16) | (p[1] << 8) | p[2];
        else
            v= (p[2] << 16) | (p[1] << 8) | p[0];
        break;
    case 4:
        v = *(uint32_t *)p;
        break;
    default:
        v=0;
    }
    
    a->pushLong((long)v);
              
}

%word blit (dx dy dw/none dh/none surf --) basic texture blit
{
    Value *p[5];
    a->popParams(p,"nnNNa",&tTexture);
    
    chkscr();
    SDL_Texture *t = tTexture.get(p[4])->t;
    SDL_Rect src,dst;
    
    int w,h;
    SDL_QueryTexture(t,NULL,NULL,&w,&h);
    
    dst.x = p[0]->toInt(); // destination x
    dst.y = p[1]->toInt(); // destination y
    dst.w = p[2]->isNone() ? w : p[2]->toInt(); // dest width
    dst.h = p[3]->isNone() ? h : p[3]->toInt(); // dest height
    src.x = 0;
    src.y = 0;
    src.w = w;
    src.h = h;
    
    if(SDL_RenderCopy(renderer,t,&src,&dst)<0){
        printf("blit error: %s\n",SDL_GetError());
    }
}

%word exblit (dx dy dw/none dh/none sx sy sw/none sh/none surf --) blit a texture to the screen
{
    Value *p[9];
    a->popParams(p,"nnNNnnNNa",&tTexture);
    
    chkscr();
    SDL_Texture *t = tTexture.get(p[8])->t;
    SDL_Rect src,dst;
    
    int w,h;
    SDL_QueryTexture(t,NULL,NULL,&w,&h);
    
    dst.x = p[0]->toInt(); // destination x
    dst.y = p[1]->toInt(); // destination y
    dst.w = p[2]->isNone() ? w : p[2]->toInt(); // dest width
    dst.h = p[3]->isNone() ? h : p[3]->toInt(); // dest height
    src.x = p[4]->toInt(); // source x
    src.y = p[5]->toInt(); // source y
    src.w = p[6]->isNone() ? w : p[6]->toInt(); // source width
    src.h = p[7]->isNone() ? h : p[7]->toInt(); // source height
    
    if(SDL_RenderCopy(renderer,t,&src,&dst)<0){
        printf("blit error: %s\n",SDL_GetError());
    }
}

%word exflipblit (dx dy dw/none dh/none sx sy sw/none sh/none angle flipbits surf --) blit a texture to the screen, flipped perhaps
{
    Value *p[11];
    a->popParams(p,"nnNNnnNNNNa",&tTexture);
    
    chkscr();
    SDL_Texture *t = tTexture.get(p[10])->t;
    SDL_Rect src,dst;
    
    int w,h;
    SDL_QueryTexture(t,NULL,NULL,&w,&h);
    
    dst.x = p[0]->toInt(); // destination x
    dst.y = p[1]->toInt(); // destination y
    dst.w = p[2]->isNone() ? w : p[2]->toInt(); // dest width
    dst.h = p[3]->isNone() ? h : p[3]->toInt(); // dest height
    src.x = p[4]->toInt(); // source x
    src.y = p[5]->toInt(); // source y
    src.w = p[6]->isNone() ? w : p[6]->toInt(); // source width
    src.h = p[7]->isNone() ? h : p[7]->toInt(); // source height
    float angle = p[8]->toInt();
    int flipbits = p[9]->toInt();
    
    int flip = SDL_FLIP_NONE;
    if(flipbits & 1) flip = SDL_FLIP_HORIZONTAL;
    if(flipbits & 2) flip = (int)flip|(int)SDL_FLIP_VERTICAL;
    
    
    
    if(SDL_RenderCopyEx(renderer,t,&src,&dst,angle,NULL,
                        (SDL_RendererFlip)flip)<0){
        printf("blit error: %s\n",SDL_GetError());
    }
}

%word flip (--) flip front and back buffer
{
    chkscr();
    SDL_RenderPresent(renderer);
}


%word clear (--) clear the screen to the background colour
{
    chkscr();
    backcol.set();
    SDL_RenderFillRect(renderer,NULL);
}

%word fillrect (x y w h --) draw a filled rectangle in current colour
{
    Value *p[4];
    a->popParams(p,"nnnn");
    
    chkscr();
    SDL_Rect r;
    
    r.x = p[0]->toInt();
    r.y = p[1]->toInt();
    r.w = p[2]->toInt();
    r.h = p[3]->toInt();
    
    forecol.set();
    SDL_RenderFillRect(renderer,&r);
}

%word line (x1 y1 x2 y2 --) draw a line in current colour
{
    Value *p[4];
    a->popParams(p,"nnnn");
    
    chkscr();
    
    forecol.set();
    SDL_RenderDrawLine(renderer,p[0]->toInt(),
                       p[1]->toInt(),
                       p[2]->toInt(),
                       p[3]->toInt());
}



%word openfont (file size -- font) open a TTF font
{
    Value *p[2];
    a->popParams(p,"sn");
    chkscr();
    
    Value *v = a->pushval();
    TTF_Font *f = TTF_OpenFont(p[0]->toString().get(),p[1]->toInt());
    if(!f){
        printf("Failed to load font %s\n",p[0]->toString().get());
        v->setNone();
    } else {
        Font *fo = new Font(f);
        tFont.set(v,fo);
    }
}

%word maketext (text font -- texture) draw text to a texture
{
    Value *p[2];
    a->popParams(p,"sa",&tFont);
    chkscr();
    
    SDL_Surface *tmp = TTF_RenderUTF8_Blended(tFont.get(p[1])->f,
                                              p[0]->toString().get(),
                                              forecol.col);
    
    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer,tmp);
    if(SDL_SetTextureBlendMode(tex,SDL_BLENDMODE_BLEND))
        printf("blend mode not supported\n");
    SDL_FreeSurface(tmp);
    
    Value *v = a->pushval();
    tTexture.set(v,new Texture(tex));
}

%word fontsize (text font -- h w) get rendered size
{
    Value *p[2];
    a->popParams(p,"sa",&tFont);
    chkscr();
    
    int w,h;
    if(TTF_SizeUTF8(tFont.get(p[1])->f,p[0]->toString().get(),&w,&h)){
        printf("Error getting size of string\n");
        a->pushNone();
        a->pushNone();
    }else{
        a->pushInt(h);
        a->pushInt(w);
    }
}

%wordargs aacircle nnn (x y radius --)
{
    aacircleRGBA(renderer,p0,p1,p2,
                 forecol.col.r,forecol.col.g,
                 forecol.col.b,forecol.col.a);
}

%wordargs aaellipse nnnn (x y xr yr --)
{
    aaellipseRGBA(renderer,p0,p1,p2,p3,
                  forecol.col.r,forecol.col.g,
                  forecol.col.b,forecol.col.a);
}


%wordargs filledcircle nnn (x y radius --)
{
    filledCircleRGBA(renderer,p0,p1,p2,
                     forecol.col.r,forecol.col.g,
                     forecol.col.b,forecol.col.a);
}

%wordargs filledellipse nnnn (x y xr yr --)
{
    filledEllipseRGBA(renderer,p0,p1,p2,p3,
                      forecol.col.r,forecol.col.g,
                      forecol.col.b,forecol.col.a);
}


// various callbacks, all initially "none"
Value onKeyDown;
Value onKeyUp;
Value onMouseMove,onMouseUp,onMouseDown;
Value onDraw;

%word ondraw (callable --) set the draw callback, of spec (--)
{
    Value *p;
    a->popParams(&p,"c");
    onDraw.copy(p);
}

%word onkeyup (callable --) set the key up callback, of spec (keysym --)
{
    Value *p;
    a->popParams(&p,"c");
    onKeyUp.copy(p);
}

%word onkeydown (callable --) set the key down callback, of spec (keysym --)
{
    Value *p;
    a->popParams(&p,"c");
    onKeyDown.copy(p);
}

%word onmousemove (callable --) set the mouse motion callback, of spec (x y)
{
    Value *p;
    a->popParams(&p,"c");
    onMouseMove.copy(p);
}
%word onmousedown (callable --) set the mouse down callback, of spec (x y button--)
{
    Value *p;
    a->popParams(&p,"c");
    onMouseDown.copy(p);
}
%word onmouseup (callable --) set the mouse up callback, of spec (x y button--)
{
    Value *p;
    a->popParams(&p,"c");
    onMouseUp.copy(p);
}

int keyMod = 0;

%word keymod (-- modifier flags for last key event)
{
    a->pushInt(keyMod);
}

%word loop (--) start the main game loop
{
    chkscr();
    while(!done){
        SDL_Event e;
        while(SDL_PollEvent(&e)){
            switch(e.type){
            case SDL_QUIT:
                done=true;break;
            case SDL_KEYDOWN:
                if(!onKeyDown.isNone()){
                    a->pushInt(e.key.keysym.sym);
                    keyMod = e.key.keysym.mod;
                    a->runValue(&onKeyDown);
                }
                break;
            case SDL_KEYUP:
                if(!onKeyUp.isNone()){
                    a->pushInt(e.key.keysym.sym);
                    a->runValue(&onKeyUp);
                }
                break;
            case SDL_MOUSEMOTION:
                if(!onMouseMove.isNone()){
                    a->pushInt(e.motion.x);
                    a->pushInt(e.motion.y);
                    a->runValue(&onMouseMove);
                }
                break;
            case SDL_MOUSEBUTTONDOWN:
                if(!onMouseDown.isNone()){
                    a->pushInt(e.motion.x);
                    a->pushInt(e.motion.y);
                    a->pushInt(e.button.button);
                    a->runValue(&onMouseDown);
                }
                break;
            case SDL_MOUSEBUTTONUP:
                if(!onMouseUp.isNone()){
                    a->pushInt(e.motion.x);
                    a->pushInt(e.motion.y);
                    a->pushInt(e.button.button);
                    a->runValue(&onMouseUp);
                }
                break;
            default:break;
            }
        }
        if(!onDraw.isNone())
            a->runValue(&onDraw);
    }
    done = false; // reset the done flag
}



%word done (--) set the done flag to end the main loop
{
    done=true;
}

%word joyct (-- count) get number of joysticks
{
    a->pushInt(SDL_NumJoysticks());
}

%wordargs joyname i (idx -- name) get joystick name
{
    a->pushString(SDL_JoystickNameForIndex(p0));
}

%wordargs joyopen i (i -- joy) open a joystick
{
    SDL_Joystick *j = SDL_JoystickOpen(p0);
    tJoystick.set(a->pushval(),j);
}

%wordargs joynumaxes A|joystick (joy -- n) number of axes
{
    a->pushInt(SDL_JoystickNumAxes(p0->j));
}

%wordargs joygetaxis iA|joystick (axis joy -- n) get axis
{
    a->pushInt(SDL_JoystickGetAxis(p1->j,p0));
}

%wordargs joynumbuttons A|joystick (joy -- n) number of buttons
{
    a->pushInt(SDL_JoystickNumButtons(p0->j));
}

%wordargs joygetbutton iA|joystick (axis joy -- n) get button
{
    a->pushInt(SDL_JoystickGetButton(p1->j,p0));
}

%wordargs joynumhats A|joystick (joy -- n) number of hats
{
    a->pushInt(SDL_JoystickNumHats(p0->j));
    
}

static void hat2xy(int code,int *x,int *y){
    switch(code){
    case SDL_HAT_LEFTUP:
        *x=-1;*y=1;break;
    case SDL_HAT_LEFT:
        *x=-1;*y=0;break;
    case SDL_HAT_LEFTDOWN:
        *x=-1;*y=-1;break;
    case SDL_HAT_UP:
        *x=0;*y=1;break;
    default:
    case SDL_HAT_CENTERED:
        *x=0;*y=0;break;
    case SDL_HAT_DOWN:
        *x=0;*y=-1;break;
    case SDL_HAT_RIGHTUP:
        *x=1;*y=1;break;
    case SDL_HAT_RIGHT:
        *x=1;*y=0;break;
    case SDL_HAT_RIGHTDOWN:
        *x=1;*y=-1;break;
    }
}

%wordargs joygethat iA|joystick (axis joy -- y x) get hat
{
    int code = SDL_JoystickGetHat(p1->j,p0);
    int x,y;
    hat2xy(code,&x,&y);
    a->pushInt(y);
    a->pushInt(x);
}

%wordargs hsv2col nnn (h s v -- col) hsv floats to rgba colour
{
    float h = p0;
    float s = p1;
    float v = p2;
    
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
    
    ArrayList<Value> *list = Types::tList->set(a->pushval());
    Types::tInteger->set(list->append(),r*255);
    Types::tInteger->set(list->append(),g*255);
    Types::tInteger->set(list->append(),b*255);
    Types::tInteger->set(list->append(),255);
}

%wordargs alpha ln (col alpha -- col) set colour alpha
{
    int r = p0->get(0)->toInt();
    int g = p0->get(1)->toInt();
    int b = p0->get(2)->toInt();
    ArrayList<Value> *list = Types::tList->set(a->pushval());
    
    Types::tInteger->set(list->append(),r);
    Types::tInteger->set(list->append(),g);
    Types::tInteger->set(list->append(),b);
    Types::tInteger->set(list->append(),p1);
}
    





%init
{
    if(showinit)
        fprintf(stderr,"Initialising SDL plugin, %s %s\n",__DATE__,__TIME__);
    
    a->ang->registerProperty("col",&forecol,"sdl");
    a->ang->registerProperty("bcol",&backcol,"sdl");
    a->ang->registerProperty("tcol",new TextureColProperty(a),"sdl");
    a->ang->registerProperty("talpha",new TextureAlphaProperty(a),"sdl");
}


%shutdown
{
    fprintf(stderr,"Closing down SDL\n");
    if(screen){
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(screen);
    }
    if(inited){
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
    }
}
