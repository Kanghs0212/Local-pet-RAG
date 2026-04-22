# 🐶 오프라인 AI 수의사 보조 엔진 (Local Pet-RAG)

## 📖 프로젝트 개요 (Overview)
본 프로젝트는 인터넷 연결이 차단된 환경에서도 **100% 로컬(On-Device)**로 동작하는 C++ 기반의 지능형 반려동물 의료 지식 검색 엔진입니다. 클라우드 API에 의존하지 않고, **C++ 로우레벨에서 RAG(검색 증강 생성) 파이프라인을 직접 구현**하여 데이터 보안성과 초고속 응답성을 확보했습니다.

단순한 검색기를 넘어, **비동기 멀티스레딩** 기반의 쾌적한 UX와 과거 대화 맥락을 기억하는 **멀티턴(Multi-turn)** 기능을 통해 실제 수의사와 상담하는 듯한 유기적인 대화 경험을 제공합니다.

## 🛠️ 기술 스택 (Tech Stack)
- **Core Language:** C++17 (System Optimization)
- **AI Inference:** llama.cpp (GGUF Quantization)
- **Models:** Qwen2.5-3B-Instruct (LLM), BGE-M3 (Embedding)
- **Concurrency:** std::async, std::future (Asynchronous Prompt Processing)
- **Data Engineering:** RapidCSV, Custom Vector DB Caching (19,000+ items)
- **Memory Management:** std::deque (Sliding Window Context)
- **Build System:** CMake, MinGW-w64 (UCRT64)

## 🚀 구현 로드맵 및 핵심 성과

### ✅ 1~3단계: 로컬 추론 인프라 구축
- **데이터 정제:** 19,000건의 수의학 Q&A 말뭉치를 C++ 고속 로드용 CSV로 전처리했습니다.
- **추론 엔진 통합:** `llama.cpp`를 로컬에서 빌드하여 외부 서버 없는 3B급 LLM 구동 환경을 구축했습니다.

### ✅ 4단계: 의미 기반 검색(Semantic Search) 및 벡터 캐싱
- **벡터 유사도 검색:** `BGE-M3` 모델로 질문의 의도를 1,024차원 벡터로 변환, **코사인 유사도(Cosine Similarity)**를 통해 정밀한 지식 검색을 수행합니다.
- **초고속 캐싱 시스템:** 3시간이 소요되는 19,000건의 임베딩 연산을 최초 1회 후 파일(`vector_db.csv`)로 저장하는 로직을 구현하여, **재실행 시 로딩 시간을 1초 미만**으로 최적화했습니다.

### ✅ 5단계: 지능형 멀티턴(Multi-turn) 대화 로직
- **슬라이딩 윈도우(Sliding Window):** `std::deque`를 활용해 최근 대화 기록 2~3개만 유지하고 오래된 기억을 폐기하여 토큰 효율성과 문맥 유지 능력을 동시에 확보했습니다.
- **샘플링 최적화:** `Repetition Penalty`, `Top-P`, `Temperature` 샘플러를 직접 구성하여 AI의 문장 반복(앵무새 증후군) 현상을 완벽히 해결했습니다.

### ✅ 6단계: 비동기 멀티스레딩(Asynchronous) UX 개선
- **Non-blocking UI:** `std::async`를 도입하여 AI가 방대한 프롬프트를 분석하는 동안 메인 스레드가 얼어붙는(Freezing) 현상을 방지했습니다.
- **실시간 피드백:** 백그라운드 스레드에서 연산이 진행되는 동안 CLI 환경에서 실시간 **로딩 스피너(Spinner)**를 구현하여 사용자에게 시스템 진행 상태를 시각적으로 전달합니다.

## 🔍 핵심 트러블슈팅 (Technical Challenges)

### 1. Windows I/O 한글 인코딩 및 입력 노이즈 박멸
- **현상:** UTF-8 설정 후에도 사용자 입력이 깨져 AI가 "기침" 등 엉뚱한 답변을 생성함.
- **해결:** `SetConsoleCP(CP_UTF8)`를 적용하여 키보드 입력을 동기화하고, Windows 엔터키의 잔재인 `\r` 문자를 물리적으로 제거하여 데이터 무결성을 확보했습니다.

### 2. 샘플러 체인 설계 및 런타임 Assert 에러
- **현상:** `GGML_ASSERT(cur_p.selected >= 0)` 에러와 함께 프로그램이 강제 종료됨.
- **원인:** 확률 필터링(Top-K, Top-P) 규칙만 있고 실제 단어를 최종 선택하는 'Distribution Sampler'가 누락됨.
- **해결:** 체인 맨 끝에 `llama_sampler_init_dist`를 추가하여 확률 분포에 기반한 최종 토큰 선택권을 부여함으로써 시스템을 안정화했습니다.

### 3. CSV 데이터 파괴 주범 '개행 문자' 소독
- **현상:** 데이터 내의 `\n` 문자가 CSV 열 구조를 파괴하여 `out_of_range` 에러 유발.
- **해결:** 텍스트 소독 함수(`sanitize_text`)를 구현하여 모든 개행 문자를 공백으로 치환하고 따옴표를 이스케이프 처리하여 19,000건의 대용량 DB 신뢰성을 확보했습니다.

## 📝 향후 발전 계획
- **Stage 7:** Qt 프레임워크 기반의 GUI 도입 및 실시간 스트리밍 답변 시각화.
- **Stage 8:** C++ 경량 웹 서버(`cpp-httplib`) 통합을 통한 RESTful API 서버로의 확장.

---
*본 프로젝트는 C++ 로우레벨 시스템 제어와 최신 LLM 추론 기술을 결합하여 고성능 로컬 AI 서비스의 기술적 가능성을 증명한 결과물입니다.*
</br>

## 💻 빌드 및 실행 방법 (How to Build)
본 프로젝트는 CMake를 기반으로 빌드됩니다.

```bash
# 1. 빌드 디렉토리 생성 및 이동
mkdir build
cd build

# 2. CMake 구성 및 빌드 시작
cmake .. -G "MinGW Makefiles"
cmake --build .

# 3. 프로그램 실행
cd ..
./build/pet_engine.exe
```

</br>
