#include <iostream>

#include <hls_stream.hpp>

int main() {
	hls::stream<int> s1;
/*
  s1 << 123;
  s1 << 234;
  int r; 
  s1 >> r; 
  std::cout << r << std::endl; 
  s1 >> r; 
  std::cout << r << std::endl; 
  s1 >> r;
  std::cout << r << std::endl; 
*/

  uint32_t a[] = {1, 2, 3, 4};
  uint32_t b[] = {1,1,1,1};
  uint32_t c[4];

  krnl_vadd(a,b,c,4);
  for(int i=0;i<4; i++) 
    std::cout << c[i] << " "; 
  std::cout << std::endl;
  std::cout << "Should be 2 3 4 5 " << std::endl;

  return(0);
}
