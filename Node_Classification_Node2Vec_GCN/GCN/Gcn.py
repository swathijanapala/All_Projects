import torch
import torch.nn as nn
import torch.nn.functional as F
import torch.optim as optim
from sklearn.metrics import accuracy_score, precision_score, recall_score, f1_score, confusion_matrix, classification_report
from sklearn.preprocessing import LabelEncoder
import networkx as nx
import numpy as np

class GraphConvolution(nn.Module):
    def __init__(self, in_features, out_features):
        super(GraphConvolution, self).__init__()
        self.weight = nn.Parameter(torch.FloatTensor(in_features, out_features))
        self.bias = nn.Parameter(torch.FloatTensor(out_features))

        nn.init.xavier_uniform_(self.weight)
        nn.init.constant_(self.bias, 0.1)

    def forward(self, input, adj):
        support = torch.mm(input, self.weight)
        output = torch.spmm(adj, support)
        output = output + self.bias
        return output

class GCN(nn.Module):
    def __init__(self, input_dim, hidden_dim, output_dim, dropout):
        super(GCN, self).__init__()
        self.gc1 = GraphConvolution(input_dim, hidden_dim)
        self.gc2 = GraphConvolution(hidden_dim, output_dim)
        self.dropout = dropout

    def forward(self, x, adj):
        x = F.relu(self.gc1(x, adj))
        x = F.dropout(x, self.dropout, training=self.training)
        x = self.gc2(x, adj)
        return F.log_softmax(x, dim=1)

def load_data(train_cites_file="../cora_train.cites", test_cites_file="../cora_test.cites", content_file="../cora.content"):
    train_graph = nx.read_edgelist(train_cites_file, create_using=nx.Graph())
    test_graph = nx.read_edgelist(test_cites_file, create_using=nx.Graph())

    combined_graph = nx.compose(train_graph, test_graph)
    node_features = {}
    labels = {}
    with open(content_file, "r") as f:
        for line in f.readlines():
            data = line.strip().split()
            paper_id = data[0]
            node_attributes = [float(x) for x in data[1:-1]]
            class_label = data[-1]
            node_features[paper_id] = np.array(node_attributes)
            labels[paper_id] = class_label

    return combined_graph, node_features, labels

def normalize_adj(adjacency_matrix):
    adjacency_matrix=adjacency_matrix+torch.eye(adjacency_matrix.size(0))
    row_sum=adjacency_matrix.sum(1)
    inverseSqrt=torch.pow(row_sum,-0.5)
    inverseSqrt[torch.isinf(inverseSqrt)]=0
    matInvSqrt=torch.diag(inverseSqrt)
    return adjacency_matrix.mm(matInvSqrt).t().mm(matInvSqrt)


def preprocess_data(graph, node_features, labels):
    X=[]
    for node in graph.nodes():
        X.append(node_features[node])
    X=torch.FloatTensor(X)

    label_encoder=LabelEncoder()
    y=label_encoder.fit_transform(list(labels.values()))
    y=torch.LongTensor(y)

    adj=nx.adjacency_matrix(graph)
    adj=torch.FloatTensor(adj.todense())

    adj=normalize_adj(adj)

    return X,y,adj

if __name__ =="__main__":

    graph, node_features,labels=load_data()
    X,y,adj=preprocess_data(graph,node_features,labels)
    model = GCN(input_dim=X.shape[1],hidden_dim=16,output_dim=7,dropout=0.4)

    optimizer=optim.Adam(model.parameters(),lr=0.001)
    criterion=nn.CrossEntropyLoss()

    model.train()
    for epoch in range(10000):
        optimizer.zero_grad()
        output = model(X, adj)
        loss = criterion(output, y)
        loss.backward()
        optimizer.step()
        if epoch % 10 == 0:
            print(f'Epoch {epoch + 1}, Loss: {loss.item()}')

    model.eval()
    with torch.no_grad():
        output = model(X, adj)
        y_pred = output.argmax(dim=1).numpy()

        accuracy = accuracy_score(y.numpy(), y_pred)
        precision = precision_score(y.numpy(), y_pred, average='macro')
        recall = recall_score(y.numpy(), y_pred, average='macro')
        f1 = f1_score(y.numpy(), y_pred, average='macro')
        cm = confusion_matrix(y.numpy(), y_pred)
        report = classification_report(y.numpy(), y_pred)

        with open('gcn_metrics.txt','w') as file:
            file.write("Accuracy: ")
            file.write(str(accuracy)+'\n')
            file.write("Precision: "+str(precision)+'\n')
            file.write("Recall: "+str(recall)+'\n')
            file.write("Macro F1-score: "+str(f1)+'\n')
            file.write("Confusion Matrix:\n"+str(cm)+'\n')
            file.write("Classification Report:\n")
            file.write(report)