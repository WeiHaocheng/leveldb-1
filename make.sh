# g++ -o client client.cpp path-to-libleveldb.a -lpthread -I path-to-leveldb-include

#g++ -g -o client client.cpp apath/libleveldb.a -lpthread -I ipath/leveldb/include

# example: 
#g++ -o client client.cpp ~/Code/leveldb/out-static/libleveldb.a -#lpthread -I ~/Code/leveldb/include

g++ -o client client.cpp ./out-static/libleveldb.a -lpthread -I /include
ulimit -c unlimited

