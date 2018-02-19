/**
 * @file noise.cpp
 * @brief  Brief description of file.
 *
 */

#include <math.h>
#include <angort/angort.h>
#include <angort/wrappers.h>
#include "../hashgets.h"
#include "perlin.h"

using namespace angort;

static WrapperType<PerlinNoise> tPerlin("PLNT");

%type perlin tPerlin PerlinNoise

%name noise
%shared

%wordargs makeperlin h (hash -- perlin) make new perlin noise object
Create a new perlin noise generator object. The hash may contain
two values: persistence and octaves. Read up on the algorithm to see
what these actually mean.
{
    PerlinNoise *n = new PerlinNoise();
    n->mPersistence = hgetfloatdef(p0,"persistence",0.5);
    n->mOctaves = hgetintdef(p0,"octaves",2);
    tPerlin.set(a->pushval(),n);
}

%wordargs get nA|perlin (float perlin -- float) get a value
Generate a value from a perlin noise generator, given an input (typically
a time or spatial coordinate).                                                                
{
    a->pushFloat(p1->get(p0));
}

%init
{
    fprintf(stderr,"Initialising NOISE plugin, %s %s\n",__DATE__,__TIME__);
}
