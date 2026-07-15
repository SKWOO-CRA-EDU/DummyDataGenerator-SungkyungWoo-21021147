#pragma once

namespace tests {

// docs/PRD.md FR-13(종단 간 재현성 및 무결성)을 검증하는 테스트 스위트를 실행한다.
// 각 테스트의 [PASS]/[FAIL] 결과를 stdout에 출력하고, 전부 통과했으면 true를 반환한다.
bool RunAllTests();

} // namespace tests
