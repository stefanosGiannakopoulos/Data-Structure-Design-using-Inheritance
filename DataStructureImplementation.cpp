#include <iostream>

using namespace std;

template <typename T>
class Container {
public:
    virtual ~Container() {}
    virtual int size() const = 0;
    virtual bool empty() const { return size() == 0; }
    virtual void clear() = 0;
};

template <typename T>
class Iterator {
public:
    Iterator(const Iterator& i) : impl(i.impl->clone()) {}
    ~Iterator() { delete impl; }
    T& operator*() { return impl->access(); }
    // prefix ++, i.e. ++i
    Iterator& operator++() {
        impl->advance();
        return *this;
    }
    // postfix ++, i.e. i++
    Iterator operator++(int) {
        Iterator result(*this);
        impl->advance();
        return result;
    }

    bool operator==(const Iterator& i) const {
        return typeid(*this) == typeid(i) && impl->equal(*(i.impl));
    }
    bool operator!=(const Iterator& i) const {
        return !(*this == i);
    };

    class Impl {
    public:
        virtual ~Impl() {}
        virtual Impl* clone() const = 0;
        virtual T& access() const = 0;
        virtual void advance() = 0;
        virtual bool equal(const Impl& i) const = 0;
    };

    Iterator(Impl* i) : impl(i) {}

private:
    Impl* impl;
};

template <typename T>
class Iterable {
public:
    virtual Iterator<T> begin() = 0;
    virtual Iterator<T> end() = 0;
};

template <typename T>
class Visitor {
public:
    virtual ~Visitor() {}
    virtual void visit(T& x) = 0;
    virtual bool finished() const { return false; }
};

template <typename T>
class Visitable {
public:
    virtual void accept(Visitor<T>& visitor) = 0;
};


// Doubly linked lists.

template <typename T>
class dlist : public Container<T>, public Iterable<T>,
    public Visitable<T> {
public:
    dlist() : the_front(nullptr), the_back(nullptr), the_size(0) {}

    dlist(const dlist& l)
        : the_front(nullptr), the_back(nullptr), the_size(0) {
        copy(l);
    }

    virtual ~dlist() override { purge(); }

    dlist& operator=(const dlist& l) {
        clear(); copy(l); return *this;
    }

    virtual int size() const override { return the_size; }

    virtual void clear() override {
        purge();
        the_front = the_back = nullptr;
        the_size = 0;
    }

    void push_back(const T& x) {
        node* p = new node(x, nullptr, the_back);
        if (the_back != nullptr) the_back->next = p;
        else the_front = p;
        the_back = p;
        ++the_size;
    }

    void push_front(const T& x) {
        node* p = new node(x, the_front, nullptr);
        if (the_front != nullptr) the_front->prev = p;
        else the_back = p;
        the_front = p;
        ++the_size;
    }

    void pop_back() {
        node* p = the_back;
        the_back = p->prev;
        if (the_back != nullptr) the_back->next = nullptr;
        else the_front = nullptr;
        delete p;
        --the_size;
    }

    void pop_front() {
        node* p = the_front;
        the_front = p->next;
        if (the_front != nullptr) the_front->prev = nullptr;
        else the_back = nullptr;
        delete p;
        --the_size;
    }

    T& back() { return the_back->data; }
    T& front() { return the_front->data; }

private:
    struct node {
        T data;
        node* next, * prev;
        node(const T& x, node* n, node* p) : data(x), next(n), prev(p) {}
    };

    node* the_front, * the_back;
    int the_size;

    void copy(const dlist& l) {
        for (node* p = l.the_front; p != nullptr; p = p->next)
            push_back(p->data);
    }

    void purge() {
        node* p = the_front;
        while (p != nullptr) {
            node* q = p;
            p = p->next;
            delete q;
        }
    }

public:
    class ListIteratorImpl : public Iterator<T>::Impl {
    private:
        typedef typename Iterator<T>::Impl Impl;
    public:
        Impl* clone() const override {
            return new ListIteratorImpl(ptr);
        }
        T& access() const override { return ptr->data; }
        void advance() override { ptr = ptr->next; }
        bool equal(const Impl& i) const override {
            return ptr == ((ListIteratorImpl*)&i)->ptr;
        }
    private:
        node* ptr;
        ListIteratorImpl(node* p) : ptr(p) {}
        friend class dlist;
    };

    Iterator<T> begin() override {
        return Iterator<T>(new ListIteratorImpl(the_front));
    }
    Iterator<T> end() override {
        return Iterator<T>(new ListIteratorImpl(nullptr));
    }

public:
    void accept(Visitor<T>& visitor) override {
        for (node* p = the_front; p != nullptr; p = p->next) {
            if (visitor.finished()) return;
            visitor.visit(p->data);
        }
    }
};


// Binary-search trees.

template <typename T>
class bstree : public Container<T>, public Iterable<T>,
    public Visitable<T> {
public:
    bstree() : root(nullptr), the_size(0) {}

    bstree(const bstree& t)
        : root(nullptr), the_size(0) {
        copy(t.root);
    }

    virtual ~bstree() override { purge(root); }

    bstree& operator=(const bstree& t) {
        clear(); copy(t.root); return *this;
    }

    virtual int size() const override { return the_size; }

    virtual void clear() override {
        purge(root);
        root = nullptr;
        the_size = 0;
    }

private:
    struct node {
        T data;
        node* left, * right, * parent;
        node(const T& x, node* l, node* r, node* p)
            : data(x), left(l), right(r), parent(p) {}
    };

    node* root;
    int the_size;

    void copy(node* t) {
        if (t != nullptr) {
            insert(t->data);
            copy(t->left);
            copy(t->right);
        }
    }

    static void purge(node* t) {
        if (t != nullptr) {
            purge(t->left);
            purge(t->right);
            delete t;
        }
    }

public:
    void insert(const T& x) {
        if (root == nullptr) {
            root = new node(x, nullptr, nullptr, nullptr);
            ++the_size;
        }
        else if (insert(root, x))
            ++the_size;
    }

private:
    static bool insert(node* t, const T& x) {
        if (x < t->data) {
            if (t->left == nullptr) {
                t->left = new node(x, nullptr, nullptr, t);
                return true;
            }
            else
                return insert(t->left, x);
        }
        else if (x > t->data) {
            if (t->right == nullptr) {
                t->right = new node(x, nullptr, nullptr, t);
                return true;
            }
            else
                return insert(t->right, x);
        }
        return false;
    }

    static node* leftdown(node* t) {
        if (t == nullptr || t->left == nullptr)
            return t;
        else
            return leftdown(t->left);
    }

    static node* leftup(node* t) {
        if (t->parent == nullptr || t->parent->left == t)
            return t->parent;
        else
            return leftup(t->parent);
    }

public:
    class TreeIteratorImpl : public Iterator<T>::Impl {
    private:
        typedef typename Iterator<T>::Impl Impl;
    public:
        Impl* clone() const override {
            return new TreeIteratorImpl(ptr);
        }
        T& access() const override { return ptr->data; }
        void advance() override {
            if (ptr != nullptr) {
                if (ptr->right != nullptr)
                    ptr = leftdown(ptr->right);
                else
                    ptr = leftup(ptr);
            }
        }
        bool equal(const Impl& i) const override {
            return ptr == ((TreeIteratorImpl*)&i)->ptr;
        }
    private:
        node* ptr;
        TreeIteratorImpl(node* p) : ptr(p) {}
        friend class bstree;
    };

    Iterator<T> begin() override {
        return Iterator<T>(new TreeIteratorImpl(leftdown(root)));
    }
    Iterator<T> end() override {
        return Iterator<T>(new TreeIteratorImpl(nullptr));
    }

public:
    void accept(Visitor<T>& visitor) override {
        accept(visitor, root);
    }

private:
    static void accept(Visitor<T>& visitor, node* t) {
        if (t != nullptr) {
            accept(visitor, t->left);
            if (visitor.finished()) return;
            visitor.visit(t->data);
            accept(visitor, t->right);
        }
    }
};


template <typename T>
void print(Iterable<T>& c) {
    for (auto& x : c)
        cout << " " << x;
}


class AddingVisitor : public Visitor<int> {
public:
    AddingVisitor() : sum(0) {}
    void visit(int& x) override { sum += x; }
    int getSum() const { return sum; }
private:
    int sum;
};

template <typename T>
class PrintingVisitor : public Visitor<T> {
public:
    void visit(T& x) override { cout << x << endl; }
};

class PZSearchingVisitor : public Visitor<int> {
public:
    PZSearchingVisitor() : found(false) {}
    void visit(int& x) override {
        if (17 <= x && x <= 42) { found = true; value = x; }
    }
    bool finished() const override { return found; }
    bool getFound() const { return found; }
    int getValue() const { return value; }
private:
    bool found;
    int value;
};


int main() {
    // Examples for dlist.
    dlist<int> l;
    for (int i = 0; i < 10; ++i) l.push_back(i);
    cout << "l of size " << l.size() << ":"; print(l); cout << endl;
    cout << "Print the list again..." << endl;
    cout << "l of size " << l.size() << ":"; print(l); cout << endl;
    for (int& x : l) x *= 2;
    cout << "l of size " << l.size() << ":"; print(l); cout << endl;
    auto i = l.begin();
    ++i; ++i;
    cout << "This should be 4: " << *i << endl;
    cout << "This should be 6: " << *(++i) << endl;
    cout << "This should be 6: " << *i++ << endl;
    cout << "This should be 8: " << *i << endl;

    cout << "------------------" << endl;

    // Examples for bstree.
    bstree<int> t;
    int x[] = { 5, 2, 8, 4, 1, 7, 6, 0, 9, 3 };
    for (int i = 0; i < 10; ++i) t.insert(x[i]);
    cout << "t of size " << t.size() << ":"; print(t); cout << endl;
    cout << "Print the tree again..." << endl;
    cout << "t of size " << t.size() << ":"; print(t); cout << endl;
    for (int& x : t) x *= 2;
    cout << "Print the tree again..." << endl;
    cout << "t of size " << t.size() << ":"; print(t); cout << endl;
    auto j = t.begin();
    ++j; ++j;
    cout << "This should be 4: " << *j << endl;
    cout << "This should be 6: " << *(++j) << endl;
    cout << "This should be 6: " << *j++ << endl;
    cout << "This should be 8: " << *j << endl;

    cout << "------------------" << endl;

    AddingVisitor v1;
    l.accept(v1);
    t.accept(v1);
    cout << "The sum is " << v1.getSum() << endl;

    PrintingVisitor<int> v2;
    t.accept(v2);

    PZSearchingVisitor v3;
    t.accept(v3);
    if (v3.getFound())
        cout << "Found: " << v3.getValue() << endl;
    else
        cout << "Not found" << endl;
}