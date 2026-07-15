#pragma once

// 계약상 저장소 인터페이스(ISampleRepository/IOrderRepository)의 인메모리 구현체.
// 실제 영속 파일 I/O는 이 저장소의 비범위(docs/PRD.md OS-02)이며, 이 구현은
// DummyDataGenerator가 "저장소를 통해서만" 데이터를 추가한다는 것을 실행/테스트하기 위한
// 데모/테스트용 어댑터다.

#include <vector>

#include "Contract.h"

namespace contract {

class InMemorySampleRepository : public ISampleRepository {
public:
    std::optional<SampleRecord> FindById(const std::string& sampleId) const override;
    std::vector<SampleRecord> FindAll() const override;
    bool Exists(const std::string& sampleId) const override;
    void Add(const SampleRecord& sample) override;
    void Update(const SampleRecord& sample) override;

private:
    std::vector<SampleRecord> records_;
};

class InMemoryOrderRepository : public IOrderRepository {
public:
    std::optional<OrderRecord> FindById(int64_t orderId) const override;
    std::vector<OrderRecord> FindAll() const override;
    std::vector<OrderRecord> FindByStatus(const std::string& status) const override;
    int64_t NextOrderId() const override;
    void Add(const OrderRecord& order) override;
    void Update(const OrderRecord& order) override;

private:
    std::vector<OrderRecord> records_;
};

} // namespace contract
