# 🐶 오프라인 AI 수의사 보조 엔진 [포켓 수의사 (PocketVet)] (Local Pet-RAG)

## 📖 프로젝트 개요 (Overview)
본 프로젝트는 인터넷 연결이 차단된 환경에서도 100% 로컬(On-Device)로 동작하는 C++ 기반의 지능형 반려동물 의료 지식 검색 엔진입니다. 클라우드 API에 의존하지 않고, **C++ 로우레벨에서 RAG(검색 증강 생성) 파이프라인을 직접 구현**하여 데이터 보안성과 초고속 응답성을 확보했습니다.

최종적으로는 단순한 터미널 프로그램을 넘어, **C++ 경량 웹 서버를 내장한 RESTful API 백엔드**와 사용자 친화적인 **웹 채팅 UI(Frontend)**까지 혼자서 구축한 **완벽한 로컬 풀스택(Full-Stack) 서비스**로 발전시켰습니다.

## 🛠️ 기술 스택 (Tech Stack)
- **Core Language:** C++17 (System Optimization)
- **Frontend UI:** HTML5, CSS3, Vanilla JavaScript (Fetch API, Server-Sent Events)
- **Backend & Network:** cpp-httplib (REST API Server), Windows Socket (ws2_32)
- **AI Inference:** llama.cpp (GGUF Quantization, Vulkan GPU Acceleration)
- **Models:** Qwen2.5-3B-Instruct (LLM), BGE-M3 (Embedding)
- **Concurrency:** std::async, std::future, std::mutex (Thread-Safe API)
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

### ✅ 6단계: 멀티스레딩(Asynchronous) 도입
- **비동기 렌더링:** `std::async`를 도입하여 AI가 연산하는 동안 백그라운드 스레드와 메인 스레드를 분리, 시스템이 멈추는(Freezing) 현상을 방지했습니다.

### ✅ 7단계: C++ RESTful API 서버 및 웹 프론트엔드 구축
- **마이크로서비스 아키텍처:** `cpp-httplib`를 활용하여 C++ 엔진 자체를 HTTP 웹 서버(포트 8080)로 변신시키고, JSON 형태로 결과를 반환하는 API(`GET /ask`) 엔드포인트를 구축했습니다.
- **웹 UI 통합 연동:** 사용자가 브라우저에서 편리하게 접근할 수 있는 채팅 인터페이스(`index.html`)를 제작하고 비동기 Fetch API 및 CORS 설정을 통해 C++ 백엔드와 완벽하게 연동했습니다.
- **동시성 제어:** 여러 웹 요청이 동시에 들어올 때 AI 메모리가 충돌하지 않도록 `std::mutex`를 활용한 Thread-safe 로직을 적용했습니다.

### ✅ 8단계: 하드웨어 가속(Vulkan GPU) 전면 도입
- **연산 병목 해소:** MSYS2 환경에 Vulkan 라이브러리(`mingw-w64-vulkan-devel`)를 통합하고 CMake 빌드 옵션(`-DGGML_VULKAN=ON`)을 적용하여, 기존 CPU 전용 추론 대비 텍스트 생성 속도를 비약적으로 향상시켰습니다.

### ✅ 9단계: SSE(Server-Sent Events) 기반 실시간 스트리밍 UX 구현
- **체감 응답성 극대화:** AI의 모든 연산이 끝날 때까지 5~10초간 대기하던 기존 Blocking API 대신, 청크(Chunk) 단위로 생성되는 토큰을 즉시 브라우저로 쏘아주는 SSE 통신 프로토콜을 도입하여 실제 사람이 타자를 치는 듯한 실시간 UX를 구현했습니다.

## 🔍 핵심 트러블슈팅 (Technical Challenges)

### 1. Windows I/O 한글 인코딩 및 입력 노이즈 박멸
- **현상:** UTF-8 설정 후에도 사용자 입력이 깨져 AI가 엉뚱한 답변을 생성함.
- **해결:** `SetConsoleCP(CP_UTF8)`를 적용하여 키보드 입력을 동기화하고, Windows 엔터키의 잔재인 `\r` 문자를 물리적으로 제거하여 데이터 무결성을 확보했습니다.

### 2. 샘플러 체인 설계 및 런타임 Assert 에러
- **현상:** `GGML_ASSERT(cur_p.selected >= 0)` 에러와 함께 프로그램 강제 종료.
- **해결:** 확률 필터링만 지정하고 실제 단어 선택기(Distribution Sampler)가 누락된 원인을 파악하여, `llama_sampler_init_dist`를 체인에 명시 추가함으로써 시스템을 안정화했습니다.

### 3. Windows Network Socket 링커 에러 및 헤더 충돌
- **현상:** 웹 서버 기능 빌드 중 `undefined reference to '__imp_WSAStartup'` 링커 에러 및 `winsock2.h` 포함 경고 발생.
- **해결:** `CMakeLists.txt`에 `target_link_libraries(pet_engine PRIVATE ws2_32)`를 추가하여 OS 소켓 통신 모듈을 명시적으로 연결했습니다. 또한 코드 상단에 `#define WIN32_LEAN_AND_MEAN` 매크로를 선언하여 구버전 Windows 헤더의 불필요한 네트워크 모듈 간섭을 원천 차단했습니다.

### 4. CSV 데이터 파괴 주범 '개행 문자' 소독 및 JSON 이스케이프
- **현상:** 원본 데이터 내의 `\n` 및 `"` 문자가 CSV 열 구조를 파괴하거나, 웹 서버의 JSON 응답 포맷을 망가뜨림.
- **해결:** 텍스트 소독 함수를 구현하여 초기 로딩 시 데이터 무결성을 확보하고, 웹 API 응답 전송 전 C++ 내부에서 이스케이프 문자(`\"`, `\\n`) 치환 작업을 수행하여 안전한 JSON 페이로드를 구성했습니다.

### 5. 스트리밍 환경의 한글 바이트 쪼개짐(UTF-8 Chunking) 에러 해결
- **현상:** 실시간 SSE 스트리밍 중, 3바이트로 구성된 한글이 1바이트씩 쪼개져 전송되면서 브라우저 화면에 마름모 물음표()로 깨지거나 글자가 기하급수적으로 중복 출력되는 현상 발생.
- **해결:** 프론트엔드에서 파편화된 조각을 이어 붙이는(`+=`) 방식 대신, C++ 백엔드에서 생성된 '누적 전체 텍스트'를 매번 전송하고 프론트엔드가 이를 화면에 덮어씌우는(`=`) 렌더링 방식으로 전환하여 멀티바이트 언어의 스트리밍 한계를 극복했습니다.

---

## 🔮 향후 발전 계획 (Future Roadmap)

- **Vision AI (멀티모달) 통합:** LLaVA 또는 Qwen-VL 모델을 도입하여, 보호자가 텍스트뿐만 아니라 "강아지 피부 상태 사진"을 업로드하면 이미지를 분석하여 RAG 지식과 결합한 진단을 내리는 시각 보조 기능을 추가할 예정입니다.
- **개인화 프로필 DB(SQLite) 구축:** C++ 내장 SQLite 데이터베이스를 연동하여 반려견의 기본 정보(이름, 나이, 견종, 기저질환 등)를 영구 저장하고, 이를 시스템 프롬프트에 자동 주입하는 'User Session RAG'를 통해 더욱 정교한 맞춤형 의료 조언을 제공할 계획입니다.

---

<br>

## 💻 빌드 및 실행 방법 (How to Build & Run)
본 프로젝트는 CMake를 기반으로 빌드됩니다.

```bash
# 1. 빌드 디렉토리 생성 및 빌드 수행 (Vulkan GPU 가속 옵션 활성화)
mkdir build
cd build
cmake .. -G "MinGW Makefiles" -DGGML_VULKAN=ON
cmake --build .

# 2. C++ 백엔드 웹 서버 실행
cd ..
./build/pet_engine.exe
# (터미널에 "🌐 웹 서버가 포트 8080에서 접속을 기다리고 있습니다..." 출력 확인)
```

프론트엔드 UI 접속 방법:
서버가 켜져 있는 상태에서 프로젝트 폴더 내의 index.html 파일을 크롬 등 웹 브라우저로 열면 직관적인 챗봇 UI를 통해 수의사 엔진을 바로 이용하실 수 있습니다. (또는 브라우저 주소창에 http://localhost:8080/ask?q=질문내용 으로 직접 API 호출도 가능합니다.)

본 프로젝트는 C++ 로우레벨 시스템 제어, 네트워크 서버 구축, 프론트엔드 연동, 최신 LLM 추론 기술을 결합하여 고성능 로컬 AI 서비스의 상용화 단계까지 도달한 기술적 성취를 증명합니다.
