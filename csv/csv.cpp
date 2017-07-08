/**
 * @file csv.cpp
 * @brief CSV file handling.
 * 
 * Usage:
 * csv$make takes a settings hash, and produces a CSV object.
 * csv$line handles a line in the CSV, typically read from a file,
 *     and returns a hash, list or none (if header or skip line
 *     was fed in).
 * csv$read reads the entire named file into a list of hashes or lists.
 * csv$cols returns the columns list.
 * 
 * Make hash parameters:
 *   `columns is a list of column names, which implies the csv has no header
 *          (default none)
 *   `list if true forces the output to be lists (default false)
 *   `nohead ignores the header even if no col names are given
 *          (names will be invented if the output is a hash)
 *          (default false)
 *   `skip defines a number of lines to ignore at the start (default zero)
 *   `delimiter sets a delimiting character (default ",")
 *   `partial if true will allow rows with less than the number of columns
 *          to be accepted, with list/hash entries created for the number
 *          present. (default false).
 *   `trim if strings should be trimmed of surrounding whitespace
 *          (default true).
 *   `comment indicates a string which starts a comment. Chars after
 *          this will be ignored, and if the result is an empty string
 *          the entire string will be ignored (default #)
 *   `types sets a column type string consisting of the chars "i", "l", "d",
 *          "f" or "s" (int,long,double,float,string). If only one char is 
 *          provided, it's used for all cols. Otherwise the char
 *          used is types[colname%strlen(types)]
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>

#include <angort/angort.h>

#include "../hashgets.h"
#include "../wrappers.h"

using namespace angort;


// clear a string list and free all its strings
inline void clearList(ArrayList<char *> *l){
    ArrayListIterator<char*> iter(l);
    for(iter.first();!iter.isDone();iter.next()){
        free(*iter.current());
    }
    l->clear();
}

// trim a string - may modify the string, and will return ptr to start
inline char *trimstr(char *s){
    while(isspace(*s))s++; // skip start
    char *p=s+strlen(s)-1; // go to end
    while(isspace(*p))*p--=0;
    return s;
}

// temp list used for each line
static ArrayList<char*> list;

// add a new string to the list (takes a length because the list
// may not be null-terminated
void addToList(ArrayList<char *> *l,const char *s,int len){
    char **newfield = l->append();
    char *newstr = (char *)malloc(len+1);
    *newfield = newstr;
    memcpy(newstr,s,len);
    newstr[len]=0;
}

// split a null-terminated line, which must not end in \n.
// If at maxcols, delimiters are ignored.
static ArrayList<char*> *splitLine(const char *s,const char delim,int maxCols=0){
    // we add characters to this as we build up fields, it's
    // the easiest way to do quoting and escaping.
    char *fbuf=(char *)alloca(strlen(s));
    
    // static list of fields, reused for each line
    
    clearList(&list);
    // if non-zero, we are in a quoted field
    char quotechar = 0; 
    int cols=0;
    char *fptr=fbuf; // field start ptr
    for(;;) {
        // two clauses here: 
        // first says "if we're at the last column and the end,
        // fill in the last column." Second says "if columns are free,
        // or we're at or before the second to last column, and there's
        // and end or unquoted delimiter." If either are true, add.
        if(((cols==maxCols-1) && *s==0) || (
              (maxCols==0 || cols<(maxCols-1) ) && 
              (*s==0 || (*s==delim && !quotechar)))){
            // delimiter found, and not in quoted field, and not
            // at maxcolumns. Add string.
            addToList(&list,fbuf,fptr-fbuf);
            cols++;
            fptr=fbuf; // restart field buffer
        } else if(quotechar && *s==quotechar){
            // end of quote
            quotechar=0;
        } else if(*s=='\'' || *s == '\"'){
            // start of quote
            quotechar = *s;
        } else if(*s=='\\') {
            // escape char   
            *fptr++ = *++s;
        } else {
            // ordinary char, add to buffer
            *fptr++ = *s;
        }
        
        if(*s==0)break;
        s++;
    }
    return &list;
}

class CSV  {
    bool noHead; // has no header - either set columns or it will invent them
    int skipLines; // skip N lines 
    bool createList; // create lists for each row, not hashes
    bool partial; // permit partial lines
    bool trim; // trim whitespace
    char delim; // delimiter
    const char *commentstart;
    
    // if true, holds a string of column type chars. These are
    // s=str,f=float,i=int. If missing, all cols are strings.
    // If too short, is modded over i.e. so that a single "f" means all
    // cols are float.
    const char *types; 
    
    int numTypes; // only valid if above is nonnull
    
    // the actual data, PER LINE. It's either a hash or a list.
    Value linev;
    
    
public:
    bool colsGiven; // true if "columns" set, so we don't override them
    int numCols;
    const char **colNames;
    
    CSV(Hash *h){ // settings hash
        Value *v;
        colNames=NULL;
        if((v=h->getSym("columns"))!=NULL){
            // predefined column headers override those in the file
            ArrayList<Value> *l = Types::tList->get(v);
            numCols = l->count();
            colNames = new const char *[numCols];
            ArrayListIterator<Value> iter(l);
            int i=0;
            for(iter.first();!iter.isDone();iter.next()){
                colNames[i++] = strdup(iter.current()->toString().get());
            }
            colsGiven=true;
        }
        else
            colsGiven=false;
        
        // do we consider the first line to be just data?
        noHead = hgetintdef(h,"nohead",0)!=0;
        
        // how many lines at the start do we skip?
        skipLines = hgetintdef(h,"skip",0);
        
        // do we create a hash for each line, or will a list do?
        createList = hgetintdef(h,"list",0)!=0;
        
        // delimiter
        delim = hgetstrdef(h,"delimiter",",")[0];
        
        // comment start
        commentstart = hgetstrdef(h,"comment","#");
        
        // partial lines accepted?
        partial = hgetintdef(h,"partial",0)!=0;
        
        // trim whitespace?
        trim = hgetintdef(h,"trim",1)!=0;
        
        // types
        types = hgetstrdef(h,"types",NULL);
        if(types && !*types)types=NULL; // don't allow empty str.
        if(types){
            types=strdup(types);
            numTypes = strlen(types);
        }
    }
    
    virtual ~CSV(){
        if(types)
            free((void *)types);
        reset();
    }
    
    void reset(){
        // do this every time we start a new file, so we can
        // get a new set of column names (and not read the first
        // line of the subsequent files as a line of data)
        if(colNames && !colsGiven){
            for(int i=0;i<numCols;i++)
                if(colNames[i])free((void *)colNames[i]);
            delete [] colNames;
            colNames=NULL;
        }
    }
    
    // process a line of input and set the value accordingly. Will throw
    // away items created from partial output if "partial" is not set.
    // MAY DAMAGE THE LINE.
    void line(Value *linev, char *s){
        int numitems=0;
        
        // are we skipping?
        if(skipLines && skipLines--){
            linev->clr();
            return;
        }
        
        // extract comment
        char *cmt;
        if(commentstart && (cmt=(char *)strstr(s,commentstart))){
            *cmt = 0;
            // is remaining string just whitespace?
            char *p;
            for(p=s;*p;p++){if(!isspace(*p))break;}
            if(!*p){
                linev->clr();
                return;
            }
        }
        
        if(!colNames && noHead){
            // no colnames and no head! We'll have to invent something.
            // we end up doing this twice, but it's only on the first line
            ArrayList<char *> *l = splitLine(s,delim,numCols);
            numCols = l->count();
            colNames = new const char *[numCols];
            for(int i=0;i<numCols;i++){
                char buf[256];
                sprintf(buf,"V%d",i);
                colNames[i]=strdup(buf);
            }
        }
            
        if(!colNames){ // no col names, but we are allowed a header
            //no, extract them
            ArrayList<char *> *l = splitLine(s,delim,0);
            numCols = l->count();
            colNames = new const char *[numCols];
            ArrayListIterator<char*> iter(l);
            int i=0;
            for(iter.first();!iter.isDone();iter.next(),i++){
                if(!strlen(*iter.current())){
                    char buf[256];
                    sprintf(buf,"V%d",i);
                    colNames[i] = strdup(buf);
                } else
                    colNames[i] = strdup(trimstr(*iter.current()));
                    
            }
        } else {
            ArrayList<char *> *l = splitLine(s,delim,numCols);
            if(createList){
                ArrayList<Value> *out = Types::tList->set(linev);
                ArrayListIterator<char*> iter(l);
                int i=0;
                for(iter.first();!iter.isDone();iter.next(),i++){
                    char *s = *iter.current();
                    Value *vout = out->append();
                    numitems++;
                    if(types){
                        switch(types[i%numTypes]){
                        case 'i':
                            Types::tInteger->set(vout,atoi(s));
                            break;
			case 'l':
			    Types::tLong->set(vout,atol(s));
			    break;
                        case 'f':
                            Types::tFloat->set(vout,atof(s));
                            break;
                        case 'd':
                            Types::tDouble->set(vout,atof(s));
                            break;
                        case 's':
                        default:
                            Types::tString->set(vout,trim?trimstr(s):s);
                            break;
                        }
                    } else {
                        Types::tString->set(vout,s);
                    }
                }
            } else {
                Hash *out = Types::tHash->set(linev);
                ArrayListIterator<char*> iter(l);
                int i=0;
                for(iter.first();!iter.isDone();iter.next(),i++){
                    char *s = *iter.current();
                    Value vout;
                    if(types){
                        switch(types[i%numTypes]){
                        case 'i':
                            Types::tInteger->set(&vout,atoi(s));
                            break;
			case 'l':
			    Types::tLong->set(&vout,atol(s));
			    break;
                        case 'f':
                            Types::tFloat->set(&vout,atof(s));
                            break;
                        case 'd':
                            Types::tDouble->set(&vout,atof(s));
                            break;
                        case 's':
                        default:
                            Types::tString->set(&vout,trim?trimstr(s):s);
                            break;
                        }
                    } else {
                        Types::tString->set(&vout,s);
                    }
                    out->setSym(colNames[i],&vout);
                    numitems++;
                }
            }
        }
        // if we didn't get at least the right number of items,
        // ignore what we just read. Of course, numitems is never
        // greater than cols because of lines being split into at
        // maximum cols columns.
        if(!partial && (numitems != numCols))
            linev->clr();
    }
};


static WrapperType<CSV> tCSV("CSVT");

%type csv tCSV CSV

%name csv
%shared



%wordargs make h (hash -- csv) start a new CSV object, given settings for reading
{
    CSV *c = new CSV(p0);
    tCSV.set(a->pushval(),c);
}

%wordargs line sA|csv (s csv -- val) parse a line, producing a hash or a list, or none if this was a skip or column header line.
{
    Value v;
    p1->line(&v,(char *)p0);
    a->pushval()->copy(&v);
}

%wordargs read As|csv (csv filename -- list|none) parse an entire file
{
    FILE *f = fopen(p1,"r");
    if(!f){
        a->pushNone();
    } else {
        p0->reset();
        Value listv,linev;
        ArrayList<Value> *list=Types::tList->set(&listv);
        char buf[8192];
        while(fgets(buf,8192,f)){
            int l=strlen(buf);
            if(l){
                buf[l-1]=0;
                p0->line(&linev,buf);
                if(!linev.isNone()){
                    list->append()->copy(&linev);
                }
            }
        }
        fclose(f);
        a->pushval()->copy(&listv);
    }
}

%wordargs reset A|csv (csv -- ) reset the CSV reader
Resets the CSV reader given, so that it will parse headers as the next
line.
{
    p0->reset();
}


%wordargs cols A|csv (csv -- list|none) get list of cols if present, or none.
{
    if(!p0->colNames){
        a->pushNone();
    } else {
        ArrayList<Value> *out = Types::tList->set(a->pushval());
        for(int i=0;i<p0->numCols;i++){
            Value *v = out->append();
            Types::tString->set(v,p0->colNames[i]);
        }
    }
    
}

%wordargs out l (csvhashlist --) output CSV to stdout
{
    ArrayListIterator<Value> iter(p0);
    ArrayList<const char *> headers;
    
    for(iter.first();!iter.isDone();iter.next()){
        Value *v = iter.current();
        Hash *h = Types::tHash->get(v);
        // if first row, get and show headers.
        if(!headers.count()){
            HashKeyIterator hki(h);
            int iidx = 0;
            for(hki.first();!hki.isDone();hki.next()){
                Value *hkv = hki.current();
                const StringBuffer& hks = hkv->toString();
                *(headers.append()) = strdup(hks.get());
                fputs(hks.get(),a->outputStream);
                iidx++;
                fputc((iidx==h->count())?'\n':',',a->outputStream);
            }
        }
        
        // show the line by getting each header (keeps order)
        for(int i=0;i<headers.count();i++){
            Value k;
            if(Value *vv = h->getSym(*headers.get(i))){
                fputs(vv->toString().get(),a->outputStream);
            }                
            else fputs("??",a->outputStream);
            fputc((i==h->count()-1)?'\n':',',a->outputStream);
        }
            
    }
    ArrayListIterator<const char *> hi(&headers);
    for(hi.first();!hi.isDone();hi.next()){
        free((void *)*hi.current());
    }
    
}

%wordargs parseline si (s maxcols -- list) parse a single comma-separated line
{
    Value v;
    ArrayList<Value> *out = Types::tList->set(&v);
    ArrayList<char *> *list = splitLine(p0,',',p1);
    
    ArrayListIterator<char*> iter(list);
    for(iter.first();!iter.isDone();iter.next()){
        Value *vv = out->append();
        Types::tString->set(vv,*iter.current());
    }
    a->pushval()->copy(&v);
}


%init
{
    fprintf(stderr,"Initialising CSV plugin, %s %s\n",__DATE__,__TIME__);
}

