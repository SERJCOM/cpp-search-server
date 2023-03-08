#include "search_server.h"



void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status,
                 const std::vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw std::invalid_argument("Invalid document_id"s);
    }
    const auto words = SplitIntoWordsNoStop(document);

    const double inv_word_count = 1.0 / words.size();
    for (const std::string& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        word_to_freqs_[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    document_ids_.push_back(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status) const {
    return FindTopDocuments(
        raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

std::vector<int>::iterator SearchServer::begin(){
    return document_ids_.begin();
}

std::vector<int>::iterator SearchServer::end(){
    return document_ids_.end();
}

 const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const {
    if(word_to_freqs_.count(document_id) == 0){
        return empty_map;
    }

    return word_to_freqs_.at(document_id); // O(log N)
 }


std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query,
                                                                   int document_id) const {

    LOG_DURATION_STREAM("Operation time"s, std::cout);
    
    const auto query = ParseQuery(raw_query);

    std::vector<std::string> matched_words;
    for (const std::string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    for (const std::string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            break;
        }
    }
    return {matched_words, documents_.at(document_id).status};
}


bool SearchServer::IsStopWord(const std::string& word) const {
    return stop_words_.count(word) > 0;
}


std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
    std::vector<std::string> words;
    for (const std::string& word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Word "s + word + " is invalid"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}


void SearchServer::RemoveDocument(int document_id){

    auto word_freq = word_to_freqs_[document_id]; //  O(log N)   нашли мапу слово-частота 

    for(const auto& word : word_freq){  // O(w)   
        word_to_document_freqs_[word.first].erase(document_id);  // находим слово и удаляем от туда айди  O(log W) + амор O(1)
    }

    auto it = std::find(document_ids_.begin(), document_ids_.end(), document_id);
    //std::cout << *it <<  " " <<  document_id << " " << document_ids_.size() << std::endl;
    document_ids_.erase(it); // log(N)

    documents_.erase( documents_.find(document_id));

    //std::cout << "size: " << document_ids_.size() << std::endl;
}



SearchServer::QueryWord SearchServer::ParseQueryWord(const std::string& text) const {
    if (text.empty()) {
        throw std::invalid_argument("Query word is empty"s);
    }
    std::string word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw std::invalid_argument("Query word "s + text + " is invalid");
    }

    return {word, is_minus, IsStopWord(word)};
}


SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {
    Query result;
    for (const std::string& word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.insert(query_word.data);
            } else {
                result.plus_words.insert(query_word.data);
            }
        }
    }
    return result;
}



double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
    return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

bool SearchServer::IsValidWord(const std::string& word){
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = 0;
    for (const int rating : ratings) {
        rating_sum += rating;
    }
    return rating_sum / static_cast<int>(ratings.size());
}



/*
цель: O(wN(logN+logW))

в цикле проходимся по всем документам; O(N)

берем мапу слов-частота O(logN)
и преобразуем в сет O(w)

O(N(logN + w))

в цикле проходимся по вектору уже добавленных (сетов) векторов потому что что мапа уже отсортирована и добавлять новые значения а потом сортировать слишком долго; O(N)
и смотрим на совпадение (сетов) векторов.  // O(N). Скорей всего векторы различаются 

если совпадения нет, то добавляем в вектор новый сет // O(1)
если есть совпадение то добавляем в другой вектор айди для последущего удаления // O(1)


проходим по новому вектору и удаляем документы которые там находятся // O(Nw(logN+logW))

*/