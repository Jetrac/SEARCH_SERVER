#include "remove_duplicates.h"

#include <string>
#include <map>
#include <set>

using namespace std;

void RemoveDuplicates(SearchServer& search_server) {
    set<int> ids_to_remove;
    set<set<string>> words_sets;

    for (auto document_id_ : search_server) {
        map<string, double> word_frequency = search_server.GetWordFrequencies(document_id_);

        set<string> words_set;
        for (auto [word, frequency] : word_frequency) {
            words_set.insert(word);
        }
        if (words_sets.count(words_set)) {
            ids_to_remove.insert(document_id_);
            continue;
        }
        words_sets.insert(words_set);
    }

    for(int id : ids_to_remove) {
        search_server.RemoveDocument(id);
        std::cout << "Found duplicate document id "s << id << std::endl;
    }
}
