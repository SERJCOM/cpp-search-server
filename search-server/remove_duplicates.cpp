#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server){

    using namespace std;

    std::vector<std::vector<string>> words_id; // size - N

    vector<int>  documents_id_to_delete;

    for(const int document_id : search_server){         // O(N)
        std::map<std::string, double> word_to_freq = search_server.GetWordFrequencies(document_id); // O(logN)   
        std::vector<string> words_; 
        for(const auto& [word, freq] : word_to_freq){
            words_.push_back(word);
        }

        bool match = false;
        
        for(const auto& words : words_id){
            if(words == words_){
                match = true;
                std::cout << "Found duplicate document id " << document_id << endl;
                break;
            }
        }
        if(match == true){
            documents_id_to_delete.push_back(document_id);
        }
        if(match == false) words_id.push_back(words_);
    }

    for(int id : documents_id_to_delete){
        search_server.RemoveDocument(id);
        //cout << "удален айди " << id << endl;
    }
}

