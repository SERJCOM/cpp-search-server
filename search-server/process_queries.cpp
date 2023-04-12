#include "process_queries.h"

#include <algorithm>
#include <execution>


std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries){

    std::vector<std::vector<Document>> result(queries.size());


    std::transform(std::execution::par, queries.begin(), queries.end(),result.begin(),
    [&search_server](const std::string& quer){
        return search_server.FindTopDocuments(quer);
    }
    );

    return result;
}

std::vector<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries){

    auto temp = ProcessQueries(search_server, queries);
    std::vector<Document> result;
    result.reserve(temp.size() * MAX_RESULT_DOCUMENT_COUNT);
    // std::transform(std::execution::par,temp.begin(), temp.end(), result.begin(), [](const std::vector<Document>& vec){
    //     return std::list<Document>(vec.begin(), vec.end());
    // });

    for(const auto& vectors : temp){
        //result.insert(result.end(), result.begin(), vectors.end());
        result.insert(result.end(), vectors.begin(), vectors.end());
    }

    return result;

}


