#include <cstdint>
#include <exception>
#include <iostream>
#include <string>

#include "Contract.h"
#include "DummyDataGenerator.h"
#include "InMemoryRepository.h"
#include "Tests.h"

namespace {

void PrintUsage(const char* programName) {
    std::cout
        << "Usage:\n"
        << "  " << programName << " <seed> <sampleCount> <orderCount>\n"
        << "  " << programName << " test\n";
}

int RunGenerate(uint64_t seed, int sampleCount, int orderCount) {
    contract::InMemorySampleRepository sampleRepo;
    contract::InMemoryOrderRepository orderRepo;

    auto result = generator::GenerateDummyData(seed, sampleCount, orderCount, sampleRepo, orderRepo);

    std::cout << "seed=" << seed << " sampleCount=" << sampleCount
        << " orderCount=" << orderCount << "\n\n";

    std::cout << "Samples (" << result.samples.size() << "):\n";
    for (const auto& sample : result.samples) {
        std::cout << "  " << sample.sampleId << " | " << sample.name
            << " | avgProductionTime=" << sample.avgProductionTime
            << " | yieldNumerator=" << sample.yieldNumerator
            << " | stockQuantity=" << sample.stockQuantity << "\n";
    }

    std::cout << "\nOrders (" << result.orders.size() << "):\n";
    for (const auto& order : result.orders) {
        std::cout << "  #" << order.orderId << " | " << order.sampleId
            << " | " << order.customerName
            << " | orderQuantity=" << order.orderQuantity
            << " | status=" << order.status
            << " | createdAt=" << order.createdAt << "\n";
    }

    return 0;
}

} // namespace

int main(int argc, char** argv) {
    if (argc == 2 && std::string(argv[1]) == "test") {
        return tests::RunAllTests() ? 0 : 1;
    }

    if (argc != 4) {
        PrintUsage(argv[0]);
        return 1;
    }

    try {
        const uint64_t seed = std::stoull(argv[1]);
        const int sampleCount = std::stoi(argv[2]);
        const int orderCount = std::stoi(argv[3]);

        if (sampleCount < 0 || orderCount < 0) {
            std::cerr << "sampleCount and orderCount must be >= 0\n";
            return 1;
        }
        if (orderCount > 0 && sampleCount == 0) {
            std::cerr << "orderCount > 0 requires sampleCount > 0 "
                "(an order must reference an existing sample)\n";
            return 1;
        }

        return RunGenerate(seed, sampleCount, orderCount);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        PrintUsage(argv[0]);
        return 1;
    }
}
