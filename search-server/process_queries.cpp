#include <execution>

#include "process_queries.h"

std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries){
    std::vector<std::vector<Document>> result(queries.size());
    std::transform(std::execution::par, queries.begin(), queries.end(), result.begin(), [&search_server](auto &query){
        return search_server.FindTopDocuments(query);
    });
    return result;
}
std::vector<Document> ProcessQueriesJoined(const SearchServer& search_server, const std::vector<std::string>& queries){
    std::vector<Document> result;
    for(auto& process : ProcessQueries(search_server, queries)){
        std::transform(process.begin(), process.end(), back_inserter(result), [](Document doc){
        return doc;
    });
    }
    return result;
}