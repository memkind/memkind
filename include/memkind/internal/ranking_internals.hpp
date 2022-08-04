// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <set>
#include <unordered_map>
#include <vector>

// defines --------------------------------------------------------------------

/// touch value: its significance is limited to preventing buffer overflow
#define HOTNESS_TOUCH_SINGLE_VALUE 1.0

static const double EXPONENTIAL_COEFFS_VALS[] = {0.9, 0.99, 0.999, 0.9999};

#define TIMESTAMP_TO_SECONDS_COEFF (1e-9)
#define EXPONENTIAL_COEFFS_NUMBER                                              \
    ((size_t)(sizeof(EXPONENTIAL_COEFFS_VALS) /                                \
              (sizeof(EXPONENTIAL_COEFFS_VALS[0]))))

/// Precalculated coeffs;
/// delta_hotness_value = touch_hotness*compensation coeff
/// Goal: to compensate for longer "retention" times
///
/// Formulae (in python):
///
/// def calculate_t10(c):
///    return -1/np.log10(c)
///
/// def calculate_compensation_coeffs(coeffs):
///     compensations=[]
///     for coeff in coeffs:
///         compensations.append(1/calculate_t10(coeff))
///     return compensations
///
/// compensation coeff:
///     1/T_10; T10 is the time after which value decreases by 90%
///
/// only relative compensation coeff values are relevant,
/// all coeffs can be multiplied by any arbitrary value, in this case:
///     21.854345326782836
static const double
    EXPONENTIAL_COEFFS_CONMPENSATION_COEFFS[EXPONENTIAL_COEFFS_NUMBER] = {
        1.00000000e+0, 9.53899645e-02, 9.49597036e-03, 9.49169617e-04};

// type declarations ----------------------------------------------------------

// class HotnessTest; // forward declaration of test class

class HotnessCoeff
{
    double value_;
    double exponentialCoeff_;
    double compensationCoeff_;

public:
    HotnessCoeff(double init_hotness, double exponential_coeff,
                 double compensation_coeff);
    void Update(double hotness_to_add, double timediff);
    double Get();
};

class SlimHotnessCoeff
{
    double value_;

public:
    SlimHotnessCoeff(double init_hotness);
    void Update(double hotness_to_add, double timediff,
                double exponential_coeff, double compensation_coeff);
    double Get();
};

class Hotness
{
    SlimHotnessCoeff coeffs[EXPONENTIAL_COEFFS_NUMBER];
    uint64_t previousTimestamp;

public:
    Hotness(double init_hotness, uint64_t init_timestamp);
    Hotness(Hotness *hotness_source, uint64_t init_timestamp);
    void Update(double hotness_to_add, uint64_t timestamp);
    double GetTotalHotness();
    uint64_t GetLastTouchTimestamp() const;
    friend class HotnessTest;
};

class PageMetadata
{
    // TODO rethink variables and their sizes
    uintptr_t startAddr;
    size_t touches = 0u;
    Hotness hotness;
    bool touched = false;

public:
    PageMetadata(uintptr_t start_addr, double init_hotness,
                 uint64_t init_timestamp);
    PageMetadata(uintptr_t start_addr, PageMetadata *hotness_source,
                 uint64_t init_timestamp);
    /// @return true if first touch since last update
    bool Touch();
    /// @brief Touch without increasing hotness
    /// @return true if first touch since last update
    bool TouchEmpty();
    void UpdateHotness(uint64_t timestamp);
    double GetHotness();
    uintptr_t GetStartAddr();
    uint64_t GetLastTouchTimestamp() const;
    void ResetTouched();
};

class Ranking
{
    std::map<double, std::set<PageMetadata *>> hotnessToPages;
    std::map<uint64_t, std::set<PageMetadata *>> leastRecentlyUsed;
    std::unordered_map<uintptr_t, PageMetadata> pageAddrToPage;
    std::set<PageMetadata *> pagesToUpdate;
    size_t totalSize = 0;

    void RemoveLRU_(PageMetadata *page);
    void RemoveHotnessToPages_(PageMetadata *page);
    void AddLRU_(PageMetadata *page);
    void AddLRU_(std::set<PageMetadata *> &&pages);
    void AddHotnessToPages_(PageMetadata *page);
    void AddHotnessToPages_(std::set<PageMetadata *> &&pages);

public:
    void AddPage(PageMetadata page);
    void AddPages(uintptr_t start_addr, size_t nof_pages,
                  uint64_t timestamp); // TODO add handling LRU
    size_t TryRemovePages(uintptr_t start_address, size_t nof_pages);
    bool Touch(uintptr_t addr);
    void Update(uint64_t timestamp, uint64_t oldest_timestamp = 0ul);
    /// @param[out] hotness highest hotness value in Ranking
    /// @return
    ///     bool: true if not empty (hotness valid)
    bool GetHottest(double &hotness);
    /// @param[out] hotness lowest hotness value in Ranking
    /// @return
    ///     bool: true if not empty (hotness valid)
    bool GetColdest(double &hotness);
    PageMetadata PopColdest();
    PageMetadata PopHottest();
    /// @return traced size, in bytes
    size_t GetTotalSize();
};
