#pragma once

#include <vector>
#include <iostream>

template<typename It>
class IteratorRange {
public:
    It begin() const {
        return begin_;
    }

    It end() const {
        return end_;
    }

    void SetBegin(It iter) {
        begin_ = iter;
    }

    void SetEnd(It iter) {
        end_ = iter;
    }

    int size() const {
        return distance(begin_, end_);
    }

private:
    It begin_;
    It end_;
};

template<typename It>
std::ostream &operator<<(std::ostream &out, const IteratorRange<It> &range) {
    for (It i = range.begin(); i != range.end(); ++i) {
        auto temp = *i;
        out << temp;
    }
    return out;
}

template<typename It>
class Paginator {
public:
    Paginator(It begin, It end, int page_size) {
        IteratorRange<It> page;
        page.SetBegin(begin);
        for (It i = begin; i - 1 != end; ++i) {
            page.SetEnd(i);
            if (page.size() == page_size) {
                pages_.push_back(page);
                page.SetBegin(i);
            }
        }
        if (page.size() != 0)
            pages_.push_back(page);
    }

    auto begin() const {
        return pages_.begin();
    }

    auto end() const {
        return pages_.end();
    }

private:
    std::vector<IteratorRange<It>> pages_;
};

template<typename Container>
auto Paginate(const Container &c, std::size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}
