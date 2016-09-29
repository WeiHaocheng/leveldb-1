#include <iostream> 
#include <string> 
#include <assert.h> 
#include "leveldb/db.h" 
using namespace std; 


int main(void) { 
	leveldb::DB *db; 
	leveldb::Options options; 
	options.create_if_missing = true; 
	
	// open 
	leveldb::Status status = leveldb::DB::Open(options,"/tmp/testdb", &db); 
	assert(status.ok()); 
	string key = "testkey"; 
	string value = "testvalue";
	cout << "###start test:" << endl;
	
	// write 
	cout << "put: " << key << "->" << value << endl;
	status = db->Put(leveldb::WriteOptions(), key, value); 
	assert(status.ok()); 
	
	// read
	cout << "get key: " << key << endl;
	status = db->Get(leveldb::ReadOptions(), key, &value); 
	assert(status.ok()); 
	cout<< "find value: " << value<<endl; 
	
	// delete 
	cout << "delete key: " << key << endl;
	status = db->Delete(leveldb::WriteOptions(), key); 
	assert(status.ok()); 
	status = db->Get(leveldb::ReadOptions(),key, &value); 
	if(!status.ok()) { 
		cerr << key << " " << status.ToString() << endl; 
	} else { 
		cout << "after delete: " << key << "->" << value << endl; 
	} 

	// close 
	cout << "all test passed. close db..." << endl;
	delete db; 
	return 0; 
}
