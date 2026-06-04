cmake_minimum_required(VERSION 3.10)
project(PetRAG)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 한글 깨짐 방지 옵션
add_compile_options(-finput-charset=UTF-8 -fexec-charset=UTF-8)

# llama.cpp의 불필요한 옵션 생략 (텍스트 모델용 기본 환경)
set(LLAMA_BUILD_TESTS OFF CACHE BOOL "Disable tests" FORCE)
set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "Disable examples" FORCE)
set(LLAMA_BUILD_SERVER OFF CACHE BOOL "Disable server" FORCE)

# llama.cpp 폴더를 프로젝트에 포함
add_subdirectory(llama.cpp)

# 엔진 실행 파일 만들기
add_executable(pet_engine main.cpp)

# 기본 필수 헤더 경로만 남기기
target_include_directories(pet_engine PRIVATE 
    ${CMAKE_SOURCE_DIR}/llama.cpp/include 
    ${CMAKE_SOURCE_DIR}/llama.cpp/ggml/include
)

# 윈도우 환경일 경우 웹 서버 통신을 위한 소켓 라이브러리 연결
if (WIN32)
    target_link_libraries(pet_engine PRIVATE llama ggml ws2_32)
else()
    target_link_libraries(pet_engine PRIVATE llama ggml)
endif()