#include "search_server.h"
#include "request_queue.h"
#include "paginator.h"
#include "remove_duplicates.h"

using namespace std;


int main() {
    SearchServer search_server("and in at"s);
    
    search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, {1, 2, 3});
    search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, {1, 2, 8});
    search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, {1, 3, 2});
    search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, {1, 1, 1});
    for(int i = 6; i < 1000; i++){
        search_server.AddDocument(i, "curly dog "s, DocumentStatus::ACTUAL, {1, 1, 1});
    }

    for(auto i : search_server.FindTopDocuments("curly")){
        cout << i << endl;
    }

    RemoveDuplicates(search_server);
    return 0;
}