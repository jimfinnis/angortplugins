/**
 * @file yaml.cpp
 * @brief  Brief description of file.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <iostream>
#include <fstream>

#include <yaml-cpp/yaml.h>


#include <angort/angort.h>
#include <angort/hash.h>

using namespace angort;

%name yaml
%shared

void mapToHash(Hash *h,YAML::Node map);


void seqToList(ArrayList<Value> *list,YAML::Node seq){
    for(YAML::const_iterator it=seq.begin();it!=seq.end();++it){
        Value v;
        if(it->IsMap()){
            Hash *h2 = Types::tHash->set(&v);
            mapToHash(h2,*it);
        } else if(it->IsSequence()){
            ArrayList<Value> *list = Types::tList->set(&v);
            seqToList(list,*it);
        } else if(it->IsScalar()){
            Types::tString->set(&v,it->as<std::string>().c_str());
        }
        list->append()->copy(&v);
    }
}

void mapToHash(Hash *h,YAML::Node map){
    for(YAML::const_iterator it=map.begin();it!=map.end();++it){
        std::string key = it->first.as<std::string>();
        Value k,v;
        Types::tString->set(&k,key.c_str());
        if(it->second.IsMap()){
            Hash *h2 = Types::tHash->set(&v);
            mapToHash(h2,it->second);
        } else if(it->second.IsSequence()){
            ArrayList<Value> *list = Types::tList->set(&v);
            seqToList(list,it->second);
        } else if(it->second.IsScalar()){
            Types::tString->set(&v,it->second.as<std::string>().c_str());
        }
        h->set(&k,&v);
    }
}

%wordargs load s (fileName -- hash) load YAML file
{
    std::ifstream yamlfile(p0);
    if(!yamlfile.is_open()){
        throw Exception(EX_NOTFOUND).set("cannot open %s",p0);
    }
    
    YAML::Node root = YAML::Load(yamlfile);
    if(!root.IsMap()){
        throw Exception(EX_BADPARAM).set("%s is not a YAML file",p0);
    }
    
    // now to convert to a hash, recursively.
    
    Value *v = a->pushval();
    Hash *h = Types::tHash->set(v);
    mapToHash(h,root);
}


%init
{
    if(showinit)
        fprintf(stderr,"Initialising YAML plugin, %s %s\n",__DATE__,__TIME__);
}
