#define _WIN32_WINNT 0x0502

#include "mtstack.h"

#include <iostream>

using namespace std;

int main()
{
  ulock::mtstack<int> stack;

  stack.push(1);
  stack.push(2);
  stack.push(3);
  stack.push(4);
  stack.push(5);
  int val = 0;
  while (stack.pop(val))
  {
    cout << val << endl;
  }
  return 0;
}
