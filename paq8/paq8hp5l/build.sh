#!/bin/sh
sed -e s/^/\"/ -e s/$/\\\\n\"/ hp5.dic > hp5dict.h
