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

#include <random>
#include <type_traits>

#ifndef CATCH_CONFIG_MAIN // for intellisense
  #define CATCH_CONFIG_MAIN
#endif
#include <catch/catch.hpp>

#include <trie.h>

namespace {

std::unique_ptr<const std::vector<std::string>> s_random_words;

void build_radom_list() {
  std::vector<std::string> random_words;

  std::random_device d;
  auto rnd_seed = d();
  std::printf("RND seed: %u\n", rnd_seed);
  std::mt19937 gen{rnd_seed};

  std::uniform_int_distribution<> dis{2, 15}; // words between 2 and 15 chars in len
  std::uniform_int_distribution<> letter_dis{0, 25}; // letters
  for (int i = 0; i != 300; ++i) {
    random_words.emplace_back();

    auto& word = random_words.back();
    word.resize(dis(gen), 'a');

    std::transform(std::begin(word), std::end(word), std::begin(word),
      [&gen, &letter_dis](char) ->char { return 'a' + static_cast<char>(letter_dis(gen)); });
  }

  // sort remove uniques
  // since we're testing set-like tree structures we can't
  // have uniques when we go to confirm if everything got inserted
  // properly since duplicates won't have 2 entries
  std::sort(std::begin(random_words), std::end(random_words));
  random_words.erase(
    std::unique(std::begin(random_words), std::end(random_words)),
    std::end(random_words));

  // shuffle
  std::shuffle(std::begin(random_words), std::end(random_words), gen);

  s_random_words = std::make_unique<std::vector<std::string>>(std::move(random_words));
}

} // namespace [anon]

struct test_listener : Catch::TestEventListenerBase {
  using TestEventListenerBase::TestEventListenerBase;

    void testRunStarting(const Catch::TestRunInfo&) override {
        build_radom_list(); // construct the list of random strings to use
    }
};
CATCH_REGISTER_LISTENER(test_listener);

TEST_CASE("impl1", "[impl1::trie]") {
  std::vector<std::string> words;
  words.push_back("cat");
  words.push_back("bat");
  words.push_back("cake");
  words.push_back("bake");
  words.push_back("abcd");
  words.push_back("somereallylongword");

  trie::impl1::trie t;
  t.insert("cat");
  t.insert("bat");
  t.insert("cake");
  t.insert("bake");
  t.insert("abcd");
  t.insert("somereallylongword");

  SECTION("ensure all words are the same") {
    auto trie_words = t.get_words();

    auto tfirst = std::begin(trie_words);
    auto tlast = std::end(trie_words);
    REQUIRE(std::all_of(std::begin(words), std::end(words),
      [tfirst, tlast](const std::string& word) { return std::find(tfirst, tlast, word) != tlast; }));
  }

  REQUIRE(t.exists("cat"));
  REQUIRE(!t.exists("catt"));
  REQUIRE(!t.exists("catt"));
  REQUIRE(t.exists("bake"));
  REQUIRE(!t.exists("bbake"));
  REQUIRE(!t.exists("bbake"));

  std::string match;
  REQUIRE(t.prefix_match("so", match));
  REQUIRE(match == "somereallylongword");

  match.clear();
  REQUIRE(t.prefix_match("ba", match));
  REQUIRE(match == "bake"); // since 'k' comes before 't' in 'bake' vs bat'

  match.clear();
  REQUIRE(!t.prefix_match("zz", match));
  REQUIRE(match.empty());

  SECTION("permutations of 'abcd'") {
    std::string abcd = "abcd";
    while (std::next_permutation(std::begin(abcd), std::end(abcd))) {
      REQUIRE(!t.exists(abcd));
    }
  }

  // fill with other garbage
  {
    for (auto& word : *s_random_words) {
      t.insert(word);
    }

    SECTION("all words were actually inserted") {
      auto random_words = *s_random_words; // copy, yuck
      auto old_size = random_words.size();
      random_words.resize(old_size + words.size());
      std::copy(std::begin(words), std::end(words), std::begin(random_words) + old_size);

      auto trie_words = t.get_words();
      auto tfirst = std::begin(trie_words);
      auto tlast = std::end(trie_words);

      REQUIRE(trie_words.size() == random_words.size());

      REQUIRE(std::all_of(std::begin(random_words), std::end(random_words),
        [tfirst, tlast](const std::string& word) { return std::find(tfirst, tlast, word) != tlast; }));
    }
  }

  SECTION("retest starting invariants") {
    REQUIRE(t.exists("cat"));
    REQUIRE(t.exists("bake"));
    REQUIRE(t.exists("somereallylongword"));

    std::string match;
    REQUIRE(t.prefix_match("somereallylongword", match));
    REQUIRE(match == "somereallylongword");

    // we can't match spaces since we never instered a word with spaces
    REQUIRE(!t.prefix_match("thing invalid", match));
  }
}

TEST_CASE("impl2", "[impl2::trie]") {
  std::vector<std::string> words;
  words.push_back("cat");
  words.push_back("bat");
  words.push_back("cake");
  words.push_back("bake");
  words.push_back("abcd");
  words.push_back("somereallylongword");

  trie::impl2::trie t;
  t.insert("cat");
  t.insert("bat");
  t.insert("cake");
  t.insert("bake");
  t.insert("abcd");
  t.insert("somereallylongword");

  SECTION("ensure all words are the same") {
    auto trie_words = t.get_words();

    auto tfirst = std::begin(trie_words);
    auto tlast = std::end(trie_words);
    REQUIRE(std::all_of(std::begin(words), std::end(words),
      [tfirst, tlast](const std::string& word) { return std::find(tfirst, tlast, word) != tlast; }));
  }

  REQUIRE(t.exists("cat"));
  REQUIRE(!t.exists("catt"));
  REQUIRE(!t.exists("catt"));
  REQUIRE(t.exists("bake"));
  REQUIRE(!t.exists("bbake"));
  REQUIRE(!t.exists("bbake"));

  std::string match;
  REQUIRE(t.prefix_match("so", match));
  REQUIRE(match == "somereallylongword");

  match.clear();
  REQUIRE(t.prefix_match("ba", match));
  REQUIRE(match == "bake"); // since 'k' comes before 't' in 'bake' vs bat'

  match.clear();
  REQUIRE(!t.prefix_match("zz", match));
  REQUIRE(match.empty());

  SECTION("permutations of 'abcd'") {
    std::string abcd = "abcd";
    while (std::next_permutation(std::begin(abcd), std::end(abcd))) {
      REQUIRE(!t.exists(abcd));
    }
  }

  // fill with other garbage
  {
    for (auto& word : *s_random_words) {
      t.insert(word);
    }

    SECTION("all words were actually inserted") {
      auto random_words = *s_random_words; // copy, yuck
      auto old_size = random_words.size();
      random_words.resize(old_size + words.size());
      std::copy(std::begin(words), std::end(words), std::begin(random_words) + old_size);

      auto trie_words = t.get_words();
      auto tfirst = std::begin(trie_words);
      auto tlast = std::end(trie_words);

      REQUIRE(trie_words.size() == random_words.size());

      REQUIRE(std::all_of(std::begin(random_words), std::end(random_words),
        [tfirst, tlast](const std::string& word) { return std::find(tfirst, tlast, word) != tlast; }));
    }
  }

  SECTION("retest starting invariants") {
    REQUIRE(t.exists("cat"));
    REQUIRE(t.exists("bake"));
    REQUIRE(t.exists("somereallylongword"));

    std::string match;
    REQUIRE(t.prefix_match("somereallylongword", match));
    REQUIRE(match == "somereallylongword");

    // we can't match spaces since we never inserted a word with spaces
    REQUIRE(!t.prefix_match("thing invalid", match));
  }
}

TEST_CASE("impl3", "[impl3::trie]") {
  std::vector<std::string> words;
  words.push_back("cat");
  words.push_back("bat");
  words.push_back("cake");
  words.push_back("bake");
  words.push_back("abcd");
  words.push_back("somereallylongword");

  trie::impl3::trie<int> t;
  t.insert("cat", 1);
  t.insert("bat", 2);
  t.insert("cake", 3);
  t.insert("bake", 4);
  t.insert("abcd", 5);
  t.insert("somereallylongword", 6);

  SECTION("ensure all words are the same") {
    auto trie_words = t.get_words();

    auto tfirst = std::begin(trie_words);
    auto tlast = std::end(trie_words);
    REQUIRE(std::all_of(std::begin(words), std::end(words),
      [tfirst, tlast](const std::string& word) { return std::find(tfirst, tlast, word) != tlast; }));
  }

  REQUIRE(t.exists("cat"));
  REQUIRE(!t.exists("catt"));
  REQUIRE(!t.exists("catt"));
  REQUIRE(t.exists("bake"));
  REQUIRE(!t.exists("bbake"));
  REQUIRE(!t.exists("bbake"));
  REQUIRE(t.exists("somereallylongword"));

  int value;
  REQUIRE(t.value_at("cat", value));
  REQUIRE(value == 1);

  value = 0;
  REQUIRE(!t.value_at("catt", value));
  REQUIRE(value == 0);

  value = 0;
  REQUIRE(t.value_at("cake", value));
  REQUIRE(value == 3);

  value = 0;
  REQUIRE(t.value_at("abcd", value));
  REQUIRE(value == 5);

  value = 0;
  REQUIRE(t.value_at("somereallylongword", value));
  REQUIRE(value == 6);

  std::string match;
  REQUIRE(t.prefix_match("so", match));
  REQUIRE(match == "somereallylongword");

  match.clear();
  REQUIRE(t.prefix_match("ba", match));
  REQUIRE(match == "bake"); // since 'k' comes before 't' in 'bake' vs bat'

  match.clear();
  REQUIRE(!t.prefix_match("zz", match));
  REQUIRE(match.empty());

  SECTION("permutations of 'abcd'") {
    std::string abcd = "abcd";
    while (std::next_permutation(std::begin(abcd), std::end(abcd))) {
      REQUIRE(!t.exists(abcd));
    }
  }

  // fill with other garbage
  {
    for (auto& word : *s_random_words) {
      t.insert(word, 10);
    }

    SECTION("all words were actually inserted") {
      auto random_words = *s_random_words; // copy, yuck
      auto old_size = random_words.size();
      random_words.resize(old_size + words.size());
      std::copy(std::begin(words), std::end(words), std::begin(random_words) + old_size);

      auto trie_words = t.get_words();
      auto tfirst = std::begin(trie_words);
      auto tlast = std::end(trie_words);

      REQUIRE(trie_words.size() == random_words.size());

      REQUIRE(std::all_of(std::begin(random_words), std::end(random_words),
        [tfirst, tlast](const std::string& word) { return std::find(tfirst, tlast, word) != tlast; }));
    }
  }

  SECTION("retest starting invariants") {
    REQUIRE(t.exists("cat"));
    REQUIRE(t.exists("bake"));
    REQUIRE(t.exists("somereallylongword"));

    int value;
    REQUIRE(t.value_at("cat", value));
    REQUIRE(value == 1);

    value = 0;
    REQUIRE(t.value_at("cake", value));
    REQUIRE(value == 3);

    value = 0;
    REQUIRE(t.value_at("abcd", value));
    REQUIRE(value == 5);

    value = 0;
    REQUIRE(t.value_at("somereallylongword", value));
    REQUIRE(value == 6);

    std::string match;
    REQUIRE(t.prefix_match("somereallylongword", match));
    REQUIRE(match == "somereallylongword");

    // we can't match spaces since we never inserted a word with spaces
    REQUIRE(!t.prefix_match("thing invalid", match));
  }
}
