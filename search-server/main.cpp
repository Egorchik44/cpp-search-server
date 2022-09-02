#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>


using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

struct Document {
    int id;
    double relevance;
};

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
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
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

class SearchServer {
public:




    void SetStopWords(const string& text) {// принимаеи на вход строку с разделёнными пробелом словами,
                                           //но не создаёт или возвращает множество,
                                           //а сразу добавляет эти слова в соответствующее поле класса.
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }





    void AddDocument(int document_id, const string& document) {//функция проводит подсчёт документов для дальнейшего вычисления IDF
                ++document_count_;
                const vector<string> words = SplitIntoWordsNoStop(document);
                const double count = 1.0 / words.size();
                for (const string& word : words) {
                    word_to_document_freqs_[word][document_id] += count;//словарь «документ → TF»
            
        }

    }




    vector<Document> FindTopDocuments(const string& raw_query) const {
        const Minus query_words = ParseMinus(raw_query);
        auto matched_documents = FindAllDocuments(query_words);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                return lhs.relevance > rhs.relevance;
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }





private:


    struct Minus {
        vector<string> plus_words;
        vector<string> minus_words;
    };


    struct MinusWord {
        string maybe_minus;
        bool minus_word;
    };


    
    map<string, map<int, double>> word_to_document_freqs_;
    int document_count_ = 0;
    set<string> stop_words_;



    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    double DoubleVariable(const string& word)const {//Рассчитывается IDF документа
        double a;
        double c = document_count_;
        a = log(c / word_to_document_freqs_.at(word).size());
        return a;
        
    }

    MinusWord ParseMinusWord(string text) const {
        bool minus_word = false;
        if (text[0] == '-') {
            minus_word = true;
            text = text.substr(1);
        }return {
            text,
            minus_word,
            
        };
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



    Minus ParseMinus(const string& text) const {
        Minus query;
        for (const string& word : SplitIntoWords(text)) {
            const MinusWord query_word = ParseMinusWord(word);
                if (query_word.minus_word) {
                    query.minus_words.push_back(query_word.maybe_minus);
                }
                else {
                    query.plus_words.push_back(query_word.maybe_minus);
                }
            
        }
        return query;
    }


    

    vector<Document> FindAllDocuments(const Minus& query) const {
        map<int, double> document_to_relevance;
        set<string> matched_words;
        vector<Document> matched_documents;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double document_freqs_ = DoubleVariable(word);
            for (const auto [document_id, relevance_freq] : word_to_document_freqs_.at(word)) {
                document_to_relevance[document_id]+= relevance_freq * document_freqs_;//Расчитывается сумма TF-IDF по словам
            }

        }
        for (const string& word : query.minus_words) {//Удаляется релевантность документов с минус словами
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, relevance_freq] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }

        }
        
        for (auto [document_id, relevance_freq] : document_to_relevance) {
            matched_documents.push_back({ document_id, relevance_freq });
        }

        return matched_documents;
    }




};

SearchServer CreateSearchServer() {// Считывает из cin стоп-слова и документ и возвращает настроенный экземпляр поисковой системы
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }

    return search_server;
}

int main() {
    
    const SearchServer search_server = CreateSearchServer();

    const string query = ReadLine();
    for (const auto& [document_id, relevance_freq] : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", "
            << "relevance = "s << relevance_freq << " }"s << endl;
    }
   
}