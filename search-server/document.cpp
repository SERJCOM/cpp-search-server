#include "document.h"

std::ostream& operator<<(std::ostream& stream, Document doc){
    stream << "{ document_id = ";
    stream << doc.id;
    stream << ", relevance = ";
    stream << doc.relevance;
    stream << ", rating = ";
    stream << doc.rating;
    stream << " }";
    return stream;
}