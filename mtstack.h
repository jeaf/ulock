#ifndef MTSTACK_H
#define MTSTACK_H

#include <Windows.h>

namespace ulock
{
  template <typename T>
  class mtstack
  {
  public:
    mtstack() : nodes(NULL), free_nodes(NULL)
    {
      nodes = static_cast<SLIST_HEADER*>(_aligned_malloc(sizeof(SLIST_HEADER), MEMORY_ALLOCATION_ALIGNMENT));
      InitializeSListHead(nodes);
      free_nodes = static_cast<SLIST_HEADER*>(_aligned_malloc(sizeof(SLIST_HEADER), MEMORY_ALLOCATION_ALIGNMENT));
      InitializeSListHead(free_nodes);
    }

    mtstack(unsigned n, const T& val = T()) : nodes(NULL), free_nodes(NULL)
    {
      nodes = static_cast<SLIST_HEADER*>(_aligned_malloc(sizeof(SLIST_HEADER), MEMORY_ALLOCATION_ALIGNMENT));
      InitializeSListHead(nodes);
      free_nodes = static_cast<SLIST_HEADER*>(_aligned_malloc(sizeof(SLIST_HEADER), MEMORY_ALLOCATION_ALIGNMENT));
      InitializeSListHead(free_nodes);
      for (unsigned i = 0; i < n; ++i)
      {
        push(val);
      }
    }

    void push(const T& obj)
    {
      void* free_node = InterlockedPopEntrySList(free_nodes);
      if (!free_node)
      {
        free_node = _aligned_malloc(sizeof(node), MEMORY_ALLOCATION_ALIGNMENT);
      }
      static_cast<node*>(free_node)->obj = obj;
      InterlockedPushEntrySList(nodes, static_cast<SLIST_ENTRY*>(free_node));
    }

    bool pop(T& obj)
    {
      SLIST_ENTRY* e = InterlockedPopEntrySList(nodes);
      if (e)
      {
        obj = reinterpret_cast<node*>(e)->obj;
        InterlockedPushEntrySList(free_nodes, e);
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
    SLIST_HEADER* nodes;
    SLIST_HEADER* free_nodes;
  };
}

#endif