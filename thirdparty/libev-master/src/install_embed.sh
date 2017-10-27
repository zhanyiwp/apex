#!/bin/sh
path=$(pwd)
path=${path%/*}
path=${path%/*}
path=${path%/*}
incpath=$path/inc/libev
libpath=$path/lib/libev

echo $incpath
echo $libpath

autoreconf --install --symlink --force && \
./configure --libdir=$libpath --includedir=$incpath  && \
make && \
make install 
