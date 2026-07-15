# PRD — DummyDataGenerator (PoC)

> 근거: 아래 명제, `docs/CONTRACT.md`만 사용. CONTRACT.md는 다른 저장소에서 확정된 계약이며 본 문서에서 수정하지 않는다.

## 0. 증명할 명제

> 동일한 시드를 주면 항상 동일한 더미 데이터가 생성된다.
> 생성된 데이터는 도메인 불변식을 하나도 위반하지 않으며, 연결된 저장소에 추가된다.

이 PoC는 위 명제 하나를 증명하는 것이 유일한 목적이다. 그 외 모든 것은 비범위다.

## 1. 범위 (In scope)

- IS-01. 시드(seed) 값을 입력받아 결정론적으로 `SampleRecord`, `OrderRecord`(CONTRACT §3)를 생성한다.
- IS-02. 동일 시드 재실행 시 동일한 샘플/주문 데이터 집합을 생성한다(값 전체 일치).
- IS-03. 생성된 각 레코드가 CONTRACT §1, §3의 필드 타입·제약(불변식)을 위반하지 않도록 보장한다.
- IS-04. 생성된 `OrderRecord.sampleId`는 동일 실행에서 먼저 생성된 `SampleRecord.sampleId` 중 하나를 참조한다(참조 무결성).
- IS-05. 생성된 데이터를 `ISampleRepository`/`IOrderRepository`(CONTRACT §5) 구현체에 `Add()`로 추가한다.
- IS-06. `Order.Status`는 CONTRACT §2의 허용된 문자열 값만 사용한다.
- IS-07. `OrderId`는 `IOrderRepository::NextOrderId()`를 통해 1부터 증가하는 값으로 부여한다.
- IS-08. `createdAt`은 CONTRACT §3의 ISO 8601 UTC 형식으로 기록한다.

## 2. 비범위 (Out of scope)

- OS-01. 콘솔 UI/메뉴/사용자 상호작용 설계 (시드·개수 등 실행 파라미터는 최소 진입점만 제공).
- OS-02. 영속 파일 포맷의 실제 읽기/쓰기, 원자적 교체, 파일 잠금 구현 (DataPersistence 저장소 책임).
- OS-03. 생산 계산 로직 전체: `ShortageQuantity`, `ActualProductionQuantity`, `TotalProductionTime`, `ProductionQueue` (ConsoleMVC 책임).
- OS-04. CONTRACT §4 상태 전이(T1~T6)의 실행/검증 로직. 생성기는 전이를 수행하지 않고 최초 유효 상태 값만 부여한다.
- OS-05. 재고 상태(Sufficient/Insufficient/Depleted) 판정 로직 (DataMonitor 저장소 책임).
- OS-06. 서로 다른 시드가 서로 다른 데이터를 생성한다는 보장(비충돌성) — 명제는 "동일 시드 → 동일 데이터"만 주장하며 그 역은 다루지 않는다.
- OS-07. 대량 데이터 생성 시 성능/메모리 최적화, 벤치마킹.
- OS-08. 동시성/멀티스레드 생성.
- OS-09. 네트워크·분산 환경에서의 생성.
- OS-10. `schemaVersion` 마이그레이션 분기 로직 (값 `1` 기록 외 처리 없음).
- OS-11. 생성 난수 알고리즘의 암호학적 안전성/품질 보증.
- OS-12. 생성 데이터의 비즈니스적 사실성(실제 고객명·상품명다움 등) — 불변식만 만족하면 값 자체는 임의.
- OS-13. 저장소 쓰기 실패 시 롤백/재시도/부분 실패 복구.
- OS-14. 로깅 프레임워크, 국제화(i18n), 외부 설정 파일 지원.
- OS-15. 단위/통합 테스트 프레임워크 도입 (빌드 및 최소 실행 검증으로 대체).
- OS-16. CONTRACT.md 자체의 수정 또는 재해석.

## 3. 기능 요구사항 (FR)

### FR-01. 시드 기반 결정론적 Sample 생성
- **Given** 정수 시드 값 S와 생성 개수 N이 주어진다.
- **When** 시드 S로 더미 Sample 데이터 N개를 생성한다.
- **Then** 동일한 S, N으로 다시 실행하면 생성된 `SampleRecord` 목록(필드 값 전체)이 이전 실행과 완전히 동일하다.

### FR-02. 시드 기반 결정론적 Order 생성
- **Given** 정수 시드 값 S, 이미 생성된 Sample 목록, 생성 개수 M이 주어진다.
- **When** 시드 S로 더미 Order 데이터 M개를 생성한다.
- **Then** 동일한 S, 동일한 Sample 목록, M으로 다시 실행하면 생성된 `OrderRecord` 목록이 이전 실행과 완전히 동일하다.

### FR-03. SampleId 고유성
- **Given** N개의 Sample을 생성한다.
- **When** 생성이 완료된다.
- **Then** 모든 `SampleRecord.sampleId`가 서로 중복되지 않는다.

### FR-04. AvgProductionTime 양수 제약
- **Given** Sample을 생성한다.
- **When** `avgProductionTime` 값을 부여한다.
- **Then** 그 값은 항상 `0`보다 크다.

### FR-05. Yield 유효 범위
- **Given** Sample을 생성한다.
- **When** `yieldNumerator` 값을 부여한다.
- **Then** `1 <= yieldNumerator <= YIELD_DENOMINATOR(10000)`이며, 실효값 `yieldNumerator / 10000`은 `(0, 1]` 구간에 속한다.

### FR-06. StockQuantity 음수 불가
- **Given** Sample을 생성한다.
- **When** `stockQuantity` 값을 부여한다.
- **Then** 그 값은 항상 `0` 이상이다.

### FR-07. Order의 OrderQuantity 양수 제약
- **Given** Order를 생성한다.
- **When** `orderQuantity` 값을 부여한다.
- **Then** 그 값은 항상 `0`보다 크다.

### FR-08. Order의 SampleId 참조 무결성
- **Given** 이미 생성된 Sample 목록이 존재한다.
- **When** Order를 생성하며 `sampleId`를 부여한다.
- **Then** 해당 `sampleId`는 연결된 `ISampleRepository::Exists()`가 `true`를 반환하는 값이다.

### FR-09. Order.Status 허용값 제약
- **Given** Order를 생성한다.
- **When** `status` 값을 부여한다.
- **Then** 그 값은 CONTRACT §2.1의 `"RESERVED"|"REJECTED"|"PRODUCING"|"CONFIRMED"|"RELEASE"` 중 하나이다.

### FR-10. OrderId 자동 증가
- **Given** 저장소에 이미 K개의 Order가 존재한다(K=0 포함).
- **When** 새 Order를 생성하며 `IOrderRepository::NextOrderId()`를 호출한다.
- **Then** 부여되는 `orderId`는 기존 최대값 + 1이며, 이후 연속 생성 시 1씩 증가한다.

### FR-11. CreatedAt 형식
- **Given** Order를 생성한다.
- **When** `createdAt` 값을 부여한다.
- **Then** 그 값은 CONTRACT §3에 명시된 ISO 8601 UTC 형식(`YYYY-MM-DDThh:mm:ssZ`)을 따른다.

### FR-12. 생성 데이터의 저장소 반영
- **Given** Sample N개, Order M개가 생성되어 있다.
- **When** 생성기가 실행을 완료한다.
- **Then** 연결된 `ISampleRepository`, `IOrderRepository`의 `Add()`가 각각 N회, M회 호출되어 데이터가 저장소에 추가되고, `FindAll()`로 조회 시 생성된 레코드 전체가 확인된다.

### FR-13. 종단 간 재현성 및 무결성 (핵심 명제 검증)
- **Given** 동일한 시드 S로 생성기를 두 번(각각 새 저장소 인스턴스에 대해) 실행한다.
- **When** 두 실행 결과를 비교한다.
- **Then** (a) 두 실행에서 생성된 Sample/Order 데이터 집합이 완전히 동일하고, (b) 두 실행 각각에서 생성된 모든 레코드가 FR-03~FR-09, FR-11의 불변식을 하나도 위반하지 않으며, (c) 두 저장소 모두에 데이터가 정상적으로 추가되어 있다.
