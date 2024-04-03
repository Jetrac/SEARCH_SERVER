#include "test_example_functions.h"

void TestExamples (SearchServer& search_server) {

    auto word_freq = search_server.GetWordFrequencies(1);

    assertm(word_freq.at("funny"s) == 0.25, "Check the frequency"s);

}
