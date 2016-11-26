#!/bin/bash

MAKEWORDS=/usr/local/share/angort/makeWords.pl

function makewords {
    mkdir -pv ../libs
    perl $MAKEWORDS $1.cpp >$1.plugin.cpp
}

