# CLAUDE.md — DummyDataGenerator

이 저장소는 독립적으로 평가되는 개인과제 PoC다. 증명 대상 명제와 범위는 `docs/PRD.md`, 데이터 계약은
`docs/CONTRACT.md`를 따른다.

## 빌드/테스트

```
msbuild DummyDataGenerator.vcxproj /p:Configuration=Debug /p:Platform=Win32
```

이 저장소에는 별도 테스트 프레임워크가 없다(PRD OS-15). 위 빌드 명령의 성공 + 실행 파일이 정상
종료(exit code 0)하는 것을 "테스트 통과"로 간주한다.

## 계약(CONTRACT.md) 수정 금지

- `docs/CONTRACT.md`는 다른 저장소(`SampleOrderSystem-SungkyungWOO-21021147`)에서 확정되어 배포된
  사본이다. 이 저장소에서 **절대 수정하지 않는다.**
- 계약을 고쳐야 할 이유를 발견하면(필드명/타입/제약이 실제 요구와 맞지 않는 등) 코드로 우회하거나
  계약을 임의로 재해석하지 말고, 즉시 작업을 멈추고 사용자에게 보고한다.
- CONTRACT.md 는 SampleOrderSystem 에서만 개정된다. 여기 것은 읽기 전용 사본이다.
- 계약 공백을 발견하면 코드로 우회하지 말고 BLOCKED 항목으로 보고하고 멈춘다.
- 작업 시작 시 CONTRACT.md 의 버전을 확인한다.

## 커밋 규칙

- [Conventional Commits](https://www.conventionalcommits.org/) 형식을 따른다: `feat:`, `fix:`,
  `docs:`, `refactor:`, `test:`, `chore:` 등.
- 1커밋 = 1논리적 변경. 서로 다른 목적의 변경을 한 커밋에 묶지 않는다.
- 커밋 메시지 본문 또는 제목에 관련 FR-ID를 명시한다 (예: `feat: implement seeded sample generation (FR-01, FR-03)`).

## 구현 시 반드시 지킬 것

- **PRD 비범위(Out of scope)에 있는 것을 구현하면 실패다.** 특히 상태 전이 실행, 생산 계산 로직
  (ShortageQuantity/ActualProductionQuantity/TotalProductionTime), 재고 상태 판정, 영속 파일
  포맷의 실제 I/O 구현은 이 저장소의 책임이 아니다 — CONTRACT.md에 선언된 인터페이스
  (`ISampleRepository`, `IOrderRepository`)만 사용한다.
- 새 기능을 시작하기 전 `docs/PRD.md`의 FR 목록과 범위/비범위를 먼저 확인한다.
