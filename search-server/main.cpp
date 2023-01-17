#include <iostream>
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>



//#include "search_server.h"

using namespace std;

/* Подставьте вашу реализацию класса SearchServer сюда */


const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
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
    int rating;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status,
                     const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    }

    template<typename Predicat>
    vector<Document> FindTopDocuments(const string& raw_query,
                                      Predicat pred) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, pred);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 if (abs(lhs.relevance - rhs.relevance) < 1e-6) {
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
    

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status = DocumentStatus::ACTUAL) const {
       return FindTopDocuments(raw_query, [&status](int document_id, DocumentStatus status_, int rating) { return status_ == status; });
    }

    int GetDocumentCount() const {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query,
                                                        int document_id) const {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
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

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

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

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = 0;
        for (const int rating : ratings) {
            rating_sum += rating;
        }
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return {text, is_minus, IsStopWord(text)};
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

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

    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template<typename Predicat>
    vector<Document> FindAllDocuments(const Query& query, Predicat pred) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                if (pred(document_id, documents_.at(document_id).status, documents_.at(document_id).rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                {document_id, relevance, documents_.at(document_id).rating});
        }
        return matched_documents;
    }
};

template <typename T>
void Print(ostream& out, const T& container) {
    bool first = false;
    for(const auto& element : container){
        if(first) out << ", ";
        out << element;
        first = true;
    }   
}

template <typename T>
ostream& operator<<(ostream& out, const vector<T>&container){
    out << "[";
    Print(out, container);
    out << "]";
    return out;
}

template <typename T>
ostream& operator<<(ostream& out, const set<T>&container){
    out << "{";
    Print(out, container);
    out << "}";
    return out;
}


template <typename K, typename T >
ostream& operator<<(ostream& out, const map<K, T>&container){
    out << "{";
    bool zap = false;
    for(const auto& [key, value] : container){
        if(zap) out << ", ";
        out << key << ": " << value;
        zap = true;
    }
    out << "}";
    return out;
}


template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
                     const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
                const string& hint) {
    if (!value) {
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))




template <typename T>
void RunTestImpl( T func, string name) {
    func();
    cerr << name << " OK"s << endl;
}

#define RUN_TEST(func)  RunTestImpl((func), #func)


// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
    }
}

void TestAddingDocuments(){
    const string content = "cat in the city"s;
    const string content1 = "dog in the city"s;
    const string content2 = "dog on the moon"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(1, content,  DocumentStatus::ACTUAL, ratings);
        server.AddDocument(2, content1,  DocumentStatus::ACTUAL, ratings);
        server.AddDocument(3, content2,  DocumentStatus::ACTUAL, ratings);

        ASSERT_EQUAL(server.FindTopDocuments("the").size(), 3);
    }
}

void TestStopWords(){
    const string content = "cat in the city"s;
    const string content1 = "dog in the city"s;
    const string content2 = "dog on the moon"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.SetStopWords("in on the");
        server.AddDocument(1, content,  DocumentStatus::ACTUAL, ratings);
        server.AddDocument(2, content1,  DocumentStatus::ACTUAL, ratings);
        server.AddDocument(3, content2,  DocumentStatus::ACTUAL, ratings);

        ASSERT_EQUAL(server.FindTopDocuments("the").size(), 0);
    }
}

void TestMinusWorld(){
    const string content = "cat in the city"s;
    const string content1 = "dog in the city"s;
    const string content2 = "dog on the moon"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(1, content,  DocumentStatus::ACTUAL, ratings);
        server.AddDocument(2, content1,  DocumentStatus::ACTUAL, ratings);
        server.AddDocument(3, content2,  DocumentStatus::ACTUAL, ratings);

        ASSERT_EQUAL(server.FindTopDocuments("cat city").size(), 2);
        ASSERT_EQUAL(server.FindTopDocuments("dog the -city").size(), 1);
    }
}

void TestRating(){
    const string content = "cat in the city"s;
    const string content1 = "dog in the city"s;
    const vector<int> ratings = {1, 2, 3};

    {
        SearchServer server;
        server.AddDocument(1, content,  DocumentStatus::ACTUAL, ratings);
        server.AddDocument(2, content1,  DocumentStatus::ACTUAL, ratings);
        ASSERT_EQUAL(server.FindTopDocuments("cat in the city")[0].id, 1);
        ASSERT_EQUAL(server.FindTopDocuments("cat in the city")[1].id, 2);
    }
}

void TestComputeRaiting(){
    const string content = "cat in the city"s;
    const string content1 = "dog in the city"s;
    {
        SearchServer server;
        server.AddDocument(1, content,  DocumentStatus::ACTUAL, {1,1,1});
        server.AddDocument(2, content1,  DocumentStatus::ACTUAL, {1,2,3});
        const auto res = server.FindTopDocuments("in the");
        ASSERT_HINT(res[1].id == 1 && res[1].rating == 1 , "id of the some element equal 1 and raiting equal 1"s );
        ASSERT_HINT(res[0].id == 2 && res[0].rating == 2 , "id of the some element equal 2 and raiting equal 2"s );
    }
}

void TestFilter(){
    const string content = "cat in the city"s;
    const string content1 = "dog in the city"s;
    {
        SearchServer server;
        server.AddDocument(1, content,  DocumentStatus::ACTUAL, {1,1,1});
        server.AddDocument(2, content1,  DocumentStatus::ACTUAL, {1,2,3});

        const auto res = server.FindTopDocuments("in the city", [](int id, DocumentStatus status, int rait){
            return rait == 1;
        });

        ASSERT_EQUAL(res.size(), 1);
    }
}

void TestSearchDocumentsByStatus(){
    const string content = "cat in the city"s;
    const string content1 = "cat in the city"s;
    {
        SearchServer server;
        server.AddDocument(1, content,  DocumentStatus::ACTUAL, {1,1,1});
        server.AddDocument(2, content1,  DocumentStatus::BANNED, {1,2,3});

        ASSERT_EQUAL(server.FindTopDocuments("cat in the city"s, DocumentStatus::BANNED).size(), 1);
    }
}

void TestRelevance(){
    const string content = "cat in the city"s;
    const string content1 = "dog in the city"s;
    {
        SearchServer server;
        server.AddDocument(1, content,  DocumentStatus::ACTUAL, {1,1,1});
        server.AddDocument(2, content1,  DocumentStatus::BANNED, {1,2,3});

        const auto temp = server.FindTopDocuments("cat city");
        ASSERT_EQUAL_HINT(temp[0].relevance, (1.0/4.0) * log(2 / 1) + (1 / 4) * log(2.0/2.0), "relevance to file CAT IN THE CITY");
    }
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestAddingDocuments);
    RUN_TEST(TestStopWords);
    RUN_TEST(TestRating);
    RUN_TEST(TestComputeRaiting);
    RUN_TEST(TestFilter);
    RUN_TEST(TestSearchDocumentsByStatus);
    RUN_TEST(TestRelevance);
}



// --------- Окончание модульных тестов поисковой системы -----------

int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
}
