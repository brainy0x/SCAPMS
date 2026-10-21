#pragma once
#include <memory>
#include <string>
#include <vector>

#include "model/PatientRecord.h"

namespace scapms::controller {

enum class SearchAlgorithm { Auto, Linear, Binary, Recursive };
enum class SearchField { Id, Name, Category };

// Hand-written search algorithms. All return indexes into the vector that was searched.
class SearchEngine {
public:
    using PatientPtr = std::shared_ptr<model::PatientRecord>;
    using PatientList = std::vector<PatientPtr>;

    // Linear search by ID (exact, case-insensitive). Works on unsorted data. -1 if absent.
    static int linearSearchById(const PatientList& list, const std::string& id);
    // Linear search, substring match on the name.
    static std::vector<int> linearSearchByName(const PatientList& list, const std::string& keyword);
    // Linear search on the category ("follow" finds Follow-Up).
    static std::vector<int> linearSearchByCategory(const PatientList& list, const std::string& keyword);

    // Binary search: the list MUST be sorted by `field` (Id or Name). Finds the first
    // record whose key is >= the search key, then collects all records that start with it.
    static std::vector<int> binarySearch(const PatientList& list, SearchField field, const std::string& key);

    // Recursive search (Name or Id, substring match): examines one record, then calls
    // itself on the rest of the list.
    static std::vector<int> recursiveSearch(const PatientList& list, SearchField field, const std::string& key);

private:
    static void recursiveSearchStep(const PatientList& list, SearchField field, const std::string& lowerKey,
                                    std::size_t index, std::vector<int>& hits);
    static std::string keyOf(const model::PatientRecord& patient, SearchField field);
};

} // namespace scapms::controller
