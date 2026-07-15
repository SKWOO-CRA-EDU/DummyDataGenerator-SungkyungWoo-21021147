> 본 문서는 `SampleOrderSystem-SungkyungWOO-21021147/docs/CONTRACT.md`의 사본이다. 원본 외 위치에서 수정하지 말 것.

# CONTRACT — 데이터 모델 · 영속 포맷 계약서

> **버전**: v2 | **최종 수정일**: 2026-07-15

> 근거: `docs/PRD.md`, `docs/DECISIONS.md`만 사용. 4개 PoC 저장소(ConsoleMVC, DataPersistence,
> DataMonitor, DummyDataGenerator)에 동일 사본 배포 — 필드명/타입/열거값/포맷/시그니처를 글자 그대로 따를 것.
> UI·구현코드는 범위 밖(각 저장소 자유). `[결정-리서치]` = SPEC/DECISIONS에 근거 없어 사용자 승인 하에
> 업계 관행을 웹 리서치로 확정한 항목(SampleId 자료형, Yield 내부표현, 타임스탬프 형식, 스키마버전 필드, Delete/참조무결성 정책).

> **이 문서는 이 저장소(SampleOrderSystem-SungkyungWOO-21021147)에서만 개정된다.**
> PoC 저장소(ConsoleMVC, DataPersistence, DataMonitor, DummyDataGenerator)에 배포된 사본은 읽기 전용이며,
> 그 사본의 버전 헤더가 이 문서의 최신 버전과 다르면 그 PoC 저장소의 사본은 낡은 것이다 — 반드시 이 문서를 기준으로 갱신할 것.

## 1. 엔티티

### 1.1 한영 매핑
| 한국어 | 영문 |
|---|---|
| 시료ID/이름/평균생산시간/수율/재고수량 | SampleId/Name/AvgProductionTime/Yield/StockQuantity |
| 주문ID/고객명/주문수/상태 | OrderId/CustomerName/OrderQuantity/Status |
| 생산큐(항목)/부족분/실생산량/총생산시간 | ProductionQueue(Item)/ShortageQuantity/ActualProductionQuantity/TotalProductionTime |
| 여유/부족/고갈 | Sufficient/Insufficient/Depleted |

### 1.2 Sample
| 필드 | 타입/제약 | 근거 |
|---|---|---|
| SampleId | `std::string`[결정-리서치], 고유 | [PRD FR-01][PRD FR-29] |
| Name | `std::string` | [PRD FR-01] |
| AvgProductionTime | `double` > 0, 등록 시 거부 | [PRD FR-03] |
| Yield | §3 `yieldNumerator` 참조, 실효값 ∈ (0,1] | [PRD FR-02][ADR-Q3][ADR-Q4] |
| StockQuantity | `int64_t` ≥ 0, 내부 로직만 변경 | [ADR-Q1][ADR-Q12] |

불변식: Yield∈(0,1](등록시 강제) [ADR-Q3/Q4] · AvgProductionTime>0 [ADR-Q12] · StockQuantity≥0(코드 보장) [ADR-Q1/Q12]

### 1.3 Order
| 필드 | 타입/제약 | 근거 |
|---|---|---|
| OrderId | `int64_t`, 자동증가(1,2,3,...) | [ADR-Q15] |
| SampleId | `std::string`, 등록 Sample 참조(존재검증) | [PRD FR-06][PRD FR-29] |
| CustomerName | `std::string` | [PRD FR-06] |
| OrderQuantity | `int64_t` > 0, 입력시 거부 | [PRD FR-07] |
| Status | §2.1 값만 허용 | [PRD FR-06~13,20] |
| CreatedAt | §3 타임스탬프 표현, 생성시 기록·불변 | [PRD FR-32] |

### 1.4 ProductionQueueItem
| 필드 | 의미 | 근거 |
|---|---|---|
| OrderId | PRODUCING 전이 대상 주문 참조 | [PRD FR-10] |
| ShortageQuantity | max(0, OrderQuantity−StockQuantity) | [ADR-Q1] |
| ActualProductionQuantity | ceil(ShortageQuantity/Yield) | [PRD FR-17] |
| TotalProductionTime | AvgProductionTime × ActualProductionQuantity | [PRD FR-18] |
| QueueOrder | FIFO(등록순) | [PRD FR-16][ADR-Q11] |

불변식: 단일 생산라인, 한 번에 하나의 항목만 실행 상태. [ADR-Q11]

## 2. 열거형 표기 문자열

**Order.Status**: `"RESERVED"` `"REJECTED"` `"PRODUCING"` `"CONFIRMED"` `"RELEASE"` [PRD §2.2][SPEC §3]

**재고 상태(모니터링 전용, Status 아님)**: `"Sufficient"`(여유) `"Insufficient"`(부족) `"Depleted"`(고갈) [PRD FR-22][ADR-Q5]

## 3. 영속 저장 포맷 스키마

> 파일 형식·경로·원자적 교체 구현은 저장소 자유, 아래 필드명/타입은 4개 저장소 공통. [PRD FR-23][PRD FR-30]

```
RootDocument
├─ schemaVersion : int = 1                        [결정-리서치, 근거: ADR-R8/PRD FR-30]
├─ samples : array<SampleRecord>
└─ orders  : array<OrderRecord>

SampleRecord                              OrderRecord
├─ sampleId          : string             ├─ orderId       : int64 (자동증가 ≥1)
├─ name               : string             ├─ sampleId      : string (samples[].sampleId 참조)
├─ avgProductionTime  : double (>0)        ├─ customerName  : string
├─ yieldNumerator     : int (1..YIELD_DENOMINATOR)  ├─ orderQuantity : int64 (>0)
└─ stockQuantity      : int64 (>=0)        ├─ status        : string (§2.1)
                                           └─ createdAt     : string (ISO 8601 UTC)
```

- `yieldNumerator`: 실효값 = `yieldNumerator / YIELD_DENOMINATOR`(정수 연산 경로). `YIELD_DENOMINATOR = 10000`(공통 상수, 0.925급 3자리 소수 표현). 정수 스케일 채택 근거는 [PRD FR-26][ADR-R4]; 분자/고정분모 10000 값은 [결정-리서치].
- `createdAt`: ISO 8601 UTC(예: `"2026-07-15T10:30:00Z"`). 기록 의무 근거 [PRD FR-32][ADR-R10]; 형식은 [결정-리서치].
- `schemaVersion`: 이름/타입/초깃값은 [결정-리서치](로더 마이그레이션 분기용).
- REJECTED/RELEASE 이후 §4 외 전이로 필드 변경 금지. [ADR-Q8]

## 4. 상태 전이표

| # | From→To | 근거 |
|---|---|---|
| T1 | RESERVED→CONFIRMED | [PRD FR-09] |
| T2 | RESERVED→PRODUCING | [PRD FR-10] |
| T3 | RESERVED→REJECTED | [PRD FR-11] |
| T4 | PRODUCING→CONFIRMED | [PRD FR-19] |
| T5 | CONFIRMED→RELEASE | [PRD FR-20] |
| T6 | CONFIRMED→REJECTED | [PRD FR-12] |

미지원: PRODUCING→REJECTED [ADR-Q6] · RELEASE→(전체) [ADR-Q8] · REJECTED→(전체) [ADR-Q8]

## 5. 저장소(Repository) 인터페이스 (선언만, 구현 금지)

```cpp
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
    virtual bool Delete(const std::string& sampleId) = 0;  // 물리적 제거만 수행. 참조무결성 검증(ADR-C2)은 호출자(Model/Controller) 책임 — 존재하지 않으면 false 반환.
};

// IOrderRepository에는 의도적으로 Delete/Remove 메서드가 없다.
// Order는 어떤 상태에서도 하드 삭제를 지원하지 않는다 [ADR-C3] — REJECTED/RELEASE 전이(Q6/Q7/Q8)가
// 논리적 삭제 역할을 하며, FR-32(감사 추적)의 영구 보존 목적과 충돌하므로 누락이 아닌 설계 결정이다.
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
```
근거: FindById/FindAll [PRD FR-04][PRD FR-08] · Exists [PRD FR-29] · FindByStatus [PRD FR-21] · NextOrderId [ADR-Q15] · Add/Update [PRD FR-01,06,09-12,19,20] · Delete(Sample만) [PRD FR-33][ADR-C1][ADR-C2] · Order Delete 미지원 [ADR-C3][PRD §6 비범위]

## 6. 계산 규칙

```
Yield(실효값)            = yieldNumerator / YIELD_DENOMINATOR                 [PRD FR-26][ADR-R4][결정-리서치]
ShortageQuantity         = max(0, OrderQuantity - StockQuantity)               [ADR-Q1]
ActualProductionQuantity = ceil(ShortageQuantity / Yield)                      [PRD FR-17]
TotalProductionTime      = AvgProductionTime × ActualProductionQuantity        [PRD FR-18][ADR-Q16]
재고 반영량(생산완료)     = floor(ActualProductionQuantity × Yield)            [PRD FR-19][ADR-Q13]
```

StockQuantity 변경: T1 `-= OrderQuantity` [PRD FR-09][ADR-Q9] · T2 `-= OrderQuantity` [PRD FR-10][ADR-Q9]
· T4 `+= floor(ActualProductionQuantity × Yield)` [PRD FR-19][ADR-Q13] · T6 `+= OrderQuantity` [PRD FR-12][ADR-Q7]

재고 상태(판단 대상 주문 OrderQuantity 기준): `Depleted: StockQuantity=0` · `Insufficient: 0<StockQuantity<OrderQuantity`
· `Sufficient: StockQuantity>=OrderQuantity` [PRD FR-22][ADR-Q5]

곱셈 기호 표기: `×` 사용. [ADR-Q16]

## CHANGELOG

| 버전 | 변경 내용 | 근거 ADR-ID | 영향받는 PoC 저장소 |
|---|---|---|---|
| v1 | 최초 버전. PRD.md/DECISIONS.md(Q1~Q16, R1~R10) 전체를 근거로 엔티티(§1)·열거형(§2)·영속 저장 포맷(§3)·상태 전이표(§4)·Repository 인터페이스(§5, Delete 없음)·계산 규칙(§6) 작성. (작성일 미상 — 이 CHANGELOG 도입 이전 소급 기록) | Q1~Q16, R1~R10 전체 | ConsoleMVC, DataPersistence, DataMonitor, DummyDataGenerator (전체 4개) |
| v2 | SPEC §8 "미션1" PoC 산출물 요구사항("데이터영속성처리 — CRUD 포함")과의 감사 결과 발견된 공백을 메움: `ISampleRepository`에 `Delete` 메서드 추가, 시료 삭제 시 참조무결성 가드(Order가 참조 중이면 삭제 거부) 명시. `IOrderRepository`는 Delete 미지원임을 의도적 설계로 명문화. 문서 최상단에 버전/최종수정일 헤더, "이 저장소에서만 개정" 원칙 추가. 본 CHANGELOG 섹션 신설(v1 소급 기록 포함). | ADR-C1, ADR-C2, ADR-C3 | ConsoleMVC, DataPersistence, DataMonitor, DummyDataGenerator (전체 4개 — Repository 인터페이스 시그니처 변경) |
