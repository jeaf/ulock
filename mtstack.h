#ifndef MTSTACK_H
#define MTSTACK_H

#include <Windows.h>

namespace ulock
{
  template <typename T>
  class mtstack
  {
  public:
    mtstack() : header(NULL)
    {
      header = static_cast<SLIST_HEADER*>(_aligned_malloc(sizeof(SLIST_HEADER), MEMORY_ALLOCATION_ALIGNMENT));
      InitializeSListHead(header);
    }

    void push(const T& obj)
    {
      node* i = static_cast<node*>(_aligned_malloc(sizeof(node), MEMORY_ALLOCATION_ALIGNMENT));
      i->obj = obj;
      InterlockedPushEntrySList(header, &(i->entry));
    }

    bool pop(T& obj)
    {
      SLIST_ENTRY* e = InterlockedPopEntrySList(header);
      if (e)
      {
        obj = reinterpret_cast<node*>(e)->obj;
        return true;
      }
      return false;
    }

  private:
    struct node
    {
      SLIST_ENTRY entry;
      T obj;
    };
    SLIST_HEADER* header;
  };
}

#endif