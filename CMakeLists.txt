cmake_minimum_required(VERSION 3.10)
project(PetRAG)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 한글 깨짐 방지 옵션
add_compile_options(-finput-charset=UTF-8 -fexec-charset=UTF-8)

# llama.cpp의 불필요한 테스트/예제 프로그램 조립을 생략
set(LLAMA_BUILD_TESTS OFF CACHE BOOL "Disable tests" FORCE)
set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "Disable examples" FORCE)
set(LLAMA_BUILD_SERVER OFF CACHE BOOL "Disable server" FORCE)

# llama.cpp 폴더를 프로젝트에 포함
add_subdirectory(llama.cpp)

# 엔진 실행 파일 만들기
add_executable(pet_engine main.cpp)

# common을 빼고, llama 라이브러리만 정확하게 연결
target_link_libraries(pet_engine llama)