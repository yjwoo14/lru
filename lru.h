#ifndef LRU_H
#define LRU_H

#include <unordered_map>
#include <list>
#include <cassert>

namespace bits {

template <typename IO>
struct Data {
	typedef typename IO::KeyType KeyType;
	typedef std::list<Data> DataList;
	typedef std::unordered_map<KeyType, typename DataList::iterator> KeyMap;
	typedef typename KeyMap::iterator KeyIterator;

	void * data;
	KeyIterator keyRef;
};

template <typename IO>
struct DirtyData : public Data<IO> {
	bool dirty;
};

template <typename IO, typename Data>
class LRUBase {
private:
	typedef typename IO::KeyType KeyType;
	typedef typename Data::DataList DataList;
	typedef typename Data::KeyMap KeyMap;
	typedef typename KeyMap::iterator KeyIterator;
	
public:
#ifndef NDEBUG
	size_t hit, miss;
#endif

	template <typename ...T>
	LRUBase(size_t capacity, T... params) 
		: _capacity(capacity), io(params...) {
		keys.reserve(capacity);
		assert(capacity <= keys.max_load_factor() * keys.bucket_count());
#ifndef NDEBUG
		hit = 0, miss = 0;
#endif
	}

	~LRUBase() {
		for (auto & d : data) {
			kick(d);
			::operator delete(d.data);
		}
	}
	
	template <typename ValueType>
	void read(const KeyType & key, ValueType & value) {
		auto it = keys.find(key);
		if (it != keys.end()) {
#ifndef NDEBUG
			hit++;
#endif
			value = *reinterpret_cast<ValueType *>(it->second->data);
			data.splice(data.begin(), data, it->second);
			return;
		}
#ifndef NDEBUG
		miss++;
#endif

		io.read(key, value);
		
		if (keys.size() >= capacity()) {
			auto it = --data.end();
			kick(*it);
			::operator delete(it->data);
			// reuse data space in back()
			data.splice(data.begin(), data, it);
			
			KeyIterator old = it->keyRef;
			keys.erase(old);
		} else {
			// Make sure the stored references to keys valid
			// http://stackoverflow.com/questions/16781886/can-we-store-unordered-maptiterator
			assert(keys.size() < (double)keys.max_load_factor() * keys.bucket_count());
			data.emplace_front();
		}
		auto begin = data.begin();
		begin->data = ::operator new(sizeof(value));
		ValueType & storage = *reinterpret_cast<ValueType *>(begin->data);
		
		storage = value;
		auto p = keys.emplace(key, begin);
		begin->keyRef = p.first;
	}

	size_t capacity() const {
		return _capacity;
	}

	size_t size() const {
		return keys.size();
	}
	
private:
	virtual void kick(const Data & target) {}

	const size_t _capacity;
	DataList data;
	KeyMap keys;
	IO io;
};

}

template <typename IO>
class ReadOnlyLRU : public bits::LRUBase<IO, bits::Data<IO>> {
public:
	typedef bits::LRUBase<IO, bits::Data<IO>> LRUBase;
	template <typename ...T>
	ReadOnlyLRU(T... params) : LRUBase(params...) {}
private:
	typedef bits::Data<IO> Data;
	void kick(const Data & target) {}
};

#endif
