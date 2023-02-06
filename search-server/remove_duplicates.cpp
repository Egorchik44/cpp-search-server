#include "remove_duplicates.h"
#include "document.h"
#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

void RemoveDuplicates(SearchServer& search_server) {
  vector <pair<int, vector<string>>>  remove_;
  for (const int document_id : search_server.GetDocument_ids()){
    vector<string> words_;
    auto lots_of_words_ = search_server.GetWordFrequencies(document_id);
    for (auto iterator = lots_of_words_.begin(); iterator != lots_of_words_.end(); iterator++){
    	words_.push_back(iterator->first);
    }
    remove_.push_back(pair(document_id, words_));
  }
   for (int i = 0; i < static_cast<int>(remove_.size()); i++){
    for (int j = i + 1; j < static_cast<int>(remove_.size()); j++){
      if (remove_[i].second == remove_[j].second) {
        cout << "Found duplicate document id " << remove_[j].first << endl;
        search_server.RemoveDocument(remove_[j].first);
        remove_.erase(remove_.begin() + j);
        j--;
      }
    }
  }
}
