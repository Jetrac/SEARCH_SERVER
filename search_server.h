#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <cmath>
#include <tuple>
#include <set>
#include <map>
#include <algorithm>
#include <stdexcept>
#include <execution>
#include <deque>
#include <type_traits>

#include "document.h"
#include "string_processing.h"
#include "concurrent_map.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;

constexpr float NUMBERS_EQUAL_CHECK = 1e-6;

class SearchServer {
public:
    SearchServer() = default;

    template<typename StringContainer>
    explicit SearchServer(const StringContainer &stop_words);

    explicit SearchServer(const std::string &stop_words_text);

    explicit SearchServer(std::string_view stop_words_view);

    void AddDocument(int document_id, std::string_view document,DocumentStatus status,const std::vector<int> &ratings);
    //------------------------------------------------------------------------------------------------------------------

    void RemoveDocument(int document_id);

    template<typename ExecutionPolicy>
    void RemoveDocument(ExecutionPolicy policy, int document_id);
    //------------------------------------------------------------------------------------------------------------------

    std::vector<Document> FindTopDocuments(std::string_view raw_query) const;

    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentStatus status) const;

    template<typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(std::string_view raw_query,DocumentPredicate document_predicate) const;

    template<typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy policy, std::string_view raw_query) const;

    template<typename ExecutionPolicy>
    std::vector<Document>FindTopDocuments(ExecutionPolicy policy, std::string_view raw_query, DocumentStatus status) const;

    template<typename ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(ExecutionPolicy policy, std::string_view raw_query,DocumentPredicate document_predicate) const;
    //------------------------------------------------------------------------------------------------------------------

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::string_view raw_query, int document_id) const;

    template<typename ExecutionPolicy>
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(ExecutionPolicy policy, std::string_view raw_query, int document_id) const;
    //------------------------------------------------------------------------------------------------------------------

    std::set<int>::const_iterator begin() const;

    std::set<int>::const_iterator end() const;
    //------------------------------------------------------------------------------------------------------------------
    int GetDocumentCount() const;

    const std::map<std::string_view, double> &GetWordFrequencies(int document_id) const;
    //------------------------------------------------------------------------------------------------------------------
private:
    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    std::set<int> document_ids_;
    std::map<int, DocumentData> documents_;
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::map<int, std::map<std::string_view, double>> document_to_word_freqs_;

    std::set<std::string, std::less<>> all_words_;

    const std::set<std::string, std::less<>> stop_words_;
private:
    //------------------------------------------------------------------------------------------------------------------

    bool IsStopWord(std::string_view word) const;

    static bool IsValidWord(std::string_view word);

    std::vector<std::string_view> SplitIntoWordsNoStop(std::string_view text) const;
    //------------------------------------------------------------------------------------------------------------------

    QueryWord ParseQueryWord(std::string_view text) const;

    Query ParseQuery(std::string_view text, bool is_parallel = false) const;
    //------------------------------------------------------------------------------------------------------------------

    static int ComputeAverageRating(const std::vector<int> &ratings);

    double ComputeWordInverseDocumentFreq(std::string_view word) const;
    //------------------------------------------------------------------------------------------------------------------

    template<typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query &query,DocumentPredicate document_predicate) const;

    template<typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const std::execution::sequenced_policy& policy, const Query &query,DocumentPredicate document_predicate) const;

    template<typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const std::execution::parallel_policy& policy, const Query &query,DocumentPredicate document_predicate) const;
    //------------------------------------------------------------------------------------------------------------------
};

//Methods with template:
//----------------------------------------------------------------------------------------------------------------------
template<typename StringContainer>
SearchServer::SearchServer(const StringContainer &stop_words) : stop_words_(MakeUniqueNonEmptyStrings(stop_words)){
    if (!std::all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
        throw std::invalid_argument("Some of stop words are invalid");
    }
}
//----------------------------------------------------------------------------------------------------------------------
template<typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const {
    const Query query = ParseQuery(raw_query, false);

    auto matched_documents = FindAllDocuments(std::execution::seq, query, document_predicate);

    std::sort(matched_documents.begin(), matched_documents.end(),[](const Document &lhs, const Document &rhs) {
        if (std::abs(lhs.relevance - rhs.relevance) < NUMBERS_EQUAL_CHECK) {
            return lhs.rating > rhs.rating;
        } else {
            return lhs.relevance > rhs.relevance;
        }
    });

    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;
}

template<typename ExecutionPolicy, typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy policy, std::string_view raw_query,DocumentPredicate document_predicate) const {
    if constexpr (!std::is_same_v<ExecutionPolicy, std::execution::parallel_policy>) {
        return FindTopDocuments(raw_query, document_predicate);
    }

    const Query query = ParseQuery(raw_query, false);

    std::vector<Document> matched_documents = FindAllDocuments(policy, query, document_predicate);

    std::sort(policy, matched_documents.begin(), matched_documents.end(),[](const Document &lhs, const Document &rhs) {
        if (std::abs(lhs.relevance - rhs.relevance) < NUMBERS_EQUAL_CHECK) {
            return lhs.rating > rhs.rating;
        } else {
            return lhs.relevance > rhs.relevance;
        }
    });

    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;
}


template<typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy policy, std::string_view raw_query) const {
    return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}

template<typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy policy, std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(policy, raw_query,[status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
    });
}
//----------------------------------------------------------------------------------------------------------------------

template<typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query &query, DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;

    for (std::string_view word: query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq]: word_to_document_freqs_.find(word)->second) {
            const DocumentData &document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    for (std::string_view word: query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, _]: word_to_document_freqs_.find(word)->second) {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    for (const auto& [document_id, relevance] : document_to_relevance) {
        matched_documents.emplace_back(document_id, relevance, documents_.at(document_id).rating);
    }
    return matched_documents;
}

template<typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const std::execution::sequenced_policy& policy, const Query &query,DocumentPredicate document_predicate) const {
    return FindAllDocuments(query, document_predicate);
}

template<typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const std::execution::parallel_policy& policy, const Query &query, DocumentPredicate document_predicate) const {
    ConcurrentMap<int, double> document_to_relevance(20000);

    std::for_each(policy, query.plus_words.begin(), query.plus_words.end(),[&](const auto &word) {
        if (word_to_document_freqs_.count(word) == 0) {
            return;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq]: word_to_document_freqs_.at(word)) {
            const DocumentData &document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
            }
        }

    });

    std::for_each(
            policy,
            query.minus_words.begin(),query.minus_words.end(),
            [&](const auto &word) {
                if (word_to_document_freqs_.count(word) == 0) {
                    return;
                }
                for (const auto [document_id, _]: word_to_document_freqs_.at(word)) {
                    document_to_relevance.ERASE(document_id);
                }
            });

    const std::map<int, double> docs(document_to_relevance.BuildOrdinaryMap());

    std::vector<Document> matched_documents(docs.size());

    std::transform(policy, docs.begin(), docs.end(), matched_documents.begin(),
                   [&](const auto &item) {
                       return Document({item.first, item.second, documents_.at(item.first).rating});
                   });

    return matched_documents;
}
//----------------------------------------------------------------------------------------------------------------------

template<typename ExecutionPolicy>
std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(ExecutionPolicy policy, std::string_view raw_query,int document_id) const {
    if constexpr (!std::is_same_v<ExecutionPolicy, std::execution::parallel_policy>) {
        return MatchDocument(raw_query, document_id);
    }

    if (raw_query.empty()) {
        throw std::invalid_argument("Invalid raw_query");
    }

    if (documents_.count(document_id) == 0) {
        throw std::out_of_range("Invalid document_id");
    }

    const Query query = ParseQuery(raw_query, true);

    if (any_of(policy,query.minus_words.begin(), query.minus_words.end(), [this, document_id](std::string_view word) {
        return word_to_document_freqs_.count(word) && word_to_document_freqs_.find(word)->second.count(document_id);
    })) {
        return {std::vector<std::string_view>(), documents_.at(document_id).status};
    }

    std::vector<std::string_view> matched_words(query.plus_words.size());

    auto words_end = copy_if(
            policy,
            query.plus_words.begin(), query.plus_words.end(),
            matched_words.begin(),
            [this, document_id](std::string_view word) {
                return word_to_document_freqs_.count(word) && word_to_document_freqs_.find(word)->second.count(document_id);
            });

    std::sort(policy, matched_words.begin(), words_end);
    matched_words.erase(unique(matched_words.begin(), words_end), matched_words.end());

    return {matched_words, documents_.at(document_id).status};
}
//----------------------------------------------------------------------------------------------------------------------
template<typename ExecutionPolicy>
void SearchServer::RemoveDocument(ExecutionPolicy policy, int document_id) {
    if constexpr (!std::is_same_v<ExecutionPolicy, std::execution::parallel_policy>) {
        RemoveDocument(document_id);
    }
    if (documents_.count(document_id) == 0) {
        throw std::invalid_argument("There is no document with this id.");
    }

    if (document_ids_.find(document_id) == document_ids_.end()) {
        throw std::invalid_argument("Invalid document_id");
    }

    const auto &document_to_word_freqs_value = document_to_word_freqs_.find(document_id)->second;

    std::vector<std::string_view> words(document_to_word_freqs_.at(document_id).size());

    transform(std::execution::par,
              document_to_word_freqs_value.begin(),
              document_to_word_freqs_value.end(),
              words.begin(),
              [](auto& word) {
                  return word.first;
              });

    for_each(std::execution::par,
             words.begin(),
             words.end(),
             [&](auto & word) {
                 word_to_document_freqs_.erase(word_to_document_freqs_.find(word));
             });

    documents_.erase(document_id);
    document_ids_.erase(document_id);
    document_to_word_freqs_.erase(document_id);
}
//----------------------------------------------------------------------------------------------------------------------
