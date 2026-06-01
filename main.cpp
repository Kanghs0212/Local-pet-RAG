#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <deque>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include "rapidcsv.h"
#include "llama.h"

//멀티스레딩
#include <future>
#include <thread>
#include <chrono>

//웹 동시접속
#include <mutex>
#include "httplib.h"

using namespace std;
std::mutex ai_mutex;

struct PetData {
    string category;
    string question;
    string answer;
    vector<float> embedding; 
};

float cosine_similarity(const vector<float>& v1, const vector<float>& v2) {
    float dot_product = 0.0f, norm_v1 = 0.0f, norm_v2 = 0.0f;
    for (size_t i = 0; i < v1.size(); i++) {
        dot_product += v1[i] * v2[i];
        norm_v1 += v1[i] * v1[i];
        norm_v2 += v2[i] * v2[i];
    }
    if (norm_v1 == 0.0f || norm_v2 == 0.0f) return 0.0f;
    return dot_product / (sqrt(norm_v1) * sqrt(norm_v2));
}

vector<float> get_ai_embedding(llama_context* ctx, llama_model* model, const string& text) {
    const llama_vocab* vocab = llama_model_get_vocab(model);
    vector<llama_token> tokens(text.length() + 100);
    int n_tokens = llama_tokenize(vocab, text.c_str(), text.length(), tokens.data(), tokens.size(), true, true);
    if (n_tokens < 0) { 
        tokens.resize(-n_tokens);
        n_tokens = llama_tokenize(vocab, text.c_str(), text.length(), tokens.data(), tokens.size(), true, true);
    }

    llama_memory_seq_rm(llama_get_memory(ctx), -1, -1, -1);
    llama_batch batch = llama_batch_get_one(tokens.data(), n_tokens);
    if (llama_encode(ctx, batch)) return {}; 

    const float* embd = llama_get_embeddings_seq(ctx, 0); 
    if (embd == nullptr) embd = llama_get_embeddings(ctx); 
    if (embd == nullptr) return {};

    int n_embd = llama_model_n_embd(model);
    return vector<float>(embd, embd + n_embd);
}

string sanitize_text(string text) {
    replace(text.begin(), text.end(), '\n', ' ');
    replace(text.begin(), text.end(), '\r', ' ');
    replace(text.begin(), text.end(), '\"', '\'');
    return text;
}

string vector_to_string(const vector<float>& vec) {
    stringstream ss;
    for (size_t i = 0; i < vec.size(); i++) {
        ss << vec[i];
        if (i < vec.size() - 1) ss << "|";
    }
    return ss.str();
}

vector<float> string_to_vector(const string& str) {
    vector<float> vec;
    stringstream ss(str);
    string token;
    while (getline(ss, token, '|')) {
        token.erase(remove(token.begin(), token.end(), '\"'), token.end());
        token.erase(remove(token.begin(), token.end(), '\r'), token.end());
        token.erase(remove(token.begin(), token.end(), '\n'), token.end());
        if (!token.empty()) {
            try { vec.push_back(stof(token)); } catch (...) {}
        }
    }
    return vec;
}

void llama_log_callback(enum ggml_log_level level, const char * text, void * user_data) { }

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    // 🌟 [핵심 해결책] 모니터뿐만 아니라 키보드 입력도 UTF-8로 강제 설정!
    SetConsoleCP(CP_UTF8); 
#endif
    llama_log_set(llama_log_callback, nullptr);
    llama_backend_init();

    cout << "=================================================\n";
    cout << " [1/3] AI 임베딩 모델 (문맥 파악 뇌) 로딩 중...\n";
    cout << "=================================================\n";
    
    llama_model_params embd_mparams = llama_model_default_params();
    llama_model* embd_model = llama_model_load_from_file("embedding_model.gguf", embd_mparams);
    if (!embd_model) { cerr << "❌ embedding_model.gguf 파일이 없습니다.\n"; return 1; }

    llama_context_params embd_cparams = llama_context_default_params();
    embd_cparams.embeddings = true; 
    embd_cparams.n_ctx = 2048;
    embd_cparams.n_batch = 2048;
    embd_cparams.n_ubatch = 2048;
    llama_context* embd_ctx = llama_init_from_model(embd_model, embd_cparams);

    cout << "=================================================\n";
    cout << " [2/3] 수의학 데이터베이스 (19,000건) 로딩 중...\n";
    cout << "=================================================\n";
    vector<PetData> database;
    
    ifstream cache_file("vector_db.csv");
    if (cache_file.good()) {
        cout << " ✅ 이전 학습 결과(vector_db.csv)를 발견했습니다! 초고속 로딩을 시작합니다.\n";
        rapidcsv::Document doc("vector_db.csv", rapidcsv::LabelParams(0, -1));
        for (size_t i = 0; i < doc.GetRowCount(); i++) {
            try {
                PetData data;
                data.category = doc.GetCell<string>("Category", i);
                data.question = doc.GetCell<string>("Question", i);
                data.answer = doc.GetCell<string>("Answer", i);
                data.embedding = string_to_vector(doc.GetCell<string>("Embedding", i));
                database.push_back(data);
            } catch (...) {
                continue; 
            }
        }
        cout << " 🚀 " << database.size() << "개의 데이터와 벡터를 1초 만에 불러왔습니다!\n";
    } else {
        // 이미 파일이 있으니 이 부분은 실행되지 않습니다.
        cerr << " ❌ 캐시 파일이 없습니다. 문제가 발생했습니다.\n";
        return 1;
    }

    cout << "=================================================\n";
    cout << " [3/3] AI 대화 모델 (수의사 뇌) 로딩 중...\n";
    cout << "=================================================\n";
    llama_model_params chat_mparams = llama_model_default_params();
    llama_model* chat_model = llama_model_load_from_file("qwen_model.gguf", chat_mparams);
    if (!chat_model) { cerr << "❌ qwen_model.gguf 파일이 없습니다.\n"; return 1; }

    llama_context_params chat_cparams = llama_context_default_params();
    chat_cparams.n_ctx = 2048;
    llama_context* chat_ctx = llama_init_from_model(chat_model, chat_cparams);
    const llama_vocab* chat_vocab = llama_model_get_vocab(chat_model);

    cout << "\n✅ RAG 시스템 준비 완료! 오프라인 수의사 엔진 가동.\n\n";

    // 과거 대화 기억장치 (서버가 켜져 있는 동안 유지)
    deque<pair<string, string>> chat_history;
    int max_history = 2; 

    // 🌟 1. 웹 서버 객체 생성
    httplib::Server svr;

    // 🌟 2. "/ask" 주소로 GET 요청이 들어왔을 때 실행될 로직 정의
    svr.Get("/ask", [&](const httplib::Request& req, httplib::Response& res) {
        
        // 브라우저가 한글(UTF-8)을 제대로 인식하도록 헤더 설정
        res.set_header("Content-Type", "application/json; charset=utf-8");
        // 어떤 웹사이트(HTML)에서든 이 API를 호출할 수 있게 허락 CORS 허용
        res.set_header("Access-Control-Allow-Origin", "*");

        // URL에 질문(?q=증상)이 없는 경우 에러 반환
        if (!req.has_param("q")) {
            res.set_content("{\"error\": \"질문 파라미터 'q'가 없습니다.\"}", "application/json");
            return;
        }

        string user_input = req.get_param_value("q");
        cout << "\n[🌐 웹 요청 수신] 질문: " << user_input << "\n";

        // 🔒 다른 요청이 처리 중이면 대기 (Thread-Safe)
        std::lock_guard<std::mutex> lock(ai_mutex);

        // --- 여기서부터 기존의 AI 임베딩 & 검색 로직 재사용 ---
        vector<float> query_embd = get_ai_embedding(embd_ctx, embd_model, user_input);

        vector<pair<float, int>> search_results; 
        for (int i = 0; i < database.size(); i++) {
            float sim = cosine_similarity(query_embd, database[i].embedding);
            search_results.push_back({sim, i});
        }
        sort(search_results.rbegin(), search_results.rend());

        string context_str = "";
        for (int i = 0; i < 2 && i < search_results.size(); i++) {
            if(search_results[i].first < 0.35f) continue; 
            context_str += "- " + database[search_results[i].second].answer + "\n";
        }
        if (context_str.empty()) context_str = "관련된 수의학 지식이 없습니다. 일반적인 건강 조언을 해주세요.";

        // 멀티턴 프롬프트 생성
        string prompt = "<|im_start|>system\n당신은 대한민국 최고의 반려동물 전문 수의사입니다. 제공된 [참고 지식]을 최우선으로 하되, 이전 대화의 문맥을 기억하여 자연스럽게 대답하세요.<|im_end|>\n";
        for (const auto& chat : chat_history) {
            prompt += "<|im_start|>user\n" + chat.first + "<|im_end|>\n";
            prompt += "<|im_start|>assistant\n" + chat.second + "<|im_end|>\n";
        }
        prompt += "<|im_start|>user\n[참고 지식]\n" + context_str + "\n[현재 반려견의 증상/질문]\n" + user_input + "\n\n위 지식을 바탕으로 조언해 주세요.<|im_end|>\n<|im_start|>assistant\n";

        vector<llama_token> tokens(prompt.length() + 100);
        int n_tokens = llama_tokenize(chat_vocab, prompt.c_str(), prompt.length(), tokens.data(), tokens.size(), true, true);
        if (n_tokens < 0) { 
            tokens.resize(-n_tokens);
            n_tokens = llama_tokenize(chat_vocab, prompt.c_str(), prompt.length(), tokens.data(), tokens.size(), true, true);
        }

        llama_memory_seq_rm(llama_get_memory(chat_ctx), -1, -1, -1);
        llama_batch batch = llama_batch_get_one(tokens.data(), n_tokens);
        
        // (API 응답 속도를 위해 이번에는 비동기 애니메이션 제외, 바로 연산)
        if (llama_decode(chat_ctx, batch)) {
            res.set_content("{\"error\": \"AI 연산 실패\"}", "application/json");
            return;
        }

        llama_sampler* smpl = llama_sampler_chain_init(llama_sampler_chain_default_params());
        llama_sampler_chain_add(smpl, llama_sampler_init_penalties(64, 1.1f, 0.0f, 0.0f));
        llama_sampler_chain_add(smpl, llama_sampler_init_top_k(40));
        llama_sampler_chain_add(smpl, llama_sampler_init_top_p(0.9f, 1));
        llama_sampler_chain_add(smpl, llama_sampler_init_temp(0.4f)); 
        llama_sampler_chain_add(smpl, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));

        string full_ai_response = "";
        for (int i = 0; i < 512; i++) {
            llama_token id = llama_sampler_sample(smpl, chat_ctx, -1);
            llama_sampler_accept(smpl, id);
            if (llama_vocab_is_eog(chat_vocab, id)) break;

            char buf[128];
            int n = llama_token_to_piece(chat_vocab, id, buf, sizeof(buf), 0, true);
            full_ai_response += string(buf, n);

            llama_batch b = llama_batch_get_one(&id, 1);
            llama_decode(chat_ctx, b);
        }
        llama_sampler_free(smpl);

        // 과거 대화 저장
        chat_history.push_back({user_input, full_ai_response});
        if (chat_history.size() > max_history) chat_history.pop_front();

        // 🌟 3. JSON으로 응답 포장 시, 텍스트 안의 따옴표나 엔터가 JSON을 망치지 않게 간단히 전처리
        string safe_response = full_ai_response;
        // 쌍따옴표 이스케이프 (\")
        size_t pos = 0;
        while ((pos = safe_response.find("\"", pos)) != string::npos) {
             safe_response.replace(pos, 1, "\\\"");
             pos += 2;
        }
        // 엔터(\n) 이스케이프 (\\n)
        pos = 0;
        while ((pos = safe_response.find("\n", pos)) != string::npos) {
             safe_response.replace(pos, 1, "\\n");
             pos += 2;
        }

        // 브라우저로 JSON 응답 발사!
        string json_result = "{\"question\": \"" + user_input + "\", \"answer\": \"" + safe_response + "\"}";
        res.set_content(json_result, "application/json");

        cout << "  => [응답 완료] 전송 길이: " << safe_response.length() << "자\n";
    });

    // 🌟 4. 서버 무한 대기 (포트 8080)
    cout << "🌐 웹 서버가 포트 8080에서 접속을 기다리고 있습니다...\n";
    cout << "👉 브라우저를 열고 테스트해보세요: http://localhost:8080/ask?q=설사\n";
    svr.listen("0.0.0.0", 8080);

    llama_free(embd_ctx);
    llama_model_free(embd_model);
    llama_free(chat_ctx);
    llama_model_free(chat_model);
    llama_backend_free();

    return 0;
}