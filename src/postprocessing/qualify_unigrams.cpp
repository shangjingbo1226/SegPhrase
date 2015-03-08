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
        len = 0;
        for (a = 0; a < size; a++) len += M[a + b * size] * M[a + b * size];
        len = sqrt(len);
        for (a = 0; a < size; a++) M[a + b * size] /= len;
        
        string word = &vocab[b * max_w];
        vector<double> vec(size, 0);
        for (a = 0; a < size; a ++) {
            vec[a] = M[a + b * size];
        }
        word2vec[word] = vec;
    }
    fclose(f);
}

unordered_map<string, double> unigrams, phrases;

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
            fromString(tokens[2], prob);
            
            if (length == 1) {
                unigrams[phrase] = prob;
            } else {
                for (size_t i = 0; i < phrase.size(); ++ i) {
                    if (phrase[i] == ' ') {
                        phrase[i] = '_';
                    }
                }
                phrases[phrase] = prob;
            }
        }
        fclose(in);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 4) {
        printf("[usage] <vector.bin> <length*.csv folder path> <output: unigram-rank>\n");
        return 0;
    }
    
    loadVector(argv[1]);
    loadPatterns(argv[2]);
    
    cerr << unigrams.size() << endl;
    cerr << phrases.size() << endl;
    
    FOR (unigram, unigrams) {
        const string &key = unigram->first;
        if (!word2vec.count(key)) {
            unigram->second = 0;
            continue;
        }
        //cerr << key << endl;
        priority_queue < pair<double, double> > heap;
        const vector<double> &v = word2vec[key];
        double maxi = 0;
        FOR (phrase, phrases) {
            const string &p = phrase->first;
            if (word2vec.count(p)) {
                //cerr << key << " v.s. " << p << endl;
                const vector<double> &vp = word2vec[p];
                double dot = 0;
                for (size_t i = 0; i < vp.size(); ++ i) {
                    dot += vp[i] * v[i];
                }
                maxi = max(maxi, dot);
                heap.push(make_pair(-dot, -phrase->second));
                if (heap.size() > K) {
                    heap.pop();
                }
            }
        }
        double sum = 0;
        double sum_weight = 0;
        while (heap.size() > 0) {
            double similarity = -heap.top().first;
            double score = -heap.top().second;
            
            similarity /= maxi;
            sum_weight += similarity;
            sum = similarity * score;
            
            heap.pop();
        }
        unigram->second = sum / sum_weight;
        //cerr << key << " " << sum / sum_weight;
    }

    vector<pair<double, string>> order;
    FOR (unigram, unigrams) {
        order.push_back(make_pair(unigram->second, unigram->first));
    }
    sort(order.rbegin(), order.rend());
    
    FILE* out = tryOpen(argv[3], "w");
    FOR (unigram, order) {
        fprintf(out, "%s,%.10f\n", unigram->second.c_str(), unigram->first);
    }
    fclose(out);
}
