#pragma once

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

