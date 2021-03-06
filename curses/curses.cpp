/**
 * @file words.cpp
 * @brief  Brief description of file.
 *
 */

#include <stdio.h>
#include <angort/angort.h>
#include <angort/hash.h>
#include <ncurses.h>

using namespace angort;

%name curses
%shared

bool cursesOpen=false;

%word init (optlist --) initialise curses 
{
    initscr();
    keypad(stdscr,TRUE);
    cursesOpen=true;
}

%wordargs echo i (bool --) set echo state
{
    if(p0)echo();else noecho();
}

%wordargs cbreak i (bool --) set cbreak state
{
    if(p0)cbreak();else nocbreak();
}

%wordargs newline i (bool --) set nl state
{
    if(p0)nl();else nonl();
}

%wordargs move ii (x y --) move cursor
{
    move(p1,p0);
}

%wordargs write s (s --) write a string
{
    addstr(p0);
}

%wordargs timeout i (timeout --) set getch timeout in milliseconds (-1 for none)
{
    timeout(p0);
}

%word getch (-- int) get character (as int, or None if timeout)
{
    int ch = getch();
    if(ch==ERR)
        a->pushNone();
    else
        a->pushInt(ch);
}

%word size (-- y x) get max xy coords
{
    int x,y;
    getmaxyx(stdscr,y,x);
    a->pushInt(y);
    a->pushInt(x);
}

%word getpos (-- y x) get xy coords
{
    int x,y;
    getyx(stdscr,y,x);
    a->pushInt(y);
    a->pushInt(x);
}

static int getcolbyname(const char *s){
    if(!strcasecmp(s,"red"))return COLOR_RED;
    else if(!strcasecmp(s,"blue"))return COLOR_BLUE;
    else if(!strcasecmp(s,"green"))return COLOR_GREEN;
    else if(!strcasecmp(s,"magenta"))return COLOR_MAGENTA;
    else if(!strcasecmp(s,"cyan"))return COLOR_CYAN;
    else if(!strcasecmp(s,"yellow"))return COLOR_YELLOW;
    else if(!strcasecmp(s,"black"))return COLOR_BLACK;
    return COLOR_WHITE;
}

%wordargs setcols l (list --) if colour, set colour pairs
List contains pairs of assignments for colours 1 up to 7,
as symbols: `red=COLOR_RED and so on.
{
    if(has_colors()){
        start_color();
        ArrayListIterator<Value> iter(p0);
        
        int pair=1;
        for(iter.first();!iter.isDone();iter.next()){
            if(pair==8)break;
            ArrayList<Value> *l = Types::tList->get(iter.current());
            const char *fcname = Types::tSymbol->get(l->get(0));
            const char *bcname = Types::tSymbol->get(l->get(1));
            
            int fc = getcolbyname(fcname);
            int bc = getcolbyname(bcname);
            
            init_pair(pair++,fc,bc);
        }
    }
}

%wordargs attrs l (list -- long) build an attribute list for passing to attrset
This list consists of symbols or integers. If an integer is found, it
is converted to a colour pair attribute, e.g. [`standout,1] is standout
with colour pair 1.
{
    ArrayListIterator<Value> iter(p0);
    unsigned int attrs=0;
    for(iter.first();!iter.isDone();iter.next()){
        Value *v = iter.current();
        if(v->t == Types::tInteger)
            attrs |= COLOR_PAIR(v->toInt()+1);
        else {
            const StringBuffer& b = v->toString();
            if(!strcasecmp(b.get(),"standout"))
                attrs |= A_STANDOUT;
            else if(!strcasecmp(b.get(),"bold"))
                attrs |= A_BOLD;
            else if(!strcasecmp(b.get(),"dim"))
                attrs |= A_DIM;
            else if(!strcasecmp(b.get(),"reverse"))
                attrs |= A_REVERSE;
            else if(!strcasecmp(b.get(),"underline"))
                attrs |= A_UNDERLINE;
            else if(!strcasecmp(b.get(),"blink"))
                attrs |= A_BLINK;
            else if(!strcasecmp(b.get(),"italic"))
#if defined(A_ITALIC)
                attrs |= A_ITALIC;
#else
            {
            }
#endif
            else
                throw RUNT(EX_CORRUPT,"").set("bad attribute name: %s",b.get());
            
        }
    }
    a->pushLong(attrs);
}

static unsigned int attrs=0;
%wordargs setattrs L (long -- ) set attributes generated by attrs
{
    unsigned int i = p0;
    attrset(i);
    attrs=i;
}
%word getattrs (-- long) get current attrs
{
    a->pushLong(attrs);
}


%word refresh ( -- ) refresh the screen
{
    refresh();
}

%word end ( -- ) end curses
{
    if(cursesOpen)
        endwin();
    cursesOpen=false;
}    

%word getstr ( -- str) read a string from user
{
    char buf[128];
    getstr(buf);
    a->pushString(buf);
}

static int cursor=1;

%wordargs setcursor i (int --) get cursor style
{
    cursor = p0;
    curs_set(p0);
}

%word getcursor (-- int) get cursor style
{
    a->pushInt(cursor);
}

%word clrtoeol (--) clear to end of line
{
    clrtoeol();
}
    

%init
{
    if(showinit)
        fprintf(stderr,"Initialising CURSES plugin, %s %s\n",__DATE__,__TIME__);
}    

%shutdown
{
    if(cursesOpen)
        endwin();
    cursesOpen=false;
}
    
