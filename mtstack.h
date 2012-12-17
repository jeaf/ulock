#ifndef MTSTACK_H
#define MTSTACK_H

#include <Windows.h>

namespace ulock
{
  template <typename T>
  class mtstack
  {
  public:
    typedef T value_type;

    mtstack() : nodes(NULL), free_nodes(NULL), _size(0)
    {
      nodes = static_cast<SLIST_HEADER*>(_aligned_malloc(sizeof(SLIST_HEADER), MEMORY_ALLOCATION_ALIGNMENT));
      InitializeSListHead(nodes);
      free_nodes = static_cast<SLIST_HEADER*>(_aligned_malloc(sizeof(SLIST_HEADER), MEMORY_ALLOCATION_ALIGNMENT));
      InitializeSListHead(free_nodes);
    }

    mtstack(unsigned n, const T& val = T()) : nodes(NULL), free_nodes(NULL), _size(0)
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

    ~mtstack()
    {
      while (SLIST_ENTRY* e = InterlockedPopEntrySList(nodes))
      {
        InterlockedDecrement(&_size);
        reinterpret_cast<node*>(e)->obj.~T();
        _aligned_free(e);
      }
      while (SLIST_ENTRY* e = InterlockedPopEntrySList(free_nodes))
      {
        _aligned_free(e);
      }
    }

    void push(const T& obj = T())
    {
      void* free_node = InterlockedPopEntrySList(free_nodes);
      if (!free_node)
      {
        free_node = _aligned_malloc(sizeof(node), MEMORY_ALLOCATION_ALIGNMENT);
      }
      ::new (&static_cast<node*>(free_node)->obj) T(obj);
      InterlockedPushEntrySList(nodes, static_cast<SLIST_ENTRY*>(free_node));
      InterlockedIncrement(&_size);
    }

    bool pop(T& obj)
    {
      SLIST_ENTRY* e = InterlockedPopEntrySList(nodes);
      if (e)
      {
        InterlockedDecrement(&_size);
        obj = reinterpret_cast<node*>(e)->obj;
        reinterpret_cast<node*>(e)->obj.~T();
        InterlockedPushEntrySList(free_nodes, e);
        return true;
      }
      return false;
    }

    unsigned int size() const
    {
      return _size;
    }

    void clear()
    {
      while (SLIST_ENTRY* e = InterlockedPopEntrySList(nodes))
      {
        InterlockedDecrement(&_size);
        reinterpret_cast<node*>(e)->obj.~T();
        InterlockedPushEntrySList(free_nodes, e);
      }
    }

  private:
    struct node
    {
      SLIST_ENTRY entry;
      T obj;
    };
    SLIST_HEADER* nodes;
    SLIST_HEADER* free_nodes;
    volatile LONG _size;
  };
}

#endif