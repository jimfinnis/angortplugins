/**
 * @file json.cpp
 * @brief  JSON parser using RapidJSON 
 * RapidJSON is 
 *  Copyright (C) 2015 THL A29 Limited, a Tencent company, and Milo Yip. All rights reserved.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/error/en.h"
#include <sys/stat.h>
#include <sys/file.h>
#include <angort/angort.h>
#include <angort/hash.h>

using namespace angort;

%name json
%shared

static void parseArrayIntoList(Value *vv, const rapidjson::Value& v);
static void parseObjectIntoHash(Value *vv, const rapidjson::Value& v);

static void parseval(Value *out,const rapidjson::Value &v){
    if(v.IsObject())
        parseObjectIntoHash(out,v);
    else if(v.IsArray())
        parseArrayIntoList(out,v);
    else if(v.IsString())
        Types::tString->setwithlen(out,v.GetString(),v.GetStringLength());
    else if(v.IsInt())
        Types::tInteger->set(out,v.GetInt());
    else if(v.IsUint())
        Types::tInteger->set(out,v.GetUint());
    else if(v.IsInt64())
        Types::tLong->set(out,v.GetInt64());
    else if(v.IsUint64())
        Types::tLong->set(out,v.GetUint64());
    else if(v.IsDouble())
        Types::tDouble->set(out,v.GetDouble());
    else if(v.IsTrue())
        Types::tInteger->set(out,1);
    else if(v.IsFalse())
        Types::tInteger->set(out,0);
    else if(v.IsNull())
        out->clr();
    
}

static void parseObjectIntoHash(Value *vv, const rapidjson::Value &v){
    Hash *h = Types::tHash->set(vv);
    for(rapidjson::Value::ConstMemberIterator itr=v.MemberBegin();
        itr!=v.MemberEnd();++itr){
        const char *key = itr->name.GetString();
        Value vv;
        parseval(&vv,itr->value);
        h->setSym(key,&vv);
    }
    
}

static void parseArrayIntoList(Value *vv, const rapidjson::Value &v){
    ArrayList<Value> *list = Types::tList->set(vv);
    for(rapidjson::SizeType i=0;i<v.Size();i++){
        parseval(list->append(),v[i]);
    }
}

static void parse(const char *s,Runtime *a){
    rapidjson::Document d;
    if(d.Parse(s).HasParseError()){
        throw RUNT(EX_FAILED,"").set("JSON parse error at offset %u: %s",
                                     (unsigned)d.GetErrorOffset(),
                                     rapidjson::GetParseError_En(d.GetParseError()));
    }
    
    Value *v  = a->pushval();
    if(d.IsObject()){
        parseObjectIntoHash(v,d);
    }
    else if(d.IsArray()){
        parseArrayIntoList(v,d);
    } else {
        throw RUNT(EX_FAILED,"").set("Top level JSON must be array or object");
    }
}


%wordargs fromstring s (string -- data) parse from string
Parse a JSON document into (typically) a hash or list, from a string.
{
    parse(p0,a);
}

%wordargs fromfile s (fn -- data) parse from filename
Parse a JSON document into (typically) a hash or list, from a file.
{
    struct stat b;
    if(stat(p0,&b)!=0){
        throw RUNT(EX_FAILED,"").set("JSON parser cannot stat file %s",p0);
    }
    off_t len = b.st_size;
    
    FILE *ff = fopen(p0,"r");
    if(!ff)
        throw RUNT(EX_FAILED,"").set("JSON parser cannot open file %s",p0);
    
    char *tmps = (char *)malloc(len+1);
    fread(tmps,1,len,ff);
    tmps[len]=0;
    fclose(ff);
    parse(tmps,a);
    free((void *)tmps);
}
