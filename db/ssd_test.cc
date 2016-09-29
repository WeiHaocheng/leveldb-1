#include "db/ssd.h"
#include "leveldb/env.h"
#include "leveldb/slice.h"
#include "leveldb/options.h"
#include <stdint.h>

namespace leveldb{
void run(){
	printf("ssd test come in\n");
	Env* env = Env::Default();
	uint64_t block_size = (1<<12);
	char* buf = new char[1<<12];
	for(int i=0;i<(1<<12);i++){
		buf[i] = 'a';
	}
	Slice block(buf,1<<12);
	const std::string ssdname = "/tmp/vssd";
	uint64_t offset;
	uint64_t  block_id;
/*
	Container* c = NewContainer(0,block_size,ssdname, env);
	c->GetCurrentInfo(offset,block_id);
	printf("offset=%06lu,block_id=%06lu\n",offset,block_id);
	for(int i=0;i<10;i++)
		c->Put(block);
	c->GetCurrentInfo(offset,block_id);
	printf("offset=%06lu,block_id=%06lu\n",offset,block_id);

	c->Get(8192,&block);
	c->GetCurrentInfo(offset,block_id);
	printf("offset=%06lu,block_id=%06lu\n",offset,block_id);
	for(int i=0;i<block.size();i++){
		printf("%c",block.data()[i]);
	}
	*/
	Options options;
	SSDCache* ssd = new SSDCache(options,ssdname);
	SSDCacheKey key(0,0);
	ssd->Insert(key,buf,1<<12);
	Cache::Handle* h = ssd->Lookup(key);
	Slice* s = reinterpret_cast<Slice*>(h);
	for(int i=0;i<1<<12;i++){
			printf("%c",s->data()[i]);
		}
	return ;
}
} //namespace leveldb

int main(){
    leveldb::run();
	return 0;
}
