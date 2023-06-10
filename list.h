#pragma once
#include <iostream>
#include <memory>

template <typename T, typename Alloc = std::allocator<T>>
class List {
  private:
    struct BaseNode {
        BaseNode* next = nullptr;
        BaseNode* prev = nullptr;
        void swap(BaseNode& temp) {
            std::swap(next, temp.next);
            std::swap(prev, temp.prev);
        }
    };
    struct Node : BaseNode {
        T val;
        Node(const T& value)
            : val(value) {}
        Node()
            : val() {}
    };

    using NodeAlloc =
        typename std::allocator_traits<Alloc>::template rebind_alloc<Node>;
    using AllocTraits =
        typename std::allocator_traits<Alloc>::template rebind_traits<Node>;

    [[no_unique_address]] NodeAlloc alloc;
    size_t sz = 0;
    mutable BaseNode fakeNode;

    void default_push() {
        iterator it = begin();
        Node* new_node = AllocTraits::allocate(alloc, 1);
        try {
            AllocTraits::construct(alloc, new_node);
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

  public:
    List(const Alloc& external_allocator = Alloc())
        : alloc(external_allocator) {
        fakeNode = {&fakeNode, &fakeNode};
    }

    List(size_t n, const Alloc& external_allocator = Alloc())
        : alloc(external_allocator) {
        fakeNode = {&fakeNode, &fakeNode};
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
        : alloc(external_allocator) {
        fakeNode = {&fakeNode, &fakeNode};
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

    NodeAlloc& get_allocator() {
        return alloc;
    }

    List(const List& other)
        : alloc(std::allocator_traits<Alloc>::select_on_container_copy_construction(
              other
                  .alloc)) {
        fakeNode = {&fakeNode, &fakeNode};
        const_iterator it = other.cbegin();
        while (it != other.cend()) {
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

    ~List() {
        while (sz != 0u) {
            pop_back();
        }
    }

    List& operator=(const List& other) {
        bool flag = AllocTraits::propagate_on_container_copy_assignment::value;

        List copy(
            flag
                ? other.alloc
                : alloc);
        const_iterator it = other.cbegin();
        while (it != other.cend()) {
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
        if (flag) {
            alloc = other.alloc;
        }
        return *this;
    }

    void swap(List& other) {
        std::swap(sz, other.sz);

        fakeNode.swap(other.fakeNode);

        if (sz == 0) {
            fakeNode.next = fakeNode.prev = &fakeNode;
        } else {
            fakeNode.next->prev = fakeNode.prev->next = &fakeNode;
        }
        if (other.sz == 0) {
            other.fakeNode.next = other.fakeNode.prev = &other.fakeNode;
        } else {
            other.fakeNode.next->prev = other.fakeNode.prev->next = &other.fakeNode;
        }
    }

    size_t size() const {
        return sz;
    }

    void push_back(const T& el) {
        insert(end(), el);
    }
    void push_front(const T& el) {
        insert(begin(), el);
    }
    void pop_back() {
        erase(--end());
    }
    void pop_front() {
        erase(begin());
    }

    template <bool IsConst>
    class CommonIterator {
      public:
        using value_type = T;
        using reference = std::conditional_t<IsConst, const T&, T&>;
        using pointer = std::conditional_t<IsConst, const T*, T*>;
        using difference_type = ptrdiff_t;
        using iterator_category = std::bidirectional_iterator_tag;

      private:
        friend List;
        BaseNode* node_ptr = nullptr;

      public:
        CommonIterator() = default;
        CommonIterator(BaseNode* node_ptr)
            : node_ptr(node_ptr) {}
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

        bool operator==(const CommonIterator& other) const {
            return node_ptr == other.node_ptr;
        }

        bool operator!=(const CommonIterator& other) const {
            return node_ptr != other.node_ptr;
        }

        reference operator*() {
            Node* real = static_cast<Node*>(node_ptr);
            return real->val;
        }

        pointer operator->() {
            Node* real = static_cast<Node*>(node_ptr);
            return &(real->val);
        }
    };

    using iterator = CommonIterator<false>;
    using const_iterator = CommonIterator<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    iterator begin() {
        return iterator(fakeNode.next);
    }
    const_iterator begin() const {
        return cbegin();
    }
    const_iterator cbegin() const {
        return const_iterator(fakeNode.next);
    }

    iterator end() {
        return iterator(&fakeNode);
    }
    const_iterator end() const {
        return cend();
    }
    const_iterator cend() const {
        return const_iterator(&fakeNode);
    }

    reverse_iterator rbegin() {
        return std::reverse_iterator(end());
    }
    const_reverse_iterator rbegin() const {
        return crbegin();
    }
    const_reverse_iterator crbegin() const {
        return std::reverse_iterator(cend());
    }

    reverse_iterator rend() {
        return std::reverse_iterator(begin());
    }
    const_reverse_iterator rend() const {
        return crend();
    }
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