#include "string_processing.h"

#include <string_view>

using namespace std;

vector<string_view> SplitIntoWords(string_view text) {
    vector<string_view> words;

    while (true) {
        size_t space = text.find(' ');
        words.emplace_back(text.substr(0, space));

        if (space == text.npos) {
            break;
        } else {
            text.remove_prefix(space + 1);
        }
    }

    return words;
}
