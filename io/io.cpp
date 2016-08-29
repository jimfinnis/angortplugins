/**
 * @file io.cpp
 * @brief  Brief description of file.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>

#include <angort/angort.h>
#include <angort/hash.h>

using namespace angort;

%name io
%shared

class FileIterator : public Iterator<Value *> {
private:
    Value v;
public:
    class File *file;
    FileIterator(class File *f);
    virtual ~FileIterator();
    virtual void first();
    virtual void next();
    virtual bool isDone() const;
    virtual Value *current(){
        return &v;
    }
};

class File : public GarbageCollected {
public:
    File(FILE *_f) : GarbageCollected () {
        f = _f;
        noclose=false;
    }
    ~File(){
        close();
    }
    
    void close(){
        if(!noclose && f){
            fclose(f);
            f=NULL;
        }
    }
    
    virtual Iterator<class Value *> *makeValueIterator();    
    
    // this stops the GC trying to iterate over the file itself!
    virtual Iterator<class Value *> *makeGCValueIterator(){
        return NULL;
    }
    FILE *f;
    bool noclose;
};

class FileType : public GCType {
public:
    FileType(){
        add("file","FILE");
    }
    
    FILE *getf(Value *v){
        if(v->t!=this)
            throw RUNT(EX_TYPE,"not a file");
        return ((File *)(v->v.gc))->f;
    }
    File *get(Value *v){
        if(v->t!=this)
            throw RUNT(EX_TYPE,"not a file");
        return (File *)(v->v.gc);
    }
    
    void set(Value *v,FILE *f){
        v->clr();
        v->t=this;
        v->v.gc = new File(f);
        incRef(v);
    }
    
    // careful with this - only for stdout, etc. which are static.
    void set(Value *v, File *f){
        v->clr();
        v->t=this;
        v->v.gc=f;
        incRef(v);
    }
};


static FileType tFile;
static File *stdinf=NULL,*stdoutf=NULL,*stderrf=NULL;

%type file tFile File

/// allocates a data buffer!
static const char *readstr(FILE *f,bool endAtEOL=false){
    int bufsize = 128;
    int ct=0;
    char *buf = (char *)malloc(bufsize+1);
    
    for(;;){
        char c = fgetc(f);
        if(c==EOF || (endAtEOL && (c=='\n' || c=='\r')) || !c)
            break;
        if(ct==bufsize){
            bufsize *= 2;
            buf = (char *)realloc(buf,bufsize+1);
        }
        buf[ct++]=c;
    }
    buf[ct]=0;
    return buf;
}

/*
 * Iterator
 */

FileIterator::FileIterator(File *f){
    file = f;
    f->incRefCt();
}

FileIterator::~FileIterator(){
    if(file->decRefCt())
        delete file;
    v.clr();
}

void FileIterator::first() {
    // we don't rewind the iterator at the start, to allow
    // us to skip initial lines etc.
//    fseek(file->f,0L,SEEK_SET);
    next();
}

void FileIterator::next() {
    const char *s = readstr(file->f,true);
    Types::tString->set(&v,s);
    free((char *)s);
}

bool FileIterator::isDone() const {
    int f = feof(file->f);
    return f!=0;
}



Iterator<class Value *> *File::makeValueIterator(){
    return new FileIterator(this);
}




/*
 * Words
 */





%word open (path mode -- fileobj) open a file, modes same as fopen()
{
    Value *p[2];
    a->popParams(p,"ss");
    FILE *f = fopen(p[0]->toString().get(),p[1]->toString().get());
    if(!f)
        a->pushNone();
    else
        tFile.set(a->pushval(),f);
}

// possibly recursive binary write
static void dowrite(FILE *f,Value *v,bool inContainer=false){
    if(v->t == Types::tInteger){
        int32_t i = (int32_t)v->toInt();
        fwrite(&i,sizeof(i),1,f);
    } else if(v->t == Types::tFloat) {
        float i = (float)v->toFloat();
        fwrite(&i,sizeof(i),1,f);
    } else if(v->t == Types::tString || v->t == Types::tSymbol) {
        const StringBuffer &sb = v->toString();
        int len = strlen(sb.get());
        if(inContainer)len++; // in containers, append a NULL
        fwrite(sb.get(),len,1,f);
    } else if(v->t == Types::tList) {
        ArrayList<Value> *list = Types::tList->get(v);
        int32_t n = list->count();
        fwrite(&n,sizeof(n),1,f);
        
        ArrayListIterator<Value> iter(list);
        for(iter.first();!iter.isDone();iter.next()){
            Value *vv = iter.current();
            fwrite(&vv->t->id,sizeof(vv->t->id),1,f);
            dowrite(f,vv,true);
        }
    } else if(v->t == Types::tHash) {
        Hash *h = Types::tHash->get(v);
        int32_t n = h->count();
        fwrite(&n,sizeof(n),1,f);
        
        HashKeyIterator iter(h);
        for(iter.first();!iter.isDone();iter.next()){
            Value *vk = iter.current();
            fwrite(&vk->t->id,sizeof(vk->t->id),1,f);
            dowrite(f,vk,true);
            
            if(h->find(vk)){
                Value *vv = h->getval();
                fwrite(&vv->t->id,sizeof(vv->t->id),1,f);
                dowrite(f,vv,true);
            } else
                throw RUNT(EX_CORRUPT,"unable to find value for key when saving hash");
        }
    } else {
        throw RUNT(EX_NOTSUP,"").set("file write of unsupported type '%s'",v->t->name);
    }
}

%wordargs close A|file (fileobj --) close file (also done on delete)
{
    p0->close();
}

static FILE *getf(Value *p,bool out){
    if(p->isNone())
        return out?stdout:stdin;
    else
        return tFile.getf(p);
}

%wordargs rewind A|file (fileobj --) reset file to start, to allow reiteration
{
    fseek(p0->f,0L,SEEK_SET);
}



%word write (value fileobj/none --) write value as binary (int/float is 32 bits) to file or stdout
{
    Value *p[2];
    a->popParams(p,"vA",&tFile);
    
    dowrite(getf(p[1],true),p[0]);
}

%word write8 (value fileobj/none --) write signed byte
{
    Value *p[2];
    a->popParams(p,"nA",&tFile);
    int8_t b = p[0]->toInt();
    fwrite(&b,sizeof(b),1,getf(p[1],true));
}

%word write16 (value fileobj/none --) write 16-bit signed integer
{
    Value *p[2];
    a->popParams(p,"nA",&tFile);
    int16_t b = p[0]->toInt();
    fwrite(&b,sizeof(b),1,getf(p[1],true));
}    

%word write32 (value fileobj/none --) write 32-bit signed integer
{
    Value *p[2];
    a->popParams(p,"nA",&tFile);
    int32_t b = p[0]->toInt();
    fwrite(&b,sizeof(b),1,getf(p[1],true));
}    

%word writeu8 (value fileobj/none --) write unsigned byte
{
    Value *p[2];
    a->popParams(p,"nA",&tFile);
    uint8_t b = p[0]->toInt();
    fwrite(&b,sizeof(b),1,getf(p[1],true));
}

%word writeu16 (value fileobj/none --) write 16-bit unsigned integer
{
    Value *p[2];
    a->popParams(p,"nA",&tFile);
    uint16_t b = p[0]->toInt();
    fwrite(&b,sizeof(b),1,getf(p[1],true));
}    

%word writeu32 (value fileobj/none --) write 32-bit unsigned integer
{
    Value *p[2];
    a->popParams(p,"nA",&tFile);
    uint32_t b = p[0]->toInt();
    fwrite(&b,sizeof(b),1,getf(p[1],true));
}    

%word writefloat (value fileobj/none --) write 32-bit float
{
    Value *p[2];
    a->popParams(p,"nA",&tFile);
    float b = p[0]->toFloat();
    fwrite(&b,sizeof(b),1,getf(p[1],true));
}    

%word readfloat (fileobj/none -- float/none) read 32-bit float
{
    Value *p;
    float i;
    a->popParams(&p,"A",&tFile);
    
    if(fread(&i,sizeof(i),1,getf(p,false))>0)
        a->pushFloat(i);
    else
        a->pushNone();
}

%word writedouble (value fileobj/none --) write 64-bit float
{
    Value *p[2];
    a->popParams(p,"nA",&tFile);
    double b = p[0]->toFloat();
    fwrite(&b,sizeof(b),1,getf(p[1],true));
}    

%word readdouble (fileobj/none -- float/none) read 64-bit float
{
    Value *p;
    double i;
    a->popParams(&p,"A",&tFile);
    
    if(fread(&i,sizeof(i),1,getf(p,false))>0)
        a->pushFloat(i);
    else
        a->pushNone();
}

%word read8 (fileobj/none -- int/none) read signed byte
{
    Value *p;
    int8_t i;
    a->popParams(&p,"A",&tFile);
    
    if(fread(&i,sizeof(i),1,getf(p,false))>0){
//        printf("PUSHING INT %d\n",i);
        a->pushInt((int)i);
    }else{
//        printf("PUSHING NONE\n");
        a->pushNone();
    }
}
%word read16 (fileobj/none -- int/none) read 16-bit signed int
{
    Value *p;
    int16_t i;
    a->popParams(&p,"A",&tFile);
    
    if(fread(&i,sizeof(i),1,getf(p,false))>0)
        a->pushInt((int)i);
    else
        a->pushNone();
}
%word read32 (fileobj/none -- int/none) read 32-bit signed int
{
    Value *p;
    int32_t i;
    a->popParams(&p,"A",&tFile);
    
    if(fread(&i,sizeof(i),1,getf(p,false))>0)
        a->pushInt((int)i);
    else
        a->pushNone();
}

%word readu8 (fileobj/none -- int/none) read unsigned byte
{
    Value *p;
    uint8_t i;
    a->popParams(&p,"A",&tFile);
    
    if(fread(&i,sizeof(i),1,getf(p,false))>0)
        a->pushInt((int)i);
    else
        a->pushNone();
}
%word readu16 (fileobj/none -- int/none) read 16-bit unsigned int
{
    Value *p;
    uint16_t i;
    a->popParams(&p,"A",&tFile);
    
    if(fread(&i,sizeof(i),1,getf(p,false))>0)
        a->pushInt((int)i);
    else
        a->pushNone();
}
%word readu32 (fileobj/none -- int/none) read 32-bit unsigned int
{
    Value *p;
    uint32_t i;
    a->popParams(&p,"A",&tFile);
    
    if(fread(&i,sizeof(i),1,getf(p,false))>0)
        a->pushInt((int)i);
    else
        a->pushNone();
}



%word readstr (fileobj/none -- str) read string until null/EOL/EOF
{
    Value *p;
    a->popParams(&p,"A",&tFile);
    
    FILE *f = getf(p,false);
    const char *s = readstr(f,true);
    a->pushString(s);
    free((char *)s);
}

%word readfilestr (fileobj/none -- str) read an entire text file
{
    Value *p;
    a->popParams(&p,"A",&tFile);
    
    FILE *f = getf(p,false);
    const char *s = readstr(f,false);
    a->pushString(s);
    free((char *)s);
}

%word eof (fileobj/none -- boolean) indicates if EOF has been read
{
    Value *p;
    a->popParams(&p,"A",&tFile);
    
    FILE *f = getf(p,false);
    a->pushInt(feof(f)?1:0);
}
    
static void doreadlist(FILE *f,Value *res);
static void doreadhash(FILE *f,Value *res);

static bool readval(FILE *f,Value *res){
    
    uint32_t typeID;
    int32_t i;
    float fl;
    
    if(fread(&typeID,sizeof(typeID),1,f)<=0)
        return false;
    
    if(typeID == Types::tInteger->id) {
        fread(&i,sizeof(i),1,f);
        Types::tInteger->set(res,(int)i);
    } else if(typeID == Types::tFloat->id) {
        fread(&fl,sizeof(fl),1,f);
        Types::tFloat->set(res,fl);
    } else if(typeID == Types::tString->id) {
        const char *s = readstr(f);
        Types::tString->set(res,s);
        free((char *)s);
    } else if(typeID == Types::tSymbol->id) {
        const char *s = readstr(f);
        int sid = SymbolType::getSymbol(s);
        Types::tSymbol->set(res,sid);
        free((char *)s);
    } else if(typeID == Types::tList->id) {
        doreadlist(f,res);
    } else if(typeID == Types::tHash->id) {
        doreadhash(f,res);
    } else
        throw RUNT(EX_NOTSUP,"").set("file read of unsupported type %x",typeID);
    return true;
}
    
static void doreadlist(FILE *f,Value *res){
    ArrayList<Value> *list = Types::tList->set(res);
    int32_t n;
    fread(&n,sizeof(n),1,f);
    for(int i=0;i<n;i++){
        Value *v = list->append();
        if(!readval(f,v))
            throw RUNT(EX_CORRUPT,"").set("premature end of list read");
    }
}
static void doreadhash(FILE *f,Value *res){
    Hash *h = Types::tHash->set(res);
    
    int32_t n;
    fread(&n,sizeof(n),1,f);
    try {
        for(int i=0;i<n;i++){
            Value k,v;
            if(!readval(f,&k))
                throw "key";
            if(!readval(f,&v))
                throw "val";
            h->set(&k,&v);
        }
    } catch(const char *s) {
        throw RUNT(EX_CORRUPT,"").set("badly formed hash in read on reading %s",s);
    }
}

%word readlist (fileobj/none -- list) read a binary list (as written by 'write')
{
    Value *p;
    a->popParams(&p,"A",&tFile);
    FILE *f = getf(p,false);
    doreadlist(f,a->pushval());
}
%word readhash (fileobj/none -- hash) read a binary hash (as written by 'write')
{
    Value *p;
    a->popParams(&p,"A",&tFile);
    FILE *f = getf(p,false);
    doreadhash(f,a->pushval());
}

%wordargs isdir s (path -- boolean/none) is the path a directory? None indicates doesn't exist.
{
    struct stat b;
    if(stat(p0,&b)==0){
        a->pushInt((b.st_mode & S_IFDIR)?1:0);
    } else
        a->pushNone();
}

%wordargs readdir s (path -- list/none) read a directory returning a hash of names-> type symbols
{
    DIR *d = opendir(p0);
    if(!d)
        a->pushNone();
    else {
        Hash* h = Types::tHash->set(a->pushval());
        while(dirent *e = readdir(d)){
            const char *t;
            switch(e->d_type){
            case DT_BLK:t="blockdev";break;
            case DT_CHR:t="chardev";break;
            case DT_DIR:t="dir";break;
            case DT_FIFO:t="fifo";break;
            case DT_LNK:t="symlink";break;
            case DT_REG:t="file";break;
            case DT_SOCK:t="sock";break;
            default:t="unknown";break;
            }
            h->setSymSym(e->d_name,t);
        }
    }
}

%word exists (path -- boolean/none) does a file/directory exist? None indicates some other problem
{
    Value *p;
    a->popParams(&p,"s");
    
    struct stat b;
    if(stat(p->toString().get(),&b)==0)
        a->pushInt(1);
    else if(errno==ENOENT)
        a->pushInt(0);
    else
        a->pushNone();
}

%word flush (fileobj/none -- ) flush the file buffer
{
    Value *p;
    a->popParams(&p,"A",&tFile);
    FILE *f = getf(p,true);
    fflush(f);
}

%word stat (path -- hash/none) read the file statistics, or none if not found
{
    Value *p;
    a->popParams(&p,"s");
    
    struct stat b;
    Value *res = a->pushval();
    
    if(stat(p->toString().get(),&b)==0){
        Hash *h = Types::tHash->set(res);
        
        h->setSymInt("mode",b.st_mode);
        h->setSymInt("uid",b.st_uid);
        h->setSymInt("gid",b.st_gid);
        h->setSymInt("size",b.st_size);
        h->setSymInt("atime",b.st_atime);
        h->setSymInt("mtime",b.st_mtime);
        h->setSymInt("ctime",b.st_ctime);
    } else
        res->clr();
}

%word stdin (-- stdin) stack shared stdin object
{
    if(!stdinf){
        stdinf=new File(stdin);stdinf->noclose=true;
        stdinf->incRefCt(); //ensure never deleted
    }
    tFile.set(a->pushval(),stdinf);
}
%word stdout (-- stdout) stack shared stdout object
{
    if(!stdoutf){
        stdoutf=new File(stdout);stdoutf->noclose=true;
        stdoutf->incRefCt(); //ensure never deleted
    }
    tFile.set(a->pushval(),stdoutf);
}
%word stderr (-- stderr) stack shared stderr object
{
    if(!stderrf){
        stderrf=new File(stderr);stderrf->noclose=true;
        stderrf->incRefCt(); //ensure never deleted
    }
    tFile.set(a->pushval(),stderrf);
}

%wordargs createfd s (name -- fd) create a file
This just creates an empty file of the given name, it's often
used in association with "lock". It will truncate any existing
file! The file descriptor is returned.
{
    int fd  = open(p0,O_RDWR|O_CREAT|O_TRUNC,S_IRWXU);
    a->pushInt(fd);
}

%wordargs closefd i (fd --) close a file created with createfd
{
    close(p0);
}

%wordargs lock i (fd --) lock a file created with createfd
Uses flock() to lock the file for exclusive use by this process.
{
    flock(p0,LOCK_EX);
}

%wordargs unlock i (fd --) unlock a file created with createfd
Uses flock() to unlock the file for exclusive use by this process.
{
    flock(p0,LOCK_UN);
}


%init
{
    fprintf(stderr,"Initialising IO plugin, %s %s\n",__DATE__,__TIME__);
}
