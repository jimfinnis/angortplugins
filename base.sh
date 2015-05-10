#!/bin/bash


function makewords {
    mkdir -pv ../libs
    perl /home/white/angort/makeWords.pl $1.cpp >$1.plugin.cpp
}

