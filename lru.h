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
	LRU(IO & io, size_t capacity, float maxLoadFactor = 1) 
		: io(io) {
		keys.max_load_factor(maxLoadFactor);
		keys.reserve(capacity);
	}
	~LRU() {
		for (auto & data : dataChain)
			::operator delete(data.data);
	}
	
	template<typename ValueType>
	void read(const KeyType & key, ValueType & value) {
		auto it = keys.find(key);
		if (it != keys.end()) {
			value = *reinterpret_cast<ValueType *>(it->second->data);
			return;
		}
		io.read(key, value);
		
		if (keys.size() >= capacity()) {
			std::cout << "limit" << std::endl;
			auto it = dataChain.end();
			--it;
			::operator delete(it->data);
			it->data = ::operator new(sizeof(ValueType));
			ValueType & space = *reinterpret_cast<ValueType *>(it->data);
			dataChain.splice(dataChain.begin(), dataChain, it);
			
			KeyIterator old = it->keyRef;
			std::cout << old->first << std::endl;
			keys.erase(old);
			
			space = value;
			auto p = keys.emplace(key, it);
			it->keyRef = p.first;
		} else {
			// Make sure the stored references to keys valid
			// http://stackoverflow.com/questions/16781886/can-we-store-unordered-maptiterator
			assert(keys.size() < (double)keys.max_load_factor() * keys.bucket_count());
			dataChain.emplace_front();
			auto it = dataChain.begin();
			it->data = ::operator new(sizeof(value));
			ValueType & space = *reinterpret_cast<ValueType *>(it->data);
			
			space = value;
			auto p = keys.emplace(key, it);
			it->keyRef = p.first;
		}
	}
	
	size_t capacity() const {
		return keys.max_load_factor() * keys.bucket_count();
	}
	
private:
	struct Data{
		void * data;
		KeyIterator keyRef;
	};
	
	DataList dataChain;
	KeyMap keys;
	IO & io;
};