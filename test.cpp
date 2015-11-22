#include <iostream>
#include <unordered_map>
#include <vector>
#include "lru.h"

using namespace std;

class IO {
public:
	typedef int KeyType;
	std::vector<int> items1;
	std::vector<float> items2;
	
	IO(const std::vector<int> & l1
	  ,const std::vector<float> & l2)
		: items1(l1), items2(l2) {} 
	
	void read(int x, void * dest, size_t size) const {
		if ((x % 2) == 0) {
			int & d = *reinterpret_cast<int *>(dest);
			d = items1[x/2];
			return;
		}
		float & d = *reinterpret_cast<float *>(dest);
		d = items2[x/2];
	}
	
	void write(int x, void * src, size_t size) {
		if ((x % 2) == 0) {
			int & s = *reinterpret_cast<int *>(src);
			items1[x/2] = s;
			return;
		}
		float & s = *reinterpret_cast<float *>(src);
		items2[x/2] = s;
	}
};

int main (int argc, char const *argv[])
{
	std::vector<int> a = {1,2,3,4};
	std::vector<float> b = {1.1,1.2,1.3,1.4};
	LRU<IO> cache(4, a, b);
	std::vector<size_t> acc = {0,1,2,3,0,4,5,6,0,7};
	int  c;
	float d;
	for (size_t i = 0 ; i < acc.size() ; ++i) {
		if ((acc[i] % 2) == 0) {
			cache.read(acc[i], &c, sizeof(int));
			std::cout << acc[i] << " -> " << c << ", ";
		} else {
			cache.read(acc[i], &d, sizeof(float));
			std::cout << acc[i] << " -> " << d << ", ";
		}
	}
	std::cout << std::endl;
	std::cout << cache.capacity() << " " << cache.hit << " " << cache.miss << std::endl;
	
	return 0;
}
