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
	IO io({1,2,3,4},{1.1,1.2,1.3,1.4});
	LRU<IO> cache(io, 4);
	for (int i = 0 ; i < 8 ; ++i) {
		if (i % 2 == 0) {
			int x;
			cache.read(i, x);
			std::cout << i << " int: " << x << std::endl;
		} else {
			float x;
			cache.read(i, x);
			std::cout << i << " float: " << x << std::endl;
		}
	}
	for (int i = 8 ; i --> 0 ;) {
		if (i % 2 == 0) {
			int x;
			cache.read(i, x);
			std::cout << i << " int: " << x << std::endl;
		} else {
			float x;
			cache.read(i, x);
			std::cout << i << " float: " << x << std::endl;
		}
	}
	
	return 0;
}