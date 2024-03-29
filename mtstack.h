#ifndef MTSTACK_H
#define MTSTACK_H

#include <Windows.h>

namespace ulock
{
  class InterlockedSizeCounter
  {
  public:
    InterlockedSizeCounter() : size_(0) {}
    unsigned int size() const {return size_;}

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

  class NullSizeCounter
  {
  protected:
    void increment_size(){}
    void decrement_size(){}
  };

  template <typename T>
  class RecyclingNodeAlloc
  {
  protected:
    struct Node
    {
      SLIST_ENTRY entry;
      T obj;
    };

    RecyclingNodeAlloc() : freenodes_(NULL)
    {
      freenodes_ = static_cast<SLIST_HEADER*>(_aligned_malloc(sizeof(SLIST_HEADER), MEMORY_ALLOCATION_ALIGNMENT));
      InitializeSListHead(freenodes_);
    }

    ~RecyclingNodeAlloc()
    {
      while (Node* n = reinterpret_cast<Node*>(InterlockedPopEntrySList(freenodes_)))
      {
        destroy_node(n);
      }
    }

    Node* alloc_node()
    {
      void* free_node = InterlockedPopEntrySList(freenodes_);
      if (!free_node)
      {
        free_node = _aligned_malloc(sizeof(Node), MEMORY_ALLOCATION_ALIGNMENT);
      }
      return static_cast<Node*>(free_node);
    }

    void free_node(Node* node)
    {
      InterlockedPushEntrySList(freenodes_, &node->entry);
    }

    void destroy_node(Node* node)
    {
      _aligned_free(node);
    }

  private:
    SLIST_HEADER* freenodes_;
  };

  template <typename T>
  class BaseNodeAlloc
  {
  protected:
    struct Node
    {
      SLIST_ENTRY entry;
      T obj;
    };

    Node* alloc_node()
    {
      return static_cast<Node*>(_aligned_malloc(sizeof(Node), MEMORY_ALLOCATION_ALIGNMENT));
    }

    void free_node(Node* node)
    {
      _aligned_free(node);
    }

    void destroy_node(Node* node)
    {
      _aligned_free(node);
    }
  };

  template <typename T, typename SizeCounter = InterlockedSizeCounter, typename NodeAlloc = RecyclingNodeAlloc<T> >
  class mtstack : public SizeCounter, protected NodeAlloc
  {
  public:
    typedef T value_type;

    mtstack() : nodes_(NULL)
    {
      nodes_ = static_cast<SLIST_HEADER*>(_aligned_malloc(sizeof(SLIST_HEADER), MEMORY_ALLOCATION_ALIGNMENT));
      InitializeSListHead(nodes_);
    }

    ~mtstack()
    {
      while (Node* n = reinterpret_cast<Node*>(InterlockedPopEntrySList(nodes_)))
      {
        decrement_size();
        n->obj.~T();
        destroy_node(n);
      }
    }

    void push(const T& obj = T())
    {
      Node* new_node = alloc_node();
      ::new (&new_node->obj) T(obj);
      InterlockedPushEntrySList(nodes_, &new_node->entry);
      increment_size();
    }

    bool pop(T& obj)
    {
      Node* n = reinterpret_cast<Node*>(InterlockedPopEntrySList(nodes_));
      if (n)
      {
        decrement_size();
        obj = n->obj;
        n->obj.~T();
        free_node(n);
        return true;
      }
      return false;
    }

    void clear()
    {
      while (Node* n = reinterpret_cast<Node*>(InterlockedPopEntrySList(nodes_)))
      {
        decrement_size();
        n->obj.~T();
        free_node(n);
      }
    }

  private:
    SLIST_HEADER* nodes_;
  };
}

#endif