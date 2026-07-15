#pragma once

#include <cstdint>
#include <vector>

#include "Contract.h"

namespace generator {

struct GenerationResult {
    std::vector<contract::SampleRecord> samples;
    std::vector<contract::OrderRecord> orders;
};

// 시드 S로 sampleCount개의 Sample과 orderCount개의 Order를 결정론적으로 생성해
// sampleRepo/orderRepo에 Add()로 추가한다. 동일한 (seed, sampleCount, orderCount)와
// 빈 저장소로 호출하면 항상 동일한 GenerationResult를 반환한다(docs/PRD.md FR-01, FR-02).
GenerationResult GenerateDummyData(
    uint64_t seed,
    int sampleCount,
    int orderCount,
    contract::ISampleRepository& sampleRepo,
    contract::IOrderRepository& orderRepo);

} // namespace generator
