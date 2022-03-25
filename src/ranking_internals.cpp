// SPDX-License-Identifier: BSD-2-Clause
/* Copyright (C) 2022 Intel Corporation. */

#include "memkind/internal/ranking_internals.hpp"
#include "memkind/internal/memkind_private.h"
#include "memkind/internal/pagesizes.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <list>
#include <map>
#include <unordered_map>
#include <vector>

// type definitions -----------------------------------------------------------

// Hotness coeff ------------

MEMKIND_EXPORT HotnessCoeff::HotnessCoeff(double init_hotness,
                                          double exponential_coeff,
                                          double compensation_coeff)
    : value_(init_hotness),
      exponentialCoeff_(exponential_coeff),
      compensationCoeff_(compensation_coeff)
{}

MEMKIND_EXPORT void HotnessCoeff::Update(double hotness_to_add, double timediff)
{
    double temp = pow(this->exponentialCoeff_, timediff);
    assert(temp <= 1 && temp >= 0 && "exponential coeff is incorrect!");
    value_ *= temp;
    double add = this->compensationCoeff_ * hotness_to_add;
    assert(add >= 0);
    // check for double overflow
    if (std::numeric_limits<double>::max() - this->value_ >= add)
        this->value_ += add;
    else
        this->value_ = std::numeric_limits<double>::max();
}

MEMKIND_EXPORT double HotnessCoeff::Get()
{
    return this->value_;
}

// Slim Hotness coeff -------

MEMKIND_EXPORT SlimHotnessCoeff::SlimHotnessCoeff(double init_hotness)
    : value_(init_hotness)
{}

MEMKIND_EXPORT void SlimHotnessCoeff::Update(double hotness_to_add,
                                             double timediff,
                                             double exponential_coeff,
                                             double compensation_coeff)
{
    double temp = pow(exponential_coeff, timediff);
    assert(temp <= 1 && temp >= 0 && "exponential coeff is incorrect!");
    value_ *= temp;
    double add = compensation_coeff * hotness_to_add;
    assert(add >= 0);
    // check for double overflow
    if (std::numeric_limits<double>::max() - this->value_ >= add)
        this->value_ += add;
    else
        this->value_ = std::numeric_limits<double>::max();
}

MEMKIND_EXPORT double SlimHotnessCoeff::Get()
{
    return this->value_;
}

// Hotness ------------------
MEMKIND_EXPORT Hotness::Hotness(double init_hotness, uint64_t init_timestamp)
    : coeffs{
          SlimHotnessCoeff(init_hotness / EXPONENTIAL_COEFFS_NUMBER),
          SlimHotnessCoeff(init_hotness / EXPONENTIAL_COEFFS_NUMBER),
          SlimHotnessCoeff(init_hotness / EXPONENTIAL_COEFFS_NUMBER),
          SlimHotnessCoeff(init_hotness / EXPONENTIAL_COEFFS_NUMBER),
      },
      previousTimestamp(init_timestamp)
{
    assert(
        EXPONENTIAL_COEFFS_NUMBER == 4u &&
        "Incorrect number of coefficients for initializer; hand-written for 4 variables");
}

MEMKIND_EXPORT void Hotness::Update(double hotness_to_add, uint64_t timestamp)
{
    // TODO assure timestamp overflow does not occur
    double seconds_diff =
        (timestamp - this->previousTimestamp) * TIMESTAMP_TO_SECONDS_COEFF;
    assert(seconds_diff >= 0 && "timestamps are not monotonic!");

    // TODO check simd optimisations
    for (size_t i = 0; i < EXPONENTIAL_COEFFS_NUMBER; ++i) {
        coeffs[i].Update(hotness_to_add, seconds_diff,
                         EXPONENTIAL_COEFFS_VALS[i],
                         EXPONENTIAL_COEFFS_CONMPENSATION_COEFFS[i]);
    }
    this->previousTimestamp = timestamp;
}

MEMKIND_EXPORT double Hotness::GetTotalHotness()
{
    // TODO check SIMD optimisations
    double ret = 0;
    for (size_t i = 0; i < EXPONENTIAL_COEFFS_NUMBER; ++i) {
        // check for double overflow
        if (std::numeric_limits<double>::max() - ret >= this->coeffs[i].Get())
            ret += this->coeffs[i].Get();
        else
            return std::numeric_limits<double>::max();
    }

    return ret;
}

// PageMetadata -------------

MEMKIND_EXPORT PageMetadata::PageMetadata(uintptr_t start_addr,
                                          double init_hotness,
                                          uint64_t init_timestamp)
    : startAddr(start_addr), hotness(init_hotness, init_timestamp)
{}

MEMKIND_EXPORT bool PageMetadata::Touch()
{
    // touches > 0: returns false, touches == 0: returns true
    return !(this->touches++); // branchless optimisation
}

MEMKIND_EXPORT void PageMetadata::UpdateHotness(uint64_t timestamp)
{
    this->hotness.Update(this->touches * HOTNESS_TOUCH_SINGLE_VALUE, timestamp);
    this->touches = 0;
}

MEMKIND_EXPORT double PageMetadata::GetHotness()
{
    return this->hotness.GetTotalHotness();
}

MEMKIND_EXPORT uintptr_t PageMetadata::GetStartAddr()
{
    return this->startAddr;
}

// Ranking ------------------

MEMKIND_EXPORT void Ranking::AddPages(uintptr_t start_addr, size_t nof_pages,
                                      uint64_t timestamp)
{
    uint64_t end_addr = start_addr + nof_pages * TRACED_PAGESIZE;
    while (start_addr != end_addr) {
        // the 3 lines that follow could be handled by:
        // auto [page_it, inserted] = this->pageAddrToPage.emplace(...)
        // but we need to support GCC 4.85 :(
        auto page_it_inserted = this->pageAddrToPage.emplace(std::make_pair(
            start_addr,
            PageMetadata(start_addr, this->highestHotness, timestamp)));
        auto page_it = page_it_inserted.first;
        bool inserted = page_it_inserted.second;
        assert(inserted && "page not constructed!");
        PageMetadata *page = &page_it->second;
        this->hotnessToPages.insert(std::make_pair(page->GetHotness(), page));
        start_addr += TRACED_PAGESIZE;
    }
    totalSize += nof_pages * TRACED_PAGESIZE;
}

MEMKIND_EXPORT void Ranking::Touch(uintptr_t addr)
{
    // calculate start address of page
    addr = (addr / TRACED_PAGESIZE) * TRACED_PAGESIZE;
    auto page_ptr_it = this->pageAddrToPage.find(addr);
    if (page_ptr_it != this->pageAddrToPage.end()) {
        PageMetadata *temp = &page_ptr_it->second;
        if (temp->Touch()) {
            this->pagesToUpdate.push_back(temp);
        }
    } // else: access on not tracked address
}

MEMKIND_EXPORT void Ranking::Update(uint64_t timestamp)
{
    for (PageMetadata *page : this->pagesToUpdate) {
        // find the page entry in hotnessToPages
        const double hotness = page->GetHotness();
        auto pages_entry = this->hotnessToPages.find(hotness);
        // find correct entry in multimap
        assert(pages_entry != this->hotnessToPages.end() &&
               "page with hotness not correctly registered!");
        while (pages_entry != this->hotnessToPages.end() &&
               pages_entry->first == hotness && pages_entry->second != page)
            ++pages_entry;
        assert(pages_entry != this->hotnessToPages.end() &&
               "page with hotness not correctly registered!");
        // remove the entry from multimap, update hotness and add again
        this->hotnessToPages.erase(pages_entry);
        page->UpdateHotness(timestamp);
        this->hotnessToPages.insert(std::make_pair(page->GetHotness(), page));
    }
    this->pagesToUpdate.clear();
}

MEMKIND_EXPORT bool Ranking::GetHottest(double &hotness)
{
    auto hottest = this->hotnessToPages.rbegin();
    bool empty = hottest == this->hotnessToPages.rend();
    hotness = !empty ? hottest->first : 0.0;

    return !empty;
}

MEMKIND_EXPORT bool Ranking::GetColdest(double &hotness)
{
    auto coldest = this->hotnessToPages.begin();
    bool empty = coldest == this->hotnessToPages.end();
    hotness = !empty ? coldest->first : 0.0;

    return !empty;
}

MEMKIND_EXPORT PageMetadata Ranking::PopColdest()
{
    // pages are sorted in ascending order, lowest hotness first
    auto coldest = this->hotnessToPages.begin();
    assert(coldest != this->hotnessToPages.end() &&
           "Pop from an empty ranking requested!");
    PageMetadata *coldest_page = coldest->second;
    this->hotnessToPages.erase(coldest);
    PageMetadata ret = *coldest_page;
    this->pageAddrToPage.erase(coldest_page->GetStartAddr());
    totalSize -= TRACED_PAGESIZE;

    return ret;
}

MEMKIND_EXPORT PageMetadata Ranking::PopHottest()
{
    // pages are sorted in ascending order, lowest hotness first
    auto hottest = this->hotnessToPages.rbegin();
    assert(hottest != this->hotnessToPages.rend() &&
           "Pop from an empty ranking requested!");
    PageMetadata *hottest_page = hottest->second;
    PageMetadata ret = *hottest_page;
    this->pageAddrToPage.erase(hottest_page->GetStartAddr());
    ++hottest;
    if (hottest != this->hotnessToPages.rend())
        this->highestHotness = hottest->first;
    this->hotnessToPages.erase(hottest.base());
    totalSize -= TRACED_PAGESIZE;

    return ret;
}

MEMKIND_EXPORT void Ranking::AddPage(PageMetadata page)
{
    if (page.GetHotness() > this->highestHotness)
        this->highestHotness = page.GetHotness();
    // the 3 lines that follow could be handled by:
    // auto [page_it, inserted] = this->pageAddrToPage.emplace(...)
    // but we need to support GCC 4.85 :(
    auto page_it_inserted =
        this->pageAddrToPage.emplace(std::make_pair(page.GetStartAddr(), page));
    auto page_it = page_it_inserted.first;
    bool inserted = page_it_inserted.second;
    assert(inserted && "page not constructed!");
    PageMetadata *inserted_page = &page_it->second;
    this->hotnessToPages.insert(
        std::make_pair(inserted_page->GetHotness(), inserted_page));
    totalSize += TRACED_PAGESIZE;
}

MEMKIND_EXPORT size_t Ranking::GetTotalSize()
{
    return this->totalSize;
}
