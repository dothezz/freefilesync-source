// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef PTR_WRAP_012384670856841394535
#define PTR_WRAP_012384670856841394535

#include <iterator>

namespace zen
{
//std::list(C++11) compatible class supporting inplace element construction for non-copyable/movable types
//may be replaced by C++11 std::list when available
template <class T>
class FixedList
{
    struct Node
    {
        Node() : next(NULL) {}
        template <class A>                                              Node(A && a)                                    : next(NULL), val(a) {}
        template <class A, class B>                                     Node(A && a, B && b)                             : next(NULL), val(a, b) {}
        template <class A, class B, class C>                            Node(A && a, B && b, C && c)                      : next(NULL), val(a, b, c) {}
        template <class A, class B, class C, class D>                   Node(A && a, B && b, C && c, D && d)               : next(NULL), val(a, b, c, d) {}
        template <class A, class B, class C, class D, class E>          Node(A && a, B && b, C && c, D && d, E && e)        : next(NULL), val(a, b, c, d, e) {}
        template <class A, class B, class C, class D, class E, class F> Node(A && a, B && b, C && c, D && d, E && e, F && f) : next(NULL), val(a, b, c, d, e, f) {}

        Node* next; //singly linked list is sufficient
        T val;
    };

public:
    FixedList() :
        first(NULL),
        lastInsert(NULL),
        sz(0) {}

    ~FixedList() { clear(); }

    template <class NodeT, class U>
    class ListIterator : public std::iterator<std::forward_iterator_tag, U>
    {
    public:
        ListIterator(NodeT* it = NULL) : iter(it) {}
        ListIterator& operator++() { iter = iter->next; return *this; }
        inline friend bool operator==(const ListIterator& lhs, const ListIterator& rhs) { return lhs.iter == rhs.iter; }
        inline friend bool operator!=(const ListIterator& lhs, const ListIterator& rhs) { return !(lhs == rhs); }
        U& operator* () { return iter->val; }
        U* operator->() { return &iter->val; }
    private:
        NodeT* iter;
    };

    typedef T value_type;
    typedef ListIterator<Node, T> iterator;
    typedef ListIterator<const Node, const T> const_iterator;
    typedef T& reference;
    typedef const T& const_reference;

    iterator begin() { return first; }
    iterator end()   { return iterator(); }

    const_iterator begin() const { return first; }
    const_iterator end  () const { return const_iterator(); }

    reference       front()       { return first->val; }
    const_reference front() const { return first->val; }

    reference&       back()       { return lastInsert->val; }
    const_reference& back() const { return lastInsert->val; }

    void emplace_back() { pushNode(new Node); }
    template <class A>                                              void emplace_back(A && a)                                    { pushNode(new Node(std::forward<A>(a))); }
    template <class A, class B>                                     void emplace_back(A && a, B && b)                             { pushNode(new Node(std::forward<A>(a), std::forward<B>(b))); }
    template <class A, class B, class C>                            void emplace_back(A && a, B && b, C && c)                      { pushNode(new Node(std::forward<A>(a), std::forward<B>(b), std::forward<C>(c))); }
    template <class A, class B, class C, class D>                   void emplace_back(A && a, B && b, C && c, D && d)               { pushNode(new Node(std::forward<A>(a), std::forward<B>(b), std::forward<C>(c), std::forward<D>(d))); }
    template <class A, class B, class C, class D, class E>          void emplace_back(A && a, B && b, C && c, D && d, E && e)        { pushNode(new Node(std::forward<A>(a), std::forward<B>(b), std::forward<C>(c), std::forward<D>(d), std::forward<E>(e))); }
    template <class A, class B, class C, class D, class E, class F> void emplace_back(A && a, B && b, C && c, D && d, E && e, F && f) { pushNode(new Node(std::forward<A>(a), std::forward<B>(b), std::forward<C>(c), std::forward<D>(d), std::forward<E>(e), std::forward<F>(f))); }

    template <class Predicate>
    void remove_if(Predicate pred)
    {
        Node* prev = NULL;
        for (auto ptr = first; ptr;)
            if (pred(ptr->val))
            {
                Node* tmp = ptr->next;
                deleteNode(ptr);
                if (prev)
                    prev->next = ptr = tmp;
                else
                    first = ptr = tmp;
                if (tmp == NULL)
                    lastInsert = prev;
            }
            else
            {
                prev = ptr;
                ptr = ptr->next;
            }
    }

    void clear() { remove_if([](T&) { return true; }); }
    bool empty() const { return first == NULL; }
    size_t size() const { return sz; }

private:
    FixedList(const FixedList&);
    FixedList& operator=(const FixedList&);

    void pushNode(Node* newNode)
    {
        ++sz;
        if (lastInsert == NULL)
        {
            assert(first == NULL);
            first = lastInsert = newNode;
        }
        else
        {
            assert(lastInsert->next == NULL);
            lastInsert->next = newNode;
            lastInsert = newNode;
        }
    }

    void deleteNode(Node* oldNode)
    {
        assert(sz > 0);
        --sz;
        delete oldNode;
    }

    Node* first;
    Node* lastInsert; //point to last insertion; required by emplace_back()
    size_t sz;
};
}


#endif //PTR_WRAP_012384670856841394535
