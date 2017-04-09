#pragma once

#include <cassert>

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace trie {

namespace impl1 {

// the stupid dumb implementation
class trie {
    struct trie_node_t_ {
        std::map<char, trie_node_t_> children;
        bool is_word;

        trie_node_t_(): is_word(false) { }
    };
    trie_node_t_ root_;
public:
    trie() { }
    ~trie() { }

    void insert(const std::string& word) {
        auto first = &root_;
        for (char c : word) {
            first = &first->children[c]; // create or pull the next node
        }
        first->is_word = true;
    }

    bool exists(const std::string& word) const {
        auto first = &root_;
        for (char c : word) {
            auto found = first->children.find(c);

            if (found == std::end(first->children)) {
                return false;
            }
            first = &found->second;
        }

        return first->is_word;
    }

    bool prefix_match(const std::string& prefix, std::string& matching_word) const {
        // first we need to find where this node is
        auto first = &root_;
        for (char c : prefix) {
            auto found = first->children.find(c);

            if (found == std::end(first->children)) {
                return false;
            }
            first = &found->second;
        }

        // matching word is at least our prefix
        matching_word = prefix;

        if (first->is_word) {
            return true;
        }

        // get the first child and depth first search find a word node
        while (!first->is_word) {
            if (first->children.empty()) {
                assert( false && "prog error" );
                return false;
            }
            auto next = std::begin(first->children);
            first = &next->second;
            matching_word += next->first;
        }

        return true;
    }

    std::vector<std::string> get_words() const {
        std::vector<std::string> ret;

        for (const auto& child : root_.children) {
            get_words_impl_(ret, std::string(1, child.first), child.second);
        }

        return ret;
    }

private:
    void get_words_impl_(std::vector<std::string>& words, std::string prefix, const trie_node_t_& node) const {
        if (node.is_word) words.push_back(prefix);
        if (node.children.empty()) return;

        for (const auto& child : node.children) {
            get_words_impl_(words, prefix + child.first, child.second);
        }
    }
};

} // namespace impl1


namespace impl2 {

namespace detail {

struct node_concept_t {
    virtual ~node_concept_t() { }

    struct visitor_t;
    struct mvisitor_t;

    virtual void accept(const visitor_t& v) const = 0;
    virtual void accept(mvisitor_t& v) = 0;
};

struct leaf_node_t;
struct branch_node_t;
struct node_concept_t::visitor_t {
    virtual void operator()(const leaf_node_t&   leaf)   const = 0;
    virtual void operator()(const branch_node_t& branch) const = 0;
};

struct node_concept_t::mvisitor_t {
    virtual void operator()(leaf_node_t&   leaf)   = 0;
    virtual void operator()(branch_node_t& branch) = 0;
};

struct leaf_node_t : node_concept_t {
    std::string data;

    void accept(const visitor_t& visitor) const override { visitor(*this); }
    void accept(mvisitor_t& mvisitor) override { mvisitor(*this); }
};

struct branch_node_t : node_concept_t {
    std::map<char, std::unique_ptr<node_concept_t>> children;
    bool is_word;

    branch_node_t(): is_word(false) { }

    void accept(const visitor_t& visitor) const override { visitor(*this); }
    void accept(mvisitor_t& mvisitor) override { mvisitor(*this); }
};

inline
std::pair<std::unique_ptr<branch_node_t>, branch_node_t*> build_branches(std::string::const_iterator first, std::string::const_iterator last) {
    std::unique_ptr<branch_node_t> root(new branch_node_t);

    auto parent = root.get();
    for (;first != last;++first) {
        auto child(new branch_node_t);
        parent->children[*first].reset(std::move(child));

        parent = child; // move to child
    }

    return std::make_pair(std::move(root), parent);
}

inline
std::unique_ptr<leaf_node_t> make_leaf(std::string::const_iterator first, std::string::const_iterator last) {
    std::unique_ptr<leaf_node_t> l(new leaf_node_t);
    l->data.append(first, last);
    return std::move(l);
}

inline
std::unique_ptr<node_concept_t> breakup_leaf(const leaf_node_t& leaf, std::string::const_iterator common_first, std::string::const_iterator common_second) {
    // first we want to find where the common prefixes end
    // since MSVC11 doesn't have the C++14 std::mismatch(first1,last1,first2,last2) form, let's just write it
    auto first1 = std::begin(leaf.data);
    auto last1  = std::end(leaf.data);
    auto first2 = common_first;
    auto last2  = common_second;
    while (first1 != last1 && first2 != last2 && *first1 == *first2) {
        ++first1; ++first2;
    }

    // base case (adding same word)
    if (first1 == last1 && first2 == last2) {
        return nullptr;
    }

    // basic first case: we consumed all of the leaf data, so let's return a branch leading down to this
    //                   node where we split
    if (first1 == last1) {
        auto root_leaf = build_branches(std::begin(leaf.data), last1);

        // since this happened we want to annotate the bottom of the tree that it _was_ a word
        root_leaf.second->is_word = true;

        // now fill in the remaining leaf
        // we use std::next here because the leaf contains data under it, not its own char as the first char
        root_leaf.second->children[*first2] = make_leaf(std::next(first2), last2);

        return std::move(root_leaf.first);
    }

    // case 2: we exhausted the word data.  Split up to the prefix part and construct a new leaf rooted at the end of the first prefix match
    if (first2 == last2) {
        auto root_leaf = build_branches(common_first, last2);

        // since this happened we want to annotate the bottom of the tree that it _was_ a word
        root_leaf.second->is_word = true;

        // now fill in the remaining leaf
        // we use std::next here because the leaf contains data under it, not its own char as the first char
        root_leaf.second->children[*first1] = make_leaf(std::next(first1), last1);

        return std::move(root_leaf.first);
    }

    // case 3: we've exhausted neither, build branches for both paths and construct two leaf nodes
    auto root_leaf = build_branches(std::begin(leaf.data), first1); // first1 is where the range differs

    // leaf for the old leaf
    {
        // we use std::next here because the leaf contains data under it, not its own char as the first char
        root_leaf.second->children[*first1] = make_leaf(std::next(first1), last1);
    }

    // leaf for the new incoming word
    {
        // we use std::next here because the leaf contains data under it, not its own char as the first char
        root_leaf.second->children[*first2] = make_leaf(std::next(first2), last2);
    }

    return std::move(root_leaf.first);
}

} // namespace detail

class trie {
    typedef detail::node_concept_t node_concept_t;

    detail::branch_node_t root_;
public:
    trie() { }
    ~trie() { }

    void insert(const std::string& word) {
        if (word.empty()) return;

        auto w_first = std::begin(word);
        auto w_last  = std::end(word);

        auto first = root_.children.find(*w_first);

        if (first == std::end(root_.children)) {
            // new leaf node
            // we use std::next here because the leaf contains data under it, not its own char as the first char
            root_.children[*w_first] = detail::make_leaf(std::next(w_first), w_last);
            return;
        }

        // behavior switching on node type
        struct insert_visitor : node_concept_t::mvisitor_t {
            std::string::const_iterator      first;
            std::string::const_iterator      last;
            detail::branch_node_t*           parent;

            insert_visitor(std::string::const_iterator& first, std::string::const_iterator& last,
                           detail::branch_node_t* parent):
                first(first), last(last), parent(parent) { }

            void operator()(detail::branch_node_t& branch) override {
                if (first == last) {
                    // annotate this node that it's a word
                    branch.is_word = true;
                    return;
                }

                auto next = branch.children.find(*first);
                if (next == std::end(branch.children)) {
                    // found place to insert leaf
                    branch.children[*first] = detail::make_leaf(std::next(first), last);
                    return;
                }

                // recurse down the branch
                ++first; // move forward
                parent = &branch;
                next->second->accept(*this);
            }
            void operator()(detail::leaf_node_t& leaf) override {
                // we need to break this leaf apart
                // --first is ok because we checked this on entry to the top-level function
                auto new_node = detail::breakup_leaf(leaf, first, last);
                if (!new_node) {
                    // this indicates no change needs to be made to the tree
                    // the prefixes matched
                    return;
                }
                parent->children[*--first] = std::move(new_node);
            }
        } visitor(++w_first, w_last, &root_);

        first->second->accept(visitor);
    }

    bool exists(const std::string& word) const {
        if (word.empty()) return false;

        auto first = std::begin(word);
        auto last  = std::end(word);
        bool ret;

        struct exists_visitor : node_concept_t::visitor_t {
            std::string::const_iterator* first;
            std::string::const_iterator* last;
            bool*                        result;

            exists_visitor(std::string::const_iterator& first, std::string::const_iterator& last, bool& result):
                first(&first), last(&last), result(&result) { *this->result = false; }

            void operator()(const detail::branch_node_t& branch) const {
                if (*first == *last && branch.is_word) {
                    *result = true;
                    return;
                }

                auto next = branch.children.find(**first);
                if (next == std::end(branch.children)) {
                    *result = false;
                    return;
                }

                ++*first; // advance
                next->second->accept(*this);
            }
            void operator()(const detail::leaf_node_t& leaf) const {
                // compare the remaining string to the leaf value
                auto lfirst = std::begin(leaf.data);
                auto llast  = std::end(leaf.data);

                *result = std::distance(lfirst, llast) == std::distance(*first, *last) &&
                          std::equal(lfirst, llast, *first);
            }
        } visitor(first, last, ret);

        root_.accept(visitor);

        return ret;
    }

    bool prefix_match(const std::string& prefix, std::string& matching_word) const {
        if (prefix.empty()) return false;
        matching_word.reserve(prefix.size()); // small optimization

        auto first = std::begin(prefix);
        auto last  = std::end(prefix);
        bool ret;

        struct prefix_match_visitor : node_concept_t::visitor_t {
            std::string::const_iterator* first;
            std::string::const_iterator* last;
            std::string*                 match;
            bool*                        result;

            prefix_match_visitor(std::string::const_iterator& first, std::string::const_iterator& last,
                                 std::string& match, bool& result):
                first(&first), last(&last), match(&match), result(&result) { *this->result = false; }

            void operator()(const detail::branch_node_t& branch) const {
                if (*first == *last) {
                    // find the first leaf we can match (alphabetical order)
                    if (branch.is_word) {
                        *result = true;
                        return;
                    }

                    // recurse down
                    auto next = std::begin(branch.children);
                    assert( next != std::end(branch.children) && "prog error" );
                    match->push_back(next->first);
                    next->second->accept(*this);
                    return;
                }

                auto next = branch.children.find(**first);
                if (next == std::end(branch.children)) {
                    *result = false;
                    return;
                }

                match->push_back(**first); // eat char
                ++*first; // advance
                next->second->accept(*this);
            }
            void operator()(const detail::leaf_node_t& leaf) const {
                if (*first == *last) {
                    match->append(leaf.data); // just append the whole node
                    *result = true;
                    return;
                }

                auto lfirst = std::begin(leaf.data);
                auto llast  = std::end(leaf.data);

                // ensure the prefix will both fit and is shared
                *result = std::distance(*first, *last) <= std::distance(lfirst, llast) &&
                          std::equal(*first, *last, lfirst);

                if (*result) {
                    match->append(lfirst, llast);
                }
                else {
                    match->clear(); // invalid
                }
            }
        } visitor(first, last, matching_word, ret);

        root_.accept(visitor);

        return ret;
    }

    std::vector<std::string> get_words() const {
        std::vector<std::string> ret;
        std::string              working_prefix;

        struct print_visitor : node_concept_t::visitor_t {
            std::string*              working_prefix;
            std::vector<std::string>* result;

            print_visitor(std::string& working_prefix, std::vector<std::string>& result):
                working_prefix(&working_prefix), result(&result) { }

            void operator()(const detail::branch_node_t& branch) const {
                if (branch.is_word) result->push_back(*working_prefix);

                for (const auto& child : branch.children) {
                    working_prefix->push_back(child.first);
                    child.second->accept(*this);
                    working_prefix->pop_back();
                }
            }
            void operator()(const detail::leaf_node_t& leaf) const {
                result->push_back(*working_prefix + leaf.data);
            }
        } visitor(working_prefix, ret);

        root_.accept(visitor);

        return ret;
    }
};

} // namespace impl2


namespace impl3 {

namespace detail {

template <typename T>
struct node_concept_t {
    virtual ~node_concept_t() { }

    struct visitor_t;
    struct mvisitor_t;

    virtual void accept(const visitor_t& v) const = 0;
    virtual void accept(mvisitor_t& v) = 0;
};

template <typename T>
struct leaf_node_t;
template <typename T>
struct branch_node_t;
template <typename T>
struct branch_value_node_t;

template <typename T>
struct node_concept_t<T>::visitor_t {
    virtual void operator()(const leaf_node_t<T>&         leaf)    const = 0;
    virtual void operator()(const branch_node_t<T>&       branch)  const = 0;
    virtual void operator()(const branch_value_node_t<T>& vbranch) const = 0;
};

template <typename T>
struct node_concept_t<T>::mvisitor_t {
    virtual void operator()(leaf_node_t<T>&         leaf)    = 0;
    virtual void operator()(branch_node_t<T>&       branch)  = 0;
    virtual void operator()(branch_value_node_t<T>& vbranch) = 0;
};

template <typename T>
struct leaf_node_t : node_concept_t<T> {
    std::string data;
    T value;

    leaf_node_t(T value): value(std::move(value)) { }

    void accept(const visitor_t& visitor) const override { visitor(*this); }
    void accept(mvisitor_t& mvisitor) override { mvisitor(*this); }
};

template <typename T>
struct branch_node_t : node_concept_t<T> {
    std::map<char, std::unique_ptr<node_concept_t<T>>> children;

    branch_node_t() { }

    virtual void accept(const visitor_t& visitor) const override { visitor(*this); }
    virtual void accept(mvisitor_t& mvisitor) override { mvisitor(*this); }
};

template <typename T>
struct branch_value_node_t : branch_node_t<T> {
    T value;

    branch_value_node_t(T value): value(std::move(value)) { }

    void accept(const visitor_t& visitor) const override { visitor(*this); }
    void accept(mvisitor_t& mvisitor) override { mvisitor(*this); }
};

template <typename T>
std::pair<std::unique_ptr<branch_node_t<T>>, branch_node_t<T>*> build_branches(std::string::const_iterator first, std::string::const_iterator last) {
    std::unique_ptr<branch_node_t<T>> root(new branch_node_t<T>);

    auto parent = root.get();
    for (;first != last;++first) {
        auto child(new branch_node_t<T>);
        parent->children[*first].reset(std::move(child));

        parent = child; // move to child
    }

    return std::make_pair(std::move(root), parent);
}

template <typename T>
std::pair<std::unique_ptr<branch_node_t<T>>, branch_value_node_t<T>*> build_branches_to_value(std::string::const_iterator first, std::string::const_iterator last, T value) {
    if (first == last) {
        std::unique_ptr<branch_value_node_t<T>> root(new branch_value_node_t<T>(std::move(value)));
        auto parent = root.get();
        return std::make_pair(std::move(root), parent);
    }

    auto short_last = std::prev(last);
    auto branches   = build_branches<T>(first, short_last);

    // the last element is where we want to place the value branch
    // short_last is a valid iterator
    auto child(new branch_value_node_t<T>(std::move(value)));
    branches.second->children[*short_last].reset(child);

    return std::make_pair(std::move(branches.first), child);
}

template <typename T>
std::unique_ptr<leaf_node_t<T>> make_leaf(std::string::const_iterator first, std::string::const_iterator last, T value) {
    std::unique_ptr<leaf_node_t<T>> l(new leaf_node_t<T>(std::move(value)));
    l->data.append(first, last);
    return std::move(l);
}

template <typename T>
std::unique_ptr<node_concept_t<T>> breakup_leaf(const leaf_node_t<T>& leaf, T leaf_value, std::string::const_iterator common_first, std::string::const_iterator common_second, T value) {
    // first we want to find where the common prefixes end
    // since MSVC11 doesn't have the C++14 std::mismatch(first1,last1,first2,last2) form, let's just write it
    auto first1 = std::begin(leaf.data);
    auto last1  = std::end(leaf.data);
    auto first2 = common_first;
    auto last2  = common_second;
    while (first1 != last1 && first2 != last2 && *first1 == *first2) {
        ++first1; ++first2;
    }

    // base case (adding same word)
    if (first1 == last1 && first2 == last2) {
        return nullptr;
    }

    // basic first case: we consumed all of the leaf data, so let's return a branch leading down to this
    //                   node where we split
    if (first1 == last1) {
        // *_to_value annotates the branch that it is a word
        auto root_leaf = build_branches_to_value(std::begin(leaf.data), last1, std::move(leaf_value));

        // now fill in the remaining leaf
        // we use std::next here because the leaf contains data under it, not its own char as the first char
        root_leaf.second->children[*first2] = make_leaf(std::next(first2), last2, std::move(value));

        return std::move(root_leaf.first);
    }

    // case 2: we exhausted the word data.  Split up to the prefix part and construct a new leaf rooted at the end of the first prefix match
    if (first2 == last2) {
        // *_to_value annotates this branch that it's a value at the end
        auto root_leaf = build_branches_to_value(common_first, last2, std::move(value));

        // now fill in the remaining leaf
        // we use std::next here because the leaf contains data under it, not its own char as the first char
        root_leaf.second->children[*first1] = make_leaf(std::next(first1), last1, std::move(leaf_value));

        return std::move(root_leaf.first);
    }

    // case 3: we've exhausted neither, build branches for both paths and construct two leaf nodes
    auto root_leaf = build_branches<T>(std::begin(leaf.data), first1); // first1 is where the range differs

    // leaf for the old leaf
    {
        // we use std::next here because the leaf contains data under it, not its own char as the first char
        root_leaf.second->children[*first1] = make_leaf(std::next(first1), last1, std::move(leaf_value));
    }

    // leaf for the new incoming word
    {
        // we use std::next here because the leaf contains data under it, not its own char as the first char
        root_leaf.second->children[*first2] = make_leaf(std::next(first2), last2, std::move(value));
    }

    return std::move(root_leaf.first);
}

} // namespace detail

template <typename T>
class trie {
    typedef detail::node_concept_t<T> node_concept_t;

    detail::branch_node_t<T> root_;
public:
    trie() { }
    ~trie() { }

    void insert(const std::string& word, T value) {
        if (word.empty()) return;

        auto w_first = std::begin(word);
        auto w_last  = std::end(word);

        auto first = root_.children.find(*w_first);

        if (first == std::end(root_.children)) {
            // new leaf node
            // we use std::next here because the leaf contains data under it, not its own char as the first char
            root_.children[*w_first] = detail::make_leaf(std::next(w_first), w_last, std::move(value));
            return;
        }

        // behavior switching on node type
        struct insert_visitor : node_concept_t::mvisitor_t {
            std::string::const_iterator      first;
            std::string::const_iterator      last;
            detail::branch_node_t<T>*        parent;
            T*                               value;

            insert_visitor(std::string::const_iterator& first, std::string::const_iterator& last,
                           detail::branch_node_t<T>* parent, T& value):
                first(first), last(last), parent(parent), value(&value) { }

            void operator()(detail::branch_node_t<T>& branch) override {
                if (first == last) {
                    // gut this branch and make it a branch value node
                    std::unique_ptr<detail::branch_value_node_t<T>> new_branch(
                        new detail::branch_value_node_t<T>(std::move(*value)));
                    new_branch->children = std::move(branch.children);

                    // re-parent (--first) is a valid iterator since this was checked at the top-level function
                    parent->children[*--first] = std::move(new_branch);
                    return;
                }

                auto next = branch.children.find(*first);
                if (next == std::end(branch.children)) {
                    // found place to insert leaf
                    branch.children[*first] = detail::make_leaf(std::next(first), last, std::move(*value));
                    return;
                }

                // recurse down the branch
                ++first; // move forward
                parent = &branch;
                next->second->accept(*this);
            }
            void operator()(detail::branch_value_node_t<T>& vbranch) override {
                if (first == last) {
                    // prefixes matched but we landed at a branch node...
                    // the user _should_ have just called reset_value()
                    return;
                }

                auto next = vbranch.children.find(*first);
                if (next == std::end(vbranch.children)) {
                    // found place for leaf
                    vbranch.children[*first] = detail::make_leaf(std::next(first), last, std::move(*value));
                    return;
                }

                // recurse down the branch
                ++first; // move forward
                parent = &vbranch;
                next->second->accept(*this);
            }
            void operator()(detail::leaf_node_t<T>& leaf) override {
                // we need to break this leaf apart
                // --first is ok because we checked this on entry to the top-level function
                auto new_node = detail::breakup_leaf(leaf, std::move(leaf.value), first, last, std::move(*value));
                if (!new_node) {
                    // this indicates no change needs to be made to the tree
                    // the prefixes matched
                    // in fact, the user should have just called reset_value()
                    return;
                }
                parent->children[*--first] = std::move(new_node);
            }
        } visitor(++w_first, w_last, &root_, value);

        first->second->accept(visitor);
    }

    bool exists(const std::string& word) const {
        auto node = lookup_node_(std::begin(word), std::end(word));

        return node != nullptr;
    }

    bool value_at(const std::string& word, T& value) const {
        auto node = lookup_node_(std::begin(word), std::end(word));

        if (!node) return false;

        bool extracted;

        struct value_extract_visitor : node_concept_t::visitor_t {
            bool* extracted;
            T*    value;

            value_extract_visitor(bool& extracted, T& value): extracted(&extracted), value(&value) { *this->extracted = false; }

            void operator()(const detail::branch_node_t<T>& branch) const {
                // no value here
            }
            void operator()(const detail::branch_value_node_t<T>& vbranch) const {
                *value     = vbranch.value;
                *extracted = true;
            }
            void operator()(const detail::leaf_node_t<T>& leaf) const {
                *value     = leaf.value;
                *extracted = true;
            }
        } visitor(extracted, value);

        node->accept(visitor);

        return extracted;
    }

    bool prefix_match(const std::string& prefix, std::string& matching_word) const {
        auto prefix_end = std::begin(prefix);
        auto node       = lookup_node_prefix_(std::begin(prefix), std::end(prefix), prefix_end);

        if (!node) return false;

        // create the matching word based on where the prefix ended
        // this is necessary since the prefix could land somewhere in 
        // a leaf node.  We only want to start with the characters from
        // branches leading down to a leaf.
        matching_word = std::string(std::begin(prefix), prefix_end);

        bool ret;

        struct prefix_match_visitor : node_concept_t::visitor_t {
            std::string* match;
            bool*        result;

            prefix_match_visitor(std::string& match, bool& result):
                match(&match), result(&result) { *this->result = false; }

            void operator()(const detail::branch_node_t<T>& branch) const {
                // recurse down
                auto next = std::begin(branch.children);
                assert( next != std::end(branch.children) && "prog error" );
                match->push_back(next->first);
                next->second->accept(*this);
            }
            void operator()(const detail::branch_value_node_t<T>& vbranch) const {
                *result = true; // done
            }
            void operator()(const detail::leaf_node_t<T>& leaf) const {
                match->append(leaf.data); // just append the whole node
                *result = true;
            }
        } visitor(matching_word, ret);

        node->accept(visitor);

        return ret;
    }

    std::vector<std::string> get_words() const {
        std::vector<std::string> ret;
        std::string              working_prefix;

        struct print_visitor : node_concept_t::visitor_t {
            std::string*              working_prefix;
            std::vector<std::string>* result;

            print_visitor(std::string& working_prefix, std::vector<std::string>& result):
                working_prefix(&working_prefix), result(&result) { }

            void operator()(const detail::branch_node_t<T>& branch) const {
                // visit children
                for (const auto& child : branch.children) {
                    working_prefix->push_back(child.first);
                    child.second->accept(*this);
                    working_prefix->pop_back();
                }
            }
            void operator()(const detail::branch_value_node_t<T>& vbranch) const {
                result->push_back(*working_prefix);

                // visit children
                for (const auto& child : vbranch.children) {
                    working_prefix->push_back(child.first);
                    child.second->accept(*this);
                    working_prefix->pop_back();
                }
            }
            void operator()(const detail::leaf_node_t<T>& leaf) const {
                result->push_back(*working_prefix + leaf.data);
            }
        } visitor(working_prefix, ret);

        root_.accept(visitor);

        return ret;
    }

private:
    const node_concept_t* lookup_node_(std::string::const_iterator first, std::string::const_iterator last) const {
        if (first == last) return nullptr;

        const node_concept_t* ret;

        struct lookup_visitor : node_concept_t::visitor_t {
            std::string::const_iterator* first;
            std::string::const_iterator* last;
            const node_concept_t**       result;

            lookup_visitor(std::string::const_iterator& first, std::string::const_iterator& last, const node_concept_t*& result):
                first(&first), last(&last), result(&result) { *this->result = nullptr; }

            void operator()(const detail::branch_node_t<T>& branch) const {
                if (*first == *last) {
                    // not found
                    return;
                }

                auto next = branch.children.find(**first);
                if (next == std::end(branch.children)) {
                    // not found
                    return;
                }

                ++*first; // advance
                next->second->accept(*this);
            }
            void operator()(const detail::branch_value_node_t<T>& vbranch) const {
                if (*first == *last) {
                    *result = &vbranch;
                    return;
                }

                auto next = vbranch.children.find(**first);
                if (next == std::end(vbranch.children)) {
                    // not found
                    return;
                }

                ++*first; // advance
                next->second->accept(*this);
            }
            void operator()(const detail::leaf_node_t<T>& leaf) const {
                // compare the remaining string to the leaf value
                auto lfirst = std::begin(leaf.data);
                auto llast  = std::end(leaf.data);

                if (std::distance(lfirst, llast) == std::distance(*first, *last) &&
                    std::equal(lfirst, llast, *first)) {
                    *result = &leaf;
                }
            }
        } visitor(first, last, ret);

        root_.accept(visitor);

        return ret;
    }

    const node_concept_t* lookup_node_prefix_(std::string::const_iterator first, std::string::const_iterator last, std::string::const_iterator& prefix_end) const {
        if (first == last) return nullptr;

        const node_concept_t* ret;

        struct lookup_prefix_visitor : node_concept_t::visitor_t {
            std::string::const_iterator* first;
            std::string::const_iterator* last;
            const node_concept_t**       result;

            lookup_prefix_visitor(std::string::const_iterator& first, std::string::const_iterator& last, const node_concept_t*& result):
                first(&first), last(&last), result(&result) { *this->result = nullptr; }

            void operator()(const detail::branch_node_t<T>& branch) const {
                if (*first == *last) {
                    // best match
                    *result = &branch;
                    return;
                }

                auto next = branch.children.find(**first);
                if (next == std::end(branch.children)) {
                    // not found
                    return;
                }

                ++*first; // advance
                next->second->accept(*this);
            }
            void operator()(const detail::branch_value_node_t<T>& vbranch) const {
                if (*first == *last) {
                    *result = &vbranch;
                    return;
                }

                auto next = vbranch.children.find(**first);
                if (next == std::end(vbranch.children)) {
                    // not found
                    return;
                }

                ++*first; // advance
                next->second->accept(*this);
            }
            void operator()(const detail::leaf_node_t<T>& leaf) const {
                // compare the remaining string to the leaf value
                auto lfirst = std::begin(leaf.data);
                auto llast  = std::end(leaf.data);

                // compare to the end of this prefix
                if (std::distance(*first, *last) <= std::distance(lfirst, llast) &&
                    std::equal(*first, *last, lfirst)) {
                    *result = &leaf;
                }
            }
        } visitor(first, last, ret);

        root_.accept(visitor);

        prefix_end = first;

        return ret;
    }
};

} // namespace impl3

} // namespace trie