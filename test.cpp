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
	
	void read(int x, int & s) const {
		s = items1[x/2];
	}
	
	void read(int x, float & s) const {
		s = items2[x/2];
	}
	
	void write(int x, const int & s) {
		items1[x/2] = s;
	}
	
	void write(int x, const float & s) {
		items2[x/2] = s;
	}
};

int main (int argc, char const *argv[])
{
	std::vector<int> a = {1,2,3,4};
	std::vector<float> b = {1.1,1.2,1.3,1.4};
	ReadOnlyLRU<IO> cache(4, a, b);
	std::vector<size_t> acc = {0,1,2,3,0,4,5,6,0,7};
	int  c;
	float d;
	for (size_t i = 0 ; i < acc.size() ; ++i) {
		if ((acc[i] % 2) == 0) {
			cache.read(acc[i], c);
			std::cout << acc[i] << " -> " << c << ", ";
		} else {
			cache.read(acc[i], d);
			std::cout << acc[i] << " -> " << d << ", ";
		}
	}
	std::cout << std::endl;
	std::cout << cache.capacity() << " " << cache.hit << " " << cache.miss << std::endl;
	
	return 0;
}
