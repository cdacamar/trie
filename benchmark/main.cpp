/*
MIT License

Copyright (c) 2017 Cameron DaCamara

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include <cstring>

#include <chrono>
#include <iostream>
#include <random>
#include <sstream>
#include <tuple>
#include <type_traits>
#include <unordered_map>

#include <include/trie.h>

#include <tiny_benchmark.h>

#define ELMS 1000000
#define ELM_COUNT_IMPL(count) " in " #count " elements"
#define ELM_COUNT_INDIRECT(count) ELM_COUNT_IMPL(count)
#define ELM_COUNT ELM_COUNT_INDIRECT(ELMS)

#define ELM_COUNT_SMALL " in 7 elements"

#define ITERATIONS 1000000
#define ITER_COUNT_IMPL(iterations) " iterations: " #iterations
#define ITER_COUNT_INDIRECT(iterations) ITER_COUNT_IMPL(iterations)
#define ITER_COUNT ITER_COUNT_INDIRECT(ITERATIONS)

const std::string long_word = "anextremelylongwordthatshouldbeallocatedontheheapandcostabunchtocompareatonoftimeshopefullythatsthethoughtwhyamistillgoing";

int main() {
    INIT(); // initailize the benchmarking lib

    std::random_device d;

    auto rnd_seed = d();
    std::cout << "RND Seed: " << rnd_seed << '\n';
    std::mt19937 gen(rnd_seed);

    // generate random words for us to use to make benchmarks between the implementations fair
    std::vector<std::string> random_words;
    std::uniform_int_distribution<> dis(10, 100); // words between 10 and 100 chars in len (so some have to be on the heap)
    std::uniform_int_distribution<> letter_dis(0, 25); // letters
    for (int i = 0;i != ELMS;++i) {
        random_words.emplace_back();

        auto& word = random_words.back();
        word.resize(dis(gen), 'a');

        std::transform(std::begin(word), std::end(word), std::begin(word),
            [&gen, &letter_dis](char c) ->char { return 'a' + static_cast<char>(letter_dis(gen)); });
    }

    SECTION("BENCHMARK [baseline (unsorted): std::vector]")
    {
        std::vector<std::string> v;
        MEASURE("", v.push_back("cat"), v);
        v.push_back("bat");
        v.push_back("cake");
        v.push_back("bake");
        v.push_back("abcd");
        v.push_back("somereallylongword");
        v.push_back(long_word);

        auto exists = [&v] (const char* str) ->bool { return std::find(std::begin(v), std::end(v), str) != std::end(v); };

        MEASURE(ELM_COUNT_SMALL, exists("cat"));
        MEASURE(ELM_COUNT_SMALL, exists("catt"));
        MEASURE(ELM_COUNT_SMALL, exists("bake"));
        MEASURE(ELM_COUNT_SMALL, exists("bbake"));
        MEASURE(ELM_COUNT_SMALL, exists("bbake"));

        auto old_size = v.size();
        v.resize(old_size + random_words.size());
        std::copy(std::begin(random_words), std::end(random_words), std::begin(v) + old_size);

        // let's shuffle this thing for reasons
        std::shuffle(std::begin(random_words), std::end(random_words), gen);

        MEASURE(ELM_COUNT, exists("cat"));
        MEASURE(ELM_COUNT, exists("catt"));
        MEASURE(ELM_COUNT, exists("bake"));
        MEASURE(ELM_COUNT, exists("bbake"));
        MEASURE(ELM_COUNT, exists("bbake"));
        MEASURE(ELM_COUNT, exists("somereallylongword"));
        MEASURE(ELM_COUNT, exists(long_word.c_str()));

        MEASURE_EXPR(ITER_COUNT,
        for (int i = 0;i != ITERATIONS;++i) {
            tiny_bench::escape(exists(long_word.c_str()));
        });
    }

    SECTION("BENCHMARK [baseline (sorted): std::vector]")
    {
        std::vector<std::string> v;
        v.push_back("cat");
        v.push_back("bat");
        v.push_back("cake");
        v.push_back("bake");
        v.push_back("abcd");
        v.push_back("somereallylongword");
        v.push_back(long_word);

        MEASURE_EXPR(" sorting 6 elms", std::sort(std::begin(v), std::end(v)));

        auto exists = [&v] (const char* str) ->bool { return std::lower_bound(std::begin(v), std::end(v), str) != std::end(v); };

        MEASURE(ELM_COUNT_SMALL, exists("cat"));
        MEASURE(ELM_COUNT_SMALL, exists("catt"));
        MEASURE(ELM_COUNT_SMALL, exists("bake"));
        MEASURE(ELM_COUNT_SMALL, exists("bbake"));
        MEASURE(ELM_COUNT_SMALL, exists("bbake"));

        auto old_size = v.size();
        v.resize(old_size + random_words.size());
        std::copy(std::begin(random_words), std::end(random_words), std::begin(v) + old_size);

        MEASURE_EXPR(" sorting " ELM_COUNT, std::sort(std::begin(v), std::end(v)));

        MEASURE(ELM_COUNT, exists("cat"));
        MEASURE(ELM_COUNT, exists("catt"));
        MEASURE(ELM_COUNT, exists("bake"));
        MEASURE(ELM_COUNT, exists("bbake"));
        MEASURE(ELM_COUNT, exists("bbake"));
        MEASURE(ELM_COUNT, exists("somereallylongword"));
        MEASURE(ELM_COUNT, exists(long_word.c_str()));

        MEASURE_EXPR(ITER_COUNT,
        for (int i = 0;i != ITERATIONS;++i) {
            tiny_bench::escape(exists(long_word.c_str()));
        });
    }

    SECTION("BENCHMARK [baseline: std::unordered_map]") {
        typedef std::unordered_map<std::string, int> map_t;
        MEASURE_EXPR(" ctor time", map_t m);

        START_MEASURE();
        m["cat"] = 1;
        m["bat"] = 2;
        m["cake"] = 3;
        m["bake"] = 4;
        m["abcd"] = 5;
        m["somereallylongword"] = 6;
        m[long_word] = 7;
        STOP_MEASURE("time to insert 6 elements");

        auto exists = [&m] (const char* str) ->bool { return m.find(str) != std::end(m); };

        MEASURE(ELM_COUNT_SMALL, exists("cat"));
        MEASURE(ELM_COUNT_SMALL, exists("catt"));
        MEASURE(ELM_COUNT_SMALL, exists("bake"));
        MEASURE(ELM_COUNT_SMALL, exists("bbake"));
        MEASURE(ELM_COUNT_SMALL, exists("bbake"));

        MEASURE_EXPR(" inserting" ELM_COUNT,
        for (const auto& word : random_words) {
            m[word] = 10;
        });

        MEASURE(ELM_COUNT, exists("cat"));
        MEASURE(ELM_COUNT, exists("catt"));
        MEASURE(ELM_COUNT, exists("bake"));
        MEASURE(ELM_COUNT, exists("bbake"));
        MEASURE(ELM_COUNT, exists("bbake"));
        MEASURE(ELM_COUNT, exists("somereallylongword"));
        MEASURE(ELM_COUNT, exists(long_word.c_str()));

        MEASURE_EXPR(ITER_COUNT,
        for (int i = 0;i != ITERATIONS;++i) {
            tiny_bench::escape(exists(long_word.c_str()));
        });
    }

    SECTION("BENCHMARK [baseline: std::map]") {
        typedef std::map<std::string, int> map_t;
        MEASURE_EXPR(" ctor time", map_t m);

        START_MEASURE();
        m["cat"] = 1;
        m["bat"] = 2;
        m["cake"] = 3;
        m["bake"] = 4;
        m["abcd"] = 5;
        m["somereallylongword"] = 6;
        m[long_word] = 7;
        STOP_MEASURE("time to insert 6 elements");

        auto exists = [&m] (const char* str) ->bool { return m.find(str) != std::end(m); };

        MEASURE(ELM_COUNT_SMALL, exists("cat"));
        MEASURE(ELM_COUNT_SMALL, exists("catt"));
        MEASURE(ELM_COUNT_SMALL, exists("bake"));
        MEASURE(ELM_COUNT_SMALL, exists("bbake"));
        MEASURE(ELM_COUNT_SMALL, exists("bbake"));

        MEASURE_EXPR(" inserting" ELM_COUNT,
        for (const auto& word : random_words) {
            m[word] = 10;
        });

        MEASURE(ELM_COUNT, exists("cat"));
        MEASURE(ELM_COUNT, exists("catt"));
        MEASURE(ELM_COUNT, exists("bake"));
        MEASURE(ELM_COUNT, exists("bbake"));
        MEASURE(ELM_COUNT, exists("bbake"));
        MEASURE(ELM_COUNT, exists("somereallylongword"));
        MEASURE(ELM_COUNT, exists(long_word.c_str()));

        MEASURE_EXPR(ITER_COUNT,
        for (int i = 0;i != ITERATIONS;++i) {
            tiny_bench::escape(exists(long_word.c_str()));
        });
    }

    SECTION("BENCHMARK [impl1]")
    {
        MEASURE_EXPR(" ctor time", trie::impl1::trie t);

        START_MEASURE();
        t.insert("cat");
        t.insert("bat");
        t.insert("cake");
        t.insert("bake");
        t.insert("abcd");
        t.insert("somereallylongword");
        t.insert(long_word);
        STOP_MEASURE("time to insert 6 elements");

        MEASURE(ELM_COUNT_SMALL, t.exists("cat"));
        MEASURE(ELM_COUNT_SMALL, t.exists("catt"));
        MEASURE(ELM_COUNT_SMALL, t.exists("bake"));
        MEASURE(ELM_COUNT_SMALL, t.exists("bbake"));
        MEASURE(ELM_COUNT_SMALL, t.exists("bbake"));

        std::string match;
        MEASURE(ELM_COUNT, t.prefix_match("so", match));

        match.clear();
        MEASURE(ELM_COUNT, t.prefix_match("ba", match));

        match.clear();
        MEASURE(ELM_COUNT, t.prefix_match("zz", match));

        // fill with other garbage
        MEASURE_EXPR(" inserting" ELM_COUNT,
        for (auto& word : random_words) {
            t.insert(word);
        });

        MEASURE(ELM_COUNT, t.exists("cat"));
        MEASURE(ELM_COUNT, t.exists("catt"));
        MEASURE(ELM_COUNT, t.exists("bake"));
        MEASURE(ELM_COUNT, t.exists("bbake"));
        MEASURE(ELM_COUNT, t.exists("bbake"));
        MEASURE(ELM_COUNT, t.exists("somereallylongword"));
        MEASURE(ELM_COUNT, t.exists(long_word));

        match.clear();
        MEASURE(ELM_COUNT, t.prefix_match("so", match));

        match.clear();
        MEASURE(ELM_COUNT, t.prefix_match("ba", match));

        match.clear();
        MEASURE(ELM_COUNT, t.prefix_match("zz", match));

        MEASURE_EXPR(ITER_COUNT,
        for (int i = 0;i != ITERATIONS;++i) {
            tiny_bench::escape(t.exists(long_word));
        });
    }

    SECTION("BENCHMARK [impl2]")
    {
        MEASURE_EXPR(" ctor time", trie::impl2::trie t);

        START_MEASURE();
        t.insert("cat");
        t.insert("bat");
        t.insert("cake");
        t.insert("bake");
        t.insert("abcd");
        t.insert("somereallylongword");
        t.insert(long_word);
        STOP_MEASURE("time to insert 6 elements");

        MEASURE(ELM_COUNT_SMALL, t.exists("cat"));
        MEASURE(ELM_COUNT_SMALL, t.exists("catt"));
        MEASURE(ELM_COUNT_SMALL, t.exists("bake"));
        MEASURE(ELM_COUNT_SMALL, t.exists("bbake"));
        MEASURE(ELM_COUNT_SMALL, t.exists("bbake"));

        std::string match;
        MEASURE(ELM_COUNT, t.prefix_match("so", match));

        match.clear();
        MEASURE(ELM_COUNT, t.prefix_match("ba", match));

        match.clear();
        MEASURE(ELM_COUNT, t.prefix_match("zz", match));

        // fill with other garbage
        MEASURE_EXPR(" inserting" ELM_COUNT,
        for (auto& word : random_words) {
            t.insert(word);
        });

        MEASURE(ELM_COUNT, t.exists("cat"));
        MEASURE(ELM_COUNT, t.exists("catt"));
        MEASURE(ELM_COUNT, t.exists("bake"));
        MEASURE(ELM_COUNT, t.exists("bbake"));
        MEASURE(ELM_COUNT, t.exists("bbake"));
        MEASURE(ELM_COUNT, t.exists("somereallylongword"));
        MEASURE(ELM_COUNT, t.exists(long_word));

        match.clear();
        MEASURE(ELM_COUNT, t.prefix_match("so", match));

        match.clear();
        MEASURE(ELM_COUNT, t.prefix_match("ba", match));

        match.clear();
        MEASURE(ELM_COUNT, t.prefix_match("zz", match));

        MEASURE_EXPR(ITER_COUNT,
        for (int i = 0;i != ITERATIONS;++i) {
            tiny_bench::escape(t.exists(long_word));
        });
    }

    SECTION("BENCHMARK [impl3]")
    {
        MEASURE_EXPR(" ctor time", trie::impl3::trie<int> t);

        START_MEASURE();
        t.insert("cat", 1);
        t.insert("bat", 2);
        t.insert("cake", 3);
        t.insert("bake", 4);
        t.insert("abcd", 5);
        t.insert("somereallylongword", 6);
        t.insert(long_word, 7);
        STOP_MEASURE("time to insert 6 elements");

        MEASURE(ELM_COUNT_SMALL, t.exists("cat"));
        MEASURE(ELM_COUNT_SMALL, t.exists("catt"));
        MEASURE(ELM_COUNT_SMALL, t.exists("bake"));
        MEASURE(ELM_COUNT_SMALL, t.exists("bbake"));
        MEASURE(ELM_COUNT_SMALL, t.exists("bbake"));

        std::string match;
        MEASURE(ELM_COUNT, t.prefix_match("so", match));

        match.clear();
        MEASURE(ELM_COUNT, t.prefix_match("ba", match));

        match.clear();
        MEASURE(ELM_COUNT, t.prefix_match("zz", match));

        // fill with other garbage
        MEASURE_EXPR(ELM_COUNT,
        for (auto& word : random_words) {
            t.insert(word, 10);
        });

        MEASURE(ELM_COUNT, t.exists("cat"));
        MEASURE(ELM_COUNT, t.exists("catt"));
        MEASURE(ELM_COUNT, t.exists("bake"));
        MEASURE(ELM_COUNT, t.exists("bbake"));
        MEASURE(ELM_COUNT, t.exists("bbake"));
        MEASURE(ELM_COUNT, t.exists("somereallylongword"));
        MEASURE(ELM_COUNT, t.exists(long_word));

        int value;
        MEASURE(ELM_COUNT, t.value_at("cat", value));
        MEASURE(ELM_COUNT, t.value_at("bake", value));
        MEASURE(ELM_COUNT, t.value_at("not in list", value));

        match.clear();
        MEASURE(ELM_COUNT, t.prefix_match("so", match));

        match.clear();
        MEASURE(ELM_COUNT, t.prefix_match("ba", match));

        match.clear();
        MEASURE(ELM_COUNT, t.prefix_match("zz", match));

        MEASURE_EXPR(ITER_COUNT,
        for (int i = 0;i != ITERATIONS;++i) {
            tiny_bench::escape(t.exists(long_word));
        });
    }
}