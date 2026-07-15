#include "Tests.h"

#include <cstdint>
#include <iostream>
#include <regex>
#include <set>
#include <string>

#include "Contract.h"
#include "DummyDataGenerator.h"
#include "InMemoryRepository.h"

namespace tests {

using namespace contract;
using namespace generator;

namespace {

bool g_anyFailure = false;

void Report(const std::string& name, bool passed, const std::string& detail = "") {
    std::cout << (passed ? "[PASS] " : "[FAIL] ") << name;
    if (!passed && !detail.empty()) std::cout << " - " << detail;
    std::cout << "\n";
    if (!passed) g_anyFailure = true;
}

bool SampleEquals(const SampleRecord& a, const SampleRecord& b) {
    return a.sampleId == b.sampleId
        && a.name == b.name
        && a.avgProductionTime == b.avgProductionTime
        && a.yieldNumerator == b.yieldNumerator
        && a.stockQuantity == b.stockQuantity;
}

bool OrderEquals(const OrderRecord& a, const OrderRecord& b) {
    return a.orderId == b.orderId
        && a.sampleId == b.sampleId
        && a.customerName == b.customerName
        && a.orderQuantity == b.orderQuantity
        && a.status == b.status
        && a.createdAt == b.createdAt;
}

bool ResultEquals(const GenerationResult& a, const GenerationResult& b) {
    if (a.samples.size() != b.samples.size() || a.orders.size() != b.orders.size()) return false;
    for (size_t i = 0; i < a.samples.size(); ++i) {
        if (!SampleEquals(a.samples[i], b.samples[i])) return false;
    }
    for (size_t i = 0; i < a.orders.size(); ++i) {
        if (!OrderEquals(a.orders[i], b.orders[i])) return false;
    }
    return true;
}

bool IsValidIso8601Utc(const std::string& value) {
    static const std::regex pattern(R"(^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}Z$)");
    return std::regex_match(value, pattern);
}

// 같은 시드로 두 번 생성 -> 결과가 완전히 동일해야 한다(FR-01, FR-02, FR-13a).
void Test_SameSeedProducesIdenticalOutput() {
    InMemorySampleRepository sampleRepoA, sampleRepoB;
    InMemoryOrderRepository orderRepoA, orderRepoB;

    auto resultA = GenerateDummyData(12345, 30, 60, sampleRepoA, orderRepoA);
    auto resultB = GenerateDummyData(12345, 30, 60, sampleRepoB, orderRepoB);

    Report("same seed -> identical output (byte-for-byte field equality)",
        ResultEquals(resultA, resultB));
}

// 다른 시드로 생성 -> 결과가 달라야 한다.
void Test_DifferentSeedProducesDifferentOutput() {
    InMemorySampleRepository sampleRepoA, sampleRepoB;
    InMemoryOrderRepository orderRepoA, orderRepoB;

    auto resultA = GenerateDummyData(1, 30, 60, sampleRepoA, orderRepoA);
    auto resultB = GenerateDummyData(2, 30, 60, sampleRepoB, orderRepoB);

    Report("different seed -> different output", !ResultEquals(resultA, resultB));
}

// 생성된 전체 데이터셋이 CONTRACT.md의 모든 불변식을 만족하는지 속성 기반(N개 시드 x N개
// 레코드 전수 검사)으로 확인한다(FR-03~FR-09, FR-11, FR-13b).
void Test_AllInvariantsSatisfied() {
    static const std::set<std::string> kAllowedStatuses = {
        "RESERVED", "REJECTED", "PRODUCING", "CONFIRMED", "RELEASE"
    };

    bool allOk = true;
    std::string firstFailureDetail;
    auto fail = [&](const std::string& reason) {
        if (allOk) firstFailureDetail = reason;
        allOk = false;
    };

    for (uint64_t seed : {1ull, 2ull, 42ull, 999ull, 123456789ull}) {
        InMemorySampleRepository sampleRepo;
        InMemoryOrderRepository orderRepo;
        auto result = GenerateDummyData(seed, 100, 200, sampleRepo, orderRepo);

        std::set<std::string> sampleIds;
        for (const auto& sample : result.samples) {
            if (!sampleIds.insert(sample.sampleId).second) fail("duplicate sampleId (I1)");
            if (!(sample.avgProductionTime > 0.0)) fail("avgProductionTime <= 0 (I2)");
            if (!(sample.yieldNumerator >= 1 && sample.yieldNumerator <= YIELD_DENOMINATOR)) {
                fail("yieldNumerator out of [1, YIELD_DENOMINATOR] (I3)");
            }
            if (!(sample.stockQuantity >= 0)) fail("stockQuantity < 0 (I4)");
        }

        std::set<int64_t> orderIds;
        for (const auto& order : result.orders) {
            if (!orderIds.insert(order.orderId).second) fail("duplicate orderId (I5)");
            if (order.orderId < 1) fail("orderId < 1 (I5)");
            if (sampleIds.find(order.sampleId) == sampleIds.end()) {
                fail("order references a sampleId that was not generated (I6)");
            }
            if (!(order.orderQuantity > 0)) fail("orderQuantity <= 0 (I7)");
            if (kAllowedStatuses.find(order.status) == kAllowedStatuses.end()) {
                fail("status not in allowed set (I8)");
            }
            if (!IsValidIso8601Utc(order.createdAt)) fail("createdAt is not ISO 8601 UTC (I9)");
        }

        if (!allOk) break;
    }

    Report("generated dataset satisfies all CONTRACT invariants "
        "(property-based, 5 seeds x 100 samples x 200 orders)",
        allOk, firstFailureDetail);
}

// 기존 데이터가 있는 저장소에 추가 -> 기존 데이터가 손상되지 않아야 한다(FR-12, FR-13c).
void Test_ExistingDataNotCorrupted() {
    InMemorySampleRepository sampleRepo;
    InMemoryOrderRepository orderRepo;

    const SampleRecord preExistingSample{"SMP-EXISTING", "Pre-existing Sample", 12.5, 5000, 42};
    sampleRepo.Add(preExistingSample);
    const OrderRecord preExistingOrder{
        1, "SMP-EXISTING", "Pre-existing Customer", 7, "RESERVED", "2020-01-01T00:00:00Z"
    };
    orderRepo.Add(preExistingOrder);

    GenerateDummyData(777, 10, 10, sampleRepo, orderRepo);

    auto sampleAfter = sampleRepo.FindById("SMP-EXISTING");
    auto orderAfter = orderRepo.FindById(1);

    const bool sampleIntact = sampleAfter.has_value() && SampleEquals(*sampleAfter, preExistingSample);
    const bool orderIntact = orderAfter.has_value() && OrderEquals(*orderAfter, preExistingOrder);
    const bool countsGrew = sampleRepo.FindAll().size() == 11 && orderRepo.FindAll().size() == 11;

    Report("adding generated data to a populated repository does not corrupt pre-existing records",
        sampleIntact && orderIntact && countsGrew,
        !sampleIntact ? "pre-existing sample changed"
        : !orderIntact ? "pre-existing order changed"
        : !countsGrew ? "unexpected record counts after generation"
        : "");
}

} // namespace

bool RunAllTests() {
    g_anyFailure = false;

    Test_SameSeedProducesIdenticalOutput();
    Test_DifferentSeedProducesDifferentOutput();
    Test_AllInvariantsSatisfied();
    Test_ExistingDataNotCorrupted();

    return !g_anyFailure;
}

} // namespace tests
