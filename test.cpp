#define _WIN32_WINNT 0x0502

#include "mtstack.h"

#include <iostream>
#include <list>

using namespace std;

template <typename T>
class critstack
{
public:
  critstack()
  {
    InitializeCriticalSection(&lock);
  }

  ~critstack()
  {
    DeleteCriticalSection(&lock);
  }

  void push(const T& val)
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

//typedef critstack<int> IntStack;
typedef ulock::mtstack<int> IntStack;
const int elem_count = 200;
const int prod_count = 50;

DWORD WINAPI producer(LPVOID param) 
{
  IntStack* stack = static_cast<IntStack*>(param);
  for (int i = 0; i < elem_count; ++i)
  {
    stack->push(i);
    //Sleep(1);
  }
  return 0;
}

DWORD WINAPI consumer(LPVOID param)
{
  IntStack* stack = static_cast<IntStack*>(param);
  int pop_count = 0;
  int val = 0;
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
  IntStack stack;

  LARGE_INTEGER begin;
  QueryPerformanceCounter(&begin);

  cout << "Launch producer threads" << endl;
  HANDLE h_prod[prod_count];
  for (unsigned i = 0; i < prod_count; ++i)
  {
    h_prod[i] = CreateThread(NULL, 0, producer, &stack, 0, NULL);
  }
  cout << "Launch consumer thread" << endl;
  HANDLE h = CreateThread(NULL, 0, consumer, &stack, 0, NULL);
  cout << "Wait for consumer thread to finish" << endl;
  WaitForSingleObject(h, 5000);

  LARGE_INTEGER end;
  QueryPerformanceCounter(&end);
  LARGE_INTEGER freq;
  QueryPerformanceFrequency(&freq);
  cout << 1000 * double(end.QuadPart - begin.QuadPart) / freq.QuadPart << " ms" << endl;

  WaitForMultipleObjects(prod_count, h_prod, TRUE, INFINITE);

  return 0;
}
