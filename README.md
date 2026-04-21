# 🐶 오프라인 AI 수의사 보조 엔진 (Local Pet-RAG)

## 📖 프로젝트 개요 (Overview)
본 프로젝트는 인터넷 연결이 차단된 환경에서도 **100% 로컬(On-Device)**로 동작하는 C++ 기반의 지능형 반려동물 의료 지식 검색 엔진입니다. 클라우드 API에 의존하지 않고, C++ 로우레벨에서 RAG(검색 증강 생성) 파이프라인을 직접 구현하여 데이터 보안성과 초고속 응답성을 확보했습니다.

</br>

🛠️ 기술 스택 (Tech Stack)
- **Core Language:** C++17 (System Optimization)
- **AI Inference:** llama.cpp (GGUF Quantization)
- **Models:** Qwen2.5-3B-Instruct (LLM), BGE-M3 (Embedding)
- **Data Engineering:** RapidCSV, Custom Vector DB Caching Logic
- **Build System:** CMake, MinGW-w64 (UCRT64)

</br>

## 🚀 구현 로드맵 및 상세 성과
### ✅ 1~3단계: 기초 인프라 및 LLM 통합 (완료)
- **데이터 정제:** 19,000건의 AI 허브 질병 Q&A 말뭉치를 C++ 고속 로드용 CSV로 전처리.

- **추론 엔진:** llama.cpp 라이브러리를 직접 빌드하여 30억 파라미터급 모델을 로컬 CPU 환경에서 구동 성공.

### ✅ 4단계: 진짜 '의미 기반' 검색 (Semantic Search) & 캐싱 (완료)
- **벡터 검색 엔진:** 단어 일치 여부만 따지는 키워드 검색을 넘어, BGE-M3 모델을 통해 문장의 의미(Context)를 1,024차원 벡터로 변환하여 코사인 유사도(Cosine Similarity) 계산.

- **벡터 데이터베이스 캐싱:** 19,000건의 데이터를 매번 임베딩하는 부하(약 3시간 소요)를 해결하기 위해, 최초 1회 연산 후 vector_db.csv에 벡터값을 영구 저장하는 캐싱 아키텍처 설계. (이후 실행 시 로딩 시간 1초 미만 달성)

### ✅ 5단계: 시스템 안정화 및 트러블슈팅 (완료)
- **메모리 최적화:** 최신 llama.cpp API의 급격한 변경(Memory 아키텍처 개편)에 대응하여 llama_memory_seq_rm 및 컨텍스트 리셋 로직을 적용, 이전 대화가 현재 답변에 간섭하는 할루시네이션(Hallucination) 박멸.

- **데이터 소독(Sanitization):** CSV 파싱 중 발생하는 out_of_range 에러를 방지하기 위해 텍스트 내 개행문자(\n) 및 특수기호를 제거하는 전처리 파이프라인 구축.

---

</br>

## 💻 빌드 및 실행 방법 (How to Build)
본 프로젝트는 CMake를 기반으로 빌드됩니다.

코드 출력
File created successfully.

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


## 🔍 핵심 트러블슈팅: Windows I/O 한글 인코딩 이슈
### [Problem]
SetConsoleOutputCP(CP_UTF8) 설정 후에도 사용자의 입력값(Input)이 깨지면서, AI가 질문의 의도를 파악하지 못하고 "기침" 등 엉뚱한 문서만 검색하는 현상 발생.

### [Solution]

- **Dual-Layer Encoding:** 출력(Output)뿐만 아니라 입력(Input) 스트림도 UTF-8로 정렬하기 위해 SetConsoleCP(CP_UTF8)를 추가 적용.

- **CRLF Stream Cleaning:** Windows 엔터키 입력 시 섞여 들어오는 캐리지 리턴(\r) 문자가 인코딩 노이즈로 작용하는 것을 확인하여, pop_back() 로직으로 버퍼를 물리적으로 소독함.

- **Shell Sync:** 실행 전 chcp 65001 명령어를 통해 터미널 환경을 엔진의 인코딩 규격과 완벽히 동기화하여 한글 처리 무결성 확보.

</br>

### 📝 향후 발전 계획 (Roadmap)
- **Stage 6:** std::async 및 멀티스레딩을 활용하여 AI 답변 생성 중 UI가 멈추지 않는 비동기 스트리밍 구현.

- **Stage 7:** Qt 프레임워크를 활용한 데스크톱 전용 GUI 개발 및 실시간 의료 차트 생성 기능 추가.