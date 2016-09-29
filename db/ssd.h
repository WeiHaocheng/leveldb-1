#ifndef STORAGE_LEVELDB_DB_SSD_CACHE_H_
#define STORAGE_LEVELDB_DB_SSD_CACHE_H_

#include <unordered_map>
#include <vector>
#include <map>
#include <string>
#include <stdint.h>
#include "leveldb/slice.h"
#include "leveldb/cache.h"
#include "util/mutexlock.h"
#include "db/dbformat.h"
#include "db/log_writer.h"
#include "db/snapshot.h"
#include "leveldb/db.h"
#include "leveldb/env.h"
#include "leveldb/options.h"
#include "port/port.h"
#include "port/thread_annotations.h"

namespace leveldb{

//whc add
//#define SSD_USED

static uint64_t CONTAINERSIZE = 1<<21;
static uint64_t CONTAINERNUM = 1<<8;
static uint64_t BUCKETLIMIT = 1<<20;

struct SSDCacheKey{
	uint64_t filenum_;
	uint64_t blocknum_;

	SSDCacheKey(uint64_t filenum,uint64_t blocknum): filenum_(filenum),blocknum_(blocknum){}
};

struct HashFun{
	std::size_t operator()(const SSDCacheKey& ssdcachekey) const {
		uint64_t k = 1315423911;
		uint64_t hash = (ssdcachekey.filenum_ % k)<<10 + (ssdcachekey.blocknum_ % k);
		return hash;
	}
};

struct EqualKey{
	bool operator()(const SSDCacheKey& left, const SSDCacheKey& right) const {
		return (left.filenum_ ==  right.filenum_ && left.blocknum_ == right.blocknum_);
	}
};

struct Bucket{
	uint64_t container_id_;
	int container_index_;
	uint64_t offset_;
	uint64_t size_;
	uint64_t real_size_;

	Bucket(): container_id_(0),container_index_(0),offset_(0),size_(0),real_size_(0){}
};

class Container;

extern Container*  NewContainer(uint64_t  id,uint64_t block_size_,const std::string& ssdname,Env* env);

class Container{
public:
	Container(uint64_t  id,uint64_t block_size_,const std::string& ssdname,Env* env);
	~Container();
	//Container*  NewContainer(uint64_t  id,uint64_t block_size_,const std::string& ssdname,Env* env);
	Status Get(uint64_t offset,uint64_t size,Slice* block);
	Status Put(const Slice& block);
	void ReUse(uint64_t newid_,uint64_t block_size);
	void GetCurrentInfo(uint64_t& offset,uint64_t& block_id){
		offset = usage_;
		block_id = last_block_id_;
	}
	uint64_t GetId(){return id_;}

	bool IsFull(){return if_full_;}

	uint64_t GetOffset(){return usage_;}

	int next_;
	void Ref();
	void UnRef();
private:
	uint64_t  id_;
	int index_;
	uint64_t block_size_;
	uint64_t space_;
	uint64_t usage_;
	uint64_t last_block_id_;
	std::string ssdname_;
	bool if_full_;
	int fd_;
	Env* env_;
	int ref_;
	//mutable port::Mutex mutex_;


	Status Open();
	Status Close();

};

class SSDCache{
public:
				SSDCache(const Options& options, const std::string& ssdname);

				~SSDCache(){}

				Cache::Handle* Insert(const SSDCacheKey& key, const char* value, size_t charge);

				Cache::Handle* Lookup(const SSDCacheKey& key);

				Status Release();

private:
				std::string ssdname_;
				std::unordered_map<SSDCacheKey,Bucket*,HashFun,EqualKey> buckets_;
				std::vector<Container> containers_;
				std::map<uint64_t,int> index_;
				mutable port::Mutex mutex_;
				uint64_t bucket_limit_ = BUCKETLIMIT;
				uint64_t  last_container_id_;
				int  first_free_;
				Options options_;
				Env* env_;

				void Ref(Container* handle);
				void UnRef(Container* handle);
				void Check();
				void GetNewBucket(Bucket* bucket,uint64_t block_size,uint64_t real_size);
				//void Erase(const SSDCacheKey& key);
};


}
#endif  // STORAGE_LEVELDB_DB_SSD_CACHE_H_
