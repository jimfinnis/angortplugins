/**
 * @file id3.cpp
 * @brief  Brief description of file.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <taglib.h>
#include <fileref.h>
#include <tag.h>

#include <angort/angort.h>
#include <angort/hash.h>

using namespace angort;

%name id3
%shared

inline void setTagStr(Hash *h,const char *k,TagLib::String str){
    h->setSymStr(k,str.toCString(true));
}

%word loadtags (fileName -- hash|none) load ID3 tags (also stores filename) in hash
{
    Value *p;
    a->popParams(&p,"s");
    char fn[1024];
    strcpy(fn,p->toString().get());
    TagLib::FileRef f(fn);
    
    p=a->pushval();
    
    if(f.isNull())
        p->setNone();
    else {
        TagLib::Tag *t= f.tag();
        Hash *res = Types::tHash->set(p);
        
        setTagStr(res,"artist",t->artist());
        setTagStr(res,"title",t->title());
        setTagStr(res,"album",t->album());
        setTagStr(res,"comment",t->comment());
        setTagStr(res,"genre",t->genre());
        res->setSymInt("year",t->year());
        res->setSymInt("track",t->track());
        setTagStr(res,"filename",fn);
    }
}

inline TagLib::String getStr(Hash *hash, const char *name){
    Value *v = hash->getSym(name);
    if(v)
        return TagLib::String(
                              v->toString().get(),
                              TagLib::String::UTF8);
    else
        return TagLib::String::null;
}

inline int getInt(Hash *hash,const char *name){
    Value *v = hash->getSym(name);
    if(v)
        return v->toInt();
    else
        return -1;
}
    

%wordargs savetags hs (hash --) save ID3 tags as loaded by loadtags
{
    TagLib::FileRef f(p1);
    TagLib::Tag *t = f.tag();
    
    t->setArtist(getStr(p0,"artist"));
    t->setTitle(getStr(p0,"title"));
    t->setAlbum(getStr(p0,"album"));
    t->setComment(getStr(p0,"comment"));
    t->setGenre(getStr(p0,"genre"));
    
    t->setTrack(getInt(p0,"track"));
    t->setYear(getInt(p0,"year"));
        
    if(!f.save())
        fprintf(stderr,"Warning: could not save to file %s\n",p1);
}


%init
{
    if(showinit)
        fprintf(stderr,"Initialising ID3 plugin, %s %s\n",__DATE__,__TIME__);
}
