import torch
from transformers import AutoTokenizer, AutoModelForCausalLM
import sys
import threading

mutex = threading.Lock()
if(len(sys.argv)!=2):
    printf("Invalid arguments\n")
    exit(1)

model_name = "openai-community/gpt2-medium"  
tokenizer = AutoTokenizer.from_pretrained(model_name)
model = AutoModelForCausalLM.from_pretrained(model_name)

# Define the airline service prompt
airline_service_prompt = (
    "You are a Chatbot specialized in airline services. "
    "Please provide relevant responses to user queries about booking flights, managing reservations, "
    "and other airline-related inquiries. "
)


def generate_response(user_question):
    full_prompt = airline_service_prompt + "\nUser: " + user_question

    input_ids = tokenizer.encode(full_prompt, return_tensors="pt", max_length=512, truncation=True)
    output = model.generate(
        input_ids, 
        max_length=120,
        pad_token_id=tokenizer.eos_token_id, 
        no_repeat_ngram_size=2,
        temperature=0.8,          
        top_k=50,                 
        top_p=0.95,              
        do_sample=True            
    )
    generated_text = tokenizer.decode(output[0], skip_special_tokens=True)
    
    return generated_text


count_var=sys.argv[1]
with open("shared_memory.txt", "r") as file:
    lines = file.readlines()

with open("shared_memory.txt", "w") as file:
    for line in lines:
        parts=line.strip().split('|')
        if parts[0]==count_var:
            user_question = parts[2]
            response = generate_response(user_question)
            response=response.replace("\n"," ")
            line = f"{line.strip()}|{response}"
            mutex.acquire()
            file.write(line)
            mutex.release()
        

#print("User:", user_question)
#print("Chatbot:", response)
