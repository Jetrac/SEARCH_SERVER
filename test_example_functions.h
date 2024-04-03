#pragma once

#include "search_server.h"

#include <iostream>
// uncomment to disable assert()
// #define NDEBUG
#include <cassert>

// Use (void) to silence unused warnings.
#define assertm(exp, msg) assert(((void)msg, exp))

using namespace std;

void TestExamples(SearchServer& search_server);
