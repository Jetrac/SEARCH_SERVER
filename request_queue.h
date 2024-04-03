#pragma once

#include "search_server.h"

#include <string>
#include <deque>
#include <vector>

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer &search_server) : search_server_(search_server) {}

    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
    template<typename DocumentPredicate>
    std::vector<Document> AddFindRequest(std::string_view raw_query, DocumentPredicate document_predicate) {
        return RequestProcessing(search_server_.FindTopDocuments(raw_query, document_predicate));
    }

    std::vector<Document> AddFindRequest(std::string_view raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(std::string_view raw_query);

    int GetNoResultRequests() const;

private:
    struct QueryResult {
        QueryResult() {
            is_empty = 0;
        }

        bool is_empty;
    };

    const SearchServer &search_server_;
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    int NoResultRequests = 0;

    std::vector<Document> RequestProcessing(std::vector<Document> result);
};
