import numpy as np
import networkx as nx
from gensim.models import Word2Vec

class Node2Vec:
    def __init__(self,graph,dimensions=1024,walk_length=100,num_walks=50,window_size=20,workers=5):
        self.graph=graph
        self.dimensions=dimensions
        self.walk_length=walk_length
        self.num_walks=num_walks
        self.window_size=window_size
        self.workers=workers
        self.walks=None
        self.model=None

    def _simulate_walks(self):
        walks=[]
        nodes=list(self.graph.nodes())
        for i in range(self.num_walks):
            np.random.shuffle(nodes)
            for node in nodes:
                walks.append(self._walk(node))
        return walks

    def _walk(self,start_node,p=1.0,q=1.0):
        walk=[start_node]
        while len(walk)<self.walk_length:
            current_node=walk[-1]
            neighbors=list(self.graph.neighbors(current_node))
            if len(neighbors)>0:
                if len(walk)==1:
                    next_node=np.random.choice(neighbors)
                else:
                    probabilities=[1.0/p if n==walk[-2] else (1.0 if self.graph.has_edge(n,walk[-2]) else 1.0/q) for n in neighbors]
                    probabilities=np.array(probabilities)/sum(probabilities)
                    next_node=np.random.choice(neighbors,p=probabilities)
                walk.append(next_node)
            else:
                break
        return [str(node) for node in walk]

    def train(self):
        print("Simulating walks...")
        self.walks = self._simulate_walks()
        print("Training Word2Vec model...")
        self.model = Word2Vec(self.walks,vector_size=self.dimensions,window=self.window_size,
                              min_count=0,sg=1,workers=self.workers,epochs=70)

    def get_embeddings(self):
        embeddings = {}
        for node in self.graph.nodes():
            embeddings[node] = self.model.wv[str(node)]
        return embeddings

def load_cora_data(train_cites_file="../cora_train.cites", test_cites_file="../cora_test.cites"):
    train_graph = nx.read_edgelist(train_cites_file, create_using=nx.DiGraph())
    test_graph = nx.read_edgelist(test_cites_file, create_using=nx.DiGraph())
    
    combined_graph = nx.compose(train_graph, test_graph)

    with open("../cora.content", "r") as f:
        for line in f.readlines():
            data = line.strip().split()
            paper_id = data[0]
            node_attributes = [int(x) for x in data[1:-1]]
            class_label = data[-1]
            combined_graph.nodes[paper_id]['features'] = np.array(node_attributes)
            combined_graph.nodes[paper_id]['label'] = class_label
    
    return combined_graph

if __name__ == "__main__":
    graph = load_cora_data()
    nodes=graph.nodes
    #print(len(nodes))
    node2vec = Node2Vec(graph)
    node2vec.train()
    embeddings = node2vec.get_embeddings()
    #print(len(embeddings['13205']))
    np.savez_compressed("node_embeddings.npz", **embeddings)
