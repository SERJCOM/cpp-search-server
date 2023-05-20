#include "search_server.h"

#include<iterator>

void SearchServer::AddDocument(int document_id, std::string_view document, DocumentStatus status,
                 const std::vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw std::invalid_argument("Invalid document_id"s);
    }

    //std::string documents_s(document);
    const auto words = SplitIntoWordsNoStop(document);

    const double inv_word_count = 1.0 / words.size();
    for (std::string_view word : words) {

        dictionary_.emplace_back(std::move(std::string(word)));

        word_to_document_freqs_[dictionary_.back()][document_id] += inv_word_count;
        word_to_freqs_[document_id][dictionary_.back()] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    document_ids_.insert(document_id);
}


int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

std::set<int>::iterator SearchServer::begin(){
    return document_ids_.begin();
}

std::set<int>::iterator SearchServer::end(){
    return document_ids_.end();
}

 const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    if(word_to_freqs_.count(document_id) == 0){
        static std::map<std::string_view, double> temp;
        return  temp;
    }

    return word_to_freqs_.at(document_id); // O(log N)
 }


bool SearchServer::IsStopWord(std::string_view word) const {
    return stop_words_.count(std::string(word)) > 0;
}


std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(std::string_view text) const {
    std::vector<std::string_view> words;
    //std::string_view copy(text);
    for (std::string_view word : SplitIntoWordsView(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Word "s + std::string(word) + " is invalid"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}



SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
    if (text.empty()) {
        throw std::invalid_argument("Query word is empty"s);
    }
    std::string_view word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw std::invalid_argument("Query word "s + std::string(text) + " is invalid");
    }

    return {word, is_minus, IsStopWord(word)};
}



SearchServer::Query SearchServer::ParseQuerySimple(std::string_view text) const {
    Query result;

    for (std::string_view word : SplitIntoWordsView(text)) {

        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            } else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }
    return result;
}

SearchServer::Query SearchServer::ParseQuery(std::string_view text) const {
    Query result = ParseQuerySimple(text);

    std::sort(result.minus_words.begin(), result.minus_words.end());
    auto last_m = std::unique(result.minus_words.begin(), result.minus_words.end());
    result.minus_words.erase(last_m, result.minus_words.end());

    std::sort(result.plus_words.begin(), result.plus_words.end());
    auto last_p = std::unique(result.plus_words.begin(), result.plus_words.end());
    result.plus_words.erase(last_p, result.plus_words.end());
    return result;
}



double SearchServer::ComputeWordInverseDocumentFreq(std::string_view word) const {
    return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

bool SearchServer::IsValidWord(std::string_view word){
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = 0;
    std::accumulate(ratings.begin(), ratings.end(), 0);

    return rating_sum / static_cast<int>(ratings.size());
}


std::tuple<std::vector<std::string_view>, DocumentStatus> 
SearchServer::MatchDocument(std::execution::parallel_policy policy, std::string_view raw_query, int document_id) const {
    
    auto query = ParseQuerySimple(raw_query);
    std::vector<std::string_view> matched_words(query.plus_words.size());

    if(std::any_of(policy, query.minus_words.begin(), query.minus_words.end(), [this, document_id](std::string_view word){
        return word_to_document_freqs_.at(word).count(document_id);
    })){
        matched_words.clear();
        return {matched_words, documents_.at(document_id).status};
    }


    auto last = std::copy_if(policy, query.plus_words.begin(), query.plus_words.end(), matched_words.begin(), [this, document_id](std::string_view word){
        return word_to_document_freqs_.at(word).count(document_id);
    });

    matched_words.erase(last, matched_words.end());

    std::sort(matched_words.begin(), matched_words.end());
    auto last_p = std::unique(matched_words.begin(), matched_words.end());
    matched_words.erase(last_p, matched_words.end());

    return {matched_words, documents_.at(document_id).status};
}


std::tuple<std::vector<std::string_view>, DocumentStatus> 
    SearchServer::MatchDocument(std::string_view raw_query, int document_id) const{
    auto query = ParseQuery(raw_query);

    std::vector<std::string_view> matched_words;
    matched_words.reserve(query.plus_words.size());

    for (std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.at(word).count(document_id)) {
            //matched_words.clear();
            return {matched_words, documents_.at(document_id).status};
        }
    }


    for (std::string_view word : query.plus_words) {
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }

return {matched_words, documents_.at(document_id).status};
}