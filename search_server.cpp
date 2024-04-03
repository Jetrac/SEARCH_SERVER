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
#include "search_server.h"

using namespace std;

//----------------------------------------------------------------------------------------------------------------------
SearchServer::SearchServer(const string& stop_words_text): SearchServer(SplitIntoWords(stop_words_text)) {}

SearchServer::SearchServer(string_view stop_words_view): SearchServer(SplitIntoWords(stop_words_view)) {}
//----------------------------------------------------------------------------------------------------------------------
void SearchServer::AddDocument(int document_id, string_view document, DocumentStatus status,const vector<int> &ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw invalid_argument("Invalid document_id"s);
    }
    const vector<string_view> words = SplitIntoWordsNoStop(document);

    const double inv_word_count = 1.0 / static_cast<double>(words.size());
    for (string_view word: words) {
        auto insert_word = all_words_.insert(string(word)); // ссылка на копию переданных в метод данных
        word_to_document_freqs_[*insert_word.first][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    document_ids_.insert(document_id);
}
//------------------------------------------------------------------------------------------------------------------
vector<Document> SearchServer::FindTopDocuments(string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
    });
}

vector<Document> SearchServer::FindTopDocuments(string_view raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

//----------------------------------------------------------------------------------------------------------------------
set<int>::const_iterator SearchServer::begin() const {
    return document_ids_.begin();
}
set<int>::const_iterator SearchServer::end() const {
    return document_ids_.end();
}
//----------------------------------------------------------------------------------------------------------------------
int SearchServer::GetDocumentCount() const {
    return static_cast<int>(documents_.size());
}

const map<string_view, double> &SearchServer::GetWordFrequencies(int document_id) const {
    auto iter = document_to_word_freqs_.find(document_id);
    if (iter != document_to_word_freqs_.end()) {
        return iter->second;
    }

    static const map<string_view, double> empty_map{};
    return empty_map;
}
//----------------------------------------------------------------------------------------------------------------------
void SearchServer::RemoveDocument(int document_id) {
    if (!documents_.count(document_id)) {
        throw invalid_argument("There is no document with this id.");
    }

    for (auto [word, frequency]: GetWordFrequencies(document_id)) {
        word_to_document_freqs_.erase(word_to_document_freqs_.find(word));
    }

    documents_.erase(document_id);
    document_ids_.erase(document_id);
    document_to_word_freqs_.erase(document_id);
}

//----------------------------------------------------------------------------------------------------------------------
tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(string_view raw_query, int document_id) const {
    if (raw_query.empty()) {
        throw invalid_argument("Invalid raw_query");
    }

    if (documents_.count(document_id) == 0) {
        throw out_of_range("Invalid document_id");
    }

    const Query query = ParseQuery(raw_query, false);

    if (any_of(query.minus_words.begin(), query.minus_words.end(),
               [this, document_id](string_view word) {
                   return word_to_document_freqs_.count(word) && word_to_document_freqs_.find(word)->second.count(document_id);
               })) {
        return {vector<string_view>(), documents_.at(document_id).status};
    }

    vector<string_view> matched_words;

    for (string_view word: query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.find(word)->second.count(document_id)) {
            matched_words.push_back(word);
        }
    }

    return {matched_words, documents_.at(document_id).status};
}


//----------------------------------------------------------------------------------------------------------------------
bool SearchServer::IsStopWord(string_view word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(string_view word) {
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

vector<string_view> SearchServer::SplitIntoWordsNoStop(string_view text) const {
    vector<string_view> words;
    for (string_view word: SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw invalid_argument("Word is invalid"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}
//----------------------------------------------------------------------------------------------------------------------
SearchServer::QueryWord SearchServer::ParseQueryWord(string_view text) const {
    if (text.empty()) {
        throw invalid_argument("Query word is empty"s);
    }

    bool is_minus = false;

    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }

    if (text.empty() || text[0] == '-' || !IsValidWord(text)) {
        throw invalid_argument("Query word is invalid");
    }
    return {text, is_minus, IsStopWord(text)};
}
//----------------------------------------------------------------------------------------------------------------------
SearchServer::Query SearchServer::ParseQuery(string_view text, bool is_parallel) const {
    Query result;
    for (string_view word: SplitIntoWords(text)) {
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            } else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }

    if (!is_parallel) {
        sort(result.minus_words.begin(), result.minus_words.end());
        result.minus_words.erase(unique(result.minus_words.begin(), result.minus_words.end()),result.minus_words.end());
        sort(result.plus_words.begin(), result.plus_words.end());
        result.plus_words.erase(unique(result.plus_words.begin(), result.plus_words.end()), result.plus_words.end());
    }

    return result;
}
//----------------------------------------------------------------------------------------------------------------------
int SearchServer::ComputeAverageRating(const vector<int> &ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

double SearchServer::ComputeWordInverseDocumentFreq(string_view word) const {
    return log(GetDocumentCount() * 1.0 / static_cast<double>(word_to_document_freqs_.find(word)->second.size()));
}
//----------------------------------------------------------------------------------------------------------------------
