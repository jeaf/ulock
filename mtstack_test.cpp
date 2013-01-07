#define _WIN32_WINNT 0x0502

#include "mtstack.h"

#include <iostream>
#include <list>
#include <sstream>
#include <vector>

using namespace std;
using namespace ulock;

struct ComplexType
{
  ComplexType()
  {
    v.push_back(3234.234);
  }

  ComplexType(const ComplexType& other) : v(other.v), s(other.s)
  {
    v = other.v;
  }

  ~ComplexType()
  {
  }

  ComplexType& operator=(const ComplexType& other)
  {
    v = other.v;
    return *this;
  }

  int a;
  double b;
  char c[50];
  std::vector<double> v;
  std::string s;
};

template <typename T, typename C = list<T> >
class critstack
{
public:
  typedef T value_type;

  critstack()
  {
    InitializeCriticalSection(&lock);
  }

  ~critstack()
  {
    DeleteCriticalSection(&lock);
  }

  void push(const T& val = T())
  {
    EnterCriticalSection(&lock);
    mList.push_back(val);
    LeaveCriticalSection(&lock);
  }

  bool pop(T& val)
  {
    EnterCriticalSection(&lock);
    if (!mList.empty())
    {
      val = mList.back();
      mList.pop_back();
      LeaveCriticalSection(&lock);
      return true;
    }
    else
    {
      LeaveCriticalSection(&lock);
      return false;
    }
  }

  void clear()
  {
    EnterCriticalSection(&lock);
    mList.clear();
    LeaveCriticalSection(&lock);
  }

private:
  CRITICAL_SECTION lock;
  C mList;
};

//typedef critstack<int> TStack;
//typedef critstack<ComplexType> TStack;
//typedef critstack<ComplexType, vector<ComplexType> > TStack;
//typedef mtstack<int> TStack;
//typedef mtstack<ComplexType, NullSizeCounter, RecyclingNodeAlloc<ComplexType> > TStack;
typedef mtstack<ComplexType, InterlockedSizeCounter, RecyclingNodeAlloc<ComplexType> > TStack;
//typedef mtstack<ComplexType, InterlockedSizeCounter, BaseNodeAlloc<ComplexType> > TStack;
//typedef mtstack<ComplexType> TStack;
const int elem_count = 2000;
const int prod_count = 50;

DWORD WINAPI producer(LPVOID param) 
{
  TStack* stack = static_cast<TStack*>(param);
  for (int i = 0; i < elem_count; ++i)
  {
    stack->push();
    //Sleep(1);
  }
  return 0;
}

DWORD WINAPI consumer(LPVOID param)
{
  TStack* stack = static_cast<TStack*>(param);
  int pop_count = 0;
  TStack::value_type val;
  while (pop_count < prod_count * elem_count)
  {
    if (stack->pop(val))
    {
      ++pop_count;
    }
  }

  return 0;
}

void test_perf()
{
  double total = 0.0;
  const unsigned loop_count = 10;
  TStack stack;
  for (unsigned kk = 0; kk < loop_count; ++kk)
  {
    stack.clear();

    LARGE_INTEGER begin;
    QueryPerformanceCounter(&begin);

    // Create consumer
    HANDLE h = CreateThread(NULL, 0, consumer, &stack, 0, NULL);

    // Create all producers
    HANDLE h_prod[prod_count];
    for (unsigned i = 0; i < prod_count; ++i)
    {
      h_prod[i] = CreateThread(NULL, 0, producer, &stack, 0, NULL);
    }
    WaitForSingleObject(h, 5000);

    LARGE_INTEGER end;
    QueryPerformanceCounter(&end);
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    double cur_time = 1000 * double(end.QuadPart - begin.QuadPart) / freq.QuadPart;
    total += cur_time;
    cout  << cur_time << " ms" << endl;

    WaitForMultipleObjects(prod_count, h_prod, TRUE, INFINITE);
  }

  cout << "Average: " << total/loop_count << endl;
}

void test_constructor_destructor()
{
  ComplexType ct;

  cout << "--------------------------" << endl;

  {
    cout << "mtstack:" << endl;
    mtstack<ComplexType> s;
    cout << "size: " << s.size() << endl;
    s.push(ct);
    cout << "size: " << s.size() << endl;
    s.pop(ct);
    cout << "size: " << s.size() << endl;
    s.pop(ct);
    cout << "size: " << s.size() << endl;
  }
  
  cout << "--------------------------" << endl;
  
  {
    cout << "std::list:" << endl;
    std::list<ComplexType> li;
    li.push_back(ct);
  }
  
  cout << "--------------------------" << endl;
}

#define ULOCK_EXPECT(cond) if (!(cond)) {\
  ostringstream oss; \
  oss << __FILE__ << "(" << __LINE__ << "): condition '" << #cond << "' was false"; \
  throw runtime_error(oss.str());}

void unit_test()
{
  cout << "Running unit tests..." << endl;
  try
  {
    mtstack<int> s;
    s.push(3);
    ULOCK_EXPECT(s.size() == 1);
    int x;
    ULOCK_EXPECT(s.pop(x));
    ULOCK_EXPECT(x == 3);
    ULOCK_EXPECT(!s.pop(x));
    s.push(4);
    s.push(5);
    s.push(6);
    ULOCK_EXPECT(s.pop(x));
    ULOCK_EXPECT(x == 6);
    ULOCK_EXPECT(s.pop(x));
    ULOCK_EXPECT(x == 5);
    ULOCK_EXPECT(s.pop(x));
    ULOCK_EXPECT(x == 4);
    ULOCK_EXPECT(!s.pop(x));
    s.push(7);
    s.push(8);
    s.push(9);
    s.clear();
    ULOCK_EXPECT(!s.pop(x));
  }
  catch (const exception& ex)
  {
    cout << "Failure: " << ex.what() << endl;
    return;
  }
  cout << "All tests successful." << endl;
}

int main()
{
  //test_constructor_destructor();
  //test_perf();
  unit_test();
  return 0;
}
