#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>

using namespace std;
 
const int MAX_RESULT_DOCUMENT_COUNT = 5;
 
string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}
 
int ReadLineWithNumber() {
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}
 
vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }
 
    return words;
}
 
struct Document {
    int id;
    double relevance;
};
 
class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }
 
    void AddDocument(int document_id, const string& document) {
        ++document_count_;
        const vector<string> words = SplitIntoWordsNoStop(document);
        for(const string& i : words){
            documents_[i][document_id] += 1.0 / words.size();
        }
        
    }
 
    vector<Document> FindTopDocuments(const string& raw_query) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query);
 
        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 return lhs.relevance > rhs.relevance;
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }
    
private:
    int document_count_ = 0;
    
    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };
 
    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };
 
    map<string, map<int, double>> documents_;
 
    set<string> stop_words_;
 
    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }
 
    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    
 
    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return {text, is_minus, IsStopWord(text)};
    }
 
    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                } else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    double IDFCalc(const string& i) const{
        return std::log(document_count_ / static_cast<double>(documents_.at(i).size()));
    }
 
    vector<Document> FindAllDocuments(const Query& query) const {
        vector<Document> matched_documents;
        map<int, double> rel;
        for(const string& word: query.plus_words){
            if(!documents_.count(word)) continue;
            
            double IDF = IDFCalc(word);
            for(const auto& [docid, tf] : documents_.at(word)){
               rel[docid] += tf * IDF;
            }
            
        }
        
        for(const string& word: query.minus_words){
            if(!documents_.count(word)) continue;
            
            for(const auto& [docid, tf] : documents_.at(word)){
                rel.erase(docid);
            }
        }
        
        
        for(const auto& [docid, relev] : rel){
            matched_documents.push_back({docid, relev});
        }
        return matched_documents;
    }
 
    static int MatchDocument(const map<string, set<int>>& content, const Query& query) {
        if (query.plus_words.empty()) {
            return 0;
        }
        set<string> matched_words;
        for (const auto& word : content) {
            if (query.minus_words.count(word.first) != 0) {
                return 0;
            }
            if (matched_words.count(word.first) != 0) {
                continue;
            }
            if (query.plus_words.count(word.first) != 0) {
                matched_words.insert(word.first);
            }
        }
        return static_cast<int>(matched_words.size());
    }
};
 
SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());
 
    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }
 
    return search_server;
}
 
int main() {
    const SearchServer search_server = CreateSearchServer();
 
    const string query = ReadLine();
    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", "
             << "relevance = "s << relevance << " }"s << endl;
    }
}
