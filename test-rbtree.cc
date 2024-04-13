#include "rbtree.h"
#include "test-utils.h"
#include <catch2/catch_all.hpp>
#include <unordered_set>
#include <set>

namespace {

struct StringNode : rbtree_node<>
{
    std::string str;

    explicit StringNode(const std::string& str) noexcept : str{str} {}
};

bool operator<(const StringNode& node0, const StringNode& node1) noexcept
{
    return node0.str < node1.str;
}

bool operator<(const StringNode& node, const char* zstr) noexcept
{
    return node.str < zstr;
}

bool operator<(const char* zstr, const StringNode& node) noexcept
{
    return zstr < node.str;
}

bool operator<(const StringNode& node, const std::string& str) noexcept
{
    return node.str < str;
}

bool operator<(const std::string& str, const StringNode& node) noexcept
{
    return str < node.str;
}

}

// -- basic tests ------------------------------------------------------------

namespace {

struct A : rbtree_node<> {
    int foo;

    A(int foo) : foo(foo) {}

    bool operator<(const A& other) const noexcept
    {
        return foo < other.foo;
    }
};

}

TEST_CASE("rbtree : basic tests")
{
    A a(1), b(2);
    rbtree<A> tree;

    REQUIRE(tree.insert(a).second == true);
    REQUIRE(tree.insert(b).second == true);
    REQUIRE(tree.insert(b).second == false);
    tree.erase(b);
    REQUIRE(tree.insert(b).second == true);
    tree.erase(a);
    tree.erase(b);
    REQUIRE(!tree.contains(a));
    REQUIRE(!tree.contains(b));
    return;
}

TEST_CASE("rbtree : simple iterator test")
{
    rbtree<StringNode> tree;
    std::set<std::string> set;
    quick_rng rng = {4848990918};

    srand(time(nullptr));

    for (int k = 0; k < 100; k++) {
        std::string r = random_ascii_string(100, rng);
        tree.insert(*(new StringNode(r)));
        set.insert(r);
    }

    std::vector<std::string> to_delete;
    int counter = 0;
    for (const std::string& str : set) {
        to_delete.push_back(str);
        counter++;
        if (counter == 10)
            break;
    }
    for (const std::string& str : to_delete) {
        set.erase(str);
        tree.erase(StringNode(str));
    }


    auto it = tree.begin();
    auto it_set = set.begin();
    while (true) {
        if (it == tree.end()) {
            break;
        }

        REQUIRE(it->str == *it_set);

        auto prev = it;
        it++;
        it_set++;

        if (it == tree.end()) {
            break;
        }
        REQUIRE(prev->str < it->str);
    }

    tree.clear_and_dispose([](StringNode* n) { delete n; });
    return;
}
// -- destructor/clear -------------------------------------------------------

TEST_CASE("rbtree: destructor/clear")
{
    A a(1), b(2), c(3), d(4), e(5);
    {
        rbtree<A> tree;
        tree.insert(a);
        tree.insert(b);
        tree.insert(c);
        tree.insert(d);
        tree.insert(e);
        // `tree`'s destructor will erase all elements here.
    }
    return;
}

// -- performance (insert) ---------------------------------------------------

namespace {

struct IntNode : rbtree_node<> {
    explicit IntNode(int foo) noexcept : foo(foo) {}

    int foo;
};

bool operator<(const IntNode& node0, const IntNode& node1) noexcept
{
    return node0.foo < node1.foo;
}

}

TEST_CASE("rbtree: performance test")
{
    constexpr size_t N = 50000;

    std::vector<IntNode*> nodes;
    nodes.resize(N);
    for (size_t k = 0; k < N; k++) {
        nodes[k] = new IntNode(int(k));
    }

    // -- insert ------------------------------------------------

    BENCHMARK("rbtree: insert") {
        rbtree<IntNode> tree;
        for (size_t k = 0; k < N; k++) {
            tree.insert(*nodes[k]);
        }
    };

    BENCHMARK("std::set: insert") {
        std::set<int> set;
        for (size_t k = 0; k < N; k++) {
            set.insert(int(k));
        }
    };

    BENCHMARK("std::unordered_set: insert") {
        std::unordered_set<int> set;
        for (size_t k = 0; k < N; k++) {
            set.insert(int(k));
        }
    };

    // -- find ---------------------------------------------------

    {
        rbtree<IntNode> tree;
        for (size_t k = 0; k < N; k++)
            tree.insert(*nodes[k]);

        int found = 0;
        BENCHMARK("rbtree: find") {
            for (size_t k = 0; k < N; k++) {
                found += int(tree.contains(*nodes[k]) == true);
            }
        };
    }

    {
        std::set<int> set;
        for (size_t k = 0; k < N; k++)
            set.insert(int(k));

        int found = 0;
        BENCHMARK("std::set: find") {
            for (size_t k = 0; k < N; k++) {
                found += int(set.find(int(k)) != set.end());
            }
        };
    }

    {
        std::unordered_set<int> set;
        for (size_t k = 0; k < N; k++)
            set.insert(int(k));

        int found = 0;
        BENCHMARK("std::unordered_set: find") {
            for (size_t k = 0; k < N; k++) {
                found += int(set.find(int(k)) != set.end());
            }
        };
    }

    // -- erase --------------------------------------------------
  
    BENCHMARK_ADVANCED("rbtree: erase")(Catch::Benchmark::Chronometer meter) {
        rbtree<IntNode> tree;
        for (size_t k = 0; k < N; k++)
            tree.insert(*nodes[k]);
        meter.measure([&] { 
            for (size_t k = 0; k < N; k++)
                tree.erase(*nodes[k]);
        });
    };
  
    BENCHMARK_ADVANCED("std::set: erase")(Catch::Benchmark::Chronometer meter) {
        std::set<int> set;
        for (size_t k = 0; k < N; k++)
            set.insert(int(k));
        meter.measure([&] { 
            for (size_t k = 0; k < N; k++)
                set.erase(int(k));
        });
    };
  
    BENCHMARK_ADVANCED(
        "std::unordered set: erase")(Catch::Benchmark::Chronometer meter)
    {
        std::unordered_set<int> set;
        for (size_t k = 0; k < N; k++)
            set.insert(int(k));
        meter.measure([&] { 
            for (size_t k = 0; k < N; k++)
                set.erase(int(k));
        });
    };
}

// -- fuzz tests -------------------------------------------------------------

TEST_CASE("rbtree: fuzz tests")
{
    using set_type = rbtree<
        StringNode, void, get_key_for_value<StringNode>, std::less<>>;
    quick_rng rng = {494894094};

    std::vector<std::string> keys;
    std::set<std::string> set;
    set_type tree;

    auto is_equal = [&]() {
        for (const std::string& str : set) {
            REQUIRE(tree.find(str) != tree.end());
            REQUIRE(tree.find(str)->str == str);
        }

        int counter = 0;
        auto it_set = set.begin();
        auto it = tree.begin();

        for (StringNode& node : tree) {
            REQUIRE(*it_set == node.str);
            it_set++;
            counter++;
        }
    };

    auto add_string = [&]() {
        std::string str = random_ascii_string(0, 128, rng);
        keys.push_back(str);
        set.insert(str);
        tree.insert(*(new StringNode(str)));

        REQUIRE(set.find(str) != set.end());
        REQUIRE(tree.find(str) != tree.end());
        REQUIRE(tree.find(str)->str == str);
    };

    auto erase_string = [&]() {
        if (keys.empty()) return;
        size_t idx = size_t(rng.next()) % keys.size();
        std::string str = keys[idx];
        keys.erase(std::next(keys.begin(), idx));
        set.erase(str);
        tree.erase(str);
        REQUIRE(set.find(str) == set.end());
        REQUIRE(tree.find(str) == tree.end());
    };

    invoker inv(rng.next());
    inv.add(90.0, add_string);
    inv.add(10.0, erase_string);
    inv.add(10.0, is_equal);
    inv.run(20000);

    tree.clear_and_dispose([](StringNode* node) { delete node; });
}

// -- clear_and_dispose ------------------------------------------------------

struct ClearTester : rbtree_node<> {
    int* counter;
    int foo;

    ClearTester(int foo, int* counter) : foo(foo), counter(counter)
    {
        (*counter)++;
    }

    ~ClearTester()
    { 
        (*counter)--;
    }

    bool operator<(const ClearTester& other) const
    {
        return foo < other.foo;
    }
};

TEST_CASE("rbtree: clear and dispose")
{
    int counter = 0;
    constexpr int N = 10000;

    rbtree<ClearTester> tree;
    for (int k = 0; k < N; k++) {
        tree.insert(*(new ClearTester(k, &counter)));
    }
    REQUIRE(counter == N);
    tree.clear_and_dispose([](ClearTester* ct) { delete ct; });
    REQUIRE(counter == 0);
}