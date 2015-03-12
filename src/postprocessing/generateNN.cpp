#include "../utils/helper.h"
#include "kd_tree.h"
#include <queue>
using namespace std;

int dimension;
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
    dimension = size;
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
        vector<double> vec;
        vec.resize(size, 0);
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
            fromString(tokens[3], prob);
            
            if (length == 1) {
                unigrams[phrase] = 0;//prob;
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

vector<Point> project(const vector<Point> &a, const vector< vector<double> > &axis)
{
    vector<Point> ret;
    for (int i = 0; i < a.size(); ++ i) {
        vector<double> x;
        for (int j = 0; j < axis.size(); ++ j) {
            double dot = 0;
            myAssert(axis[j].size() == a[i].size(), "wrong dimensions!");
            for (int d = 0; d < axis[j].size(); ++ d) {
                dot += axis[j][d] * a[i][d];
            }
            x.push_back(dot);
        }
        ret.push_back(Point(a[i].name, x));
    }
    return ret;
}

void normalize(vector<Point> &a)
{
    for (int i = 0; i < a.size(); ++ i) {
        double norm = 0;
        for (int d = 0; d < a[i].size(); ++ d) {
            norm += sqr(a[i][d]);
        }
        norm = sqrt(norm);
        for (int d = 0; d < a[i].size(); ++ d) {
            a[i][d] /= norm;
        }
    }
}

int main(int argc, char *argv[])
{
    int D, K;
    if (argc != 7 || sscanf(argv[3], "%d", &D) != 1 || sscanf(argv[4], "%d", &K) != 1) {
        printf("[usage] <vector.bin> <length*.csv folder path> <D> <K> <unigram2phrase-nn> <word2word-nn>\n");
        return 0;
    }
    
    loadVector(argv[1]);
    loadPatterns(argv[2]);
    
    cerr << unigrams.size() << endl;
    cerr << phrases.size() << endl;
    
    vector< vector<double> > axis(D, vector<double>(dimension, 0));
    double sqrt3 = sqrt(3.0);
    for (int i = 0; i < D; ++ i) {
        for (int j = 0; j < dimension; ++ j) {
            axis[i][j] = sample_normal();
            /*double roll = next_double2();
            if (roll < 1.0 / 6) {
                axis[i][j] = sqrt3;
            } else if (roll < 1.0 / 3) {
                axis[i][j] = -sqrt3;
            }*/
        }
    }
    for (int i = 0; i < D; ++ i) {
        double sum = 0;
        for (int j = 0; j < dimension; ++ j) {
            sum += sqr(axis[i][j]);
        }
        sum = sqrt(sum);
        for (int j = 0; j < dimension; ++ j) {
            axis[i][j] /= sum;
        }
    }

    vector<Point> unigramPoints, phrasesPoints, wordsPoints;

    FOR (unigram, unigrams) {
        if (word2vec.count(unigram->first)) {
            Point cur(unigram->first, word2vec[unigram->first]);
            unigramPoints.push_back(cur);
            wordsPoints.push_back(cur);
        }
    }
    
    FOR (phrase, phrases) {
        if (word2vec.count(phrase->first)) {
            Point cur(phrase->first, word2vec[phrase->first]);
            phrasesPoints.push_back(cur);
            wordsPoints.push_back(cur);
        }
    }
    
    normalize(unigramPoints);
    normalize(phrasesPoints);
    normalize(wordsPoints);
    
    vector<Point> projUnigramPoints, projPhrasesPoints, projWordsPoints;
    projUnigramPoints = project(unigramPoints, axis);
    projPhrasesPoints = project(phrasesPoints, axis);
    projWordsPoints = project(wordsPoints, axis);
    
/*    normalize(projUnigramPoints);
    normalize(projPhrasesPoints);
    normalize(projWordsPoints);*/
    
    
    for (int i = 0; i < wordsPoints.size(); ++ i) {
        word2vec[wordsPoints[i].name] = wordsPoints[i].x;
    }
    
    cerr << "vectors projected" << endl;
    
    KDTree phraseTree(projPhrasesPoints);
    vector<vector<pair<string, double>>> u2p(unigramPoints.size(), vector<pair<string,double>>(K, make_pair("", 0.0)));
    #pragma omp parallel for schedule(dynamic, 1000)
    for (int i = 0; i < unigramPoints.size(); ++ i) {
//        if (i % 1000 == 0) {
//            cerr << i << " " << unigramPoints[i].name<< endl;
//        }
        vector<string> sim = phraseTree.query(projUnigramPoints[i], 100);
        
        vector< pair<double, string> > order;
        const vector<double> &u = unigramPoints[i].x;
        for (int j = 0; j < sim.size(); ++ j) {
            double dot = 0;
            const vector<double> &v = word2vec[sim[j]];
            myAssert(u.size() == v.size(), "dimension mismatch!");
            for (int d = 0; d < u.size(); ++ d) {
                dot += u[d] * v[d];
            }
            order.push_back(make_pair(dot, sim[j]));
        }
        sort(order.rbegin(), order.rend());
        if (order.size() > K) {
            order.resize(K);
        }
        
        myAssert(order.size() == K, "not enough neighbors!");
        
        double maxi = order[0].first;
        for (int j = 0; j < order.size(); ++ j) {
            u2p[i][j].first = order[j].second;
            u2p[i][j].second = order[j].first / maxi;
        }
    }
    cerr << "u2p nn done" << endl;
    
    FILE* out = tryOpen(argv[5], "w");
    for (int i = 0; i < unigramPoints.size(); ++ i) {
        fprintf(out, "%s", unigramPoints[i].name.c_str());
        for (int j = 0; j < K; ++ j) {
            fprintf(out, "\t%s\t%.10f", u2p[i][j].first.c_str(), u2p[i][j].second);
        }
        fprintf(out, "\n");
    }
    fclose(out);
    cerr << "u2p output done" << endl;
    
    KDTree wordTree(projWordsPoints);
    vector<vector<pair<string, double>>> w2w(wordsPoints.size(), vector<pair<string,double>>(K, make_pair("", 0.0)));
    #pragma omp parallel for schedule(dynamic, 1000)
    for (int i = 0; i < wordsPoints.size(); ++ i) {
        vector<string> sim = wordTree.query(projWordsPoints[i], 100);
        
        vector< pair<double, string> > order;
        const vector<double> &u = wordsPoints[i].x;
        for (int j = 0; j < sim.size(); ++ j) {
            double dot = 0;
            const vector<double> &v = word2vec[sim[j]];
            myAssert(u.size() == v.size(), "dimension mismatch!");
            for (int d = 0; d < u.size(); ++ d) {
                dot += u[d] * v[d];
            }
            order.push_back(make_pair(dot, sim[j]));
        }
        sort(order.rbegin(), order.rend());
        if (order.size() > K) {
            order.resize(K);
        }
        
        myAssert(order.size() == K, "not enough neighbors!");
        
        double maxi = order[0].first;
        for (int j = 0; j < order.size(); ++ j) {
            w2w[i][j].first = order[j].second;
            w2w[i][j].second = order[j].first / maxi;
        }
    }

    cerr << "w2w nn done" << endl;
    
    out = tryOpen(argv[6], "w");
    for (int i = 0; i < wordsPoints.size(); ++ i) {
        fprintf(out, "%s", wordsPoints[i].name.c_str());
        for (int j = 0; j < K; ++ j) {
            fprintf(out, "\t%s\t%.10f", w2w[i][j].first.c_str(), w2w[i][j].second);
        }
        fprintf(out, "\n");
    }
    fclose(out);
    cerr << "w2w output done" << endl;
}
