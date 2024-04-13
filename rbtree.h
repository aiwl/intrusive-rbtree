#pragma once

#include <utility>
#include <type_traits>
#include <iterator>
#include <cassert>

template <typename T>
struct get_key_for_value {
    using value_type = T;
    using key_type = T;

    const key_type& operator()(const value_type& value) const noexcept
    {
        return value;
    }
};

// -- red-black tree node ----------------------------------------------------

template <
    typename T, typename Tag = void,
    typename GetKeyForValue = get_key_for_value<T>,
    typename Compare = std::less<T>>
class rbtree;

template <typename T, typename Tag, bool Const>
class rbtree_iterator;

// Note, we could check in the destructor of this class whether or not the
// node is still part of an rbtree to notify the user when a node is
// wrongly destructed. However, we decide against it for now, s.t. classes
// inheriting this node may have trivial destructors.
template <typename Tag = void>
struct rbtree_node {
    //B3_NO_COPY_AND_MOVE(rbtree_node);
  
    template<typename T, typename Tag_, typename GetKeyForValue, typename Compare>
    friend class rbtree;

    template<typename T, typename Tag_, bool Const_>
    friend class rbtree_iterator;

protected:
    rbtree_node() noexcept = default;
  
    void set_red() noexcept { is_red_ = true; }
    void set_black() noexcept { is_red_ = false; }
    bool is_red() const noexcept { return is_red_; }

    void set_parent(rbtree_node* parent)
    { 
        parent_ = parent;
    }
    
    rbtree_node* parent() noexcept
    { 
        return parent_; 
    }
    
    const rbtree_node* parent() const noexcept
    { 
        return parent_;
    }
  
    bool unlinked() const noexcept
    {
        return parent() == nullptr && left == nullptr && right == nullptr;
    }
  
    void set_red(bool val) noexcept
    {
        if (val) set_red();
        else set_black();
    }
  
    bool is_black() const noexcept
    { 
        return !is_red();
    }
    
    void reset () noexcept
    {
        left = nullptr;
        right = nullptr;
        is_red_ = false;
        parent_ = nullptr;
    }

    rbtree_node<Tag>* left = nullptr;
    rbtree_node<Tag>* right = nullptr;

private:
    bool is_red_ = false;
    rbtree_node<Tag>* parent_ = nullptr;
};

// -- red-black tree iterator ------------------------------------------------

template <typename T, typename Tag, bool Const>
class rbtree_iterator {
    using node_pointer = std::conditional_t<
        Const, const rbtree_node<Tag>*, rbtree_node<Tag>*>;

public:
    using difference_type = ptrdiff_t;
    using value_type = std::conditional_t<Const, const T, T>;
    using pointer = value_type*;
    using reference = value_type&;
    using iterator_category = std::bidirectional_iterator_tag;

private:
    node_pointer curr_;

private:
    static node_pointer next(node_pointer curr) noexcept
    {
        node_pointer next = nullptr;

        if (curr->right) {
            next = curr->right;

            while (next->left)
                next = next->left;
        } else {
            next = curr->parent();

            while (next->right == curr) {
                curr = next;
                next = curr->parent();
            }

            if (curr->right == next)
                next = curr;
        }

        return next;
    }

    static node_pointer prev(node_pointer curr) noexcept
    {
        node_pointer next = nullptr;

        if (curr->left) {
            next = curr->left;

            while (next->right)
                next = next->right;
        } else {
            next = curr->parent();

            while (next->left == curr) {
                curr = next;
                next = curr->parent();
            }

            if (curr->right == next)
                next = curr;
        }

        return next;
    }

public:
    explicit rbtree_iterator(node_pointer curr) noexcept : curr_{curr}
    {}

    pointer get() const noexcept
    { 
        return static_cast<pointer>(curr_);
    }

    reference operator*() const noexcept
    { 
        return *operator->();
    }

    pointer operator->() const noexcept
    { 
        return get();
    }

    rbtree_iterator& operator++() noexcept
    {
        curr_ = next(curr_);
        return *this;
    }

    rbtree_iterator operator++(int) noexcept
    {
        rbtree_iterator rv = *this;
        operator++();
        return rv;
    }

    rbtree_iterator& operator--() noexcept
    {
        curr_ = prev(curr_);
        return *this;
    }

    rbtree_iterator operator--(int) noexcept
    {
        rbtree_iterator rv = *this;
        operator--();
        return rv;
    }

    friend bool operator==(
        const rbtree_iterator& it0, const rbtree_iterator& it1) noexcept
    {
        return it0.curr_ == it1.curr_;
    }

    friend bool operator!=(
        const rbtree_iterator& it0, const rbtree_iterator& it1) noexcept
    {
        return !(it0 == it1);
    }
};

// -- red-black tree ---------------------------------------------------------

// Implementation of an rbtree which serves as the foundation for implementing
// the "classic" set and map containers (as well as their intrusive counter parts).
// 
// Note: The tree returns mutating references its elements s.t. classes using
// it for their implementation can do mutations on them. However, be aware
// that the "key part" of the element must not be changed, since this would
// invalidate the tree structure and leads to UB.
template<typename T, typename Tag, typename GetKeyForValue, typename Compare>
class rbtree : public GetKeyForValue, public Compare {
    //B3_NO_COPY(rbtree)

    static_assert(
        std::is_base_of_v<rbtree_node<Tag>, T>, "`T` must be a rbtree node.");

public:
    using value_type = T;
    using key_type = typename GetKeyForValue::key_type;
    using node_type = rbtree_node<Tag>;
    using iterator = rbtree_iterator<T, Tag, false>;
    using const_iterator = rbtree_iterator<T, Tag, true>;
  
private:
    node_type head_;

private:
    template <typename Key0, typename Key1>
    bool is_less_than(const Key0& key0, const Key1& key1) const noexcept
    {
        return (*static_cast<const Compare*>(this))(key0, key1);
    }

    const key_type& to_key(const value_type& value) const noexcept
    {
        return (*static_cast<const GetKeyForValue*>(this))(value);
    }

    const key_type& to_key(const node_type* node) const noexcept
    {
        assert(node != nullptr);
        return to_key(to_value(node));
    }

    value_type& to_value(node_type* node) noexcept
    {
        return *static_cast<value_type*>(node);
    }

    const value_type& to_value(const node_type* node) const noexcept
    {
        return *static_cast<const value_type*>(node);
    }

    node_type* to_node(T& type) noexcept {
        return static_cast<node_type*>(&type);
    }

    node_type* root() noexcept
    {
        return head_.parent();
    }

    const node_type* root() const noexcept
    {
        return head_.parent(); 
    }

    void set_root(node_type* n) noexcept
    {
        if (n) n->set_parent(&head_);
        head_.set_parent(n); 
    }

    void rotate_left(node_type* x) noexcept
    {
        assert(x->right != nullptr);
        node_type* y = x->right; 
        x->right = y->left;
        if (y->left)
            y->left->set_parent(x);
        y->set_parent(x->parent());
        if (x->parent() == &head_)
            set_root(y);
        else if (x == x->parent()->left)
            x->parent()->left = y;
        else
            x->parent()->right = y;
        x->set_parent(y);
        y->left = x;
    }

    void rotate_right(node_type* x) noexcept
    {
        assert(x->left != nullptr);
        node_type* y = x->left; 
        x->left = y->right;
        if (y->right)
            y->right->set_parent(x);
        y->set_parent(x->parent());
        if (x->parent() == &head_)
            set_root(y);
        else if (x == x->parent()->left)
            x->parent()->left = y;
        else
            x->parent()->right = y;
        x->set_parent(y);
        y->right = x;
    }

    void insert_fixup(node_type* z) noexcept
    {
        while (z->parent() != &head_
               && z->parent()->is_red()
               && z->parent()->parent() != &head_)
        {
            if (z->parent() == z->parent()->parent()->left) {
                node_type* y = z->parent()->parent()->right;
                if (y && y->is_red()) {
                    z->parent()->set_black();
                    y->set_black();
                    z->parent()->parent()->set_red();
                    z = z->parent()->parent();
                } else {
                    if (z == z->parent()->right) {
                        z = z->parent();
                        rotate_left(z);
                    }
                    z->parent()->set_black();
                    z->parent()->parent()->set_red();
                    rotate_right(z->parent()->parent());
                }
            } else {
                node_type* y = z->parent()->parent()->left;
                if (y && y->is_red()) {
                    z->parent()->set_black();
                    y->set_black();
                    z->parent()->parent()->set_red();
                    z = z->parent()->parent();
                } else {
                    if (z == z->parent()->left) {
                        z = z->parent();
                        rotate_right(z);
                    }
                    z->parent()->set_black();
                    z->parent()->parent()->set_red();
                    rotate_left(z->parent()->parent());
                }
            }
        }
        root()->set_black();
    }

    template <typename Self, typename Key>
    static auto find_node(Self* self, const Key& key) noexcept 
        -> decltype(self->begin())
    {
        using iterator_type = decltype(self->begin());

        auto x = self->root();
        while (x) {
            if (self->is_less_than(key, self->to_key(x))) {
                x = x->left;
            } else {
                if (self->is_less_than(self->to_key(x), key)) {
                    x = x->right;
                } else {
                    return iterator_type(x);
                }
            }
        }
        return self->end();
    }

    void transplant(node_type* u, node_type* v) noexcept
    {
        if (u->parent() == &head_)
            set_root(v);
        else if (u == u->parent()->left)
            u->parent()->left = v;
        else
            u->parent()->right = v;
        if (v)
            v->set_parent(u->parent());
    }

    void erase_node(node_type* z) noexcept
    {
        node_type* x = nullptr;
        node_type* y = z;
        int y_is_red = y->is_red();

        if (head_.parent() == z)
            head_.set_parent(x);
        if (head_.left == z)
            head_.left = z->right ? find_minimum(z->right) : z->parent();
        if (head_.right == z)
            head_.right = z->left ? find_maximum(z->left) : z->parent();

        if (z->left == nullptr) {
            x = z->right;
            transplant(z, z->right);
        } else if (z->right == nullptr) {
            x = z->left;
            transplant(z, z->left);
        } else {
            y = find_minimum(z->right);
            y_is_red = y->is_red();
            x = y->right;

            if (y->parent() != z) {
                transplant(y, y->right);
                y->right = z->right;
                y->right->set_parent(y);
            }

            transplant(z, y);
            y->left = z->left;
            y->left->set_parent(y);
            y->set_red(z->is_red());
        }

        if (!y_is_red && x)
            erase_fixup(x);
    }

    void erase_fixup(node_type* x)
    {
        while (x != root() && x->is_black()) {

            if (x == x->parent()->left) {
                node_type* w = x->parent()->right;
                assert(w);

                if (w->is_red()) {
                    w->set_black();
                    x->parent()->set_red();
                    rotate_left(x->parent());
                    w = x->parent()->right;
                    assert(w);
                }

                if ((!w->left || w->left->is_black()) &&
                    (!w->right || w->right->is_black())) {
                    w->set_red();
                    x = x->parent();
                } else {
                    if (!w->right || w->right->is_black()) {
                        w->left->set_black();
                        w->set_red();
                        rotate_right(w);
                        w = x->parent()->right;
                        assert(w);
                    }

                    w->set_red(x->parent()->is_red());
                    x->parent()->set_black();
                    if (w->right)
                        w->right->set_black();
                    rotate_left(x->parent());
                    x = root();
                }
            } else {
                node_type* w = x->parent()->left;
                assert(w);

                if (w->is_red()) {
                    w->set_black();
                    x->parent()->set_red();
                    rotate_right(x->parent());
                    w = x->parent()->left;
                    assert(w);
                }

                if ((!w->right || w->right->is_black()) &&
                    (!w->left || w->left->is_black())) {
                    w->set_red();
                    x = x->parent();
                } else {
                    if (!w->left || w->left->is_black()) {
                        w->right->set_black();
                        w->set_red();
                        rotate_left(w);
                        w = x->parent()->left;
                        assert(w);
                    }

                    w->set_red(x->parent()->is_red());
                    x->parent()->set_black();
                    if (w->left)
                        w->left->set_black();
                    rotate_right(x->parent());
                    x = root();
                }
            }
        }
        x->set_black();
    }

    node_type* find_minimum(node_type* x) noexcept
    {
        while (x->left)
            x = x->left;
        return x;
    }


    node_type* find_maximum(node_type* x) noexcept
    {
        while (x->right)
            x = x->right;
        return x;
    }

    static void reset_node(node_type* node) noexcept
    {
        node->set_parent(nullptr);
        node->left = nullptr;
        node->right = nullptr;
    }

    // Unlinks all nodes from the subtree defined by `x` (including `x`)
    // and disposes them via `disposer`.
    template <typename Disposer>
    void clear_and_dispose_helper(Disposer disposer, node_type* x) noexcept
    {
        while (x) {
            node_type* s = x->left;

            if (s) {
                // Right rotate until `x` has no left child anymore.
                x->left = s->right;
                s->right = x;
            } else {
                // If `x` has no left child anymore we can erase it.
                s = x->right;
                //reset_node(x);
                disposer(&to_value(x));
            }

            x = s;
        }
    }

    const GetKeyForValue& get_get_key_for_value() const noexcept
    {
        return *static_cast<const GetKeyForValue*>(this);
    }

    GetKeyForValue& get_get_key_for_value() noexcept
    {
        return *static_cast<GetKeyForValue*>(this);
    }

    const Compare& get_compare() const noexcept
    {
        return *static_cast<const Compare*>(this);
    }

    Compare& get_compare() noexcept
    {
        return *static_cast<Compare*>(this);
    }

public:
    rbtree(
        const GetKeyForValue& get_key = GetKeyForValue(),
        const Compare& compare = Compare()) noexcept :
        GetKeyForValue(get_key), Compare(compare)
    {
        head_.left = &head_;
        head_.right = &head_;
    }
  
    rbtree(rbtree&& other) noexcept :
        rbtree(other.get_get_key_for_value(), other.get_compare())
    {
        swap(other);
    }
  
    rbtree& operator=(rbtree&& other) noexcept
    {
        swap(other);
        return *this;
    }
  
    ~rbtree() noexcept
    {
        clear();
    }

    template <
        typename Cloner, typename Disposer,
        typename = std::enable_if_t<std::is_invocable_r_v<T*, Cloner, const T*>>,
        typename = std::enable_if_t<std::is_invocable_v<Disposer, T*>>>
    rbtree clone(Cloner cloner, Disposer disposer) const
    {
        rbtree rv(get_get_key_for_value(), get_compare());

        if (empty())
            return rv;

        // Rollback if cloner throws.
        bool rollback = true;
        auto _ = finally([&]() {
            if (!rollback)
                return;

            node_type* x = rv.root();
            while (x) {
                node_type* save = x->left;

                if (save) {
                    x->left = save->right;
                    save->right = x;
                } else {
                    save = x->right;
                    disposer(static_cast<value_type*>(x));
                }
                x = save;
            }
        });

        // Clone the tree.
        const node_type* node_orig = root();
        node_type* node_clone =
            cloner(static_cast<const value_type*>(node_orig));

        node_clone->set_parent(&rv.head_);
        node_clone->set_red(node_orig->is_red());

        rv.head_.set_parent(node_clone);
        rv.head_.left = node_clone;
        rv.head_.right = node_clone;

        while (true) {
            if (node_orig->left && !node_clone->left) {
                node_orig = node_orig->left;
                
                node_type* parent_clone = node_clone;
                node_clone = cloner(static_cast<const value_type*>(node_orig));
                node_clone->set_red(node_orig->is_red());
                parent_clone->left = node_clone;
                node_clone->set_parent(parent_clone);

                if (node_orig->left == head_.left)
                    rv.head_.left = node_clone;
            } else if (node_orig->right && !node_clone->right) {
                node_orig = node_orig->right;
                
                node_type* parent_clone = node_clone;
                node_clone = cloner(static_cast<const value_type*>(node_orig));
                node_clone->set_red(node_orig->is_red());
                parent_clone->right = node_clone;
                node_clone->set_parent(parent_clone);

                if (node_orig->right == head_.right)
                    rv.head_.right = node_clone;
            } else {
                node_orig = node_orig->parent();
                node_clone = node_clone->parent();

                if (node_orig == &head_)
                    break;
            }
        }

        // Everything succeeded. Set `rollback` to false to not trigger
        // the finally.
        rollback = false;

        return rv;
    }

    // -- iterators ----------------------------------------------------------

    const_iterator cbegin() const noexcept
    {
        return const_iterator(head_.left);
    }

    const_iterator cend() const noexcept
    {
        return const_iterator(&head_);
    }

    const_iterator begin() const noexcept
    {
        return const_iterator(head_.left);
    }

    const_iterator end() const noexcept
    {
        return const_iterator(&head_);
    }

    iterator begin() noexcept
    {
        return iterator(head_.left);
    }

    iterator end() noexcept
    {
        return iterator(&head_);
    }

    // -- capacity -----------------------------------------------------------

    bool empty() const noexcept
    {
        return root() == nullptr;
    }

    // -- lookup -------------------------------------------------------------

    bool contains(const key_type& key) const noexcept
    {
        return end() != find_node(this, key);
    }

    template <
        typename Key, typename Comp = Compare,
        typename = typename Comp::is_transparent>
    bool contains(const Key& key) const noexcept
    {
        return end() != find_node(this, key);
    }

    const_iterator find(const key_type& key) const noexcept
    {
        return find_node(this, key);
    }

    template <
        typename Key, typename Comp = Compare,
        typename = typename Comp::is_transparent>
    const_iterator find(const Key& key) const noexcept
    {
        return find_node(this, key);
    }

    iterator find(const key_type& key) noexcept
    { 
        return find_node(this, key);
    }

    template <
        typename Key, typename Comp = Compare,
        typename = typename Comp::is_transparent>
    iterator find(const Key& key) noexcept
    {
        return find_node(this, key);
    }

    // -- modifiers ----------------------------------------------------------

    template<typename Key, typename Fn>
    std::pair<iterator, bool> insert_for_key(const Key& key, Fn&& fn)
    {
        node_type* y = nullptr;
        node_type* x = root();

        while (x) {
            y = x;

            if (is_less_than(key, to_key(x))) {
                x = y->left;
            } else {
                if (is_less_than(to_key(x), key))
                    x = y->right;
                else {
                    // The tree already corresponds an entry with `key`.
                    return { iterator(x), false };
                }
            }
        }

        node_type* z = std::forward<Fn>(fn)();
        return insert_parent(y, z);
    }

    std::pair<iterator, bool>
    insert(value_type& value) noexcept
    {
        node_type* y = nullptr;
        node_type* x = root();

        while (x) {
            y = x;

            if (is_less_than(to_key(value), to_key(x))) {
                x = y->left;
            } else {
                if (is_less_than(to_key(x), to_key(value)))
                    x = y->right;
                else {
                    // The tree already corresponds an entry with `key`.
                    return { iterator(x), false };
                }
            }
        }

        node_type* z = to_node(value);      
        z->reset();
        return insert_parent(y, z);
    }
  
    std::pair<iterator, bool>
    insert_parent(node_type* y, node_type* z) noexcept
    {
        if (y == nullptr) {
            set_root(z);
            head_.left = root();
            head_.right = root();
        } else if (is_less_than(to_key(z), to_key(y))) {
            z->set_parent(y);
            y->left = z;

            if (head_.left == y)
                head_.left = z;
        } else {
            z->set_parent(y);
            y->right = z;

            if (head_.right == y)
                head_.right = z;
        }

        z->left = nullptr;
        z->right = nullptr;
        z->set_red();

        insert_fixup(z);
        return { iterator(z), true };
    }

    // Note, `key` *must* be part of this tree.
    value_type* erase(const key_type& key) noexcept
    {
        auto it = find_node(this, key);
        if (it == end())
            return nullptr;
        erase_node(it.get());
        reset_node(it.get());
        return &to_value(it.get());
    }
  
    template <
        typename Key, typename Comp = Compare,
        typename = typename Comp::is_transparent>
    value_type* erase(const Key& key) noexcept
    {
        auto it = find_node(this, key);
        if (it == end())
            return nullptr;
        erase_node(it.get());
        reset_node(it.get());
        return &to_value(it.get());
    }
  
    // Clears the rbtree simply by resetting the head node.
    void clear() noexcept
    {
        head_.set_parent(&head_);
        head_.left = nullptr;
        head_.right = nullptr;
    }

    // Clears the rbtree by iterating over each element and invoking
    // a disposer functor on them.
    template <typename Disposer>
    void clear_and_dispose(Disposer disposer)
    {
        static_assert(std::is_invocable_v<Disposer, value_type*>);

        clear_and_dispose_helper(disposer, root());
        set_root(nullptr);
        head_.left = nullptr;
        head_.right = nullptr;
    }

    void swap(rbtree& other) noexcept
    {
        std::swap(get_compare(), other.get_compare());
        std::swap(get_get_key_for_value(), other.get_get_key_for_value());

        if (head_.parent() && other.head_.parent()) {
            assert(head_.is_red() == other.head_.is_red());

            std::swap(head_.left, other.head_.left);
            std::swap(head_.right, other.head_.right);

            node_type* parent = head_.parent();
            head_.set_parent(other.head_.parent());
            other.head_.set_parent(parent);

            head_.parent()->set_parent(&head_);
            other.head_.parent()->set_parent(&other.head_);
        } else if (head_.parent()) {
            other.head_.left = head_.left;
            other.head_.right = head_.right;
            other.head_.set_parent(head_.parent_);
            other.head_.parent()->set_parent(&other.head_);

            head_.left = &head_;
            head_.right = &head_;
            head_.set_parent(nullptr);
        } else if (other.head_.parent()) {
            head_.left = other.head_.left;
            head_.right = other.head_.right;
            head_.set_parent(other.head_.parent());
            head_.parent()->set_parent(&head_);

            other.head_.left = &other.head_;
            other.head_.right = &other.head_;
            other.head_.set_parent(nullptr);
        } else {
            // do nothing; both trees are empty.
        }
    }
};
