# CONTRACT — 데이터 모델 · 영속 포맷 계약서

> **버전**: v3 | **최종 수정일**: 2026-07-15

> **동결 선언**: v3 시점으로 저장소 계층 인터페이스는 동결되었다.
> 이후 개정은 새로운 도메인 요구가 발생한 경우에 한한다. 설계 편의를 이유로 한 개정은 하지 않는다.

> 근거: `docs/PRD.md`, `docs/DECISIONS.md`만 사용. 4개 PoC 저장소(ConsoleMVC, DataPersistence,
> DataMonitor, DummyDataGenerator)에 동일 사본 배포 — 필드명/타입/열거값/포맷/시그니처를 글자 그대로 따를 것.
> UI·구현코드는 범위 밖(각 저장소 자유). `[결정-리서치]` = SPEC/DECISIONS에 근거 없어 사용자 승인 하에
> 업계 관행을 웹 리서치로 확정한 항목(SampleId 자료형, Yield 내부표현, 타임스탬프 형식, 스키마버전 필드, Delete/참조무결성 정책, 오류 어휘).

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
| OrderId | `int64_t`, 자동증가(1,2,3,...), Order의 유일한 키 | [ADR-Q15][ADR-E5] |
| SampleId | `std::string`, 등록 Sample 참조(존재검증은 Model 책임, §7 비범위 참조) | [PRD FR-06][PRD FR-29][ADR-E9] |
| CustomerName | `std::string` | [PRD FR-06] |
| OrderQuantity | `int64_t` > 0, 입력시 거부 | [PRD FR-07] |
| Status | §2 `OrderStatus` 열거값만 허용 | [PRD FR-06~13,20] |
| CreatedAt | §3 타임스탬프 표현, 생성시 기록·불변 | [PRD FR-32] |

표시용 주문번호 필드는 신설하지 않는다 — `OrderId`가 유일한 진실 원천이며 화면 표기는 View가 이로부터 파생한다. [ADR-E5]

### 1.4 ProductionQueueItem
| 필드 | 의미 | 근거 |
|---|---|---|
| OrderId | PRODUCING 전이 대상 주문 참조 | [PRD FR-10] |
| ShortageQuantity | max(0, OrderQuantity−StockQuantity) | [ADR-Q1] |
| ActualProductionQuantity | ceil(ShortageQuantity/Yield) | [PRD FR-17] |
| TotalProductionTime | AvgProductionTime × ActualProductionQuantity | [PRD FR-18] |
| QueueOrder | FIFO(등록순) | [PRD FR-16][ADR-Q11] |

불변식: 단일 생산라인, 한 번에 하나의 항목만 실행 상태. [ADR-Q11]

## 2. 열거형

**Order.Status** — C++ 열거형(§5 시그니처에서 사용) 및 영속 저장 문자열(§3 wire format) 매핑:

| `OrderStatus` (열거형) | 저장 문자열 | 근거 |
|---|---|---|
| `OrderStatus::Reserved` | `"RESERVED"` | [PRD §2.2][SPEC §3] |
| `OrderStatus::Rejected` | `"REJECTED"` | 〃 |
| `OrderStatus::Producing` | `"PRODUCING"` | 〃 |
| `OrderStatus::Confirmed` | `"CONFIRMED"` | 〃 |
| `OrderStatus::Release` | `"RELEASE"` | 〃 |

`IOrderRepository::FindByStatus`는 문자열이 아닌 `OrderStatus` 열거형을 인자로 받는다 — 열거값 외 문자열 입력이라는 실패 상황 자체를 컴파일 타임에 불가능하게 만든다. [ADR-E6]

**재고 상태(모니터링 전용, Status 아님)**: `"Sufficient"`(여유) `"Insufficient"`(부족) `"Depleted"`(고갈) [PRD FR-22][ADR-Q5]

## 3. 영속 저장 포맷 스키마

> 파일 형식·경로·원자적 교체 구현은 저장소 자유, 아래 필드명/타입은 4개 저장소 공통. [PRD FR-23][PRD FR-30]

```
RootDocument
├─ schemaVersion : int = 1                        [결정-리서치, 근거: ADR-R8/PRD FR-30]
├─ samples : array<SampleRecord>
└─ orders  : array<OrderRecord>

SampleRecord                              OrderRecord
├─ sampleId          : string             ├─ orderId       : int64 (자동증가 ≥1, 유일한 키) [ADR-E5]
├─ name               : string             ├─ sampleId      : string (samples[].sampleId 참조)
├─ avgProductionTime  : double (>0)        ├─ customerName  : string
├─ yieldNumerator     : int (1..YIELD_DENOMINATOR)  ├─ orderQuantity : int64 (>0)
└─ stockQuantity      : int64 (>=0)        ├─ status        : string (§2의 5개 값 중 하나만 유효 — §6 불변식)
                                           └─ createdAt     : string (ISO 8601 UTC)
```

- `yieldNumerator`: 실효값 = `yieldNumerator / YIELD_DENOMINATOR`(정수 연산 경로). `YIELD_DENOMINATOR = 10000`(공통 상수, 0.925급 3자리 소수 표현). 정수 스케일 채택 근거는 [PRD FR-26][ADR-R4]; 분자/고정분모 10000 값은 [결정-리서치].
- `createdAt`: ISO 8601 UTC(예: `"2026-07-15T10:30:00Z"`). 기록 의무 근거 [PRD FR-32][ADR-R10]; 형식은 [결정-리서치].
- `schemaVersion`: 이름/타입/초깃값은 [결정-리서치]. 저장된 값이 프로그램이 아는 버전과 다르면 로더는 `SchemaVersionMismatch`를 던지고 종료한다(변환하지 않음 — §7 비범위).
- `orderId`: 표시용 문자열을 별도로 저장하지 않는다. 화면 표기는 View가 `orderId`(및 필요 시 `createdAt`)로부터 순수 파생한다. [ADR-E5]
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

상태 전이 적법성(§4 외 전이 시도 거부)은 Repository가 검증하지 않는다 — Model의 책임이다. [ADR-E9][PRD FR-27]

## 5. 실패 범주 및 오류 어휘

### 5.1 3범주

계약이 표현하는 모든 결과는 다음 3범주 중 정확히 하나에 속한다. 각 범주의 관용구는 정확히 하나다. [ADR-E1]

| 범주 | 의미 | 관용구 |
|---|---|---|
| **[R]** 정상 결과 | 부재는 실패가 아니다 | `std::optional<T>` / `std::vector<T>` |
| **[D]** 도메인 거부 | 저장소는 정상이나 요청이 거부됨 | `[[nodiscard]] WriteOutcome` |
| **[F]** 치명 | 저장소를 신뢰할 수 없음 | 예외(아래 3종), 앱 경계에서 1회 포착 후 종료 |

### 5.2 오류 어휘 (6개, 저장 방식과 무관)

```cpp
[[nodiscard]] enum class WriteOutcome { Ok, NotFound, DuplicateKey };

class StorageUnavailable : public std::runtime_error { /* 저장소를 읽거나 쓸 수 없음 */ };
class StorageCorrupted   : public std::runtime_error { /* 읽었으나 해석 불가 또는 불변식 위반 */ };
class SchemaVersionMismatch : public std::runtime_error { /* 저장된 스키마 버전이 프로그램이 아는 버전과 다름 */ };
```

구현 라이브러리의 예외 타입(`std::ios_base::failure`, `std::filesystem::filesystem_error` 등)이나 파일시스템 용어가 이 6개 어휘 밖으로 새어나오면 결함이다 — Repository 구현체는 내부 예외를 반드시 이 어휘로 재포장(wrap)한다. [ADR-E2]

### 5.3 참조무결성

**수임자**: Model(도메인 서비스). Controller가 아니다. [ADR-E8]

**절차**: Model은 `ISampleRepository::Delete` 호출 전에 `IOrderRepository::FindBySampleId(sampleId)`를 호출해 참조를 확인한다. 결과가 비어 있지 않으면 `Delete`를 호출하지 않고, View를 통해 거부 메시지를 표시한다(Controller는 조율만 한다 — CLAUDE.md §2). `ISampleRepository::Delete` 자체는 참조를 검사하지 않는다. [ADR-E9][ADR-C2]

### 5.4 Repository 검증 범위

| 검증하는 것 | 검증하지 않는 것 |
|---|---|
| 키 유일성(Add 시 DuplicateKey) | 참조무결성(Model 책임, §5.3) |
| 대상 존재(Update/Delete 시 NotFound) | 상태 전이 적법성(Model 책임, §4) |
| 저장 매체 상태(StorageUnavailable/Corrupted/SchemaVersionMismatch) | 수량 범위(Model 책임) |
| | 그 밖의 모든 도메인 규칙 |

[ADR-E9]

## 6. 불변식

- Sample: §1.2 불변식(Yield/AvgProductionTime/StockQuantity).
- ProductionQueueItem: §1.4 불변식(단일 생산라인).
- **모든 `OrderStatus` 값에 대한 `FindByStatus` 결과 크기의 합은 `FindAll` 결과의 크기와 같다.**
- **저장된 Order.status 값은 §2의 5개 열거값 중 하나여야 한다.**

위 두 불변식 위반이 발견되면 `StorageCorrupted`로 취급한다. [ADR-E10]

## 7. 저장소(Repository) 인터페이스 (선언만, 구현 금지)

```cpp
struct SampleRecord {
    std::string sampleId; std::string name;
    double avgProductionTime; int yieldNumerator; int64_t stockQuantity;
};
struct OrderRecord {
    int64_t orderId; std::string sampleId; std::string customerName;
    int64_t orderQuantity; OrderStatus status; std::string createdAt;
};
constexpr int YIELD_DENOMINATOR = 10000;

// 예외(§5.2): StorageUnavailable, StorageCorrupted, SchemaVersionMismatch.
// 아래 모든 메서드는 이 3개 예외를 던질 수 있다(각 메서드 주석에서 반복 표기하지 않음).
// 이 3개 예외 외에는 메서드가 던지지 않는다.

class ISampleRepository {
public:
    virtual ~ISampleRepository() = default;

    // [R] FindById/FindAll — 정상 결과. 부재는 실패가 아니다.
    virtual std::optional<SampleRecord> FindById(const std::string& sampleId) const = 0;
    virtual std::vector<SampleRecord> FindAll() const = 0;

    // [D] Add/Update/Delete — 도메인 거부는 WriteOutcome으로 표현한다.
    virtual WriteOutcome Add(const SampleRecord& sample) = 0;      // 키 중복 시 DuplicateKey
    virtual WriteOutcome Update(const SampleRecord& sample) = 0;   // 대상 없음 시 NotFound
    virtual WriteOutcome Delete(const std::string& sampleId) = 0;  // 대상 없음 시 NotFound.
                                                                    // 참조무결성은 검증하지 않는다(§5.3/§5.4) — 호출 전 확인은 Model 책임.
};

// IOrderRepository에는 의도적으로 Delete/Remove 메서드가 없다.
// Order는 어떤 상태에서도 하드 삭제를 지원하지 않는다 [ADR-C3] — REJECTED/RELEASE 전이(Q6/Q7/Q8)가
// 논리적 삭제 역할을 하며, FR-32(감사 추적)의 영구 보존 목적과 충돌하므로 누락이 아닌 설계 결정이다.
//
// IOrderRepository에는 의도적으로 NextOrderId 메서드가 없다 [ADR-E4].
// 채번(자동증가 정수, ADR-Q15는 유지)은 도메인 서비스(Model)의 책임이다:
// Model이 FindAll()로 기존 최대 orderId를 구해 +1 하고, Add()의 DuplicateKey가 최종 방어선이 된다.
class IOrderRepository {
public:
    virtual ~IOrderRepository() = default;

    // [R] 정상 결과. 부재는 실패가 아니다.
    virtual std::optional<OrderRecord> FindById(int64_t orderId) const = 0;
    virtual std::vector<OrderRecord> FindAll() const = 0;
    virtual std::vector<OrderRecord> FindByStatus(OrderStatus status) const = 0;  // 문자열 아님 [ADR-E6]
    virtual std::vector<OrderRecord> FindBySampleId(const std::string& sampleId) const = 0;  // 신설 [ADR-E7]

    // [D] 도메인 거부는 WriteOutcome으로 표현한다.
    virtual WriteOutcome Add(const OrderRecord& order) = 0;     // 키 중복 시 DuplicateKey
    virtual WriteOutcome Update(const OrderRecord& order) = 0;  // 대상 없음 시 NotFound.
                                                                 // 상태 전이 적법성은 검증하지 않는다(§5.4) — Model 책임.
};
```

근거: FindById/FindAll [PRD FR-04][PRD FR-08] · FindByStatus [PRD FR-21][ADR-E6] · FindBySampleId [ADR-C2][ADR-E7][ADR-E8]
· Add/Update [PRD FR-01,06,09-12,19,20][ADR-E1] · Delete(Sample만) [PRD FR-33][ADR-C1][ADR-C2][ADR-E8]
· Order Delete 미지원 [ADR-C3][PRD §6 비범위] · NextOrderId 미제공 [ADR-E4] · Exists 제거 [ADR-E3]

### 7.1 메서드별 범주 표 (11개 메서드)

> §7 코드의 블록 주석(범주별 묶음 표기)과 내용은 동일하다. 대조를 기계적으로 할 수 있도록 메서드별 한 행으로 병기한다.

| 메서드 | 범주 | 반환 관용구 |
|---|---|---|
| `ISampleRepository::FindById` | [R] | `std::optional<SampleRecord>` |
| `ISampleRepository::FindAll` | [R] | `std::vector<SampleRecord>` |
| `ISampleRepository::Add` | [D] | `WriteOutcome` (Ok/DuplicateKey) |
| `ISampleRepository::Update` | [D] | `WriteOutcome` (Ok/NotFound) |
| `ISampleRepository::Delete` | [D] | `WriteOutcome` (Ok/NotFound) |
| `IOrderRepository::FindById` | [R] | `std::optional<OrderRecord>` |
| `IOrderRepository::FindAll` | [R] | `std::vector<OrderRecord>` |
| `IOrderRepository::FindByStatus` | [R] | `std::vector<OrderRecord>` |
| `IOrderRepository::FindBySampleId` | [R] | `std::vector<OrderRecord>` |
| `IOrderRepository::Add` | [D] | `WriteOutcome` (Ok/DuplicateKey) |
| `IOrderRepository::Update` | [D] | `WriteOutcome` (Ok/NotFound) |

[F] 범주(예외)는 메서드별 행이 아니라 §5.2/§7 상단의 폐쇄 진술 1곳에 집중한다 — 11개 메서드 모두 `StorageUnavailable`/`StorageCorrupted`/`SchemaVersionMismatch`를 던질 수 있고, 이 3개 외에는 던지지 않는다. [ADR-E2]

## 8. 계산 규칙

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

## 9. 비범위 (Repository 계약 관점)

| 항목 | 결정 | 근거 |
|---|---|---|
| 스키마 마이그레이션 | 버전이 다르면 `SchemaVersionMismatch`를 던지고 종료한다. 변환하지 않는다. | [ADR-E11] |
| 부분 손상 복구 | 레코드 하나가 깨지면 저장소 전체를 `StorageCorrupted`로 간주한다. 손상 레코드를 건너뛰고 나머지를 반환하는 것을 금지한다(조용한 스킵은 은닉된 데이터 손실이 된다). | [ADR-E11] |
| 외부 조작으로 생긴 중복 레코드 탐지 | 정상 경로는 `Add`의 `DuplicateKey`로 차단된다. 저장 파일을 직접 편집해 생긴 중복은 탐지되면 `StorageCorrupted`로 취급하되, 탐지를 보장하지는 않는다. | [ADR-E11] |
| 동시 쓰기 | 쓰기 프로세스는 하나로 가정한다. 모니터 도구는 읽기 전용이다. | [ADR-Q10][ADR-E11] |
| 참조무결성 검증(Repository 내부) | Repository는 검증하지 않는다 — Model이 호출 전에 확인한다(§5.3). | [ADR-E8][ADR-E9] |
| 상태 전이 적법성 검증(Repository 내부) | Repository는 검증하지 않는다 — Model이 검증한다(§4). | [ADR-E9] |

## CHANGELOG

| 버전 | 변경 내용 | 근거 ADR-ID | 영향받는 PoC 저장소 |
|---|---|---|---|
| v1 | 최초 버전. PRD.md/DECISIONS.md(Q1~Q16, R1~R10) 전체를 근거로 엔티티(§1)·열거형(§2)·영속 저장 포맷(§3)·상태 전이표(§4)·Repository 인터페이스(§5, Delete 없음)·계산 규칙(§6) 작성. (작성일 미상 — 이 CHANGELOG 도입 이전 소급 기록) | Q1~Q16, R1~R10 전체 | ConsoleMVC, DataPersistence, DataMonitor, DummyDataGenerator (전체 4개) |
| v2 | SPEC §8 "미션1" PoC 산출물 요구사항("데이터영속성처리 — CRUD 포함")과의 감사 결과 발견된 공백을 메움: `ISampleRepository`에 `Delete` 메서드 추가, 시료 삭제 시 참조무결성 가드(Order가 참조 중이면 삭제 거부) 명시. `IOrderRepository`는 Delete 미지원임을 의도적 설계로 명문화. 문서 최상단에 버전/최종수정일 헤더, "이 저장소에서만 개정" 원칙 추가. 본 CHANGELOG 섹션 신설(v1 소급 기록 포함). | ADR-C1, ADR-C2, ADR-C3 | ConsoleMVC, DataPersistence, DataMonitor, DummyDataGenerator (전체 4개 — Repository 인터페이스 시그니처 변경) |
| v3 | AUDIT-CONTRACT-v2.md(B-02 재감사) 42행 전체를 (a)표현/(b)불가능화/(c)범위밖선언 중 하나로 닫음. 실패를 [R]/[D]/[F] 3범주로 재정의하고 오류 어휘 6개(WriteOutcome 3값 + 예외 3종) 확정. `Exists` 제거(FindById로 대체). `NextOrderId` 제거(채번은 Model 책임, ADR-Q15 유지). `FindByStatus` 인자를 문자열→`OrderStatus` 열거형으로 교체. `IOrderRepository::FindBySampleId` 신설. Sample/Order의 Add/Update, Sample의 Delete가 `void`/`bool`→`WriteOutcome`으로 변경. 참조무결성 수임자를 Model 단독으로 확정(ADR-C2의 "Model/Controller" 병기 및 FindBySampleId 미신설 판단을 Superseded 처리). Repository 검증 범위·불변식·비범위 절 신설. 문서 최상단에 동결 선언 추가. | ADR-E1~E11 | ConsoleMVC, DataPersistence, DataMonitor, DummyDataGenerator (전체 4개 — Repository 인터페이스 시그니처 전면 변경) |
