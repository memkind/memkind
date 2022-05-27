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

// with 4 kB TRACED_PAGESIZE, 4 MB can be touched per cycle
#define TO_TOUCH_MAX (1024ul)

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

MEMKIND_EXPORT Hotness::Hotness(Hotness *hotness_source, uint64_t init_timestamp)
    : coeffs{
          SlimHotnessCoeff(0lu),
          SlimHotnessCoeff(0lu),
          SlimHotnessCoeff(0lu),
          SlimHotnessCoeff(0lu),
      },
      previousTimestamp(init_timestamp)
{
    assert(
        EXPONENTIAL_COEFFS_NUMBER == 4u &&
        "Incorrect number of coefficients for initializer; hand-written for 4 variables");
    if (hotness_source)
        for (size_t i = 0lu; i < EXPONENTIAL_COEFFS_NUMBER; ++i)
            this->coeffs[i] = hotness_source->coeffs[i];
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

MEMKIND_EXPORT uint64_t Hotness::GetLastTouchTimestamp() const
{
    return this->previousTimestamp;
}

// PageMetadata -------------

MEMKIND_EXPORT PageMetadata::PageMetadata(uintptr_t start_addr,
                                          double init_hotness,
                                          uint64_t init_timestamp)
    : startAddr(start_addr), hotness(init_hotness, init_timestamp)
{}

MEMKIND_EXPORT PageMetadata::PageMetadata(uintptr_t start_addr,
                                          PageMetadata *hotness_source,
                                          uint64_t init_timestamp)
    : startAddr(start_addr),
      hotness(hotness_source ? &hotness_source->hotness : nullptr,
              init_timestamp)
{}

MEMKIND_EXPORT bool PageMetadata::Touch()
{
    this->touches++;
    return this->TouchEmpty();
}

MEMKIND_EXPORT bool PageMetadata::TouchEmpty()
{
    // negation: touched -> not first touch, not touched -> first touch
    bool first_touch = !this->touched;
    this->touched = true;
    return first_touch;
}

MEMKIND_EXPORT void PageMetadata::UpdateHotness(uint64_t timestamp)
{
    this->hotness.Update(this->touches * HOTNESS_TOUCH_SINGLE_VALUE, timestamp);
    this->touches = 0;
    this->touched = false;
}

MEMKIND_EXPORT double PageMetadata::GetHotness()
{
    return this->hotness.GetTotalHotness();
}

MEMKIND_EXPORT uintptr_t PageMetadata::GetStartAddr()
{
    return this->startAddr;
}

MEMKIND_EXPORT uint64_t PageMetadata::GetLastTouchTimestamp() const
{
    return this->hotness.GetLastTouchTimestamp();
}

// Ranking - private --------

MEMKIND_EXPORT void Ranking::AddLRU_(PageMetadata *page)
{
    std::set<PageMetadata *> same_timestamp_pages;
    same_timestamp_pages.insert(page);
    this->AddLRU_(std::move(same_timestamp_pages));
}

MEMKIND_EXPORT void Ranking::AddLRU_(std::set<PageMetadata *> &&pages)
{
    if (pages.begin() != pages.end()) { // not an empty set
        auto timestamp = (*pages.begin())->GetLastTouchTimestamp();
        auto current_timestamp_it = this->leastRecentlyUsed.find(timestamp);
        if (current_timestamp_it == this->leastRecentlyUsed.end()) {
            this->leastRecentlyUsed.insert(std::make_pair(timestamp, pages));
        } else {
            for (auto page : pages) {
                current_timestamp_it->second.insert(page);
            }
        }
    }
}

MEMKIND_EXPORT void Ranking::AddHotnessToPages_(PageMetadata *page)
{
    std::set<PageMetadata *> same_hotness_pages;
    same_hotness_pages.insert(page);
    this->AddHotnessToPages_(std::move(same_hotness_pages));
}

MEMKIND_EXPORT void
Ranking::AddHotnessToPages_(std::set<PageMetadata *> &&pages)
{
    if (pages.begin() != pages.end()) { // not an empty set
        auto hotness = (*pages.begin())->GetHotness();
        auto current_hotness_it = this->hotnessToPages.find(hotness);
        if (current_hotness_it == this->hotnessToPages.end()) {
            this->hotnessToPages.insert(std::make_pair(hotness, pages));
        } else {
            for (auto page : pages) {
                current_hotness_it->second.insert(page);
            }
        }
    }
}

MEMKIND_EXPORT void Ranking::RemoveLRU_(PageMetadata *page)
{
    // find set
    auto last_timestamp_set_it =
        this->leastRecentlyUsed.find(page->GetLastTouchTimestamp());
    assert(last_timestamp_set_it != this->leastRecentlyUsed.end() &&
           "Request to remove non-existent page");
    // remove from set
    auto page_it = last_timestamp_set_it->second.find(page);
    assert(page_it != last_timestamp_set_it->second.end() &&
           "Request to remove non-existent page");
    last_timestamp_set_it->second.erase(page_it);
    // erase the whole set, if empty
    if (last_timestamp_set_it->second.empty())
        this->leastRecentlyUsed.erase(last_timestamp_set_it);
}

MEMKIND_EXPORT void Ranking::RemoveHotnessToPages_(PageMetadata *page)
{
    // find the set of page entries in hotnessToPages
    const double hotness = page->GetHotness();
    auto hotness_set_it = this->hotnessToPages.find(hotness);
    assert(hotness_set_it != this->hotnessToPages.end() &&
           "page with hotness not correctly registered!");
    // remove from set
    auto page_it = hotness_set_it->second.find(page);
    assert(page_it != hotness_set_it->second.end() &&
           "Request to remove non-existent page");
    hotness_set_it->second.erase(page);
    // erase the whole set, if empty
    if (hotness_set_it->second.empty()) {
        this->hotnessToPages.erase(hotness_set_it);
    }
}

// Ranking - public ---------

size_t Ranking::TryRemovePages(uintptr_t start_address, size_t nof_pages)
{
    size_t removed = 0ul;
    for (uintptr_t pstart_address = start_address;
         pstart_address < start_address + nof_pages * TRACED_PAGESIZE;
         pstart_address += TRACED_PAGESIZE) {
        auto addr_page_it = this->pageAddrToPage.find(pstart_address);
        if (addr_page_it == this->pageAddrToPage.end())
            continue;
        // we have an entry to remove, remove from all internal structures
        PageMetadata *metadata_ptr = &addr_page_it->second;
        this->RemoveHotnessToPages_(metadata_ptr);
        this->RemoveLRU_(metadata_ptr);
        this->pagesToUpdate.erase(metadata_ptr);
        this->pageAddrToPage.erase(addr_page_it);
        ++removed;
    }

    return removed;
}

MEMKIND_EXPORT void Ranking::AddPages(uintptr_t start_addr, size_t nof_pages,
                                      uint64_t timestamp)
{
    uint64_t end_addr = start_addr + nof_pages * TRACED_PAGESIZE;
    std::set<PageMetadata *> pages;
    PageMetadata *hotness_source = nullptr;
    auto hottest_it = this->hotnessToPages.rbegin();
    if (hottest_it != this->hotnessToPages.rend()) {
        auto page_meta_it = hottest_it->second.begin();
        assert(page_meta_it != hottest_it->second.end() && "corrupted data");
        hotness_source = *page_meta_it;
    }
    while (start_addr != end_addr) {
        // the 3 lines that follow could be handled by:
        // auto [page_it, inserted] = this->pageAddrToPage.emplace(...)
        // but we need to support GCC 4.85 :(
        auto page_it_inserted = this->pageAddrToPage.emplace(std::make_pair(
            start_addr, PageMetadata(start_addr, hotness_source, timestamp)));
        auto page_it = page_it_inserted.first;
        bool inserted = page_it_inserted.second;
        assert(inserted && "Page address duplicated!");
        PageMetadata *page = &page_it->second;
        pages.insert(page);
        start_addr += TRACED_PAGESIZE;
    }
    auto pages_clone = pages;
    this->AddHotnessToPages_(std::move(pages));
    this->AddLRU_(std::move(pages_clone));
    totalSize += nof_pages * TRACED_PAGESIZE;
}

MEMKIND_EXPORT bool Ranking::Touch(uintptr_t addr)
{
    bool touched = false;
    // calculate start address of page
    addr = (addr & ~((uintptr_t)TRACED_PAGESIZE - 1ul));
    auto page_ptr_it = this->pageAddrToPage.find(addr);
    if (page_ptr_it != this->pageAddrToPage.end()) {
        PageMetadata *temp = &page_ptr_it->second;
        if (temp->Touch()) {
            this->pagesToUpdate.insert(temp);
        }
        touched = true;
    } // else: access on not tracked address

    return touched;
}

MEMKIND_EXPORT void Ranking::Update(uint64_t timestamp,
                                    uint64_t oldest_timestamp)
{
    size_t to_touch_i = 0ul;
    auto to_touch_it =
        this->leastRecentlyUsed
            .begin(); // TODO test begin vs rbegin (already checked)
    while (to_touch_i++ < TO_TOUCH_MAX &&
           to_touch_it != this->leastRecentlyUsed.end() &&
           (to_touch_it->first) < oldest_timestamp) {
        for (auto &element : to_touch_it->second) {
            if (element->TouchEmpty())
                this->pagesToUpdate.insert(element);
        }
        ++to_touch_it;
    }

    std::set<PageMetadata *> same_timestamp_pages;

    for (PageMetadata *page : this->pagesToUpdate) {

        this->RemoveHotnessToPages_(page);
        this->RemoveLRU_(page);

        // update the hotness and add the page entry again to the
        // hotnessToPages map
        page->UpdateHotness(timestamp);

        this->AddHotnessToPages_(page);
        // LRU is handled later, in batch (for efficiency)
        same_timestamp_pages.insert(page);
    }
    this->AddLRU_(std::move(same_timestamp_pages));

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
    auto coldest_set_it = this->hotnessToPages.begin();
    assert(coldest_set_it != this->hotnessToPages.end() && "Pop on empty set!");
    auto coldest_it = coldest_set_it->second.begin();
    assert(coldest_it != coldest_set_it->second.end() &&
           "Empty set in hotnessToPages!");
    PageMetadata *coldest_page = *coldest_it;
    PageMetadata ret = *coldest_page;

    this->RemoveLRU_(coldest_page);
    this->pagesToUpdate.erase(coldest_page);

    // remove the page entry from the hotnessToPage map
    coldest_set_it->second.erase(coldest_it);
    if (coldest_set_it->second.empty()) {
        this->hotnessToPages.erase(coldest_set_it);
    }
    this->pageAddrToPage.erase(ret.GetStartAddr());
    totalSize -= TRACED_PAGESIZE;

    return ret;
}

MEMKIND_EXPORT PageMetadata Ranking::PopHottest()
{
    // pages are sorted in ascending order, lowest hotness first
    auto hottest_set_it = this->hotnessToPages.rbegin();
    assert(hottest_set_it != this->hotnessToPages.rend() &&
           "Pop on empty set!");
    auto hottest_it = hottest_set_it->second.rbegin();
    assert(hottest_it != hottest_set_it->second.rend() &&
           "Empty set in hotnessToPages!");
    PageMetadata *hottest_page = *hottest_it;
    PageMetadata ret = *hottest_page;

    this->RemoveLRU_(hottest_page);
    this->pagesToUpdate.erase(hottest_page);

    // remove the page entry from the hotnessToPage map
    hottest_set_it->second.erase(--(hottest_it.base()));
    if (hottest_set_it->second.empty()) {
        this->hotnessToPages.erase(--(hottest_set_it.base()));
    }
    this->pageAddrToPage.erase(ret.GetStartAddr());

    totalSize -= TRACED_PAGESIZE;

    return ret;
}

MEMKIND_EXPORT void Ranking::AddPage(PageMetadata page)
{
    // the 3 lines that follow could be handled by:
    // auto [page_it, inserted] = this->pageAddrToPage.emplace(...)
    // but we need to support GCC 4.85 :(
    auto page_it_inserted =
        this->pageAddrToPage.emplace(std::make_pair(page.GetStartAddr(), page));
    auto page_it = page_it_inserted.first;
    assert(page_it != this->pageAddrToPage.end() && "page not constructed!");
    bool inserted = page_it_inserted.second;
    assert(inserted && "page already exists!");

    // add the page entry to the hotnessToPages map
    PageMetadata *inserted_page = &page_it->second;

    this->AddHotnessToPages_(inserted_page);
    this->AddLRU_(inserted_page);

    totalSize += TRACED_PAGESIZE;
}

MEMKIND_EXPORT size_t Ranking::GetTotalSize()
{
    return this->totalSize;
}
