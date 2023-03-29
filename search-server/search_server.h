#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <tuple>
#include <map>
#include <set>
#include <cmath>
#include <execution>
#include "document.h"
#include "read_input_functions.h"
#include "string_processing.h"
#include "log_duration.h"
#include "concurrent_map.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5; 
const double EPSILON = 1e-6; 

class SearchServer {
public:
    
    
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);
    explicit SearchServer(const std::string& stop_words_text);
    
    void AddDocument(int document_id, std::string_view document, DocumentStatus status,
                     const std::vector<int>& ratings);

    template <typename DocumentPredicate, typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query, DocumentPredicate document_predicate) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const;

    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query, DocumentStatus status) const;
    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentStatus status) const;
    
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query) const;
    std::vector<Document> FindTopDocuments(std::string_view raw_query) const;

    int GetDocumentCount() const;

    int GetDocumentId(int index) const;


    auto begin(){
        return document_ids_.begin();
    }

    auto end(){
        return document_ids_.end();
    }

    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::string_view raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::parallel_policy&,const std::string_view raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::sequenced_policy&,const std::string_view raw_query, int document_id) const;

    void RemoveDocument(int document_id);
    void RemoveDocument(const std::execution::parallel_policy&, int document_id);
    void RemoveDocument(const std::execution::sequenced_policy&, int document_id);

    void RemoveDuplicates(SearchServer& search_server);

    auto& GetWordsWithIds(){
        return words_with_ids_;
    }
private:

    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    const std::set<std::string, std::less<>> stop_words_;
    std::map<std::string, std::map<int, double>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;
    std::map<int, std::map<std::string_view, double>> word_freqs_;
    std::map<int, std::set<std::string>> words_with_ids_;

    bool IsStopWord(std::string_view word) const;

    static bool IsValidWord(std::string_view word);

    std::vector<std::string_view> SplitIntoWordsNoStop(std::string_view text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(std::string_view text) const;

    
    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    Query ParseQuery(std::string_view text) const;

    Query ParseQueryParallel(std::string_view text) const;
        
    double ComputeWordInverseDocumentFreq(const std::string_view word) const;

     template<typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const;

    template<typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const std::execution::sequenced_policy& policy, std::string_view raw_query, DocumentPredicate document_predicate) const;
    
    template<typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const std::execution::parallel_policy& policy, std::string_view raw_query, DocumentPredicate document_predicate) const;
};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer &stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words))  // Extract non-empty stop words
{
    if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
        throw std::invalid_argument("Some of stop words are invalid");
    }
}


template <typename DocumentPredicate, typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query, DocumentPredicate document_predicate) const {
    auto matched_documents = FindAllDocuments(policy, raw_query, document_predicate);

    std::sort(policy,
        matched_documents.begin(), matched_documents.end(),
        [](const Document& lhs, const Document& rhs) {
            if (std::abs(lhs.relevance - rhs.relevance) < EPSILON) {
                return lhs.rating > rhs.rating;
            }
            else {
                return lhs.relevance > rhs.relevance;
            }
        });

    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const {
    return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(policy, raw_query,
        [&status](int document_id, DocumentStatus new_status, int rating) {
            return new_status == status;
    });
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query) const {
    return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}

template<typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const {
    const auto query = ParseQuery(raw_query);
    std::map<int, double> relevance_doc;

    for (auto word : query.plus_words) {
        if (word_to_document_freqs_.count(std::string(word)) == 0) {
            continue;
        }
        const double inverse_document_ = ComputeWordInverseDocumentFreq(word);
        for (auto [document_id, term_freq] : word_to_document_freqs_.at(std::string(word))) {
            const DocumentData& documents_data = documents_.at(document_id);
            if (document_predicate(document_id, documents_data.status, documents_data.rating)) {
                relevance_doc[document_id] += term_freq * inverse_document_;
            }
        }
    }
    
    for (auto word : query.minus_words) {
        if (word_to_document_freqs_.count(std::string(word)) == 0) {
            continue;
        }
        for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(std::string(word))) {
            relevance_doc.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    for (auto [document_id, relevance] : relevance_doc) {
        matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}

template<typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const std::execution::sequenced_policy& policy, std::string_view raw_query, DocumentPredicate document_predicate) const {
    std::map<int, double> relevance_doc;
    const auto query = ParseQuery(raw_query);

    for (auto word : query.plus_words) {
        if (word_to_document_freqs_.count(std::string(word)) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(std::string(word))) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                relevance_doc[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    for (auto word : query.minus_words) {
        if (word_to_document_freqs_.count(std::string(word)) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(std::string(word))) {
            relevance_doc.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : relevance_doc) {
        matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}

template<typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const std::execution::parallel_policy& policy, std::string_view raw_query, DocumentPredicate document_predicate) const {
    ConcurrentMap<int, double> relevance_doc(10);
    const auto query = ParseQuery(raw_query);

    std::for_each(policy,
        query.minus_words.begin(), query.minus_words.end(),
        [this, &relevance_doc](std::string_view word) {
            if (word_to_document_freqs_.count(std::string(word))) {
                for (const auto [document_id, _] : word_to_document_freqs_.at(std::string(word))) {
                    relevance_doc.Erase(document_id);
                }
            }
    });

    std::for_each(policy,
        query.plus_words.begin(), query.plus_words.end(),
        [this, &document_predicate, &relevance_doc](std::string_view word) {
            if (word_to_document_freqs_.count(std::string(word))) {
                const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
                for (const auto [document_id, term_freq] : word_to_document_freqs_.at(std::string(word))) {
                    const auto& document_data = documents_.at(document_id);
                    if (document_predicate(document_id, document_data.status, document_data.rating)) {
                        relevance_doc[document_id].ref_to_value += term_freq * inverse_document_freq;
                    }
                }
            }
    });

    std::map<int, double> document_reduced = relevance_doc.BuildOrdinaryMap();
    std::vector<Document> matched_documents;
    matched_documents.reserve(document_reduced.size());

    for (const auto [document_id, relevance] : document_reduced) {
        matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}
