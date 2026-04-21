# 🐶 오프라인 AI 수의사 보조 엔진 (Local Pet-RAG)

## 📖 프로젝트 개요 (Overview)
본 프로젝트는 인터넷 연결이 차단된 환경에서도 **100% 로컬(On-Device)**로 동작하는 C++ 기반의 지능형 반려동물 의료 지식 검색 엔진입니다. 클라우드 API에 의존하지 않고, **C++ 로우레벨에서 RAG(검색 증강 생성) 파이프라인을 직접 구현**하여 데이터 보안성과 초고속 응답성을 확보했습니다.

단순한 단답형 검색의 한계를 넘어, 사용자의 이전 질문과 답변의 문맥을 기억하는 **'멀티턴(Multi-turn) 대화 기능'**을 통해 사용자와의 연속적인 상호작용이 가능한 지능형 수의사 보조 시스템을 지향합니다.

</br>

## 🛠️ 기술 스택 (Tech Stack)
- **Core Language:** C++17 (System Optimization)
- **AI Inference:** llama.cpp (GGUF Quantization)
- **Models:** Qwen2.5-3B-Instruct (LLM), BGE-M3 (Embedding)
- **Data Engineering:** RapidCSV, Custom Vector DB Caching Logic
- **Memory Management:** std::deque (Sliding Window), Context Re-initialization
- **Build System:** CMake, MinGW-w64 (UCRT64)

</br>

## 🚀 구현 로드맵 및 상세 성과

### ✅ 1~3단계: 기초 인프라 및 LLM 통합
- **데이터 전처리:** AI 허브의 '반려견 질병 관련 Q&A 말뭉치' 19,000건을 확보하고, Python 스크립트를 통해 C++ 엔진이 고속으로 처리할 수 있는 CSV 형태로 정제하였습니다.
- **추론 엔진 구축:** `llama.cpp` 라이브러리를 로컬 환경에서 직접 빌드하고 프로젝트에 통합하여, 30억 파라미터급 LLM을 외부 서버 없이 CPU만으로 구동하는 데 성공하였습니다.

### ✅ 4단계: 진짜 '의미 기반' 검색(Semantic Search) 및 벡터 캐싱
- **의미론적 검색:** 단순 키워드 매칭의 한계를 극복하기 위해 `BGE-M3` 임베딩 모델을 도입했습니다. 사용자의 질문을 1,024차원의 고차원 벡터로 변환하고 **코사인 유사도(Cosine Similarity)**를 계산하여 질문의 의도를 정확히 파악합니다.
- **대용량 벡터 DB 캐싱:** 19,000건의 데이터를 매번 실시간 임베딩하는 부하(CPU 기준 약 3시간 소요)를 해결하기 위해, 최초 1회 연산 후 결과를 `vector_db.csv`에 저장하는 캐싱 아키텍처를 설계했습니다. 이 결과, 이후 실행 시 **로딩 시간을 1초 미만**으로 단축하는 성능 최적화를 달성했습니다.

### ✅ 5단계: 멀티턴 기억력 구현 (Multi-turn Context Window)
- **Sliding Window 전략:** 대화가 길어질수록 발생하는 토큰 한도 초과 및 연산량 증가를 막기 위해 `std::deque` 자료구조를 활용했습니다. 최신 2~3개의 대화 쌍만 유지하고 오래된 기록은 자동으로 폐기하는 슬라이딩 윈도우 방식을 적용했습니다.
- **Prompt Injection:** 채팅 모델의 특수 토큰 구조(ChatML)에 맞춰 과거 대화 기록을 시스템 프롬프트와 현재 질문 사이에 동적으로 삽입하여 AI가 대화의 맥락(Context)을 완벽히 인지하도록 설계했습니다.

</br>

## 🔍 핵심 트러블슈팅 (Technical Challenges & Solutions)

### 1. Windows I/O 한글 인코딩 및 입력 노이즈 해결
- **문제:** `SetConsoleOutputCP(CP_UTF8)` 설정 후에도 사용자 입력값(`cin`)이 깨져 들어오면서, AI가 질문의 의도를 파악하지 못하고 "기침" 등 엉뚱한 문서만 검색하는 현상 발생.
- **원인:** Windows 터미널의 입력 스트림 인코딩(CP949)과 AI 모델의 인코딩(UTF-8) 불일치.
- **해결:** `SetConsoleCP(CP_UTF8)`를 추가 적용하여 입력 스트림을 동기화하고, Windows 엔터키 입력 시 섞여 들어오는 캐리지 리턴(`\r`) 문자를 `pop_back()` 로직으로 제거하여 인코딩 노이즈를 완벽히 차단했습니다.

### 2. 샘플러 체인 구조 설계 및 런타임 에러 방지
- **문제:** `GGML_ASSERT(cur_p.selected >= 0)` 에러와 함께 프로그램이 강제 종료됨.
- **원인:** 확률 필터링 규칙(Top-K, Top-P)만 설정하고, 실제로 단어를 최종 선택하는 'Distribution Sampler'가 누락되어 발생한 인덱스 에러.
- **해결:** 샘플러 체인 맨 끝에 `llama_sampler_init_dist`를 추가하여 확률 분포에 따른 최종 단어 선택권을 부여함으로써 시스템 안정성을 확보했습니다.

### 3. 고장난 라디오 현상(Repetition Loop) 및 앵무새 증후군 치료
- **문제:** 소형 모델(3B)의 특성상 특정 문장이나 단어를 무한 반복하며 대화가 루프에 빠지는 현상 발생.
- **해결:** `Repetition Penalty` 샘플러를 도입하여 최근 사용된 토큰에 페널티를 부여하고, 온도(Temperature) 값을 `0.4`로 미세 조정하여 일관성과 창의성 사이의 최적 균형점을 찾았습니다.

### 4. 데이터 무결성 확보 및 RAG 할루시네이션 제어
- **문제:** CSV 파싱 중 개행 문자(`\n`)로 인한 `out_of_range` 에러 발생 및 검색된 지식을 사용자의 실제 상황으로 착각하여 조언하는 현상.
- **해결:** 저장 전 텍스트 소독 함수(`sanitize_text`)를 통해 데이터 무결성을 확보하고, 시스템 프롬프트에 "사용자가 직접 언급하지 않은 사실을 지어내지 말 것"이라는 강력한 제약 조건을 명시하여 지식 검색 데이터와 유저 데이터를 엄격히 분리했습니다.

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

### 📝 향후 발전 계획 (Roadmap)
- **Stage 6:** std::async 및 멀티스레딩을 활용하여 AI 답변 생성 중 UI가 멈추지 않는 비동기 스트리밍 구현.

- **Stage 7:** Qt 프레임워크를 활용한 데스크톱 전용 GUI 개발 및 실시간 의료 차트 생성 기능 추가.