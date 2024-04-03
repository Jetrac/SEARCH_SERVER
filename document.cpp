#include "document.h"
#include <iostream>

using namespace std;

ostream &operator<<(ostream &out, const Document &doc) {
    out << "{ document_id = " << doc.id << ", relevance = " << doc.relevance << ", rating = " << doc.rating << " }";
    return out;
}
