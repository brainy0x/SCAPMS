#pragma once
#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "model/PatientRecord.h"

namespace scapms::controller {

enum class SortAlgorithm { Selection, Insertion, Merge, Quick };
enum class SortField { Id, Name, Age, AppointmentDate };

struct SortStats {
    std::size_t comparisons = 0;
    std::size_t moves = 0;
    double milliseconds = 0.0;
};

// Hand-written sorting algorithms. They are templates so the same code sorts patients
// (by any field) and also the small helper lists used by the reports. No std::sort.
class SortEngine {
public:
    using PatientPtr = std::shared_ptr<model::PatientRecord>;
    using PatientLess = std::function<bool(const PatientPtr&, const PatientPtr&)>;

    template <typename T, typename Less>
    static SortStats sort(std::vector<T>& items, SortAlgorithm algorithm, Less less) {
        SortStats stats;
        auto start = std::chrono::steady_clock::now();
        switch (algorithm) {
        case SortAlgorithm::Selection: selectionSort(items, less, stats); break;
        case SortAlgorithm::Insertion: insertionSort(items, less, stats); break;
        case SortAlgorithm::Merge: {
            std::vector<T> buffer(items.size());
            mergeSortRange(items, buffer, 0, items.size(), less, stats);
            break;
        }
        case SortAlgorithm::Quick:
            if (!items.empty())
                quickSortRange(items, 0, static_cast<std::ptrdiff_t>(items.size()) - 1, less, stats);
            break;
        }
        stats.milliseconds =
            std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
        return stats;
    }

    template <typename T, typename Less>
    static bool isSorted(const std::vector<T>& items, Less less) {
        for (std::size_t i = 1; i < items.size(); ++i)
            if (less(items[i], items[i - 1])) return false;
        return true;
    }

    static PatientLess comparatorFor(SortField field);
    static std::string algorithmName(SortAlgorithm algorithm);
    static std::string fieldName(SortField field);
    // Plain-English reason why the chosen algorithm suits a list of `n` records.
    static std::string explain(SortAlgorithm algorithm, std::size_t n);

private:
    // Selection sort: repeatedly select the smallest remaining element. O(n^2)
    // comparisons but at most n-1 swaps.
    template <typename T, typename Less>
    static void selectionSort(std::vector<T>& v, Less& less, SortStats& s) {
        for (std::size_t i = 0; i + 1 < v.size(); ++i) {
            std::size_t smallest = i;
            for (std::size_t j = i + 1; j < v.size(); ++j) {
                ++s.comparisons;
                if (less(v[j], v[smallest])) smallest = j;
            }
            if (smallest != i) {
                std::swap(v[i], v[smallest]);
                ++s.moves;
            }
        }
    }

    // Insertion sort: grow a sorted prefix by inserting each element into place.
    // O(n^2) worst case, but O(n) when the data is already (nearly) sorted.
    template <typename T, typename Less>
    static void insertionSort(std::vector<T>& v, Less& less, SortStats& s) {
        for (std::size_t i = 1; i < v.size(); ++i) {
            T key = v[i];
            std::ptrdiff_t j = static_cast<std::ptrdiff_t>(i) - 1;
            while (j >= 0) {
                ++s.comparisons;
                if (!less(key, v[static_cast<std::size_t>(j)])) break;
                v[static_cast<std::size_t>(j) + 1] = v[static_cast<std::size_t>(j)];
                ++s.moves;
                --j;
            }
            v[static_cast<std::size_t>(j + 1)] = key;
        }
    }

    // Merge sort: divide and conquer, stable, O(n log n) in every case.
    template <typename T, typename Less>
    static void mergeSortRange(std::vector<T>& v, std::vector<T>& buf, std::size_t lo, std::size_t hi,
                               Less& less, SortStats& s) {
        if (hi - lo < 2) return;
        std::size_t mid = lo + (hi - lo) / 2;
        mergeSortRange(v, buf, lo, mid, less, s);
        mergeSortRange(v, buf, mid, hi, less, s);
        std::size_t i = lo, j = mid, k = lo;
        while (i < mid && j < hi) {
            ++s.comparisons;
            if (less(v[j], v[i])) buf[k++] = v[j++];   // take from the right only if strictly smaller => stable
            else buf[k++] = v[i++];
            ++s.moves;
        }
        while (i < mid) { buf[k++] = v[i++]; ++s.moves; }
        while (j < hi) { buf[k++] = v[j++]; ++s.moves; }
        for (std::size_t x = lo; x < hi; ++x) v[x] = buf[x];
    }

    // Quick sort: partition around a pivot (middle element, which avoids the worst case
    // on already-sorted input) and recurse. Average O(n log n), sorts in place.
    template <typename T, typename Less>
    static void quickSortRange(std::vector<T>& v, std::ptrdiff_t lo, std::ptrdiff_t hi, Less& less,
                               SortStats& s) {
        if (lo >= hi) return;
        std::ptrdiff_t mid = lo + (hi - lo) / 2;
        std::swap(v[static_cast<std::size_t>(mid)], v[static_cast<std::size_t>(hi)]);
        T pivot = v[static_cast<std::size_t>(hi)];
        std::ptrdiff_t store = lo;
        for (std::ptrdiff_t j = lo; j < hi; ++j) {
            ++s.comparisons;
            if (less(v[static_cast<std::size_t>(j)], pivot)) {
                if (store != j) {
                    std::swap(v[static_cast<std::size_t>(store)], v[static_cast<std::size_t>(j)]);
                    ++s.moves;
                }
                ++store;
            }
        }
        std::swap(v[static_cast<std::size_t>(store)], v[static_cast<std::size_t>(hi)]);
        ++s.moves;
        quickSortRange(v, lo, store - 1, less, s);
        quickSortRange(v, store + 1, hi, less, s);
    }
};

} // namespace scapms::controller
