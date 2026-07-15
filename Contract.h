#pragma once

// docs/CONTRACT.md §5 의 구조체/인터페이스 선언을 그대로 옮긴 것이다.
// 필드명/타입/시그니처를 이 저장소에서 임의로 바꾸지 않는다. 수정이 필요하다고 판단되면
// 코드로 우회하지 말고 사용자에게 보고한다.

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace contract {

struct SampleRecord {
    std::string sampleId; std::string name;
    double avgProductionTime; int yieldNumerator; int64_t stockQuantity;
};
struct OrderRecord {
    int64_t orderId; std::string sampleId; std::string customerName;
    int64_t orderQuantity; std::string status; std::string createdAt;
};
constexpr int YIELD_DENOMINATOR = 10000;

class ISampleRepository {
public:
    virtual ~ISampleRepository() = default;
    virtual std::optional<SampleRecord> FindById(const std::string& sampleId) const = 0;
    virtual std::vector<SampleRecord> FindAll() const = 0;
    virtual bool Exists(const std::string& sampleId) const = 0;
    virtual void Add(const SampleRecord& sample) = 0;
    virtual void Update(const SampleRecord& sample) = 0;
};

class IOrderRepository {
public:
    virtual ~IOrderRepository() = default;
    virtual std::optional<OrderRecord> FindById(int64_t orderId) const = 0;
    virtual std::vector<OrderRecord> FindAll() const = 0;
    virtual std::vector<OrderRecord> FindByStatus(const std::string& status) const = 0;
    virtual int64_t NextOrderId() const = 0;
    virtual void Add(const OrderRecord& order) = 0;
    virtual void Update(const OrderRecord& order) = 0;
};

} // namespace contract
