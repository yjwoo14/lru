#ifndef LRU_H
#define LRU_H

#include <unordered_map>
#include <list>
#include <cassert>
#include <algorithm>

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
struct DirtyData {
	typedef typename IO::KeyType KeyType;
	typedef std::list<DirtyData> DataList;
	typedef std::unordered_map<KeyType, typename DataList::iterator> KeyMap;
	typedef typename KeyMap::iterator KeyIterator;

	DirtyData(): dirty(false) {}

	void * data;
	KeyIterator keyRef;
	bool dirty:1;
	size_t size:31;
};

template <typename IO, typename D, typename L>
class LRUBase {
private:
	typedef D Data;
	typedef typename Data::DataList DataList;
	typedef typename Data::KeyMap KeyMap;
	typedef typename KeyMap::iterator KeyIterator;
	
public:
	typedef typename IO::KeyType KeyType;

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
			static_cast<L *>(this)->kick(d);
			::operator delete(d.data);
		}
	}
	
	void read(const KeyType & key, void * value, size_t size) {
		auto it = keys.find(key);
		if (it != keys.end()) {
#ifndef NDEBUG
			hit++;
#endif
			std::copy(reinterpret_cast<const uint8_t*>(it->second->data), 
					  reinterpret_cast<const uint8_t*>(it->second->data)+size, 
					  reinterpret_cast<uint8_t*>(value));
			data.splice(data.begin(), data, it->second);
			return;
		}
#ifndef NDEBUG
		miss++;
#endif

		io.read(key, value, size);
		
		if (keys.size() >= capacity()) {
			auto it = --data.end();
			static_cast<L*>(this)->kick(*it);
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
		begin->data = ::operator new(size);
		std::copy(reinterpret_cast<const uint8_t *>(value), 
				  reinterpret_cast<const uint8_t *>(value) + size, 
				  reinterpret_cast<uint8_t *>(begin->data));
		auto p = keys.emplace(key, begin);
		begin->keyRef = p.first;
	}

	size_t capacity() const {
		return _capacity;
	}

	size_t size() const {
		return keys.size();
	}

	virtual void write(const KeyType & key, const void * value, size_t size) {}
	
private:
	template <typename>
	friend class ReadOnlyLRU;

	template <typename>
	friend class LRU;

	virtual void kick(const Data & target) {}

	const size_t _capacity;
	DataList data;
	KeyMap keys;
	IO io;
};

}

template <typename IO>
class ReadOnlyLRU : public bits::LRUBase<IO, bits::Data<IO>, ReadOnlyLRU<IO>> {
private:
	typedef bits::LRUBase<IO, bits::Data<IO>, ReadOnlyLRU<IO>> LRUBase;
	typedef typename LRUBase::Data Data;

public:
	typedef typename LRUBase::KeyType KeyType;
	template <typename ...T>
	ReadOnlyLRU(T... params) : LRUBase(params...) {}

private:
	template <typename, typename, typename>
	friend class bits::LRUBase;

	void kick(const Data & target) override {}
};

template <typename IO>
class LRU : public bits::LRUBase<IO, bits::DirtyData<IO>, LRU<IO>> {
private:
	typedef bits::LRUBase<IO, bits::DirtyData<IO>, LRU<IO>> LRUBase;
	typedef typename LRUBase::Data Data;
	typedef typename Data::KeyIterator KeyIterator;

public:
	typedef typename LRUBase::KeyType KeyType;
	template <typename ...T>
	LRU(T... params) : LRUBase(params...) {}

	virtual void write(const KeyType & key, const void * value, size_t size) override {
		auto & keys = static_cast<LRUBase*>(this)->keys;
		auto & data = static_cast<LRUBase*>(this)->data;
		auto it = keys.find(key);
		if (it != keys.end()) {
			std::copy(reinterpret_cast<const uint8_t*>(value), 
					  reinterpret_cast<const uint8_t*>(value)+size, 
					  reinterpret_cast<uint8_t*>(it->second->data));
			data.splice(data.begin(), data, it->second);
			data.begin()->dirty = true;
			return;
		}

		if (keys.size() >= LRUBase::capacity()) {
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
		begin->data = ::operator new(size);
		std::copy(reinterpret_cast<const uint8_t *>(value), 
				  reinterpret_cast<const uint8_t *>(value) + size, 
				  reinterpret_cast<uint8_t *>(begin->data));
		auto p = keys.emplace(key, begin);
		begin->keyRef = p.first;
		data.begin()->dirty = true;
	}

private:
	template <typename, typename, typename>
	friend class bits::LRUBase;

	void kick(const Data & target) override {
		if (!target.dirty) return;
		const KeyType & key = target.keyRef->first;
		LRUBase::io.write(key, target.data, target.size);
	}

};

#endif
