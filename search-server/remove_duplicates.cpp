#include "remove_duplicates.h"
#include "document.h"
#include <algorithm>
#include <iostream>
#include <map>
#include <string>
#include <vector>


void RemoveDuplicates(SearchServer& search_server) {
  map<int, vector<string>>  remove_;
  for (auto document_id = search_server.begin(); document_id != search_server.end(); document_id++){
    vector<string> words_;
    auto lots_of_words_ = search_server.GetWordFrequencies(*document_id);
    for (auto iterator = lots_of_words_.begin(); iterator != lots_of_words_.end(); iterator++){
    	words_.push_back(iterator->first);
    }
    remove_.insert({*document_id, words_});
  }
  for (auto pos_outer = remove_.begin(); pos_outer != remove_.end(); pos_outer++){
    for (auto pos_inner = ++remove_.begin(); pos_inner != remove_.end(); pos_inner++){
    	if((pos_outer->second) == (pos_inner->second) && pos_outer->first != pos_inner->first) {
    	    cout << "Found duplicate document id " << pos_inner->first << endl;
    	    search_server.RemoveDocument(pos_inner->first);
       	    remove_.erase(pos_inner);
    	}
    }
  }
}

