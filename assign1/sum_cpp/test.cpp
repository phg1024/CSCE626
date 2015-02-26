#include <iostream>
#include <vector>
#include <algorithm>
#include <iterator>
#include <numeric>
using namespace std;

// prefix scan function
template <typename Iter, typename Func, typename T>
inline void scanl(Iter start, Iter end, Func f, T init) {
  T acc = 0;
  while( start <= end ) {
    acc = f(acc, *start);
    *start = acc;    
    ++start;
  }  
}

template <typename Iter, typename Func>
inline void map(Iter start, Iter end, Func f) {
  while( start <= end ) {
    f(*start);
    ++start;
  }
}

int main(int argc, char **argv) {
  int n = 10;
  vector<int> v(n);
  for(int i=0;i<n;++i) v[i] = i;
  
  vector<int> p1 = v;
  scanl(p1.begin(), p1.end(), [](int a, int b) { return a+b; }, 0);
    
  vector<int> p2 = v;
  for(int i=1;i<p2.size();++i) {
    p2[i] += p2[i-1];
  }
  
  vector<int> p3(v.size());
  std::partial_sum(v.begin(), v.end(), p3.begin(), std::plus<int>());
  
  vector<int> p4 = v;
  map(p4.begin(), p4.end(), [](int &a) { a+=3; });
  
  std::ostream_iterator<int> out_it (std::cout," ");
  
  std::copy ( p1.begin(), p1.end(), out_it );
  cout << endl;

  std::copy ( p2.begin(), p2.end(), out_it );
  cout << endl;

  std::copy ( p3.begin(), p3.end(), out_it );    
  cout << endl;
  
  std::copy ( p4.begin(), p4.end(), out_it );    
  cout << endl;  
  return 0;
}
