import numpy as np
from sklearn.linear_model import LogisticRegression
from sklearn.metrics import accuracy_score,confusion_matrix,precision_score, recall_score, f1_score,classification_report
from sklearn.preprocessing import LabelEncoder

def data_loading():
    with np.load("node_embeddings.npz") as data:
        embeddings = {key: value for key, value in data.items()}
    with open("../cora.content", "r") as f:
        node_labels=[]
        for line in f.readlines():
            data = line.strip().split()
            paper_id = data[0]
            class_label = data[-1]
            embeddings[paper_id] = (embeddings[paper_id], class_label)
            node_labels.append(class_label)

    label_encoder=LabelEncoder()
    labels_encoded=label_encoder.fit_transform(node_labels)

    label_mapping=dict(zip(node_labels,labels_encoded))

    for paper_id, (_, class_label) in embeddings.items():
        #print(label_mapping[class_label])
        embeddings[paper_id]=(embeddings[paper_id][0],label_mapping[class_label])

    return embeddings

def train_test_split(data, train_ratio=0.80):
    node_ids = list(data.keys())
    np.random.shuffle(node_ids)
    train_size = int(len(node_ids) * train_ratio)
    train_data = {node_id: data[node_id] for node_id in node_ids[:train_size]}
    test_data = {node_id: data[node_id] for node_id in node_ids[train_size:]}
    return train_data, test_data

if __name__=="__main__":

    data=data_loading()
    train_data,test_data=train_test_split(data)

    train_embeddings,y_train=zip(*[(emb, label) for emb, label in train_data.values()])
    test_embeddings,y_test_encoded=zip(*[(emb, label) for emb, label in test_data.values()])

    train_embeddings=np.array(train_embeddings)
    test_embeddings=np.array(test_embeddings)

    lr_model=LogisticRegression(C=10,max_iter=3000,solver="liblinear",penalty="l2")
    lr_model.fit(train_embeddings,y_train)

    y_pred_encoded=lr_model.predict(test_embeddings)

    
    accuracy = accuracy_score(y_test_encoded, y_pred_encoded)
    precision = precision_score(y_test_encoded, y_pred_encoded, average='macro')
    recall = recall_score(y_test_encoded, y_pred_encoded, average='macro')
    f1 = f1_score(y_test_encoded, y_pred_encoded, average='macro')
    cm = confusion_matrix(y_test_encoded, y_pred_encoded)

    report = classification_report(y_test_encoded, y_pred_encoded)
    with open('lr_metrics.txt', 'w') as file:
        file.write("Accuracy: ")
        file.write(str(accuracy)+'\n')
        file.write("Precision: "+str(precision)+'\n')
        file.write("Recall: "+str(recall)+'\n')
        file.write("Macro F1-score: "+str(f1)+'\n')
        file.write("Confusion Matrix:\n")
        file.write(str(cm))
        file.write("\nClassification Report:\n")
        file.write(report)

   
    