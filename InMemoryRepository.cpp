#include "InMemoryRepository.h"

#include <algorithm>
#include <iterator>

namespace contract {

std::optional<SampleRecord> InMemorySampleRepository::FindById(const std::string& sampleId) const {
    auto it = std::find_if(records_.begin(), records_.end(),
        [&](const SampleRecord& r) { return r.sampleId == sampleId; });
    if (it == records_.end()) return std::nullopt;
    return *it;
}

std::vector<SampleRecord> InMemorySampleRepository::FindAll() const {
    return records_;
}

bool InMemorySampleRepository::Exists(const std::string& sampleId) const {
    return std::any_of(records_.begin(), records_.end(),
        [&](const SampleRecord& r) { return r.sampleId == sampleId; });
}

void InMemorySampleRepository::Add(const SampleRecord& sample) {
    records_.push_back(sample);
}

void InMemorySampleRepository::Update(const SampleRecord& sample) {
    auto it = std::find_if(records_.begin(), records_.end(),
        [&](const SampleRecord& r) { return r.sampleId == sample.sampleId; });
    if (it != records_.end()) *it = sample;
}

std::optional<OrderRecord> InMemoryOrderRepository::FindById(int64_t orderId) const {
    auto it = std::find_if(records_.begin(), records_.end(),
        [&](const OrderRecord& r) { return r.orderId == orderId; });
    if (it == records_.end()) return std::nullopt;
    return *it;
}

std::vector<OrderRecord> InMemoryOrderRepository::FindAll() const {
    return records_;
}

std::vector<OrderRecord> InMemoryOrderRepository::FindByStatus(const std::string& status) const {
    std::vector<OrderRecord> matched;
    std::copy_if(records_.begin(), records_.end(), std::back_inserter(matched),
        [&](const OrderRecord& r) { return r.status == status; });
    return matched;
}

int64_t InMemoryOrderRepository::NextOrderId() const {
    int64_t maxId = 0;
    for (const auto& r : records_) maxId = std::max(maxId, r.orderId);
    return maxId + 1;
}

void InMemoryOrderRepository::Add(const OrderRecord& order) {
    records_.push_back(order);
}

void InMemoryOrderRepository::Update(const OrderRecord& order) {
    auto it = std::find_if(records_.begin(), records_.end(),
        [&](const OrderRecord& r) { return r.orderId == order.orderId; });
    if (it != records_.end()) *it = order;
}

} // namespace contract
