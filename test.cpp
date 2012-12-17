#define _WIN32_WINNT 0x0502

#include "mtstack.h"

#include <iostream>

using namespace std;

DWORD WINAPI producer(LPVOID param) 
{
  ulock::mtstack<int>* stack = static_cast<ulock::mtstack<int>*>(param);
  for (int i = 0; i < 10000; ++i)
  {
    stack->push(i);
    //Sleep(1);
  }
  return 0;
}

DWORD WINAPI consumer(LPVOID param)
{
  ulock::mtstack<int>* stack = static_cast<ulock::mtstack<int>*>(param);
  int pop_count = 0;
  int val = 0;
  while (pop_count < 9999)
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
  ulock::mtstack<int> stack;

  LARGE_INTEGER begin;
  QueryPerformanceCounter(&begin);

  cout << "Launch producer threads" << endl;
  CreateThread(NULL, 0, producer, &stack, 0, NULL);
  CreateThread(NULL, 0, producer, &stack, 0, NULL);
  CreateThread(NULL, 0, producer, &stack, 0, NULL);
  CreateThread(NULL, 0, producer, &stack, 0, NULL);
  cout << "Launch consumer thread" << endl;
  HANDLE h = CreateThread(NULL, 0, consumer, &stack, 0, NULL);
  cout << "Wait for consumer thread to finish" << endl;
  WaitForSingleObject(h, 5000);

  LARGE_INTEGER end;
  QueryPerformanceCounter(&end);
  LARGE_INTEGER freq;
  QueryPerformanceFrequency(&freq);
  cout << double(end.QuadPart - begin.QuadPart) / freq.QuadPart << " seconds" << endl;

  return 0;
}
