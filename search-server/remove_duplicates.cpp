#include "remove_duplicates.h"


void RemoveDuplicates(SearchServer& search_server){
    std::set<int> remove_;
    auto words_with_ids_ = search_server.GetWordsWithIds();
    int count = (search_server.end() - search_server.begin());  
    for(int i = 0; i < count; i++){
            if(words_with_ids_[i] == search_server.GetWordsWithIds()[i+1]){
                remove_.insert(i+1);
            }
    }
    for(auto i : remove_){
        std::cout << "Found duplicate document id " << i << std::endl;
        search_server.RemoveDocument(i);
    }
}

