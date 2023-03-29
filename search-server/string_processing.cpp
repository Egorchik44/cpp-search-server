#include "string_processing.h"
#include <vector>
#include <string>
#include <sstream>

std::vector<std::string> SplitIntoWords(const std::string& text) {
    std::vector<std::string> words;
    std::istringstream iss(text);
    std::string word;
    while (iss >> word) {
        words.push_back(word);
    }
    return words;
}

std::vector<std::string_view> SplitIntoWordsView(std::string_view str) {
    std::vector<std::string_view> result;

    auto begin = str.find_first_not_of(' ');
    while (begin != std::string_view::npos) {
        const auto end = str.find(' ', begin);
        result.emplace_back(str.substr(begin, end - begin));
        begin = str.find_first_not_of(' ', end);
    }

    return result;
}