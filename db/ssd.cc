/*
 * ssd.cc
 *
 *  Created on: 2016年8月21日
 *      Author: whc
 */
#include <stdio.h>
#include <stdlib.h>
#include<iostream>

#include "db/ssd.h"

#include "db/filename.h"
#include "leveldb/env.h"
#include "leveldb/slice.h"
#include "util/mutexlock.h"

namespace leveldb{
Container::Container(uint64_t  id,uint64_t block_size,const std::string& ssdname,Env* env)
							:id_(id),
							 block_size_(block_size),
							 ssdname_(ssdname),
							 space_(CONTAINERSIZE),
							 usage_(0),
							 last_block_id_(0),
							 if_full_(false),
							 env_(env),
							 next_(-1),
							 ref_(0){
	std::string fname = ContainerName(ssdname_,id_);
	//std::cout<<fname<<std::endl;
	 WritableFile* file;
	 Status s;
	  s = env->NewWritableFile(fname, &file);
	  if (!s.ok()) {
	      printf("make container error\n");
	  }
	  file->Close();
}

Container::~Container(){

}

void Container::Ref(){
	//MutexLock l(&mutex_);
	ref_++;
}

void Container::UnRef(){
	//MutexLock l(&mutex_);
	ref_--;
}
void Container::ReUse(uint64_t new_id,uint64_t block_size){
	Status s;

	std::string oldfname = ContainerName(ssdname_,id_);
	s = env_->DeleteFile(oldfname);
	if (!s.ok()) {
			      printf("container reuse  delete old file error\n");
	}

	id_ = new_id;
	block_size_ = block_size;
	std::string fname = ContainerName(ssdname_,id_);

	//printf("fname is %s\n",fname.c_str());
		 WritableFile* file;
		  s = env_->NewWritableFile(fname, &file);
		  if (!s.ok()) {
		      printf("container reuse  error\n");
		  }
		  file->Close();
		  space_ =  CONTAINERSIZE;
		  usage_ =  0;
		  last_block_id_ = 0;
		  if_full_ = false;
		  next_ = -1;
		  ref_ = 0;
}

Container* NewContainer(uint64_t  id,uint64_t block_size,const std::string& ssdname,Env* env){
	return new Container(id,block_size,ssdname,env);
}

Status Container::Put(const Slice& block){
	WritableFile* file;
	Status s;
	if (if_full_){
		printf("try to put to a full container\n");
		return s;
	}
	std::string fname = ContainerName(ssdname_,id_);
	 s = env_->NewAppendableFile(fname, &file);
		  if (!s.ok()) {
		      printf( "container put  error\n");
		      return s;
		  }
	if (block.size()  !=  block_size_){
		printf("container block size is wrong\n");
		return s;
	}
	s = file->Append(block);
	if(!s.ok()){
		printf("container put  append error\n ");
	}
	file->Sync();
	file->Close();

	usage_+=block_size_;
	space_-=block_size_;
	if (space_==0){
		if_full_ = true;
	}
	last_block_id_++;

	return s;
}

Status Container::Get(uint64_t offset,uint64_t size,Slice* block){
	RandomAccessFile* file;
	Status s;
	if(offset>usage_){
			printf("try to get a too large offset\n");
			return s;
	}
	std::string fname = ContainerName(ssdname_,id_);
	s = env_->NewRandomAccessFile(fname, &file);
	if (!s.ok()) {
			      printf( "container get open  error\n");
			     std::cout<<fname<<std::endl;
			     std::cout<<ssdname_<<std::endl;
			      return s;
			  }
	char* buf = new char[size];
	s = file->Read(offset,size,block,buf);
	if(!s.ok()){
		printf("get read  from container error\n");
		return s;
	}
	return s;
}

SSDCache::SSDCache(const Options& options, const std::string& ssdname):
	ssdname_(ssdname),
	options_(options),
	last_container_id_(0),
	first_free_(-1),
	env_(Env::Default()){

		uint64_t block_minsize = 1<<12;

		uint64_t block_size =  block_minsize;
		while(block_size<=CONTAINERSIZE){
			index_.insert(std::make_pair(block_size,-1));
			block_size*=2;
		}

		for(int i=0;i<CONTAINERNUM;i++){
			Container* c = NewContainer(last_container_id_++,block_minsize,ssdname, env_);
			if (i<CONTAINERNUM-1)
				c->next_ = i+1;
			containers_.push_back(*c);
			int len = containers_.size();
			//printf("len =%d,next = %d\n",len, containers_[len-1].next_);
		}

		first_free_ = 0;

		block_size =  block_minsize;
		while(block_size<=CONTAINERSIZE){
				//printf("SSDCache(): block_size = %lu\n",block_size);
				Bucket* bucket = new Bucket();
				GetNewBucket(bucket,block_size,block_size);
				index_[block_size] = bucket->container_index_;
				block_size*=2;
		}
}

void SSDCache::Ref(Container* handle){
	MutexLock l(&mutex_);
	handle->Ref();
}

void SSDCache::UnRef(Container* handle){
	MutexLock l(&mutex_);
	handle->UnRef();
}

Status SSDCache::Release(){
	Status s;
	if(!s.ok()){
		printf("Release error!\n");
	}
	return s;
}

void  SSDCache::Check(){
	return ;
}

void SSDCache::GetNewBucket(Bucket* bucket,uint64_t block_size,uint64_t real_size){

	int index_cur = index_[block_size];
	if(index_cur>=0 && !containers_[index_cur].IsFull()){
		bucket->container_id_ =  containers_[index_cur].GetId();
		bucket->container_index_ =  index_cur;
		bucket ->size_ = block_size;
		bucket->real_size_ = real_size;
		bucket->offset_ = containers_[index_cur].GetOffset();
		return ;
	}

	if(first_free_ ==  -1){
		Status s = Release();
				if(!s.ok()){
					printf("GetNewBucket Release error\n");
				}
	}

		bucket->container_index_ = first_free_;
		bucket->container_id_ = last_container_id_++;
		bucket->size_ = block_size;
		bucket->real_size_ = real_size;
		bucket->offset_ = 0;

		first_free_ = containers_[bucket->container_index_].next_;
		//printf("GetNewBuckt: bucket->container_index_ = %d\n",bucket->container_index_);

		containers_[bucket->container_index_].ReUse(bucket->container_id_,block_size);
		//printf("GetNewBuckt: first_free_ = %d\n",first_free_);
		containers_[bucket->container_index_].next_ = index_[block_size];
		index_[block_size] =bucket->container_index_ ;

}


Cache::Handle*  SSDCache::Insert(const SSDCacheKey& key, const char* value, size_t charge){
		Bucket* bucketCur;

		uint64_t block_size = 1;
		while(block_size < charge){
				block_size*=2;
		}
		if(block_size<4096)
				block_size = 4096;

		if(buckets_.find(key)!=buckets_.end()){
			bucketCur = buckets_[key];
			if(containers_[bucketCur->container_index_].GetId() != bucketCur->container_id_){
					bucketCur = new Bucket();
					GetNewBucket(bucketCur,block_size,charge);
					buckets_.erase(key);
					buckets_.insert(std::make_pair(key,bucketCur));
			}
		}else{
			bucketCur = new Bucket();
			GetNewBucket(bucketCur,block_size,charge);
			buckets_.insert(std::make_pair(key,bucketCur));
			if(buckets_.size()>bucket_limit_)
					Check();
		}

		Container c = containers_[bucketCur->container_index_];
		if(c.GetId()!=bucketCur->container_id_){
			printf("SSDCache insert make bucket  error\n");
		}

		char contentspace[block_size];
		memcpy(contentspace,value,charge);
		Slice content(contentspace,block_size);
		Status s = c.Put(content);
		if(!s.ok()){
			printf("SSDCache insert error\n");
		}

		return reinterpret_cast<Cache::Handle*>(bucketCur);
}

Cache::Handle* SSDCache::Lookup(const SSDCacheKey& key){
	Bucket* bucketCur=NULL;
	Slice*  content = new Slice();
	if(buckets_.find(key) == buckets_.end()){
		return NULL;
	}else{
		bucketCur = buckets_[key];
		Container c = containers_[bucketCur->container_index_];
		if(c.GetId() != bucketCur->container_id_)
				return NULL;
		Status s = c.Get(bucketCur->offset_,bucketCur->real_size_,content);
		if(!s.ok()){
					printf("SSDCache lookup error\n");
				}
	}
	return reinterpret_cast<Cache::Handle*>(content);
}

}



