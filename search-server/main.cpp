#include "document.h"
#include "search_server.h"
#include "request_queue.h"
#include "remove_duplicates.h"
#include "paginator.h"
#include <iostream>
#include <string>

int main() {
    SearchServer search_server("and with"s);

  search_server.AddDocument(1, "funny pet and nasty rat", DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "funny pet with curly hair", DocumentStatus::ACTUAL, {1, 2});

    search_server.AddDocument(3, "funny pet with curly hair", DocumentStatus::ACTUAL, {1, 2});

  search_server.AddDocument(4, "funny pet and curly hair", DocumentStatus::ACTUAL, {1, 2});

   search_server.AddDocument(5, "funny funny pet and nasty nasty rat", DocumentStatus::ACTUAL, {1, 2});

   search_server.AddDocument(6, "funny pet and not very nasty rat", DocumentStatus::ACTUAL, {1, 2});

   search_server.AddDocument(7, "very nasty rat and not very funny pet", DocumentStatus::ACTUAL, {1, 2});

   search_server.AddDocument(8, "pet with rat and rat and rat", DocumentStatus::ACTUAL, {1, 2});

    search_server.AddDocument(9, "nasty rat with curly hair", DocumentStatus::ACTUAL, {1, 2});

  

    cout << "Before duplicates removed: "s << search_server.GetDocumentCount() << endl;
    RemoveDuplicates(search_server);
    cout << "After duplicates removed: "s << search_server.GetDocumentCount() << endl;
}