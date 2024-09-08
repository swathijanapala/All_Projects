import networkx as nx
import numpy as np
from numpy.linalg import norm
from collections import Counter
from random import sample
import string
import nltk
import math
from nltk.corpus import stopwords
from nltk.stem import WordNetLemmatizer
lemmatizer = WordNetLemmatizer()
eng_stopwords = set(stopwords.words('english'))



def segmentation(filename):
	file = open(filename,'r')
	text = file.read()
	sentences = text.split('\n')
	file.close()
	return sentences[:-1]

def value(sentences_list,word):
	count=0
	length=len(sentences_list)
	for i in range(length):
		if word in sentences_list[i].split(' '):
			count=count+1
	if(count):
		return(math.log(length/count))
	else:
		return 0

def tf_idf(sentence_list,sentence,word):
	count=sentence.count(word)
	idf=value(sentence_list,word)
	return count*idf

def tf_id_valueof_each_sentence(sentences_list):
	weights=[]
	for sentence in sentences_list:
		s=sentence.split(' ')
		w=[]
		for j in s:
			w.append(tf_idf(sentences_list,s,j))
		weights.append(w)
	return weights
	

def matrix(sentences_list,unique_words):
	mat=[]
	for word in unique_words:
		value=[]
		for i in sentences_list:
			value.append(tf_idf(sentences_list,i.split(),word))	
		mat.append(value)
	return mat

def vector(mat,i):
	u=[]
	for k in range(len(mat)):
		u.append(mat[k][i])
	return u

def cosine_sim(u,v):
	A= np.array(u)
	B= np.array(v)
	cosine = np.dot(A,B)/(norm(A)*norm(B))
	return cosine

def unique(data):
	unique=[]
	for k in range(len(data)):
		for m in data[k].split():
			if m not in unique:
				unique.append(m)
	return unique

def sim(mat,i,j,sent_list):
	u=vector(mat,sent_list.index(i))
	v=vector(mat,sent_list.index(j))
	return u,v

def calc_pagerank(nodes,data_len,mat):
	g = nx.Graph()
	g.add_nodes_from(nodes)
	for i in range(data_len):
		u=vector(mat,i)
		for j in range(i+1,data_len):
			v=vector(mat,j)
			c=cosine_sim(u,v)
			g.add_edge(i,j,weight=c)
	pagerank=nx.pagerank(g)
	return(pagerank)

#MMR
def removing_redundancy(r_s,alpha,top,sentence_list,pagerank,mat):
	s=dict()
	first_ind=0
	s[first_ind]=r_s[first_ind]
	r_s.pop(first_ind)
	while(top): 
		new_score=-1000
		for k_r,v_r in r_s.items():
			maxi=0
			for k_s,v_s in s.items():
				u,v=sim(mat,v_r,v_s,sentence_list)
				similar=cosine_sim(u,v)
				if(maxi<similar):
					maxi=similar

			score=(alpha*(pagerank[k_r])-(1-alpha)*maxi)
			if(new_score<score):
				new_score=score
				key_val=k_r
		s[key_val]=r_s[key_val]
		r_s.pop(key_val)
		top=top-1
	return s
	


#Task1


def find_nearest_cluster(key,sent,centroids):
	u=vector(mat,key)
	max_sim=0
	for v in range(len(centroids)):
		similarity=cosine_sim(u,centroids[v])
		if similarity>max_sim:
			max_sim=similarity
			cluster_num=v
	return cluster_num

def find_new_centroid(matrix,length):
	new_centroid=[]
	for i in range(length):
		l=mat[i]
		n=[]
		for j in range(len(matrix)):
			n.append(l[matrix[j]])
		mean=np.mean(n)
		
		new_centroid.append(mean)
	return new_centroid

def cluster_finding(k_val,length,sentence_list,centroids,cluster):
	matrix=[]
	for i in range(k_val):
		matrix.append([])
	for k,v in sentence_list.items():
		cluster_num=find_nearest_cluster(k,v,centroids)
		cluster[cluster_num].append(v)
		matrix[cluster_num].append(k)
	
	for j in range(k_val):
		centroids[j]=find_new_centroid(matrix[j],length)
	return cluster,centroids,matrix

def match(old_clust,new_clust):
	for k,v in new_clust.items():
		if v not in old_clust.values():
			return False
	return True

def k_means_clustering(k,length,sentence_list):
	centroids=dict()
	numbers=[i for i in range(0,1000)]
	for i in range(k):
		centroids[i]=sample(numbers,length)

	cluster=dict()
	for j in range(k):
		cluster[j]=[]
	
	old_clust,old_cent,old_matrix=cluster_finding(k,length,sentence_list,centroids,cluster)
	while(True):
		new_clust,new_cent,new_matrix=cluster_finding(k,length,sentence_list,old_cent,old_clust)
		if(match(old_clust,new_clust)):
			break;
		else:
			old_clust,old_cent,old_matrix=new_clust,new_cent,new_matrix

	return old_clust,old_cent,old_matrix
		




#Task-2

def closest_to_centroid(centroids,matrix):
	ind=0
	max_sim=0
	for i in matrix:
		u=vector(mat,i)
		similarity=cosine_sim(u,centroids)
		if(similarity>max_sim):
			max_sim=similarity
			ind=i
	return ind

def finding_bigrams(sent):
	sent=sent.split()
	s=[]
	for i in range(len(sent)-1):
		s.append(""+sent[i]+" "+sent[i+1])
	return s

def s2_or_not(s1,s2):
	count=0
	for i in s2:
		if i in s1:
			count=count+1
		if count>=3:
			return 1
	return 0

def dfs(graph):
	l=list(nx.dfs_preorder_nodes(graph, source="start"))
	return l

def sentence_graph(s1_bigrams,s2):
	g = nx.DiGraph()
	nodes=["start"]
	for i in range(len(s1_bigrams)):
		nodes.append(s1_bigrams[i])
	nodes.append("end")
	g.add_nodes_from(nodes)
	for i in range(len(nodes)-1):
		g.add_edge(nodes[i],nodes[i+1],weight=1)

	nodes2=[]
	comman=[]
	for i in s2:
		if i in nodes:
			comman.append(i)
		else:
			nodes2.append(i)
	
	g.add_nodes_from(nodes2)
	g.add_edge(nodes[0],nodes2[0],weight=1)
	for i in range(len(nodes2)-1):
		if( (nodes2[i] not in comman) or (nodes2[i+1] not in comman) ):
			g.add_edge(nodes2[i],nodes2[i+1],weight=1)
	g.add_edge(nodes2[len(nodes2)-1],nodes[len(nodes)-1],weight=1)
	
	new_sent=dfs(g)
	return new_sent

def obtaining_new_sentences(k,clusters,centroids,sentence_list1,matrix):
	new_list=[]
	priority=[]
	for i in range(k):
		sentence_list=sentence_list1.copy()
		inter=closest_to_centroid(centroids[i],matrix[i])
		priority.append(inter)
		s1=sentence_list[inter]
		del sentence_list[inter]
		s1_bigrams=finding_bigrams(s1)
		s2_bigrams=[]
		for k,v in sentence_list.items():
			s2=finding_bigrams(v)
			if(s2_or_not(s1_bigrams,s2)):
				s2_bigrams=s2
				break;
		
		if(len(s2_bigrams)==0):
			new_list.append(s1_bigrams)
		else:
			print("entered")
			new_sent=sentence_graph(s1_bigrams,s2_bigrams)
			new_list.append(new_sent)
			
	return new_list,priority






sentences_list=segmentation("input.txt")

data_len=len(sentences_list)
unique_words=unique(sentences_list)
length=len(unique_words)

mat=matrix(sentences_list,unique_words)

nodes=[i for i in range(data_len)]
d=dict(zip(nodes,sentences_list))

pagerank=calc_pagerank(nodes,data_len,mat)
top=int(input("Enter top value:"))
print(removing_redundancy(d,0.6,top,sentences_list,pagerank,mat))


k=int(input("Enter num of clusters: "))
clusters,centroids,indexes=k_means_clustering(k,length,d)


sent,priority_list=obtaining_new_sentences(k,clusters,centroids,d,indexes)
list1=[]

for cluster in sent:
	new=""
	for j in cluster:
		if j=="start":
			continue;
		else:
			new=new+(j.split())[0]+" "

	new=new+(j.split())[1]
	list1.append(new)


final=dict(zip(priority_list,list1))

myKeys = list(final.keys())
myKeys.sort()
final_summary = [final[i] for i in myKeys]

with open("Summary_SentenceGraph.txt","w") as file:
	file.write("SUMMARY:\n")
	file.write("\n")
	for i in range(k):
		file.write(final_summary[i])
		file.write("\n")

print(final_summary[:k])

from rouge_score import rouge_scorer

reference_summary = """
In the quiet town of Porbandar, on a warm October day in 1869, Mohandas Karamchand Gandhi was born into a modest family.
Little did the world know that this unassuming child would grow up to become a beacon of hope and a symbol of nonviolent resistance.
He advocated for civil disobedience and self-reliance, urging his fellow citizens to spin their own cloth and reject foreign-made goods.
At the shore, he defied British law by making salt from seawater,igniting a wave of civil disobedience across the nation.
His legacy reminds us that even in the face of seemingly insurmountable odds,change can be achieved through nonviolent means.
"""
generated_summary = final_summary[:k]  # from your code

# Initialize the ROUGE scorer
scorer = rouge_scorer.RougeScorer(['rouge1', 'rouge2', 'rougeL'], use_stemmer=True)

# Calculate ROUGE scores
scores = scorer.score(reference_summary, generated_summary)

# Display the ROUGE scores
print("ROUGE-1: ", scores['rouge1'])
print("ROUGE-2: ", scores['rouge2'])
print("ROUGE-L: ", scores['rougeL'])









