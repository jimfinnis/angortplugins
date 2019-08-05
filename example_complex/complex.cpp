#include <angort/angort.h>
#include <math.h>

// set the angort namespace.
using namespace angort;

// declare the name of the library and set a namespace for it -
// DO NOT use C++ namespace stuff after this.

%name complex

// say that we are making a shared library.
%shared

// you could probably use a BasicWrapper here, but this shows
// a full example. This is the actual complex number object,
// which needs to inherit GarbageCollected so it has a reference
// count Angort can use.

class Complex : public GarbageCollected {
public:
    double r,i;
    
    virtual ~Complex(){}
    Complex(double _r,double _i){
        r = _r;
        i = _i;
    }
};

// This is the type singleton, which inherits GCType because it's
// a garbage-collected type. 

class ComplexType : public GCType {
public:
    // initialisation - the first value is the type name, the second
    // is a type-ID which must be 4 bytes long. I really need to get
    // rid of these, they were originally for serialisation.
    
    ComplexType(){
        add("complex","CPLT");
    }
    
    virtual ~ComplexType(){}
    
    // given a value, get a pointer to the complex number - checking
    // that it is one.
    
    Complex *get(const Value *v) const {
        if(v->t!=this)
            throw RUNT(EX_TYPE,"not a complex number");
        return (Complex *)(v->v.gc);
    }
    
    // given a value, set it to be a complex number. We do this by
    // clearing the value (which will decrement the refcount of any
    // garbage-collected thing it's pointing to), setting the type
    // pointer to this type object, creating the complex number object,
    // and incrementing the refcount (which will increment the refcount
    // in the complex number object).
    
    void set(Value *v,double r,double i){
        v->clr();
        v->t=this;
        v->v.gc = new Complex(r,i);
        incRef(v);
    }
    
    // convert the complex to a string - this allocates a string and
    // sets the "allocated" flag, so that Angort knows it needs to
    // refcount it.
    virtual const char *toString(bool *allocated,const Value *v) const {
        char buf[128];
        Complex *w = get(v);
        snprintf(buf,128,"%f+%fi",w->r,w->i);
        *allocated=true;
        return strdup(buf);
    }
};

// declare the singleton object
static ComplexType tC;

// for argument lists in %wordargs - the type "complex" has the 
// type object tC (declared above) and contains objects of the type
// Complex. This only works on GC types.

%type complex tC Complex

// "complex" takes two arguments, both doubles. These get automatically
// mapped onto p0 and p1 when makeWords.pl processes this line.

%wordargs complex dd (r i -- complex)
{
    // a->pushval() will call pushval() inside the angort object,
    // passed automatically as "a" to word functions. This will
    // return a pointer to the new stack top./ We the type object's
    // set() method to set it to be a new complex number from the arguments.
    
    tC.set(a->pushval(),p0,p1);
}

// "real" takes one argument, of type "A". Uppercase types are selected
// types we defined, which are given in a list after the bar. A is the first,
// B is the second and so on. For example, if we had a word which took
// four args, which are a string, a complex, and two of type "wibble"
// we could use the argument spec "sABB|complex,wibble".

%wordargs real A|complex (complex -- real part)
{
    // pushDouble is shorthand for Types::tDouble->set(a->pushval(),..)
    // and will push a double onto the stack.
    // p0 is the complex number, deferenced automatically to a Complex
    // object pointer.
    
    a->pushDouble(p0->r);
}

%wordargs img A|complex (complex -- img part)
{
    a->pushDouble(p0->i);
}

// declare a binary operation. The two types are given with the binop's opcode
// name between them. This can be: equals,nequals,add,mul,div,sub,and,or,gt,
// lt,le,lt,mod,cmp.
// In the function, lhs and rhs are Value pointers and so will need to converted
// to the appropriate type.

%binop complex mul complex
{
    Complex *l = tC.get(lhs); // use tC to get Complex pointer from Value
    Complex *r = tC.get(rhs);
    // calculate result
    double real = (l->r * r->r) - (l->i * r->i);
    double img = (l->r * r->i) + (l->i * r->r);
    // push new complex.
    tC.set(a->pushval(),real,img);
}

%binop complex add complex
{
    Complex *l = tC.get(lhs);
    Complex *r = tC.get(rhs);
    tC.set(a->pushval(),l->r+r->r,l->i+r->i);
}

%binop complex add double
{
    Complex *l = tC.get(lhs);
    float r = rhs->toDouble();
    tC.set(a->pushval(),l->r+r,l->i);
}


// initialisation code. There is also an optional %shutdown.

%init
{
    if(showinit)
        fprintf(stderr,"Initialising COMPLEX plugin, %s %s\n",__DATE__,__TIME__);
}    
