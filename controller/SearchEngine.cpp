#include "controller/SearchEngine.h"

#include "utils/Exceptions.h"
#include "utils/StringUtils.h"

namespace scapms::controller {

std::string SearchEngine::keyOf(const model::PatientRecord& patient, SearchField field) {
    switch (field) {
    case SearchField::Id: return patient.id();
    case SearchField::Name: return patient.name();
    case SearchField::Category: return patient.category();
    }
    return "";
}

int SearchEngine::linearSearchById(const PatientList& list, const std::string& id) {
    std::string key = utils::toLower(utils::trim(id));
    for (std::size_t i = 0; i < list.size(); ++i)
        if (utils::toLower(list[i]->id()) == key) return static_cast<int>(i);
    return -1;
}

std::vector<int> SearchEngine::linearSearchByName(const PatientList& list, const std::string& keyword) {
    std::vector<int> hits;
    for (std::size_t i = 0; i < list.size(); ++i)
        if (utils::containsIgnoreCase(list[i]->name(), keyword)) hits.push_back(static_cast<int>(i));
    return hits;
}

std::vector<int> SearchEngine::linearSearchByCategory(const PatientList& list, const std::string& keyword) {
    std::vector<int> hits;
    for (std::size_t i = 0; i < list.size(); ++i)
        if (utils::containsIgnoreCase(list[i]->category(), keyword)) hits.push_back(static_cast<int>(i));
    return hits;
}

std::vector<int> SearchEngine::binarySearch(const PatientList& list, SearchField field, const std::string& key) {
    if (field == SearchField::Category)
        throw utils::SearchException("Binary search is only available for Patient ID and Patient Name.");
    std::string lowerKey = utils::toLower(utils::trim(key));

    // Lower bound: the first position whose (lower-cased) key is not smaller than the search key.
    std::size_t low = 0, high = list.size();
    while (low < high) {
        std::size_t mid = low + (high - low) / 2;
        if (utils::toLower(keyOf(*list[mid], field)) < lowerKey) low = mid + 1;
        else high = mid;
    }

    std::vector<int> hits;
    for (std::size_t i = low; i < list.size(); ++i) {
        if (!utils::startsWithIgnoreCase(keyOf(*list[i], field), lowerKey)) break;
        hits.push_back(static_cast<int>(i));
    }
    return hits;
}

std::vector<int> SearchEngine::recursiveSearch(const PatientList& list, SearchField field, const std::string& key) {
    if (field == SearchField::Category)
        throw utils::SearchException("Recursive search is only available for Patient ID and Patient Name.");
    std::vector<int> hits;
    recursiveSearchStep(list, field, utils::toLower(utils::trim(key)), 0, hits);
    return hits;
}

void SearchEngine::recursiveSearchStep(const PatientList& list, SearchField field, const std::string& lowerKey,
                                       std::size_t index, std::vector<int>& hits) {
    if (index >= list.size()) return;                                  // base case: end of list
    if (utils::toLower(keyOf(*list[index], field)).find(lowerKey) != std::string::npos)
        hits.push_back(static_cast<int>(index));
    recursiveSearchStep(list, field, lowerKey, index + 1, hits);       // recursive case: the rest
}

} // namespace scapms::controller
