#pragma once

#include <vector>
#include <string>
#include <deque>
#include "search_server.h"

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server):server_(search_server) {}
    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
        std::vector<Document> temp = server_.FindTopDocuments(raw_query, document_predicate);
        QueryResult temp_query;
        temp_query.count_documents = temp.size();
        AddQueryResult(temp_query);
        return temp;
    }
    
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status) ;
    
    std::vector<Document> AddFindRequest(const std::string& raw_query);

    int GetNoResultRequests() const ;
    
private:

    struct QueryResult {
        int count_documents;
    };

    void AddQueryResult(const QueryResult& query);
    
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer& server_;
    int count_ = 0;
};
