/**
 * @file png.cpp
 * @brief  Brief description of file.
 *
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

//#define PNG_DEBUG 3
#include <png.h>
#include "nvbdflib-1.0/nvbdflib.h"

#include <angort/angort.h>
#include <angort/hash.h>
#include <angort/wrappers.h>

using namespace angort;


struct Image : GarbageCollected {
    uint32_t **rows;
    int width,height;
public:
    Image(int w,int h) : GarbageCollected("img"){
        width=w;
        height=h;
        
        rows = new uint32_t * [height];
        for(int i=0;i<height;i++){
            rows[i] = new uint32_t[width];
            memset(rows[i],0,sizeof(uint32_t)*width);
        }
    }
    ~Image(){
        for(int i=0;i<height;i++){
            delete [] rows[i];
        }
        delete [] rows;
    }
    inline uint32_t *pix(int x,int y){
        return rows[y]+x;
    }
};

class ImageType : public GCType {
public:
    ImageType(){
        add("PNGI","PNG");
    }
    
    Image *get(Value *v){
        if(v->t!=this)
            throw RUNT(EX_TYPE,"").set("Expected PNG image, not %s",v->t->name);
        return (Image *)(v->v.gc);
    }
    
    void set(Value *v,int w,int h) {
        v->clr();
        v->t = this;
        v->v.gc = new Image(w,h);
        incRef(v);
    }
};
static ImageType tImage;



struct Font : GarbageCollected {
    BDF_FONT *font;
public:
    
    Font(const char *fn) : GarbageCollected("fnt"){
        FILE *a = fopen(fn,"r");
        if(!a)
            throw RUNT(EX_BADPARAM,"").set("Cannot open font file : %s",fn);
        
        font = bdfReadFile(a);
        if(!font)
            throw RUNT(EX_BADPARAM,"").set("Font file is not correct: %s",fn);
        fclose(a);
    }
    ~Font(){
        bdfFree(font);
    }
};

class FontType : public GCType {
public:
    FontType(){
        add("PNGF","PNGFont");
    }
    
    Font *get(Value *v){
        if(v->t!=this)
            throw RUNT(EX_TYPE,"").set("Expected PNG Font, not %s",v->t->name);
        return (Font *)(v->v.gc);
    }
    
    void set(Value *v,const char *fn) {
        v->clr();
        v->t = this;
        v->v.gc = new Font(fn);
        incRef(v);
    }
};
static FontType tFont;

static BasicWrapperType<uint32_t> tCol("ICOL");
              
%name png
%shared

%type img tImage Image
%type font tFont Font
%type col4 tCol uint32_t

// current drawing colour
static uint32_t curcol=0xffffffff;

// need this so the callback knows where to write text;
// it's set in the text draw word(s).
static Image *currentImageForText;

static void bdfCallback(int x,int y,int iswhite){
    if(iswhite){
        if(x>=0 && x<currentImageForText->width && 
           y>=0 && y<currentImageForText->height){
            *currentImageForText->pix(x,y) = curcol;
        }
    }
}


%init
{
    if(showinit)
        fprintf(stderr,"Initialising PNG image plugin, %s %s\n",__DATE__,__TIME__);
    bdfSetDrawingFunction(bdfCallback);
}


%wordargs make ii (width height -- image) make a new blank image
{
    tImage.set(a->pushval(),p0,p1);
}

%wordargs write As|img (image filename --) write an image to a file
{
    FILE *f = fopen(p1,"wb");
    if(!f)
        throw RUNT(EX_CORRUPT,"").set("cannot open PNG for write: %s",p0);
    
    png_structp png=NULL;
    png_infop info=NULL;
    
    try {
        png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING,NULL,NULL,NULL);
        if(!png)
            throw RUNT(EX_FAILED,"cannot create PNG write struct");
        png_infop info = png_create_info_struct(png);
        if(!info)
            throw RUNT(EX_FAILED,"cannot create PNG info struct");
        
        // shudder - will this play nicely with C++ exceptions?
        if(setjmp(png_jmpbuf(png)))
            throw RUNT(EX_FAILED,"error in png write");
        // header
        png_init_io(png,f);
        png_set_IHDR(png,info,p0->width,p0->height,
                     8,PNG_COLOR_TYPE_RGBA,PNG_INTERLACE_NONE,
                     PNG_COMPRESSION_TYPE_BASE,PNG_FILTER_TYPE_BASE);
        png_write_info(png,info);
        // data
        for(int i=0;i<p0->height;i++){
            png_write_row(png,(png_bytep)p0->rows[i]);
        }
        // and done
        png_write_end(png,NULL);
    }
    catch(RuntimeException &e){
        if(f)fclose(f);
        if(info)png_free_data(png,info,PNG_FREE_ALL,-1);
        if(png)png_destroy_write_struct(&png,NULL);
        throw(e);
    }
}

%wordargs w A|img (img -- width)
{
    a->pushInt(p0->width);
}
%wordargs h A|img (img -- height)
{
    a->pushInt(p0->height);
}


%wordargs mkcol l ([r,g,b,a] -- col) make colour from bytes
{
    uint32_t r = p0->get(0)->toInt();
    uint32_t g = p0->get(1)->toInt();
    uint32_t b = p0->get(2)->toInt();
    uint32_t al = p0->get(3)->toInt();
    
    tCol.set(a->pushval(),(al<<24)+(b<<16)+(g<<8)+r);
}

%word getcol (-- col) get colour
{
    tCol.set(a->pushval(),curcol);
}

%wordargs col A|col4 (col --) set colour
{
    curcol = *p0; // because wrappers always return a ptr
}
    
          
%wordargs set iiA|img (x y img --) set a pixel
{
    if(p0>=0 && p0<p2->width && p1>=0 && p1<p2->height){
        *p2->pix(p0,p1) = curcol;
    }
}

%wordargs rect iiiiA|img (x y w h img --)
{
    int x=p0,y=p1,w=p2,h=p3;
    
    int x2 = x+w-1;
    int y2 = y+h-1;
    
    if(x2<0 || y2<0)return;
    
    if(x<0)x=0;
    if(y<0)y=0;
    
    if(y2>=p4->height)y2=p4->height-1;
    if(x2>=p4->width)x2=p4->width-1;
    w = (x2-x)+1;
    
    for(int r=y;r<=y2;r++){
        uint32_t *row = p4->rows[r]+x;
        for(int i=0;i<w;i++){
            *row++=curcol;
        }
    }
}

%wordargs loadfont s (filename -- font) Load a BDF font file
{
    Value v;
    tFont.set(&v,p0);
    a->pushval()->copy(&v);
}

%wordargs text siiAB|font,img (string x y font img --) draw a string using a BDF font
{
    bdfSetDrawingAreaSize(p4->width,p4->height);
    currentImageForText=p4;
    bdfPrintString(p3->font,p1,p2,const_cast<char *>(p0));
}
