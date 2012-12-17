#define _WIN32_WINNT 0x0502

#include "mtstack.h"

#include <iostream>
#include <list>

using namespace std;

struct LargeStruct
{
  LargeStruct()
  {
    a = 3;
    b[22] = 234234.234;
    b[33] = 9999.22;
    c[30] = 'c';
  }
  int a;
  double b[50];
  char c[50];
  char d[50];
  char e[50];
};

template <typename T>
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

private:
  CRITICAL_SECTION lock;
  list<T> mList;
};

//typedef critstack<int> TStack;
//typedef critstack<LargeStruct> TStack;
//typedef ulock::mtstack<int> TStack;
typedef ulock::mtstack<LargeStruct> TStack;
const int elem_count = 2000;
const int prod_count = 100;

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

int main()
{
  double total = 0.0;
  const unsigned loop_count = 10;
  for (unsigned kk = 0; kk < loop_count; ++kk)
  {
    TStack stack;

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

  return 0;
}
