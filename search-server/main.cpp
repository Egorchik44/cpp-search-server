#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const float magic_const = 1e-6;
string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {
    Document() = default;

    Document(int id, double relevance, int rating)
        : id(id)
        , relevance(relevance)
        , rating(rating) {
    }

    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};

template <typename StringContainer>
set<string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    set<string> non_empty_strings;
    for (const string& str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(str);
        }
    }

    return non_empty_strings;
}

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

static bool IsValidWord(const string& word) {
        return none_of(word.begin(), word.end(), [](char c) {
           return c >= '\0' && c < ' ';
        });
    }

template <typename T>
static bool IsValidVect(const T& vec) { 
    if (!all_of(vec.cbegin(), vec.cend(), IsValidWord)) 
    {
        throw invalid_argument("invalid_argument");
    }
    return true;
    }

class SearchServer {
public:
    inline static constexpr int INVALID_DOCUMENT_ID = -1;

    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
           IsValidVect(stop_words_); 
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status,
    const vector<int>& ratings) {
        if (document_id < 0 || documents_.count(document_id)) {
            throw invalid_argument("invalid_argument");
        }
        if (!IsValidWord(document)){                   
            throw invalid_argument("invalid_argument");
        }
        const vector<string> words = SplitIntoWordsNoStop(document);
        for (const string& word : words) {
            if (!IsValidWord(word)){
                throw invalid_argument("invalid_argument");
            }
        }              
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
             word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        docs_id_list_.push_back(document_id);        
        documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    }

    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, DocumentPredicate document_predicate) const {
        vector<Document> results;
        for (auto word : SplitIntoWords(raw_query)){
            if (!IsStopWord(word)){           
                break;
            } else{
                continue;
            }  
        }
        const Query query = ParseQuery(raw_query);
        for (string word : query.minus_words){
            QueryWord query_word = ParseQueryWord(word);
            if (query_word.is_minus){
                throw invalid_argument("invalid_argument");
            }
            if (query_word.data.empty()){
                throw invalid_argument("invalid_argument");
            }
            if (query_word.data[0] == '-'){  
                throw invalid_argument("invalid_argument");
            }
            if (!IsValidWord(word)){ 
                throw invalid_argument("invalid_argument");
            }
        }
        if (!query.plus_words.empty()){
            for (string word : query.plus_words){
                if (!IsValidWord(word)){            
                    throw invalid_argument("invalid_argument");
                }
            }
        } 
        auto matched_documents = FindAllDocuments(query, document_predicate);
        sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
            if (abs(lhs.relevance - rhs.relevance) < magic_const) {
                return lhs.rating > rhs.rating;
            } else {
                return lhs.relevance > rhs.relevance;
            }
        });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }     
        results=matched_documents; 
        return matched_documents;
        }    
    
    
    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
        return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating)         {return document_status == status;});
    }
    
    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

        int GetDocumentId(int index) const {   
        for(int i=0; i<docs_id_list_.size();i++){
            if(i == index) {
                return docs_id_list_[i];
            }
        }
        if(index>docs_id_list_.size()){
            throw out_of_range("out_of_range");
        }
        return SearchServer::INVALID_DOCUMENT_ID;
    }
    
    explicit SearchServer(const string& stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text))  
    {
        if (!IsValidWord(stop_words_text)){     
            throw invalid_argument("invalid_argument");
        }   
    }
    
    int GetDocumentCount() const {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        if(!raw_query.empty()){
            if(raw_query[raw_query.size()-1]=='-'){
                throw invalid_argument("invalid_argument"); 
            }        
        }
        if (!(documents_.count(document_id) > 0)) {
            throw invalid_argument("invalid_argument");
        }
        for (auto word : SplitIntoWords(raw_query)){
            if (!IsValidWord(word)){
                throw invalid_argument("invalid_argument");
            }
        }
        Query query = ParseQuery(raw_query);
         if (!query.minus_words.empty()){
            for (string word : query.minus_words){
                QueryWord query_word = ParseQueryWord(word);
                if (query_word.is_minus ){
                   throw invalid_argument("invalid_argument");
                }
                if (query_word.data[0] == '-'){
                    throw invalid_argument("invalid_argument");
                }
                if (!IsValidWord(word)){
                    throw invalid_argument("invalid_argument");
                }
            }
        }
        
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
       return  tie(matched_words, documents_.at(document_id).status );
        }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    int document_id = 0;
    const set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;
    vector<int> docs_id_list_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = 0;
        for (const int rating : ratings) {
            rating_sum += rating;
        }
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return { text, is_minus, IsStopWord(text) };
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
            
                if (query_word.is_minus) {
                
                    query.minus_words.insert(query_word.data);
                }
                else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        
        return query;
    }

    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query& query,
        DocumentPredicate document_predicate) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                { document_id, relevance, documents_.at(document_id).rating });
        }
        return matched_documents;
    }
};

// ==================== для примера =========================
 
    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query& query,
        DocumentPredicate document_predicate) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                { document_id, relevance, documents_.at(document_id).rating });
        }
        return matched_documents;
    }
};

void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
            "Stop words must be excluded from documents"s);
    }
}
void TestAddDocument() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    // Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
    // находит нужный документ
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("cat"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
        server.AddDocument(33, "cat into door"s, DocumentStatus::ACTUAL, { 2, 3, 4 });
        const auto found_docs1 = server.FindTopDocuments("cat"s);
        ASSERT_EQUAL(found_docs1.size(), 2);
        const Document& doc01 = found_docs1[0];
        ASSERT_EQUAL(doc01.id, 33);
    }

    // Затем убеждаемся, что поиск этого же слова, входящего в список стоп-слов,
    // возвращает пустой результат
    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
    }
}

void TestMinusWords() {
    {
        SearchServer server;
        server.AddDocument(2, "красная синяя кошка за окном"s, DocumentStatus::ACTUAL, { 1,2,3 });
        server.AddDocument(34, "красная черная кошка за окном"s, DocumentStatus::ACTUAL, { 1,2,3 });
        const auto found_docs = server.FindTopDocuments("красная кошка черная"s);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, 34);
        
        const auto found_docs1 = server.FindTopDocuments("красная кошка -черная"s);
        ASSERT_EQUAL(found_docs1.size(), 1);
        const Document& doc1 = found_docs1[0];
        ASSERT_EQUAL(doc1.id, 2);
    }
}

void TestMatchDocument() {
    SearchServer server;
    server.AddDocument(1, "красный белый синий"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    
    auto [found_doc, status] = server.MatchDocument("белый синий"s, 1);
    ASSERT_EQUAL(found_doc.size(), 2);
    
    auto [found_doc1, status1]  = server.MatchDocument("красный -синий"s, 1);
    ASSERT_EQUAL(found_doc1.size(), 0);
    
    auto [found_doc2, status2]  = server.MatchDocument(""s, 1);
    ASSERT_EQUAL(found_doc2.size(), 0);
}





void TestSortDocument() {
    SearchServer server;
    {
        server.AddDocument(1, "красный оранжевый белый"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
        server.AddDocument(2, "красный синий кот"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
        server.AddDocument(3, "красный оранжевый олигарх"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
        server.AddDocument(4, "красный синий белый кот"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
        server.AddDocument(5, "красный синий синий кот"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
        const auto foundDocuments = server.FindTopDocuments("красный синий кот"s);
        
    const vector<int> expectedDocumentIds = { 5, 3, 4 };
 
    ASSERT_EQUAL(server.GetDocumentCount(), 5);
 
    ASSERT_EQUAL(foundDocuments.size(), expectedDocumentIds.size());
 
 
    bool is_sorted_by_relevance =
        is_sorted(foundDocuments.begin(), foundDocuments.end(),
            [](const Document& left, const Document& right) { return left.relevance > right.relevance; });
 
    ASSERT_HINT(is_sorted_by_relevance, "sorted by relevance"s);
    }
}




void TestRating() { 
    SearchServer server; 
    server.AddDocument(1, "red cat under roof"s, DocumentStatus::ACTUAL, { 1,2,3 }); 
    server.AddDocument(2, "black cat is running"s, DocumentStatus::ACTUAL, { 1,2,3,9 }); 
    const auto found_docs = server.FindTopDocuments("cat"s); 
    ASSERT_EQUAL(found_docs[0].rating, (1 + 2 + 3 + 9 ) / 4); 
    ASSERT_EQUAL(found_docs[1].rating, (1 + 2 + 3) / 3); 
} 

 
void TestPredicate() {
    string text = "red cat under roof";
    vector<int> rate = { 1,2,3 };
   
    SearchServer server;
    
    server.AddDocument(0, "white cat and fashionable collar"s,        DocumentStatus::ACTUAL, {8, -3});
    server.AddDocument(1, "fluffy cat fluffy tail"s,       DocumentStatus::ACTUAL, {7, 2, 7});
    server.AddDocument(2, "well-groomed dog expressive eyes"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    server.AddDocument(3, "well-groomed starling Eugene"s,         DocumentStatus::REMOVED, {9});
    
    
    const auto found_docs = server.FindTopDocuments(text, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::ACTUAL; });
    ASSERT_EQUAL(found_docs.size(), 2); 
    const Document& first_doc = found_docs[0]; //1
    ASSERT_EQUAL(first_doc.id , 1); 
    const Document& second_doc = found_docs[1]; //1
    ASSERT_EQUAL(second_doc.id , 0);     
        
        
    const auto found_docs1 = server.FindTopDocuments(text, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; });
    ASSERT_EQUAL(found_docs1.size(), 1);
    const Document& doc1 = found_docs1[0]; 
    ASSERT_EQUAL(doc1.id , 0); 
    
    
    
}


void TestStatus() {
    SearchServer server;
    server.AddDocument(22, "red cat under roof"s, DocumentStatus::ACTUAL, { 1,2,3 });
    server.AddDocument(23, "black cat is running"s, DocumentStatus::IRRELEVANT, { 1,2,3 });
    server.AddDocument(24, "black cat in runnings"s, DocumentStatus::BANNED, { 1,2,3 });
    server.AddDocument(25, "orange cat after runnings"s, DocumentStatus::REMOVED, { 1,2,3 });
    const auto found_docs = server.FindTopDocuments("cat"s, DocumentStatus::ACTUAL);
    ASSERT_EQUAL(found_docs.size() , 1);
    const Document& doc0 = found_docs[0];
    ASSERT_EQUAL(doc0.id , 22);
    const auto found_docs1 = server.FindTopDocuments("cat"s, DocumentStatus::IRRELEVANT);
    ASSERT_EQUAL(found_docs1.size() , 1);
    const Document& doc01 = found_docs1[0];
    ASSERT_EQUAL(doc01.id , 23);
    const auto found_docs2 = server.FindTopDocuments("cat"s, DocumentStatus::BANNED);
    ASSERT_EQUAL(found_docs2.size() , 1);
    const Document& doc02 = found_docs2[0];
    ASSERT_EQUAL(doc02.id , 24);
    const auto found_docs3 = server.FindTopDocuments("cat"s, DocumentStatus::REMOVED);
    ASSERT_EQUAL(found_docs3.size() , 1);
    const Document& doc03 = found_docs3[0];
    ASSERT_EQUAL(doc03.id , 25);
}

void TestCorrectRelevance() {
    SearchServer server;
    server.AddDocument(22, "house cristall and gold"s, DocumentStatus::ACTUAL, { 1,2,3 });
    server.AddDocument(23, "car green sky house"s, DocumentStatus::ACTUAL, { 1,2,3 });
    server.AddDocument(24, "yellow black white house"s, DocumentStatus::ACTUAL, {1,2,3});
    const auto found_docs = server.FindTopDocuments("cristall house gold"s);
    double ans1, ans2, ans3;
    ans1 = 2.0 * log(3.0) * (0.25) + log(1.0) * (0.25);
    ans2 = log(1.0) * (0.25);
    ans3 = log(3.0) * (0.25) + log(1.0) * (0.25);
    ASSERT_EQUAL(found_docs[0].relevance , ans1);
    ASSERT_EQUAL(found_docs[1].relevance , ans2);
    const auto found_docs1 = server.FindTopDocuments("house green"s);
    ASSERT_EQUAL(found_docs1[0].relevance, ans3);
}
/*
Разместите код остальных тестов здесь
*/

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestAddDocument);
    RUN_TEST(TestMinusWords);
    RUN_TEST(TestMatchDocument);
    RUN_TEST(TestRating);
    RUN_TEST(TestPredicate);
    RUN_TEST(TestStatus);
    RUN_TEST(TestCorrectRelevance);
}
int main() {
    TestSearchServer()
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});
    cout << "ACTUAL by default:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s)) {
        PrintDocument(document);
    }
    cout << "BANNED:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED)) {
        PrintDocument(document);
    }
    cout << "Even ids:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
        PrintDocument(document);
    }
    return 0;
}