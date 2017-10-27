rebuild:
	make -C thirdparty/cryptopp-CRYPTOPP_5_6_5  && make  -C thirdparty/cryptopp-CRYPTOPP_5_6_5 install_embed
	make clean all -C base
#make rebuild -C agent

install_libev:
	cd thirdparty/libev-master/src && sh install_embed.sh  && cd ../../..

install_mariadb:
	install_prefix=$(shell pwd) && cd thirdparty/mariadb-connector-c-3.0.0-src/ && /usr/bin/cmake -DCMAKE_INSTALL_PREFIX=$$install_prefix  . && make && make install && cd ../../   

install_hiredis:
	make -C thirdparty/hiredis-master  && make  -C thirdparty/hiredis-master install_embed
	
install_zk:
	cd thirdparty/zookeeper-3.4.9/src/c && sh install_embed.sh && cd ../../../..











































