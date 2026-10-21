#include "controller/SortEngine.h"

#include "utils/StringUtils.h"

namespace scapms::controller {

namespace {
bool idLess(const SortEngine::PatientPtr& a, const SortEngine::PatientPtr& b) {
    return utils::toLower(a->id()) < utils::toLower(b->id());
}
std::string apptKey(const SortEngine::PatientPtr& p) {
    std::string k = p->nextAppointmentDateTime();
    return k.empty() ? std::string("9999-99-99 99:99") : k;   // patients without one go last
}
} // namespace

SortEngine::PatientLess SortEngine::comparatorFor(SortField field) {
    switch (field) {
    case SortField::Id:
        return [](const PatientPtr& a, const PatientPtr& b) { return idLess(a, b); };
    case SortField::Name:
        return [](const PatientPtr& a, const PatientPtr& b) {
            std::string an = utils::toLower(a->name()), bn = utils::toLower(b->name());
            if (an != bn) return an < bn;
            return idLess(a, b);
        };
    case SortField::Age:
        return [](const PatientPtr& a, const PatientPtr& b) {
            if (a->age() != b->age()) return a->age() < b->age();
            return idLess(a, b);
        };
    case SortField::AppointmentDate:
        return [](const PatientPtr& a, const PatientPtr& b) {
            std::string ak = apptKey(a), bk = apptKey(b);
            if (ak != bk) return ak < bk;
            return idLess(a, b);
        };
    }
    return [](const PatientPtr& a, const PatientPtr& b) { return idLess(a, b); };
}

std::string SortEngine::algorithmName(SortAlgorithm algorithm) {
    switch (algorithm) {
    case SortAlgorithm::Selection: return "Selection Sort";
    case SortAlgorithm::Insertion: return "Insertion Sort";
    case SortAlgorithm::Merge: return "Merge Sort";
    case SortAlgorithm::Quick: return "Quick Sort";
    }
    return "Sort";
}

std::string SortEngine::fieldName(SortField field) {
    switch (field) {
    case SortField::Id: return "Patient ID";
    case SortField::Name: return "Patient Name";
    case SortField::Age: return "Age";
    case SortField::AppointmentDate: return "Appointment Date";
    }
    return "";
}

std::string SortEngine::explain(SortAlgorithm algorithm, std::size_t n) {
    std::string size = "The clinic currently holds " + std::to_string(n) + " record(s). ";
    switch (algorithm) {
    case SortAlgorithm::Selection:
        return size +
               "Selection Sort makes at most n-1 swaps, so it moves each record only once. That suits "
               "small lists such as a single clinic's register, where its O(n^2) comparisons are "
               "negligible. For hundreds of records, choose Merge Sort instead.";
    case SortAlgorithm::Insertion:
        return size +
               "Insertion Sort runs in close to O(n) when the list is already nearly sorted, which is "
               "exactly the case after a few new patients were registered since the last sort. It is "
               "also stable and needs no extra memory.";
    case SortAlgorithm::Merge:
        return size +
               "Merge Sort guarantees O(n log n) time whatever the input order and is stable (patients "
               "with equal keys keep their order). It is the best choice for large or randomly "
               "ordered registers, at the cost of a temporary buffer.";
    case SortAlgorithm::Quick:
        return size +
               "Quick Sort averages O(n log n) and sorts in place with no extra buffer. The middle "
               "element is used as pivot so already-sorted data does not trigger the O(n^2) worst "
               "case. It is not stable.";
    }
    return size;
}

} // namespace scapms::controller
