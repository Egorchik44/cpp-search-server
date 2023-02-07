#include "remove_duplicates.h"
#include "document.h"
#include "log_duration.h"
#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

void RemoveDuplicates(SearchServer& search_server) {
  map<set<string>, int>  remove_;
  set<int> deleted_;
  for (auto document_id = search_server.begin(); document_id != search_server.end(); document_id++){
    set<string> words_;
    auto lots_of_words_ = search_server.GetWordFrequencies(*document_id);
    for (auto iterator = lots_of_words_.begin(); iterator != lots_of_words_.end(); iterator++){
    	words_.insert(iterator->first);
    }
    if(remove_.count(words_)){
    	deleted_.insert(*document_id);
    }else{
    	remove_.insert({words_, *document_id});
    }
  }

  for(auto id : deleted_){
          cout << "Found duplicate document id " << id << endl;
          search_server.RemoveDocument(id);
      }
}
