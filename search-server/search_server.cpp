#include "search_server.h"
#include <string>
#include <vector>
#include <algorithm>
#include <tuple>
#include <cmath>
#include <numeric>
#include "document.h"
#include "string_processing.h"


SearchServer::SearchServer(const std::string& stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))
{
}

void SearchServer::AddDocument(int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw std::invalid_argument("Invalid document_id");
    }
    const auto words = SplitIntoWordsNoStop(document);
    
    const double inv_word_count = 1.0 / words.size();
    for (const std::string_view word : words) {
        word_to_document_freqs_[std::string{word}][document_id] += inv_word_count;
        word_freqs_[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    document_ids_.insert(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(std::execution::seq, raw_query, status);
}

std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query) const {
    return FindTopDocuments(std::execution::seq, raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}


std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::string_view raw_query, int document_id) const {
    const Query query = ParseQuery(raw_query);

    std::vector<std::string_view> matched_words;
    for (const std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(std::string(word)) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(std::string(word)).count(document_id)) {
            matched_words.clear();
            return { {}, documents_.at(document_id).status };
        }
    }
    for (const std::string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(std::string(word)) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(std::string(word)).count(document_id)) {
            matched_words.push_back(word);
        }
    }

    return { matched_words, documents_.at(document_id).status };
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::sequenced_policy& policy, std::string_view raw_query, int document_id) const {
    return MatchDocument(raw_query, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::parallel_policy& policy, std::string_view raw_query, int document_id) const {
    if ((document_id < 0) || (documents_.count(document_id) == 0)) {
            throw std::invalid_argument("document_id out of range");
        }

    const Query& query = ParseQueryParallel(raw_query);
    const auto& word_freqs = word_freqs_.at(document_id);
    
    if (std::any_of(query.minus_words.begin(),
                    query.minus_words.end(),
                    [&word_freqs](const std::string_view word) {
                        return word_freqs.count(word) > 0;
                    })) {
        return { {}, documents_.at(document_id).status };
    }

    std::vector<std::string_view> matched_words;
    matched_words.reserve(query.plus_words.size());
    std::copy_if(query.plus_words.begin(),
                 query.plus_words.end(),
                 std::back_inserter(matched_words),
                 [&word_freqs](const std::string_view word) {
                     return word_freqs.count(word) > 0;
                 });

    std::sort(policy, matched_words.begin(), matched_words.end());
    auto it = std::unique(matched_words.begin(), matched_words.end());
    matched_words.erase(it, matched_words.end());

    return { matched_words, documents_.at(document_id).status };
    
}



const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    static const std::map<std::string_view, double> dummy;
    if (word_freqs_.count(document_id) == 0) {
        return dummy;
    }
    else {
        return word_freqs_.at(document_id);
    }
}


bool SearchServer::IsStopWord(std::string_view word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(std::string_view word) {
    // A valid word must not contain special characters
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(std::string_view text) const {
    std::vector<std::string_view> words;
    for (const auto& word : SplitIntoWordsView(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("наличие недопустимых символов");
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}


int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) { 
    if (ratings.empty()) { 
        return 0; 
    } 
    auto rating_sum = std::accumulate(ratings.begin(),ratings.end(),0); 
    return rating_sum / static_cast<int>(ratings.size()); 
} 

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view  text) const { 
    if (text.empty()) { 
        throw std::invalid_argument("Query word is empty"); 
    } 
     
    bool is_minus = false; 
    if (text[0] == '-') { 
        is_minus = true; 
        text = text.substr(1); 
    } 
    if (text.empty() || text[0] == '-' || !IsValidWord(text)) { 
        throw std::invalid_argument("Query word " + std::string(text) + " is invalid"); 
    } 
 
    return {text, is_minus, IsStopWord(text)};
}



SearchServer::Query SearchServer::ParseQuery(std::string_view text) const {
    Query result;

    for (const std::string_view word : SplitIntoWordsView(text)) {
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            } else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }

    std::sort(result.minus_words.begin(), result.minus_words.end());
    std::sort(result.plus_words.begin(), result.plus_words.end());

    result.minus_words.erase(std::unique(result.minus_words.begin(), result.minus_words.end()), result.minus_words.end());
    result.plus_words.erase(std::unique(result.plus_words.begin(), result.plus_words.end()), result.plus_words.end());

    result.minus_words.shrink_to_fit();
    result.plus_words.shrink_to_fit();

    return result;
}

SearchServer::Query SearchServer::ParseQueryParallel(std::string_view text) const {
    Query result;

    for (auto word : SplitIntoWordsView(text)) {
        const QueryWord query_word(ParseQueryWord(word));
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            }
            else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }
    return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(std::string(word)).size());
}

void SearchServer::RemoveDocument(int document_id){
    documents_.erase(document_id);
    word_freqs_.erase(document_id);
    document_ids_.erase(document_id);
    for(auto &[word, documents] : word_to_document_freqs_){
        documents.erase(document_id);
    }
}

void SearchServer::RemoveDocument(const std::execution::parallel_policy&, int document_id){
    documents_.erase(document_id);
    document_ids_.erase(document_id);
    const auto& word_freqs = word_freqs_.at(document_id);
    std::vector<std::string> words(word_freqs.size());
    transform(std::execution::par, word_freqs.begin(), word_freqs.end(), words.begin(), [](const auto& word) {
        return word.first;
    });
    for_each(std::execution::par, words.begin(), words.end(), [this, document_id](const auto& word) {
        word_to_document_freqs_.at(word).erase(document_id);
    });
    word_freqs_.erase(document_id);
}

void SearchServer::RemoveDocument(const std::execution::sequenced_policy&, int document_id){
    documents_.erase(document_id);
    document_ids_.erase(document_id);
    const auto& word_freqs = word_freqs_.at(document_id);
    std::vector<std::string> words(word_freqs.size());
    transform(std::execution::seq, word_freqs.begin(), word_freqs.end(), words.begin(), [](const auto& word) {
        return word.first;
    });
    for(auto& word : words){
        word_to_document_freqs_.at(word).erase(document_id);
    }
    word_freqs_.erase(document_id);
}