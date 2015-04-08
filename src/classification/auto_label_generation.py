import sys
from random import shuffle
from math import sqrt
from sklearn import cluster

if len(sys.argv) != 6:
    print '[usage] <knowledge base small> <knowledge base large> <feature table> <patterns> <generated label>'
    sys.exit(-1)

knowledge_base = sys.argv[1]
knowledge_base_large = sys.argv[2]
feature_table = sys.argv[3]
patterns = sys.argv[4]
generated_label = sys.argv[5]

def normalizeMatrix(matrix):
    for i in xrange(dimension):
        sum = 0
        sum2 = 0;
        for j in xrange(len(matrix)):
            sum += matrix[j][i]
            sum2 += matrix[j][i] * matrix[j][i]
        avg = sum / len(matrix)
        avg2 = sum2 / len(matrix)
        variance = avg2 - avg * avg
        stderror = sqrt(variance)
        for j in xrange(len(matrix)):
            matrix[j][i] = (matrix[j][i] - avg)
            if stderror > 1e-8:
                matrix[j][i] /= stderror
    return matrix

def normalize(word):
    word = word.lower()
    result = []
    for i in xrange(len(word)):
        if word[i].isalpha() or word[i] == '\'':
            result.append(word[i])
        else:
            result.append(' ')
    word = ''.join(result);
    return ' '.join(word.split())

groundtruth = {}
for line in open(knowledge_base, 'r'):
    word = line.strip()
    #word = normalize(word)
    groundtruth[word] = True
kb_phrases_all= set()
for line in open(knowledge_base_large, 'r'):
    word = line.strip()
    word = normalize(word)
    kb_phrases_all.add(word) 

patterns_support = list()
for line in open(patterns, 'r'):
    tokens = line.split(',')
    patterns_support.append((tokens[0].strip(), int(tokens[1])))
sorted_patterns = sorted(patterns_support, key=lambda tup: -tup[1])
patterns_candidates = set([tup[0] for tup in sorted_patterns[:len(sorted_patterns) / 2]])

# loading
dimension = 0
attributes = []
forbid = ['outsideSentence', 'log_occur_feature' , 'constant', 'frequency']
matrixWiki = []
phraseWiki = []
matrixOther = []
phraseOther = []
for line in open(feature_table, 'r'):
    tokens = line.split(',')
    if tokens[0] == 'pattern':
        attributes = tokens
        #print attributes
        continue
    coordinates = []
    for i in xrange(1, len(tokens)):
        if attributes[i] in forbid:
            continue
        coordinates.append(float(tokens[i]))
    dimension = len(coordinates)
    if tokens[0] in groundtruth and tokens[0] in patterns_candidates:
        matrixWiki.append(coordinates)
        phraseWiki.append(tokens[0])
    else:
        matrixOther.append(coordinates)
        phraseOther.append(tokens[0])

# normalization
matrixWiki = normalizeMatrix(matrixWiki)
matrixOther = normalizeMatrix(matrixOther)

# k-means
kmeans = cluster.MiniBatchKMeans(n_clusters = min(200, len(matrixWiki)), max_iter = 300, batch_size = 5000)
kmeans.fit(matrixWiki)
labelsWiki = kmeans.labels_
bins = []
for i in xrange(1000):
    bins.append([])

for i in xrange(len(labelsWiki)):
    bins[labelsWiki[i]].append(phraseWiki[i])

labels = []
for bin in bins:
    shuffle(bin)
    if len(bin) > 0:
        labels.append(bin[0] + '\t1\n')
npos = len(labels)
# k-means
kmeans = cluster.MiniBatchKMeans(n_clusters = min(npos * 2, len(matrixOther)), max_iter = 300, batch_size = 5000)
kmeans.fit(matrixOther)
labelsOther = kmeans.labels_
bins = []
for i in xrange(min(npos * 2, len(matrixOther))):
    bins.append([])

for i in xrange(len(labelsOther)):
    bins[labelsOther[i]].append(phraseOther[i])

for bin in bins:
    shuffle(bin)
    if len(bin) > 0:
        for i in xrange(len(bin)):
            if bin[i] not in kb_phrases_all:
                labels.append(bin[i] + '\t0\n')
                break
        
out = open(generated_label, 'w')
out.write(''.join(labels))
out.close()

print len(labels), 'generated,', npos, 'positive'

