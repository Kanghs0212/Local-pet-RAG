import os
import json
import csv
import glob

# 1. 'data' 폴더 JSON 파일 검색 
json_files = glob.glob("data/**/*.json", recursive=True) 

# 2. 결과 CSV 파일 생성
with open("pet_data.csv", "w", encoding="utf-8-sig", newline="") as f:
    writer = csv.writer(f)
    
    # CSV 헤더
    writer.writerow(["Category", "Question", "Answer"])
    
    for file in json_files:
        try:
            with open(file, "r", encoding="utf-8") as jf:
                data = json.load(jf)
                
                # 데이터 추출
                category = f"{data['meta']['department']} - {data['meta']['disease']}"
                question = data['qa']['input'].replace("\n", " ") 
                answer = data['qa']['output'].replace("\n", " ")
                
                writer.writerow([category, question, answer])
                
        except Exception as e:
            print(f"오류 발생 ({file}): {e}")

print(f"총 {len(json_files)}개의 데이터가 pet_data.csv로 성공적으로 변환되었습니다.")