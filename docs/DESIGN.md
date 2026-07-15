# DESIGN — DummyDataGenerator (PoC)

> 근거: `docs/CONTRACT.md`(수정 금지), `docs/PRD.md`. 설계 문서이며 구현 코드는 포함하지 않는다.

## 1. 난수 시드 설계

- 생성기는 `std::mt19937_64`(또는 `std::mt19937`) 엔진 하나를 **명시적 `uint64_t seed` 파라미터**로만
  초기화한다. 진입점(함수/클래스 생성자)은 시드를 필수 인자로 받으며, 기본 인자·오버로드·
  `std::random_device`·`std::chrono::system_clock::now()` 기반 fallback을 두지 않는다.
- 엔진 인스턴스는 하나만 만들어 생성 세션 전체(Sample 생성 → Order 생성)에서 순서대로 소비한다.
  필드별로 엔진을 나눠 만들면 호출 순서·횟수 변화에 따라 재현성이 깨지므로 금지한다.
- 분포(`std::uniform_int_distribution` 등)는 매 호출 시 새로 만들어도 되지만, 반드시 같은 엔진에서
  **같은 순서로만** `operator()`를 호출해야 한다. "무엇을 몇 번째로 뽑는가"의 순서 자체가 계약의
  일부다(예: SampleId → Name → AvgProductionTime → yieldNumerator → StockQuantity 순으로 고정).
- 문자열(SampleId/Name/CustomerName) 생성도 시드로부터 파생된 인덱스로 사전 정의된 테이블/패턴에서
  선택하여, 로케일·시스템 폰트·정렬 등 플랫폼 의존 요소가 결과에 섞이지 않게 한다.
- "바이트 단위로 동일"의 검증 대상은, 저장소에 넣기 직전의 `SampleRecord`/`OrderRecord` 값 시퀀스로
  한다(영속 파일 직렬화 방식은 DataPersistence 저장소 책임이라 비범위).

## 2. 저장 경로 설계

- 생성기 모듈은 `ISampleRepository&`, `IOrderRepository&`만 알고, 그 뒤가 파일인지 메모리인지 몰라야
  한다 — 파일 경로, `std::ofstream`, 직렬화 코드는 이 저장소에 두지 않는다.
- 생성 함수 시그니처는 대략 다음 형태다(설계 수준):
  `void GenerateDummyData(uint64_t seed, int sampleCount, int orderCount, ISampleRepository& sampleRepo, IOrderRepository& orderRepo)`.
  저장소는 외부에서 주입(DI)받고, 생성기 자신은 구현체를 `new`하지 않는다.
- 각 레코드는 만들어지는 즉시 해당 레코드의 `Add()`를 호출한다. `orderId` 채번은 반드시
  `IOrderRepository::NextOrderId()`를 통해서만 얻는다(자체 카운터 금지).
- `sampleId` 참조 검증은 생성기가 들고 있는 로컬 캐시가 아니라 `ISampleRepository::Exists()` 호출로
  확인한다 — "저장소를 통해서만"이라는 원칙을 참조 무결성 체크에도 동일하게 적용한다.

## 3. 불변식 목록과 생성 시점 보장 방법

| # | 불변식 (CONTRACT 근거) | 생성 시점 보장 방법 |
|---|---|---|
| I1 | `SampleId` 고유 [§1.2] | 채번 시 `sampleRepo.Exists(candidate)`가 `false`일 때까지 재시도(같은 RNG 스트림에서 순차적으로 다음 후보 소비) |
| I2 | `AvgProductionTime > 0` [§1.2] | `uniform_real_distribution<double>(lowerBound, upperBound)`를 `lowerBound > 0`인 개구간으로 정의 |
| I3 | `Yield`(`yieldNumerator`) ∈ [1, 10000], 실효값 (0,1] [§1.2, §3] | `uniform_int_distribution<int>(1, YIELD_DENOMINATOR)`로 정수 자체를 그 범위 안에서만 추출 |
| I4 | `StockQuantity >= 0` [§1.2] | `uniform_int_distribution<int64_t>(0, upperBound)` |
| I5 | `OrderId` 자동증가(1,2,3,...) [§1.3, ADR-Q15] | 직접 채번하지 않고 매 Order 생성 시 `orderRepo.NextOrderId()` 반환값을 그대로 사용 |
| I6 | Order의 `SampleId`는 등록된 Sample 참조(존재 검증) [§1.3] | 이번 세션에서 만든 Sample 목록 중 하나를 RNG로 선택 후 `sampleRepo.Exists(selected)`가 `true`임을 확인하고 사용 |
| I7 | `OrderQuantity > 0` [§1.3] | `uniform_int_distribution<int64_t>(1, upperBound)` |
| I8 | `Status` ∈ 허용 문자열 집합 [§2] | 리터럴 문자열 배열(`{"RESERVED","REJECTED",...}`)에서 인덱스만 RNG로 추출 |
| I9 | `CreatedAt`은 ISO 8601 UTC, 생성 시 기록·불변 [§1.3, §3] | 레코드 생성과 동시에 한 번만 포맷팅해 필드에 대입, 이후 재대입 경로 없음 |
| I10 | 단일 생산라인/전이 관련 불변식 [§1.4, ADR-Q11], REJECTED/RELEASE 이후 필드 변경 금지 [§3] | 생성기는 ProductionQueue·상태 전이를 실행하지 않으므로(비범위) 위반 경로 자체가 없음 |

공통 원칙: 모든 수치 불변식은 사후 검증/클램핑이 아니라, 애초에 위반값을 뽑을 수 없는 분포 경계로
설계한다. 저장소 상태에 의존하는 I1(고유성)·I6(참조 존재)만 저장소 조회(`Exists`)로 사후 확인한다.

## 4. 생성 순서 (참조 무결성 보장)

**Sample 전체 생성 후 Order 생성** — 두 단계로 완전히 분리한다.

1. 시드로 초기화한 RNG로 `SampleRecord`를 N개 생성하고, 하나 만들 때마다 즉시 `sampleRepo.Add()`로 저장.
2. 1단계가 모두 끝난 뒤에만 Order 생성 단계로 진입한다. `OrderRecord`를 M개 생성하되, 각 Order의
   `sampleId`는 이미 저장소에 존재가 확정된 Sample 집합(1단계 결과) 중에서만 RNG로 선택한다.

Order는 Sample을 참조하는 종속 엔티티(1:N)이므로, 참조 대상이 저장소에 먼저 존재해야만 참조 시점에
`Exists()`가 항상 `true`를 반환할 수 있다. 두 종류를 인터리빙하는 것도 이론상 가능하지만, "이번에
만든 것만 참조할 수 있다"는 제약이 생겨 선택 로직이 복잡해지고 재현성 검증도 어려워진다.

일반화: N개 엔티티 타입 간 참조 그래프가 있을 때, 그 그래프를 위상 정렬(topological order)한 순서
대로 타입 단위 전체를 완료한 뒤 다음 타입으로 넘어간다. 이 PoC에서는 Sample → Order 2단계로 끝난다.
