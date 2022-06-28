#include <iostream>
#include <deque>
#include <string>

using namespace std;

namespace hls {
template<typename T>
class stream {
private:
  deque<T> q; 
  string name;
public:

  stream(string s) : name(s) {}
  stream() : name("") {}

  void operator<<(T x) {
    q.push_back(x);
  }

  void operator>>(T& x) {
    x = q.front();
    q.pop_front();
    return; 
  }

  T read() { 
    T temp = q.front();
    q.pop_front();
    return temp; 
  }

};
}

#include "krnl_vadd.cpp"

int main() {
	hls::stream<int> s1;
  s1 << 123;
  s1 << 234;
  int r; 
  s1 >> r; 
  std::cout << r << std::endl; 
  s1 >> r; 
  std::cout << r << std::endl; 
  s1 >> r;
  std::cout << r << std::endl; 

  uint32_t a[] = {1, 2, 3, 4};
  uint32_t b[] = {1,1,1,1};
  uint32_t c[4];

  krnl_vadd(a,b,c,4);
  for(int i=0;i<4; i++) 
    std::cout << c[i]; 
  std::cout << std::endl;

  return(0);
}
