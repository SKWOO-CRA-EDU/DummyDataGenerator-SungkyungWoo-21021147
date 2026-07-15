#include "DummyDataGenerator.h"

#include <array>
#include <chrono>
#include <cstdio>
#include <random>
#include <stdexcept>
#include <string>

namespace generator {

using namespace contract;

namespace {

// baseEpoch + [0, tenYearsInSeconds] 범위의 정수 초를 ISO 8601 UTC 문자열로 변환한다.
// 실행 시점의 실제 시각(system_clock::now())은 절대 쓰지 않는다 — 그러면 같은 시드로도
// 실행할 때마다 createdAt이 달라져 재현성이 깨진다. 대신 결정론적으로 계산된 정수 오프셋을
// <chrono> 달력 변환으로만 문자열화한다(로케일/타임존에 의존하는 gmtime류는 쓰지 않는다).
std::string FormatIso8601Utc(int64_t epochSeconds) {
    using namespace std::chrono;
    const sys_seconds timePoint{seconds{epochSeconds}};
    const auto dayPoint = floor<days>(timePoint);
    const year_month_day ymd{dayPoint};
    const hh_mm_ss<seconds> time{floor<seconds>(timePoint - dayPoint)};

    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%04d-%02u-%02uT%02lld:%02lld:%02lldZ",
        static_cast<int>(ymd.year()),
        static_cast<unsigned>(ymd.month()),
        static_cast<unsigned>(ymd.day()),
        static_cast<long long>(time.hours().count()),
        static_cast<long long>(time.minutes().count()),
        static_cast<long long>(time.seconds().count()));
    return std::string(buffer);
}

// FR-03: SampleId 고유성. 저장소에 이미 존재하는 값과 겹치면 같은 RNG 스트림에서 다음
// 후보를 계속 뽑는다(사후 삭제/치환이 아니라 재시도로 불변식을 보장).
std::string GenerateUniqueSampleId(std::mt19937_64& rng, const ISampleRepository& sampleRepo) {
    std::uniform_int_distribution<int> candidateDist(0, 999999);
    constexpr int kMaxAttempts = 1'000'000;
    for (int attempt = 0; attempt < kMaxAttempts; ++attempt) {
        char buffer[16];
        std::snprintf(buffer, sizeof(buffer), "SMP-%06d", candidateDist(rng));
        std::string candidate(buffer);
        if (!sampleRepo.Exists(candidate)) return candidate;
    }
    throw std::runtime_error("GenerateUniqueSampleId: exhausted candidate space");
}

} // namespace

GenerationResult GenerateDummyData(
    uint64_t seed,
    int sampleCount,
    int orderCount,
    ISampleRepository& sampleRepo,
    IOrderRepository& orderRepo) {

    std::mt19937_64 rng(seed);
    GenerationResult result;

    // 필드를 뽑는 순서 자체가 재현성 계약의 일부다: SampleId -> Name -> AvgProductionTime
    // -> yieldNumerator -> StockQuantity 순으로 항상 고정한다.
    std::uniform_real_distribution<double> avgProductionTimeDist(0.01, 1000.0);
    std::uniform_int_distribution<int> yieldNumeratorDist(1, YIELD_DENOMINATOR);
    std::uniform_int_distribution<int64_t> stockQuantityDist(0, 100'000);
    std::uniform_int_distribution<int> nameSuffixDist(0, 999'999);

    result.samples.reserve(static_cast<size_t>(sampleCount > 0 ? sampleCount : 0));
    for (int i = 0; i < sampleCount; ++i) {
        SampleRecord sample;
        sample.sampleId = GenerateUniqueSampleId(rng, sampleRepo);
        sample.name = "Sample-" + std::to_string(nameSuffixDist(rng));
        sample.avgProductionTime = avgProductionTimeDist(rng);
        sample.yieldNumerator = yieldNumeratorDist(rng);
        sample.stockQuantity = stockQuantityDist(rng);

        sampleRepo.Add(sample);
        result.samples.push_back(sample);
    }

    if (orderCount > 0 && !result.samples.empty()) {
        static const std::array<std::string, 5> kAllowedStatuses = {
            "RESERVED", "REJECTED", "PRODUCING", "CONFIRMED", "RELEASE"
        };
        constexpr int64_t kBaseEpochSeconds = 1'577'836'800; // 2020-01-01T00:00:00Z
        constexpr int64_t kTenYearsInSeconds = 315'360'000;

        std::uniform_int_distribution<size_t> sampleIndexDist(0, result.samples.size() - 1);
        std::uniform_int_distribution<int64_t> orderQuantityDist(1, 10'000);
        std::uniform_int_distribution<size_t> statusDist(0, kAllowedStatuses.size() - 1);
        std::uniform_int_distribution<int64_t> createdAtOffsetDist(0, kTenYearsInSeconds);

        result.orders.reserve(static_cast<size_t>(orderCount));
        for (int i = 0; i < orderCount; ++i) {
            OrderRecord order;
            order.orderId = orderRepo.NextOrderId();

            // FR-08: 참조 무결성은 로컬 캐시가 아니라 저장소 조회(Exists)로 확인한다.
            const std::string& referencedSampleId = result.samples[sampleIndexDist(rng)].sampleId;
            if (!sampleRepo.Exists(referencedSampleId)) {
                throw std::logic_error("GenerateDummyData: referential integrity violated");
            }

            order.sampleId = referencedSampleId;
            order.customerName = "Customer-" + std::to_string(nameSuffixDist(rng));
            order.orderQuantity = orderQuantityDist(rng);
            order.status = kAllowedStatuses[statusDist(rng)];
            order.createdAt = FormatIso8601Utc(kBaseEpochSeconds + createdAtOffsetDist(rng));

            orderRepo.Add(order);
            result.orders.push_back(order);
        }
    }

    return result;
}

} // namespace generator
