/**
 * @file cv.cpp
 * @brief Very basic OpenCV, initially concentrating on simple camera work
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>

#include <opencv2/opencv.hpp>

#include <angort/angort.h>
#include <angort/wrappers.h>

#include "../hashgets.h"

using namespace cv;
using namespace angort;

static WrapperType<VideoCapture> tCapture("CVCP");
static WrapperType<Mat> tMat("CMAT");

%type capture tCapture VideoCapture
%type image tMat Mat

%name cv
%shared

class IntProperty : public Property {
public:
    int value;
    IntProperty(int v):value(v){}
    virtual void postSet(){
        value = v.toInt();
    }
    virtual void preGet(){
        Types::tInteger->set(&v,value);
    }
};

IntProperty textSize(2);
IntProperty textThickness(1);

void writeText(int x,int y,Mat& img,const std::string& txt){
    putText(img,txt,Point(x,y),
            FONT_HERSHEY_PLAIN,textSize.value,
            Scalar(255,255,255),
            textThickness.value,
            CV_AA
            );
}

void getLoc(const char *loc,Mat& img,const std::string& txt,int& x,int& y){
    int w = img.cols;
    int h = img.rows;
    int baseLine;
    
    Size s = getTextSize(txt,FONT_HERSHEY_PLAIN,textSize.value,
                         textThickness.value,&baseLine);
    if(!strcmp(loc,"topleft")){
        x=10;y=s.height+10;
    } else if(!strcmp(loc,"top")){
        x=(w-s.width)/2;y=s.height+10;
    } else if(!strcmp(loc,"topright")){
        x=w-s.width-10;y=s.height+10;
    } else if(!strcmp(loc,"left")){
        x=10;y=(h-s.height)/2;
    } else if(!strcmp(loc,"centre")){
        x=(w-s.width)/2;y=(h-s.height)/2;
    } else if(!strcmp(loc,"right")){
        x=w-s.width-10;y=(h-s.height)/2;
    } else if(!strcmp(loc,"bottomleft")){
        x=10;y=h-10;
    } else if(!strcmp(loc,"bottom")){
        x=(w-s.width)/2;y=h-10;
    } else if(!strcmp(loc,"bottomright")){
        x=w-s.width-10;y=h-10;
    } else {
        printf("Bad location: should be topleft,top,topright,left,centre,right,bottomleft,bottom,bottomright");
        x=(w-s.width)/2;y=h-10; // use bottom by default
    }
}

void writeTextLoc(const char *loc,Mat& img,const std::string& txt){
    int x,y;
    getLoc(loc,img,txt,x,y);
    writeText(x,y,img,txt);
}


%wordargs opencap i (camnumber -- capture) open a capture device
{
    VideoCapture *cap = new VideoCapture(p0);
    if(!cap->isOpened())
            throw RUNT(EX_BADPARAM,"cannot open capture device");
    tCapture.set(a->pushval(),cap);
}

%wordargs capture A|capture (capture -- image) capture an image
{
    Mat *m = new Mat();
    *p0 >> *m; // does the capture
    tMat.set(a->pushval(),m);
}

%wordargs resize Aii|image (image width height -- image) resize an image
{
    Size newsize(p1,p2);
    Mat *m = new Mat();
    resize(*p0,*m,newsize);
    tMat.set(a->pushval(),m);
}

%wordargs window s (name --) open a window. Must do waitkey to see it.
{
    namedWindow(p0,WINDOW_AUTOSIZE);
}

%wordargs imshow As|image (image windowname --) show an image. Must do waitkey to see it.
{
    imshow(p1,*p0);
}

%wordargs waitkey i (ms -- key) wait for a key and show any window
{
    a->pushInt(waitKey(p0));
}

%wordargs size A|image (image -- width height) get image size
{
    a->pushInt(p0->cols);
    a->pushInt(p0->rows);
}

%wordargs textxy siiA|image (string x y image --) write text to image
{
    writeText(p1,p2,*p3,std::string(p0));
}

%wordargs text ssA|image (string loc image --) write text to image
{
    writeTextLoc(p1,*p2,std::string(p0));
}

%wordargs write As|image (image filename --) write image to file
{
    imwrite(p1,*p0);
}


%init
{
    if(showinit)
        fprintf(stderr,"Initialising OPENCV plugin, %s %s\n",__DATE__,__TIME__);
    a->ang->registerProperty("textsize",&textSize,"cv");
    a->ang->registerProperty("textthickness",&textThickness,"cv");

}

