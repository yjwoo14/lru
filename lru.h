#ifndef LRU_H
#define LRU_H

#include <unordered_map>
#include <list>
#include <cassert>

template <typename IO>
class LRU {
private:
	struct Data;
	typedef typename IO::KeyType KeyType;
	typedef std::list<Data> DataList;
	typedef std::unordered_map<KeyType, typename DataList::iterator> KeyMap;
	typedef typename KeyMap::iterator KeyIterator;
	
public:
#ifndef NDEBUG
	size_t hit, miss;
#endif

	template <typename ...T>
	LRU(size_t capacity, T... params) 
		: _capacity(capacity), io(params...) {
		keys.reserve(capacity);
		assert(capacity <= keys.max_load_factor() * keys.bucket_count());
#ifndef NDEBUG
		hit = 0, miss = 0;
#endif
	}

	~LRU() {
		for (auto & d : data)
			::operator delete(d.data);
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
			auto it = data.end();
			--it;
			::operator delete(it->data);
			it->data = ::operator new(sizeof(ValueType));
			ValueType & space = *reinterpret_cast<ValueType *>(it->data);
			data.splice(data.begin(), data, it);
			
			KeyIterator old = it->keyRef;
			keys.erase(old);
			
			space = value;
			auto p = keys.emplace(key, it);
			it->keyRef = p.first;
		} else {
			// Make sure the stored references to keys valid
			// http://stackoverflow.com/questions/16781886/can-we-store-unordered-maptiterator
			assert(keys.size() < (double)keys.max_load_factor() * keys.bucket_count());
			data.emplace_front();
			auto it = data.begin();
			it->data = ::operator new(sizeof(value));
			ValueType & space = *reinterpret_cast<ValueType *>(it->data);
			
			space = value;
			auto p = keys.emplace(key, it);
			it->keyRef = p.first;
		}
	}

	size_t capacity() const {
		return _capacity;
	}

	size_t size() const {
		return keys.size();
	}
	
private:
	struct Data{
		void * data;
		KeyIterator keyRef;
	};
	
	const size_t _capacity;
	DataList data;
	KeyMap keys;
	IO io;
};

#endif
