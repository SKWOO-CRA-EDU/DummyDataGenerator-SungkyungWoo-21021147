# DummyDataGenerator

`docs/CONTRACT.md`(수정 금지)와 `docs/PRD.md`, `docs/DESIGN.md`에 따라, 시드 기반으로
결정론적인 더미 `SampleRecord`/`OrderRecord` 데이터를 생성해 저장소 인터페이스
(`ISampleRepository`/`IOrderRepository`)에 추가하는 PoC다.

증명 대상 명제: "동일한 시드를 주면 항상 동일한 더미 데이터가 생성된다. 생성된 데이터는
도메인 불변식을 하나도 위반하지 않으며, 연결된 저장소에 추가된다."

## 빌드

```
msbuild DummyDataGenerator.vcxproj /p:Configuration=Debug /p:Platform=Win32
```

## 사용법

```
DummyDataGenerator.exe <seed> <sampleCount> <orderCount>
DummyDataGenerator.exe test
```

- `seed`: 부호 없는 64비트 정수. RNG(`std::mt19937_64`)를 초기화하는 유일한 입력이며,
  시스템 시간이나 `random_device`는 절대 쓰지 않는다 — 같은 시드는 항상 같은 결과를 낸다.
- `sampleCount`: 생성할 `SampleRecord` 개수 (0 이상).
- `orderCount`: 생성할 `OrderRecord` 개수 (0 이상). `orderCount > 0`이면 참조할 Sample이
  있어야 하므로 `sampleCount > 0`이 필요하다.
- `test`: `docs/PRD.md` FR-13(종단 간 재현성·무결성)을 검증하는 내장 테스트 스위트를
  실행한다. 모두 통과하면 종료 코드 0, 하나라도 실패하면 1을 반환한다.

이 실행 파일은 데모/테스트용 인메모리 저장소(`InMemorySampleRepository`,
`InMemoryOrderRepository`)에 데이터를 추가하고 그 내용을 표준 출력으로 보여준다. 실제 영속
파일 I/O는 이 저장소의 책임이 아니다(DataPersistence 저장소 참고, `docs/PRD.md` OS-02).

### 예시: 시드 42로 Sample 5개, Order 8개 생성

```
$ DummyDataGenerator.exe 42 5 8
seed=42 sampleCount=5 orderCount=8

Samples (5):
  SMP-755155 | Sample-639031 | avgProductionTime=752.148 | yieldNumerator=1363 | stockQuantity=90327
  ...

Orders (8):
  #1 | SMP-755155 | Customer-530161 | orderQuantity=4462 | status=CONFIRMED | createdAt=2021-03-23T04:35:05Z
  ...
```

같은 명령을 다시 실행해도 위 출력은 한 글자도 다르지 않다. 시드를 바꾸면(`43` 등) 다른
결과가 나온다.

### 예시: 테스트 실행

```
$ DummyDataGenerator.exe test
[PASS] same seed -> identical output (byte-for-byte field equality)
[PASS] different seed -> different output
[PASS] generated dataset satisfies all CONTRACT invariants (property-based, 5 seeds x 100 samples x 200 orders)
[PASS] adding generated data to a populated repository does not corrupt pre-existing records
```

## 테스트 항목

`Tests.cpp`는 다음 네 가지를 검증한다.

1. **같은 시드로 두 번 생성 → 결과가 완전히 동일** — 같은 시드, 같은 개수로 두 번(각각 빈
   저장소에 대해) 생성해 모든 필드가 일치하는지 비교한다.
2. **다른 시드로 생성 → 결과가 다름** — 서로 다른 두 시드로 생성한 결과가 다른지 비교한다.
3. **생성된 전체 데이터셋이 모든 불변식을 만족함** — 여러 시드 × 다수 레코드(5개 시드 ×
   Sample 100개 × Order 200개)에 대해 SampleId 고유성, AvgProductionTime>0,
   yieldNumerator 범위, StockQuantity≥0, OrderId 고유성·자동증가, SampleId 참조 무결성,
   OrderQuantity>0, Status 허용값, CreatedAt ISO 8601 형식을 전수 검사한다.
4. **기존 데이터가 있는 저장소에 추가 → 기존 데이터가 손상되지 않음** — 저장소에 미리 넣어둔
   Sample/Order 레코드가 이후 생성을 거쳐도 값이 그대로인지, 레코드 수가 정상적으로
   늘어나는지 확인한다.

## 문서

- `docs/CONTRACT.md` — 데이터 모델/영속 포맷 계약(수정 금지)
- `docs/PRD.md` — 범위/비범위, FR-01~FR-13
- `docs/DESIGN.md` — 시드 설계, 저장 경로 설계, 불변식 보장 전략, 생성 순서
- `CLAUDE.md` — 빌드/테스트, 계약 수정 금지, 커밋 규칙
