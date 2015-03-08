#include "../utils/helper.h"
#include <cassert>
#include <omp.h>

typedef unordered_map<string, double> MAP_S_D;
typedef unordered_map<string, string> MAP_S_S;

const double INF = 1e10;

double penalty;
string suffix;

double segmentationByDP(const string &pattern, MAP_S_D &prob)
{
    vector<string> tokens = splitBy(pattern, ' ');
    vector<double> f(tokens.size() + 1, 0);
    f[0] = 1;
    for (int i = 0; i < (int)tokens.size(); ++ i) {
        string phrase = "";
        for (int j = i; j < (int)tokens.size(); ++ j) {
            if (phrase.size()) {
                phrase += " ";
            }
            phrase += tokens[j];
            if (j - i + 1 == (int)tokens.size()) {
                continue;
            }
            if (prob.count(phrase)) {
                double cur = f[i] * prob[phrase] * penalty;
//                if (j + 1 != (int)tokens.size()) {
//                    cur *= penalty;
//                }
                if (cur > f[j + 1]) {
                    f[j + 1] = cur;
                }
            }
        }
    }
    return f[tokens.size()];
}

vector<string> printStat(MAP_S_S &patterns, MAP_S_D &num, MAP_S_D &den)
{
	string f1_name = "F1." + suffix;
	string f4_name = "F4." + suffix;
    vector<string> ret(1, f1_name + "," + f4_name);
    char temp[1000];
//    cerr << patterns.size() << " v.s. " << num.size() << endl;
    FOR (iter, patterns) {
        const string &pattern = iter->first;
        double best = segmentationByDP(pattern, den);
        double f1 = 0, f4 = 0;
        /*if (pattern == "support vector machine") {
            cerr << num[pattern] << endl;
            cerr << best << endl;
        }*/
        if (num.count(pattern)) {
            if (best > 1e-9) {
                f1 = num[pattern] / best;
                f4 = num[pattern] * log(f1);
            } else {
                f1 = f4 = INF;
            }
        } else {
            f1 = 0;
            f4 = 0;
        }
        sprintf(temp, "%.10f,%.10f", f1, f4);
/*        if (pattern == "support vector machine") {
            cerr << f1 << " " << f4 << endl;
            cerr << temp << endl;
        }*/
        ret.push_back(temp);
    }
    return ret;
}

MAP_S_D prob, oldProb;

void loadProb(const string &prefix)
{
    for (int length = 1; length <= 6; ++ length) {
        char filename[255];
        sprintf(filename, "%s%d.csv", prefix.c_str(), length);
        FILE* in = tryOpen(filename, "r");
        
        if (in == NULL) {
            cerr << "[Warning] No length " << length << " phrases." << endl;
            continue;
        }
        getLine(in);
        double sum = 0;
        for (;getLine(in);) {
            vector<string> tokens = splitBy(line, ',');
    		string pattern = tolower(tokens[0]);
    		double value;
    		fromString(tokens[2], value);
    		prob[pattern] = value;
    		sum += value;
        }
        //fprintf(stderr, "sum %d = %.6f\n", length, sum);
        fclose(in);
    }
    //cerr << "# prob = " << prob.size() << endl;
}

void loadPattern(const string &filename)
{
	FILE* in = tryOpen(filename, "r");
	for (;getLine(in);) {
		vector<string> tokens = splitBy(line, ',');
		string pattern = tolower(tokens[0]);
		int occurrence;
		fromString(tokens[1], occurrence);

		oldProb[pattern] = occurrence;
	}
	fclose(in);
//	cerr << "# old prob = " << oldProb.size() << endl;
}

MAP_S_S patterns;

vector<string> loadFeatureTable(const string &filename)
{
	FILE* in = tryOpen(filename, "r");
	vector<string> ret;
	getLine(in);
	ret.push_back(line);
	for (;getLine(in);) {
		vector<string> tokens = splitBy(line, ',');
		string pattern = tolower(tokens[0]);
		patterns[pattern] = line;
	}
	FOR (iter, patterns) {
        ret.push_back(iter->second);
	}
	fclose(in);
//	cerr << "# Pattern = " << patterns.size() << endl;
	return ret;
}

vector<string> sentences;

void loadSentences(const string &filename)
{
	FILE* in = tryOpen(filename, "rb");
	size_t size;
	Binary::read(in, size);
	sentences.resize(size);
	for (size_t i = 0; i < size; ++ i) {
		Binary::read(in, sentences[i]);
	}
	//cerr << "# Sentences Loaded = " << size << endl;
}


int main(int argc, char* argv[])
{
    if (argc != 8) {
        cerr << "[usage] <folder-prefix for new prob> <prev-featureTable.csv> <patterns.csv> <sentencesWithPunc.buf> <new-featureTable.csv> <saved penalty> <recompute times>" << endl;
        return -1;
    }

    suffix = argv[7];

    if (true) {
        FILE* in = tryOpen(argv[6], "r");
        fscanf(in, "%lf", &penalty);
        fclose(in);
    }

    loadProb(argv[1]);
    vector<string> compounds = loadFeatureTable(argv[2]);
    loadPattern(argv[3]);

    loadSentences(argv[4]);

    omp_set_num_threads(10);
    int corpusTokensN = 0;
    #pragma omp parallel for schedule(dynamic, 1000) reduction(+:corpusTokensN)
    for (size_t sentenceID = 0; sentenceID < sentences.size(); ++ sentenceID) {
        string alpha = sentences[sentenceID];
        for (size_t i = 0; i < alpha.size(); ++ i) {
            if (isalpha(alpha[i])) {
                alpha[i] = tolower(alpha[i]);
            } else {
                if (alpha[i] != '\'') {
					alpha[i] = ' ';
				}
            }
        }
        corpusTokensN += splitBy(alpha, ' ').size();
    }
//    cerr << "# corpus tokens = " << corpusTokensN << endl;
    FOR (iter, oldProb) {
        iter->second /= (double)corpusTokensN / splitBy(iter->first, ' ').size();
    }

//    cerr << "# update prob = " << prob.size() << endl;

    double lambda = 0.5;
    MAP_S_D hybrid;
    FOR (iter, oldProb) {
        double value = 0;
        if (prob.count(iter->first)) {
            value = prob[iter->first];
        }
        hybrid[iter->first] = value * lambda + iter->second * (1 - lambda);
    }

    vector<string> stat = printStat(patterns, prob, prob);
//    cerr << "stat ready" << endl;

    FILE* out = tryOpen(argv[5], "w");
    for (size_t i = 0; i < compounds.size(); ++ i) {
        fprintf(out, "%s,%s\n", compounds[i].c_str(), stat[i].c_str());
        /*if (compounds[i].find("support vector machine,") == 0) {
            cerr << compounds[i] + "," + stat[i] << endl;
        }*/
    }
    fclose(out);
    cerr << "recompute features done." << endl;
}

