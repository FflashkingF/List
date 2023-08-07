#pragma once
#include <iostream>
#include <memory>
#include <type_traits>

template <typename T, typename Alloc = std::allocator<T>>
class List {
 private:
  struct BaseNode {
    BaseNode* next = this;  // or nullptr
    BaseNode* prev = this;  // or nullptr
  };
  struct Node : BaseNode {
    T val;
    Node(const T& value) noexcept(std::is_nothrow_copy_constructible_v<T>)
        : val(value) {}
    Node() noexcept(std::is_nothrow_default_constructible_v<T>) = default;
    Node(T&& value) noexcept(std::is_nothrow_move_constructible_v<T>)
        : val(std::move(value)) {}  // noexcept is not necessary so that's it
  };

  using NodeAllocator =
      typename std::allocator_traits<Alloc>::template rebind_alloc<Node>;
  using AllocTraits =
      typename std::allocator_traits<Alloc>::template rebind_traits<Node>;

  [[no_unique_address]] NodeAllocator alloc;
  size_t sz = 0;
  BaseNode fakeNode;

  void default_push() {
    Node* new_node = AllocTraits::allocate(alloc, 1);
    try {
      AllocTraits::construct(alloc, new_node);
    } catch (...) {
      AllocTraits::deallocate(alloc, new_node, 1);
      throw;
    }
    ++sz;
    new_node->next = fakeNode.next;
    new_node->prev = &fakeNode;
    fakeNode.next->prev = new_node;
    fakeNode.next = new_node;
  }

  template <bool IsConst>
  class CommonIterator {
   private:
    friend List;
    // std::conditional_t<IsConst, const Node*, Node*> node_ptr = nullptr;
    BaseNode* node_ptr = nullptr;

   public:
    using value_type = std::conditional_t<IsConst, const T&, T&>;
    using reference = std::conditional_t<IsConst, const T&, T&>;
    using pointer = std::conditional_t<IsConst, const T*, T*>;
    using difference_type = ptrdiff_t;
    using iterator_category = std::bidirectional_iterator_tag;
    // real ok in iterator_traits

    CommonIterator() = default;
    CommonIterator(BaseNode* node_ptr) : node_ptr(node_ptr) {}
    CommonIterator(const CommonIterator&) = default;

    CommonIterator& operator=(const CommonIterator&) = default;
    operator CommonIterator<true>() const {
      return CommonIterator<true>(node_ptr);
    }

    CommonIterator& operator++() {
      node_ptr = node_ptr->next;
      return *this;
    }

    CommonIterator operator++(int) {
      auto copy = *this;
      node_ptr = node_ptr->next;
      return copy;
    }

    CommonIterator& operator--() {
      node_ptr = node_ptr->prev;
      return *this;
    }

    CommonIterator operator--(int) {
      auto copy = *this;
      node_ptr = node_ptr->prev;
      return copy;
    }

    bool operator==(const CommonIterator&) const = default;

    reference operator*() const {
      Node* real = static_cast<Node*>(node_ptr);
      return real->val;
    }

    pointer operator->() const {
      Node* real = static_cast<Node*>(node_ptr);
      return &(real->val);
    }
  };

 public:
  List(const Alloc& external_allocator = Alloc())
      : alloc(external_allocator), fakeNode{&fakeNode, &fakeNode} {}

  List(size_t n, const Alloc& external_allocator = Alloc())
      : alloc(external_allocator), fakeNode{&fakeNode, &fakeNode} {
    while (sz < n) {
      try {
        default_push();
      } catch (...) {
        while (sz != 0u) {
          pop_back();
        }
        throw;
      }
    }
  }

  List(size_t n, const T& el, const Alloc& external_allocator = Alloc())
      : alloc(external_allocator), fakeNode{&fakeNode, &fakeNode} {
    while (sz < n) {
      try {
        push_back(el);
      } catch (...) {
        while (sz != 0u) {
          pop_back();
        }
        throw;
      }
    }
  }

  NodeAllocator get_allocator() const { return alloc; }

  List(const List& another)
      : alloc(
            AllocTraits::select_on_container_copy_construction(another.alloc)),
        fakeNode{&fakeNode, &fakeNode} {
    const_iterator it = another.cbegin();
    while (it != another.cend()) {
      try {
        push_back(*it);
        ++it;
      } catch (...) {
        while (sz != 0u) {
          pop_back();
        }
        throw;
      }
    }
  }

  List(List&& another)
      : alloc(std::move(another.alloc)),
        sz(another.sz),
        fakeNode{&fakeNode, &fakeNode} {
    if (sz) {
      fakeNode = another.fakeNode;
      another.sz = 0;
      another.fakeNode = {&another.fakeNode, &another.fakeNode};
    }
  }

  ~List() {
    while (sz != 0u) {
      pop_back();
    }
  }

  List& operator=(const List& another) {
    List copy(alloc);
    const_iterator it = another.cbegin();
    while (it != another.cend()) {
      try {
        copy.push_back(*it);
        ++it;
      } catch (...) {
        while (copy.size()) {
          copy.pop_back();
        }
        throw;
      }
    }
    swap(copy);
    if (AllocTraits::propagate_on_container_copy_assignment::value) {
      alloc = another.alloc;
    }
    return *this;
  }

  List& operator=(List&& another) {
    List copy(std::move(another));
    swap(copy);
    if (!AllocTraits::propagate_on_container_swap::value &&
        AllocTraits::propagate_on_move_assignment::value) {
      std::swap(alloc, copy.alloc);
    }
    return *this;
  }

  void swap(List& another) {
    std::swap(sz, another.sz);
    std::swap(fakeNode, another.fakeNode);
    if (sz == 0) {
      fakeNode.next = fakeNode.prev = &fakeNode;
    } else {
      fakeNode.next->prev = fakeNode.prev->next = &fakeNode;
    }
    if (another.sz == 0) {
      another.fakeNode.next = another.fakeNode.prev = &another.fakeNode;
    } else {
      another.fakeNode.next->prev = another.fakeNode.prev->next =
          &another.fakeNode;
    }
    if (AllocTraits::propagate_on_container_swap::value) {
      std::swap(alloc, another.alloc);
    }
  }

  size_t size() const { return sz; }

  void push_back(const T& el) { insert(end(), el); }
  void push_front(const T& el) { insert(begin(), el); }
  void pop_back() { erase(--end()); }
  void pop_front() { erase(begin()); }

  using iterator = CommonIterator<false>;
  using const_iterator = CommonIterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  iterator begin() { return iterator(fakeNode.next); }
  const_iterator begin() const { return cbegin(); }
  const_iterator cbegin() const { return const_iterator(fakeNode.next); }

  iterator end() { return iterator(&fakeNode); }
  const_iterator end() const { return cend(); }
  const_iterator cend() const { return const_iterator(fakeNode.next->prev); }

  reverse_iterator rbegin() { return std::reverse_iterator(end()); }
  const_reverse_iterator rbegin() const { return crbegin(); }
  const_reverse_iterator crbegin() const {
    return std::reverse_iterator(cend());
  }

  reverse_iterator rend() { return std::reverse_iterator(begin()); }
  const_reverse_iterator rend() const { return crend(); }
  const_reverse_iterator crend() const {
    return std::reverse_iterator(cbegin());
  }

  void insert(const_iterator it, const T& el) {
    Node* new_node = AllocTraits::allocate(alloc, 1);
    try {
      AllocTraits::construct(alloc, new_node, el);
    } catch (...) {
      AllocTraits::deallocate(alloc, new_node, 1);
      throw;
    }
    ++sz;

    BaseNode* prev = it.node_ptr->prev;
    prev->next = new_node;
    it.node_ptr->prev = new_node;

    new_node->next = it.node_ptr;
    new_node->prev = prev;
  }

  void erase(const_iterator it) {
    --sz;
    Node* node_to_delete = static_cast<Node*>(it.node_ptr);
    BaseNode* prev = node_to_delete->prev;
    BaseNode* next = node_to_delete->next;
    prev->next = next;
    next->prev = prev;
    AllocTraits::destroy(alloc, node_to_delete);
    AllocTraits::deallocate(alloc, node_to_delete, 1);
  }
};