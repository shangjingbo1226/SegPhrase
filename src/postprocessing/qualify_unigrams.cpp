#include "../utils/helper.h"
#include <queue>
using namespace std;

const int K = 3;
const long long max_size = 2000;         // max length of strings
const long long N = 40;                  // number of closest words that will be shown
const long long max_w = 50;              // max length of vocabulary entries

unordered_map<string, vector<double>> word2vec;

void loadVector(string filename)
{
    FILE *f;
    char st1[max_size];
    char file_name[max_size], st[100][max_size];
    float dist, len, bestd[N], vec[max_size];
    long long words, size, a, b, c, d, cn, bi[100];
    char ch;
    float *M;
    char *vocab;
    f = tryOpen(filename, "rb");
    if (f == NULL) {
        printf("Input file not found\n");
        return;
    }
    fscanf(f, "%lld", &words);
    fscanf(f, "%lld", &size);
    vocab = (char *)malloc((long long)words * max_w * sizeof(char));
    M = (float *)malloc((long long)words * (long long)size * sizeof(float));
    if (M == NULL) {
        printf("Cannot allocate memory: %lld MB    %lld  %lld\n", (long long)words * size * sizeof(float) / 1048576, words, size);
        return;
    }
    
    for (b = 0; b < words; b++) {
        a = 0;
        while (1) {
            vocab[b * max_w + a] = fgetc(f);
            if (feof(f) || (vocab[b * max_w + a] == ' ')) break;
            if ((a < max_w) && (vocab[b * max_w + a] != '\n')) a++;
        }
        vocab[b * max_w + a] = 0;
        for (a = 0; a < size; a++) fread(&M[a + b * size], sizeof(float), 1, f);
        string word = &vocab[b * max_w];
        double norm = 0;
        vector<double> vec;
        vec.resize(size, 0);
        for (a = 0; a < size; a ++) {
            vec[a] = M[a + b * size];
            norm += vec[a] * vec[a];
        }
        norm = sqrt(norm);
        for (size_t d = 0; d < vec.size(); ++ d) {
            vec[d] /= norm;
        }
        word2vec[word] = vec;
    }
    fclose(f);
}

unordered_map<string, double> unigrams, phrases, finalScoreMapping;

void loadPatterns(string folder)
{
    const int maxLen = 6;
    for (int length = 1; length <= maxLen; ++ length) {
        ostringstream filename;
        filename << "length" << length << ".csv";
        
        FILE* in = tryOpen(folder + "/" + filename.str(), "r");
        if (in == NULL) {
            continue;
        }
        while (getLine(in)) {
            vector<string> tokens = splitBy(line, ',');
            string phrase = tokens[0];
            double prob;
            fromString(tokens[3], prob);
            
            if (length == 1) {
                unigrams[phrase] = 0;//prob;
                finalScoreMapping[phrase] = 0;
            } else {
                for (size_t i = 0; i < phrase.size(); ++ i) {
                    if (phrase[i] == ' ') {
                        phrase[i] = '_';
                    }
                }
                phrases[phrase] = prob;
                finalScoreMapping[phrase] = prob;
            }
        }
        fclose(in);
    }
}

void loadNN(string filename, unordered_map<string, vector<pair<string, double>>> &neighbors)
{
    FILE* in = tryOpen(filename, "r");
    while (getLine(in)) {
        vector<string> tokens = splitBy(line, '\t');
        string w = tokens[0];
        for (int i = 1; i + 1 < tokens.size(); i += 2) {
            string neighbor = tokens[i];
            myAssert(finalScoreMapping.count(neighbor), "wrong neighbor!! " + neighbor + "\n" + line);
            double similarity;
            fromString(tokens[i + 1], similarity);
            neighbors[w].push_back(make_pair(neighbor, similarity));
        }
    }
    fclose(in);
}

unordered_set<string> stopwords;

void loadStopwords(string filename)
{
    FILE* in = tryOpen(filename, "r");
    for (;getLine(in);) {
        for (int i = 0; line[i]; ++ i) {
            line[i] = tolower(line[i]);
        }
        stopwords.insert(line);
    }
    fclose(in);
}

int main(int argc, char *argv[])
{
    double alpha = 0;
    int maxIter;
    if (argc != 9 || sscanf(argv[5], "%lf", &alpha) != 1 || alpha < 0 || alpha > 1 || sscanf(argv[7], "%d", &maxIter) != 1) {
        printf("[usage] <vector.bin> <length*.csv folder path> <u2p-nn> <w2w-nn> <alpha: ratio for keep the previous value> <output: unified-rank> <max iterations> <stopwords.txt>\n");
        return 0;
    }
    
    loadStopwords(argv[8]);
    
    loadVector(argv[1]);
    loadPatterns(argv[2]);
    
    unordered_map<string, vector<pair<string, double>>> u2p, w2w;
    loadNN(argv[3], u2p);
    loadNN(argv[4], w2w);    
    
    vector<string> unigramList;
    vector<string> wordList;
    FOR (unigram, unigrams) {
        if (word2vec.count(unigram->first)) {
            unigramList.push_back(unigram->first);
            wordList.push_back(unigram->first);
        }
    }
    FOR (phrase, phrases) {
        if (word2vec.count(phrase->first)) {
            wordList.push_back(phrase->first);
        }
    }

    #pragma omp parallel for schedule(dynamic, 1000)
    for (int i = 0; i < unigramList.size(); ++ i) {
        const string &key = unigramList[i];
        if (stopwords.count(key)) {
            continue;
        }
        myAssert(u2p.count(key), "missing key in u2p " + key);
        const vector<pair<string, double>> &neighbors = u2p[key];
        double sum = 0;
        double sum_weight = 0;
        FOR (iter, neighbors) {
            double similarity = iter->second;
            string phrase = iter->first;
            double score = finalScoreMapping[phrase];

            sum_weight += similarity;
            sum += similarity * score;
        }
        sum_weight = 3;
        finalScoreMapping[key] = unigrams[key] = sum / sum_weight;
    }
    cerr << "unigram initialized" << endl;
    
    for (int iter = 0; iter < maxIter; ++ iter) {
        //cerr << "iter " << iter << endl;
        vector<double> newScores(wordList.size(), 0);
        #pragma omp parallel for schedule(dynamic, 1000)
        for (size_t i = 0; i < wordList.size(); ++ i) {
            const string &key = wordList[i];
            if (stopwords.count(key)) {
                continue;
            }
            myAssert(w2w.count(key), "missing key in w2w " + key);
            const vector<pair<string, double>> &neighbors = w2w[key];
            double sum = 0, sum_weight = 0;
            FOR (neighbor, neighbors) {
                const string &wj = neighbor->first;
                const double &similarity = neighbor->second;
                double score = finalScoreMapping[wj];
                sum_weight += similarity;
                sum += similarity * score;
            }
//            sum_weight = 3;
            newScores[i] = sum / sum_weight;
        }
        for (size_t i = 0; i < wordList.size(); ++ i) {
            finalScoreMapping[wordList[i]] = finalScoreMapping[wordList[i]] * alpha + newScores[i] * (1 - alpha);
        }
    }
    cerr << maxIter << " iterations done" << endl;
    
    FOR (phrase, phrases) {
        finalScoreMapping[phrase->first] = phrase->second;
    }
    
    vector<pair<double, string>> order;
    FOR (w, finalScoreMapping) {
        order.push_back(make_pair(w->second, w->first));
    }
    sort(order.rbegin(), order.rend());
    
    FILE* out = tryOpen(argv[6], "w");
    FOR (unigram, order) {
        fprintf(out, "%s,%.10f\n", unigram->second.c_str(), unigram->first);
    }
    fclose(out);
    
    return 0;
}
