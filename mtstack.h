#ifndef MTSTACK_H
#define MTSTACK_H

#include <Windows.h>

namespace ulock
{
  class InterlockedSizeCounter
  {
  public:
    InterlockedSizeCounter() : size_(0) {}
    unsigned int size() {return size_;}

  protected:
    void increment_size()
    {
      InterlockedIncrement(&size_);
    }

    void decrement_size()
    {
      InterlockedDecrement(&size_);
    }

  private:
    volatile LONG size_;
  };

  template <typename T, typename SizeCounter = InterlockedSizeCounter>
  class mtstack : public SizeCounter
  {
  public:
    typedef T value_type;

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

    ~mtstack()
    {
      while (SLIST_ENTRY* e = InterlockedPopEntrySList(nodes))
      {
        decrement_size();
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
      increment_size();
    }

    bool pop(T& obj)
    {
      SLIST_ENTRY* e = InterlockedPopEntrySList(nodes);
      if (e)
      {
        decrement_size();
        obj = reinterpret_cast<node*>(e)->obj;
        reinterpret_cast<node*>(e)->obj.~T();
        InterlockedPushEntrySList(free_nodes, e);
        return true;
      }
      return false;
    }

    void clear()
    {
      while (SLIST_ENTRY* e = InterlockedPopEntrySList(nodes))
      {
        decrement_size();
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
  };
}

#endif