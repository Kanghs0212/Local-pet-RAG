#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <fstream>
#ifdef _WIN32
#include <windows.h>
#endif
#include "rapidcsv.h"
#include "llama.h"

using namespace std;

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

    string user_input;
    while (true) {
        cout << "🐶 증상을 말씀해주세요 (종료: quit): ";
        getline(cin, user_input);
        
        // 🌟 [안전장치] 윈도우 환경에서 엔터치면 같이 들어가는 보이지 않는 쓰레기값(\r) 완벽 제거
        if (!user_input.empty() && user_input.back() == '\r') {
            user_input.pop_back();
        }
        
        if (user_input == "quit") break;
        if (user_input.empty()) continue;

        vector<float> query_embd = get_ai_embedding(embd_ctx, embd_model, user_input);

        vector<pair<float, int>> search_results; 
        for (int i = 0; i < database.size(); i++) {
            float sim = cosine_similarity(query_embd, database[i].embedding);
            search_results.push_back({sim, i});
        }
        sort(search_results.rbegin(), search_results.rend());

        string context_str = "";
        cout << "\n[🔍 AI가 분석한 유사도 Top 2 문서]\n";
        for (int i = 0; i < 2 && i < search_results.size(); i++) {
            int idx = search_results[i].second;
            float sim_score = search_results[i].first;
            
            if(sim_score < 0.35f) continue; 
            
            context_str += "- " + database[idx].answer + "\n";
            cout << i+1 << ". (유사도 점수: " << sim_score << ") [" << database[idx].category << "]\n";
        }
        if (context_str.empty()) context_str = "관련된 수의학 지식이 없습니다. 일반적인 건강 조언을 해주세요.";

        string prompt = "<|im_start|>system\n당신은 대한민국 최고의 반려동물 전문 수의사입니다. 반드시 제공된 [참고 지식]만을 기반으로 답변해야 하며, 지식에 없는 내용은 지어내지 마세요. 질문과 관련 없는 증상(예: 기침 등)은 절대 언급하지 마세요. 답변은 3~4문장의 자연스러운 한국어 구어체로 작성하세요.<|im_end|>\n<|im_start|>user\n[참고 지식]\n" + context_str + "\n[현재 반려견의 증상]\n" + user_input + "\n\n보호자인 저에게 이 증상에 대한 원인과 대처법을 수의사로서 조언해 주세요.<|im_end|>\n<|im_start|>assistant\n";

        cout << "\n[🧠 대화형 AI의 종합 분석 및 답변 생성 중...]\n\n";

        vector<llama_token> tokens(prompt.length() + 100);
        int n_tokens = llama_tokenize(chat_vocab, prompt.c_str(), prompt.length(), tokens.data(), tokens.size(), true, true);
        if (n_tokens < 0) { 
            tokens.resize(-n_tokens);
            n_tokens = llama_tokenize(chat_vocab, prompt.c_str(), prompt.length(), tokens.data(), tokens.size(), true, true);
        }

        llama_memory_seq_rm(llama_get_memory(chat_ctx), -1, -1, -1);
        
        llama_batch batch = llama_batch_get_one(tokens.data(), n_tokens);
        if (llama_decode(chat_ctx, batch)) continue;

        llama_sampler* smpl = llama_sampler_chain_init(llama_sampler_chain_default_params());
        llama_sampler_chain_add(smpl, llama_sampler_init_greedy());

        for (int i = 0; i < 512; i++) {
            llama_token id = llama_sampler_sample(smpl, chat_ctx, -1);
            llama_sampler_accept(smpl, id);
            
            if (llama_vocab_is_eog(chat_vocab, id)) break;

            char buf[128];
            int n = llama_token_to_piece(chat_vocab, id, buf, sizeof(buf), 0, true);
            string piece(buf, n);
            cout << piece; 
            fflush(stdout);

            llama_batch b = llama_batch_get_one(&id, 1);
            llama_decode(chat_ctx, b);
        }
        llama_sampler_free(smpl);
        cout << "\n\n------------------------------------------------\n\n";
    }

    llama_free(embd_ctx);
    llama_model_free(embd_model);
    llama_free(chat_ctx);
    llama_model_free(chat_model);
    llama_backend_free();

    return 0;
}