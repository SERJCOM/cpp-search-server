#pragma once

#include <string>
#include <vector>
#include <stdexcept>
#include <map>
#include <algorithm>
#include <cmath>
#include <utility>

#include "document.h"
#include "string_processing.h"
#include "read_input_functions.h"
#include "log_duration.h"
#include <execution>
#include <string_view>
#include <set>
#include <functional>
#include <deque>
#include <type_traits>
#include "concurrent_map.h"
#include <future>

const int MAX_RESULT_DOCUMENT_COUNT = 5;

using namespace std::literals;

class SearchServer {
public:

    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);
    
    explicit SearchServer()
        : SearchServer(
            SplitIntoWordsView(" "s))  // Invoke delegating constructor from string container
    {}

    explicit SearchServer(std::string_view stop_words_text)
        : SearchServer(
            SplitIntoWordsView(stop_words_text))  // Invoke delegating constructor from string container
    {}


    explicit SearchServer(const std::string& stop_words_text)
        : SearchServer(
            SplitIntoWordsView(stop_words_text))  // Invoke delegating constructor from string container
    {}


    void AddDocument(int document_id, std::string_view document, DocumentStatus status,
                     const std::vector<int>& ratings) ;

    template <typename DocumentPredicate, typename Policy>
    std::vector<Document> FindTopDocuments(Policy policy, std::string_view raw_query,
                                      DocumentPredicate document_predicate) const ;


    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(std::string_view raw_query,
                                      DocumentPredicate document_predicate) const ;


    template<typename Policy>
    std::vector<Document> FindTopDocuments(Policy polity, std::string_view raw_query, DocumentStatus status) const;


    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentStatus status) const{
        return FindTopDocuments(std::execution::seq, raw_query, status);
    }

    template<typename Policy>
    std::vector<Document> FindTopDocuments(Policy policy, std::string_view raw_query) const ;

    std::vector<Document> FindTopDocuments(std::string_view raw_query) const {
        return FindTopDocuments(std::execution::seq, raw_query);
    }

    int GetDocumentCount() const;

    std::set<int>::iterator begin();
    std::set<int>::iterator end();


    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

    //template<typename Policy>
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::execution::parallel_policy policy, std::string_view raw_query,
                                                        int document_id) const ;

    std::tuple<std::vector<std::string_view>, DocumentStatus> 
    MatchDocument( std::string_view raw_query, int document_id) const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> 
    MatchDocument(std::execution::sequenced_policy policy, std::string_view raw_query, int document_id) const {
        return MatchDocument(raw_query, document_id);
    }

    

    template<typename ExecutionPolicy>
    void RemoveDocument(ExecutionPolicy policy, int document_id);

    void RemoveDocument( int document_id){
        RemoveDocument(std::execution::seq, document_id);
    }

    static const int COUNT_BALLS = 8;
    
private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    std::set<std::string> stop_words_;
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_; // O(logW + logN)
    //std::map<std::string_view, int> word_to_document_;
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;
    std::map<int, std::map<std::string_view, double>> word_to_freqs_; // document_id to word to freqs  O(log N) + O(log W) = O(logN + logW)
    //std::map<std::string, double> empty_map;
    std::deque<std::string> dictionary_;

    bool IsStopWord(std::string_view word) const ;

    static bool IsValidWord(std::string_view word);

    std::vector<std::string_view> SplitIntoWordsNoStop(std::string_view text) const ;

    static int ComputeAverageRating(const std::vector<int>& ratings) ;

    // template <typename StringContainer>
    // void AppendStopWords(const StringContainer& stop_words);

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(std::string_view text) const ;

    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    Query ParseQuery(std::string_view text) const ;
    Query ParseQuerySimple(std::string_view text) const ;

    // Existence required
    double ComputeWordInverseDocumentFreq(std::string_view word) const ;

    template <typename DocumentPredicate, typename Policy>
    std::vector<Document> FindAllDocuments(Policy policy,
        const Query& query, DocumentPredicate document_predicate) const ;
};





template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words) // Extract non-empty stop words
{
    for(std::string_view word : MakeUniqueNonEmptyStrings(stop_words)){
        stop_words_.insert(std::move(std::string(word)));
    }

    if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
        throw std::invalid_argument("Some of stop words are invalid"s);
    }
}





template <typename DocumentPredicate, typename Policy>
    std::vector<Document> SearchServer::FindTopDocuments(Policy policy, std::string_view raw_query,
                                      DocumentPredicate document_predicate) const {

    const auto query = ParseQuery(raw_query);
    
    auto matched_documents = FindAllDocuments(policy, query, document_predicate);
    
    sort(policy, matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                if (std::abs(lhs.relevance - rhs.relevance) < 1e-6) {
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


template <typename DocumentPredicate>
    std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query,
DocumentPredicate document_predicate) const {
    return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
}

template<typename Policy>
std::vector<Document> SearchServer::FindTopDocuments(Policy policy, std::string_view raw_query, DocumentStatus status) const {
return FindTopDocuments(policy,
    raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
    });
}   

template<typename Policy>
    std::vector<Document> SearchServer::FindTopDocuments(Policy policy , std::string_view raw_query) const {
    return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}

template <typename DocumentPredicate, typename Policy>
std::vector<Document> SearchServer::FindAllDocuments(Policy policy, const Query& query,
                                    DocumentPredicate document_predicate) const {
    ConcurrentMap<int, double> document_to_relevance(8);


    std::for_each(policy,query.plus_words.begin(), query.plus_words.end(), [&](std::string_view word){

        if (word_to_document_freqs_.count(word)) {
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
                }
            }
        }
    });

    
   std::for_each(policy, query.minus_words.begin(), query.minus_words.end(), [&](std::string_view word){
        if (word_to_document_freqs_.count(word) ) {
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }
   });


    auto temp_map = document_to_relevance.BuildOrdinaryMap();
        
    int size = temp_map.size();
         
    std::vector<Document> matched_documents(size);

    int actual_size = 0;
    std::transform(temp_map.begin(), temp_map.end(), matched_documents.begin(), [this, &actual_size](auto i){
        actual_size++;
        return Document(i.first, i.second, documents_.at(i.first).rating);
    });

    matched_documents.resize(actual_size);

    return matched_documents;
}

template<typename ExecutionPolicy>
void SearchServer::RemoveDocument(ExecutionPolicy policy, int document_id){

    auto& word_freq = word_to_freqs_[document_id]; //  O(log N)   нашли мапу слово-частота 


    std::vector<std::string_view> v_words(word_freq.size());

    std::transform(policy, word_freq.begin(), word_freq.end(), v_words.begin(), [](auto& word){
      return word.first;                                                                            
    });

    std::for_each(policy, v_words.begin(), v_words.end(), [&](std::string_view word){
    
        word_to_document_freqs_[word].erase(document_id);
    });


    auto it = std::find(policy, document_ids_.begin(), document_ids_.end(), document_id);
    document_ids_.erase(it); // log(N)

    documents_.erase( documents_.find(document_id));

    word_to_freqs_.erase(word_to_freqs_.find(document_id));
}
