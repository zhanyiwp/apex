#!/bin/sh
orgpath=$(pwd)
path=$(pwd)
path=${path%/*}
path=${path%/*}
path=${path%/*}
path=${path%/*}
incpath=$path/inc/zookeeper
libpath=$path/lib/zookeeper

echo $incpath
echo $libpath

chmod +x configure \
&& ./configure --libdir=$libpath --includedir=$incpath \
&& sed -i 's/^install_embed:.*$//' Makefile \
&& echo "install_embed: install-libLTLIBRARIES" >> Makefile \
&& make \
&& make install_embed
