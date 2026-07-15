# DummyDataGenerator

## 증명 대상 명제

> 동일한 시드를 주면 항상 동일한 더미 데이터가 생성된다. 생성된 데이터는 도메인 불변식을
> 하나도 위반하지 않으며, 연결된 저장소에 추가된다.

이 저장소는 이 명제 하나만을 증명하는 개인과제 PoC다 (`docs/PRD.md`).

## 이 조각이 담당하는 위치

`docs/CONTRACT.md`는 `SampleOrderSystem-SungkyungWOO-21021147`에서 확정되어 4개의 독립 PoC
저장소에 동일 사본으로 배포된다: `ConsoleMVC`(UI/생산 계산), `DataPersistence`(파일 영속 I/O),
`DataMonitor`(재고 상태 판정), 그리고 이 저장소 `DummyDataGenerator`.

이 저장소는 그중 "시드 기반 더미 데이터 생성" 한 조각만 맡는다. `ISampleRepository` /
`IOrderRepository`(CONTRACT §5) 인터페이스만을 통해 데이터를 저장소에 추가하며, 실제 파일
포맷/영속 구현, 생산 계산, 재고 상태 판정, 상태 전이 실행은 다른 PoC 저장소의 책임이다.

## 빌드

```
msbuild DummyDataGenerator.vcxproj /p:Configuration=Debug /p:Platform=Win32
```

## 테스트

```
DummyDataGenerator.exe test
```

이 저장소에는 별도 테스트 프레임워크가 없다(PRD OS-15). 위 빌드 성공 + 이 명령의 종료 코드
0을 "테스트 통과"로 간주한다. 내부적으로 다음 네 가지를 검증한다:

1. 같은 시드로 두 번 생성 → 결과가 완전히 동일
2. 다른 시드로 생성 → 결과가 다름
3. 생성된 전체 데이터셋이 모든 CONTRACT 불변식을 만족함 (다수 시드 × 다수 레코드에 대한 전수 검사)
4. 기존 데이터가 있는 저장소에 추가 생성해도 기존 데이터가 손상되지 않음

## 실행 예시

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

- `seed`: 부호 없는 64비트 정수. `std::mt19937_64` RNG를 초기화하는 유일한 입력이며,
  시스템 시간이나 `random_device`는 절대 쓰지 않는다.
- `sampleCount`, `orderCount`: 생성할 레코드 개수(0 이상). `orderCount > 0`이면 참조할
  Sample이 있어야 하므로 `sampleCount > 0`이 필요하다.

## 비범위 (무엇을 의도적으로 만들지 않았는가)

이 PoC는 "시드 → 동일 데이터 → 불변식 만족 → 저장소 반영" 한 명제만 증명하면 되므로, 그
외의 모든 책임은 다른 저장소나 다른 관심사로 명시적으로 넘긴다 (`docs/PRD.md` §2):

- **영속 파일 포맷의 실제 I/O**(읽기/쓰기/원자적 교체/파일 잠금) — `DataPersistence` 저장소
  책임. 여기서는 데모용 인메모리 저장소만 구현해 `ISampleRepository`/`IOrderRepository`
  계약이 지켜지는지만 확인한다.
- **생산 계산 로직**(`ShortageQuantity`/`ActualProductionQuantity`/`TotalProductionTime`/
  `ProductionQueue`) — `ConsoleMVC` 책임. 이 값들은 CONTRACT 계약상 생성기가 다루는 필드가
  아니다.
- **상태 전이(T1~T6) 실행/검증** — 생성기는 전이를 수행하지 않고 최초 유효 상태 값만 부여한다.
- **재고 상태(Sufficient/Insufficient/Depleted) 판정** — `DataMonitor` 저장소 책임.
- **서로 다른 시드가 서로 다른 데이터를 생성한다는 보장(비충돌성)** — 명제는 "동일 시드 →
  동일 데이터"만 주장하며 그 역(비충돌)은 다루지 않는다.
- **콘솔 UI/메뉴, 대량 생성 성능 최적화, 동시성, 네트워크/분산 환경, 로깅/i18n/외부 설정,
  단위 테스트 프레임워크 도입** — 이 PoC의 증명 대상 명제와 무관하거나(OS-15 참고) 별도
  저장소/과제 범위이므로 최소 CLI 진입점 이상으로 다루지 않는다.
- **CONTRACT.md 자체의 수정/재해석** — 이 계약은 `SampleOrderSystem` 저장소에서만 개정되며,
  여기서는 읽기 전용 사본으로만 사용한다.

## 문서

- `docs/CONTRACT.md` — 데이터 모델/영속 포맷 계약(수정 금지, v2)
- `docs/PRD.md` — 범위/비범위, FR-01~FR-13
- `docs/DESIGN.md` — 시드 설계, 저장 경로 설계, 불변식 보장 전략, 생성 순서
- `CLAUDE.md` — 빌드/테스트, 계약 수정 금지, 커밋 규칙
