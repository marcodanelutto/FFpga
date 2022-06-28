#include <iostream>
#include <deque>

using namespace std;

template<typename T>
class hls_stream {
private:
  deque<T> q; 
public:

  void operator<<(T x) {
    q.push_back(x);
  }

  void operator>>(T& x) {
    x = q.front();
    q.pop_front();
    return; 
  }

};

int main() {
  hls_stream<int> s1;
  s1 << 123;
  s1 << 234;
  int r; 
  s1 >> r; 
  std::cout << r << std::endl; 
  s1 >> r; 
  std::cout << r << std::endl; 
  s1 >> r;
  std::cout << r << std::endl; 
  return(0);
}
