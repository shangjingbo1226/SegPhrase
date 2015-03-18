#include "../utils/helper.h"
#include <omp.h>
#include <cassert>

void makeLog(unordered_map<string, double> &prob, const vector<string> &allPhrases)
{
    #pragma omp parallel for schedule(static)
    for (size_t i = 0; i < allPhrases.size(); ++ i) {
        const string &phrase = allPhrases[i];
        if (prob.count(phrase)) {
            prob[phrase] = log(prob[phrase]);
        }
	}
//    cerr << "    log finish" << endl;
}

const double INF = 1e100;

int nthreads, maxLen;
double discard;
string outputFile;

unordered_map<string, int> unigrams, unigram2id, phrase2id, partsInPhrase;
vector<string> allUnigrams, allPhrases;

unordered_map<string, double> prob, logistic;
vector<string> sentences;
vector< vector<string> > sentencesTokens;

void normalize()
{
	double sumList[nthreads][maxLen];
    memset(sumList, 0, sizeof(sumList));
    #pragma omp parallel for schedule(static)
    for (size_t i = 0; i < allPhrases.size(); ++ i) {
        const string &phrase = allPhrases[i];
        if (prob.count(phrase)) {
            int tid = omp_get_thread_num();
            sumList[tid][partsInPhrase[phrase] - 1] += prob[phrase];
        }
	}
    double sum[maxLen];
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < maxLen; ++ i) {
        sum[i] = 0;
        for (int j = 0; j < nthreads; ++ j) {
            sum[i] += sumList[j][i];
        }
    }

	#pragma omp parallel for schedule(static)
    for (size_t i = 0; i < allPhrases.size(); ++ i) {
        const string &phrase = allPhrases[i];
        if (prob.count(phrase)) {
            prob[phrase] /= sum[partsInPhrase[phrase] - 1];
        }
	}
}

bool getPenalty(const string &filename, double penalty)
{
    unordered_map<string, double> pr;
    vector<double> pw(maxLen + 1, 1);// P(length)
    double totalPw = 1;
    for (int i = 1; i <= maxLen; ++ i) {
        pw[i] = pw[i - 1] / penalty;
        totalPw += pw[i];
    }
    for (int i = 0; i <= maxLen; ++ i) {
        pw[i] /= totalPw;
    }

    normalize();

    for (size_t i = 0; i < allPhrases.size(); ++ i) {
        const string &phrase = allPhrases[i];
        if (prob.count(phrase)) {
            int parts = partsInPhrase[phrase];
            double &value = prob[phrase];
            double pgood = value * pw[parts - 1] * logistic[phrase];
            pr[phrase] = pgood;
        }
    }

    FILE* in = tryOpen(filename, "r");
    int total = 0, wrong =  0;
    for (;getLine(in);) {
        vector<string> tokens = splitBy(line, '\t');
        if (tokens.size() < 2) {
            continue;
        }
        string pattern = tolower(tokens[0]);
        int label;
        fromString(tokens[1], label);
        if (label == 1) {
            total += 1;
            if (pr.count(pattern)) {
                tokens = splitBy(pattern, ' ');
                vector<double> f(tokens.size() + 1, 0);
                vector<int> pre(tokens.size() + 1, -1);
                f[0] = 1;
                pre[0] = 0;
                for (size_t i = 0 ; i < tokens.size(); ++ i) {
                    string token = "";
                    size_t j = i;
                    while (j < tokens.size()) {
                        if (j == i) {
                            token = tokens[i];
                        } else {
                            token += " ";
                            token += tokens[j];
                        }
                        if (prob.count(token)) {
                            double p = pr[token];
                            if (f[i] * p > f[j + 1]) {
                                f[j + 1] = f[i] * p;
                                pre[j + 1] = i;
                            }
                        }
                        ++ j;
                    }
                }
                wrong += pre[tokens.size()] != 0;
            } else {
                wrong += 1;
            }
        }
    }
    fclose(in);
//    cerr << "# separable labels = " << total << ", wrong = " << wrong << endl;
    return total * discard >= wrong;
}

string finalSegmentation;

string dumpResult(int round, const string &prefix, const vector<double> &pw, bool lastIter)
{
//    cerr << "    start to dump ranking results" << endl;
    char folder[256];
    sprintf(folder, "%siter%d_discard%.2f", prefix.c_str(), round, discard);
    if (lastIter) {
        system(("mkdir " + (string)folder).c_str());
    }

    FILE* out[maxLen];
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < maxLen; ++ i) {
        if (lastIter) {
            char file[255];
            sprintf(file, "%s/length%d.csv", folder, i + 1);
            out[i] = tryOpen(file, "w");
        }
    }

    vector< pair<double, string> > orderList[nthreads][maxLen];
    char outputString[nthreads][10000];
    #pragma omp parallel for schedule(dynamic, 1000)
    for (size_t i = 0; i < allPhrases.size(); ++ i) {
        const string &phrase = allPhrases[i];
        if (prob.count(phrase)) {
            int parts = partsInPhrase[phrase];
            double &value = prob[phrase];
            double pgood = value * pw[parts - 1] * logistic[phrase];

            int tid = omp_get_thread_num();
            if (lastIter) {
                sprintf(outputString[tid], "%s,%.10f,%.10f,%.10f", phrase.c_str(), pgood, value * logistic[phrase], logistic[phrase]);
                orderList[tid][parts - 1].push_back(make_pair(pgood, outputString[tid]));
            }
            value = pgood;
        }
    }

    if (lastIter) {
        #pragma omp parallel for schedule(static)
        for (int i = 0; i < maxLen; ++ i) {
            vector< pair<double, string> > order;
            for (int tid = 0; tid < nthreads; ++ tid) {
                FOR (iter, orderList[tid][i]) {
                    order.push_back(*iter);
                }
            }
            sort(order.rbegin(), order.rend());
            FOR (iter, order) {
                fprintf(out[i], "%s\n", iter->second.c_str());
            }
            fclose(out[i]);
        }
    }

    return folder;
}

void DP(int round, double penalty, bool needSegmentResult = false, bool onlyDump = false)
{
//    cerr << "round = " << round << ", penalty = " << penalty << endl;
    vector<double> pw(maxLen + 1, 1);// P(length)
    double totalPw = 1;
    for (int i = 1; i <= maxLen; ++ i) {
        pw[i] = pw[i - 1] / penalty;
        totalPw += pw[i];
    }
    for (int i = 0; i <= maxLen; ++ i) {
        pw[i] /= totalPw;
    }

    normalize();

    string newpath = dumpResult(round, outputFile, pw, needSegmentResult || onlyDump);
//    cerr << "    previous results dumped" << endl;

    if (onlyDump) {
        return;
    }

    // initialize
    makeLog(prob, allPhrases);
	vector< vector<int> > occur(nthreads, vector<int>());
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < nthreads; ++ i) {
        occur[i].resize(allPhrases.size(), 0);
    }

	vector< string > parsed(sentences.size(), "");

    double energy = 0;
	#pragma omp parallel for schedule(dynamic, 1000) reduction(+:energy)
    for (size_t sentenceID = 0; sentenceID < sentences.size(); ++ sentenceID) {
    	vector<string> &tokens = sentencesTokens[sentenceID];
    	vector<double> f(tokens.size() + 1, -INF);
    	vector<int> pre(tokens.size() + 1, -1);
    	f[0] = 0;
    	pre[0] = 0;
    	for (size_t i = 0 ; i < tokens.size(); ++ i) {
    		if (f[i] < -1e80) {
    			continue;
    		}
    		string token = "";
    		size_t j = i;
    		while (j < tokens.size()) {
    			if (j == i) {
    				token = tokens[i];
    			} else {
    				token += " ";
    				token += tokens[j];
    			}
    			if (prob.count(token)) {
    				double p = prob[token];
    				if (f[i] + p > f[j + 1]) {
    					f[j + 1] = f[i] + p;
    					pre[j + 1] = i;
    				}
    			} else {
    				if (j > maxLen + i) {
    					break;
    				}
    			}
    			++ j;
    		}
    	}
    	energy += f[tokens.size()];
    	if (true) {
    		int tid = omp_get_thread_num();
    		int i = (int)tokens.size();
            vector<string> segments;
    		while (i > 0) {
    			int j = pre[i];
    			string token = "";
    			for (int k = j; k < i; ++ k) {
    				if (k > j) {
    					token += " ";
    				}
    				token += tokens[k];
    			}
                assert(phrase2id.count(token));
    			++ occur[tid][phrase2id[token]];
    			i = j;

                if (needSegmentResult) {
                    for (size_t k = 0; k < token.size(); ++ k) {
                        if (token[k] == ' ') {
                            token[k] = '_';
                        }
                    }
                    segments.push_back(token);
                }
    		}

            if (needSegmentResult) {
                string &ret = parsed[sentenceID];
                for (int _ = (int)segments.size() - 1; _ >= 0; -- _) {
                    if (segments[_] == "") {
                        continue;
                    }
                    if (ret.size()) {
                        ret += " ";
                    }
                    ret += segments[_];
                }
                if (tokens.size() != 0) {
                    assert(ret != "");
                }
            }
    	}
    }

//    cerr << "    energy = " << energy << endl;

    vector<int> sum(allPhrases.size(), 0);
    #pragma omp parallel for schedule(static)
    for (size_t i = 0; i < allPhrases.size(); ++ i) {
        int &value = sum[i];
        for (int tid = 0; tid < nthreads; ++ tid) {
            value += occur[tid][i];
        }
    }

    prob.clear();
    for (size_t i = 0; i < allPhrases.size(); ++ i) {
        if (sum[i] > 0) {
            prob[allPhrases[i]] = sum[i];
        }
    }
//    cerr << "    # candidate phrases = " << prob.size() << endl;

    if (needSegmentResult) {
        finalSegmentation = "";
        int cnt = 0;
        for (size_t i = 0; i < parsed.size(); ++ i) {
            if (parsed[i] == "") {
                ++ cnt;
                assert(sentencesTokens[i].size() == 0);
                continue;
            }
            if (finalSegmentation.size() && finalSegmentation[finalSegmentation.size() - 1] != '\n') {
                finalSegmentation += " ";
            }
            finalSegmentation += parsed[i] + "\n";
        }
//        cerr << "======= cnt = " << cnt << " ==========" << endl;
        FILE* out = tryOpen(newpath + "/segmented.txt", "w");
        fprintf(out, "%s\n", finalSegmentation.c_str());
        fclose(out);
    }
}

void loadPattern(const string &filename)
{
	FILE* in = tryOpen(filename, "r");
	for (;getLine(in);) {
		vector<string> tokens = splitBy(line, ',');
		string pattern = tolower(tokens[0]);
		if (logistic.count(pattern)) {
			int occurrence;
			fromString(tokens[1], occurrence);
			prob[pattern] = occurrence;
			allPhrases.push_back(pattern);
		}
	}
	fclose(in);

//	cerr << "# Pattern = " << prob.size() << endl;

	FOR (iter, unigrams) {
		logistic[iter->first] = 1.0;
	}
}

void loadLogistic(const string &filename)
{
	FILE* in = tryOpen(filename, "r");
	for (;getLine(in);) {
		vector<string> tokens = splitBy(line, ',');
		string pattern = tolower(tokens[0]);
		double pred;
		fromString(tokens[1], pred);
		logistic[pattern] = pred;
	}
	fclose(in);

//	cerr << "# logistic = " << logistic.size() << endl;
}

void loadSentences(const string &filename)
{
	FILE* in = tryOpen(filename, "rb");
	size_t size;
	Binary::read(in, size);
	sentences.resize(size);
	sentencesTokens.resize(size);
	for (size_t i = 0; i < size; ++ i) {
		Binary::read(in, sentences[i]);
	}
//	cerr << "# Sentences Loaded = " << size << endl;

	Binary::read(in, size);
	allUnigrams.resize(size);

	for (size_t i = 0; i < size; ++ i) {
		Binary::read(in, allUnigrams[i]);
        unigram2id[allUnigrams[i]] = i;
		allPhrases.push_back(allUnigrams[i]);
	}
//	cerr << "# Unigrams Loaded = " << size << endl;
	fclose(in);

	vector< vector<int> > unigramsList(nthreads, vector<int>(allUnigrams.size(), 0));

	#pragma omp parallel for schedule(dynamic, 1000)
	for (size_t i = 0; i < sentences.size(); ++ i) {
		vector<string> &tokens = sentencesTokens[i];
		tokens = splitBy(sentences[i], ' ');

		int tid = omp_get_thread_num();
		FOR (token, tokens) {
			++ unigramsList[tid][unigram2id[*token]];
		}
	}

    vector<int> sum(allUnigrams.size(), 0);
	#pragma omp parallel for schedule(static)
	for (size_t i = 0; i < allUnigrams.size(); ++ i) {
		int &value = sum[i];
		for (int tid = 0; tid < nthreads; ++ tid) {
            value += unigramsList[tid][i];
		}
	}
    for (size_t i = 0; i < allUnigrams.size(); ++ i) {
        unigrams[allUnigrams[i]] = sum[i];
    }
//	cerr << "# the = " << unigrams["the"] << endl;
}

int main(int argc, char* argv[])
{
	int maxIter;
	if (argc != 10 || sscanf(argv[2], "%d", &nthreads) != 1 || sscanf(argv[5], "%lf", &discard) != 1 || sscanf(argv[6], "%d", &maxIter) != 1) {
		cerr << "[Usage] <input-sentence-buffer> <nthreads> <logistic> <pattern> <discard> <maxIter> <outputFile> <labels> <save penalty>" << endl;
		return -1;
	}
    outputFile = argv[7];

	omp_set_num_threads(nthreads);

	loadSentences(argv[1]);
	loadLogistic(argv[3]);
	loadPattern(argv[4]);

//    cerr << "# all phrase candidates = " << allPhrases.size() << endl;

	FOR (iter, unigrams) {
		logistic[iter->first] = 1.0;
		prob[iter->first] = iter->second;
	}

    maxLen = 0;
    vector<int> temp(allPhrases.size(), 0);
    #pragma omp parallel for schedule(dynamic, 1000) reduction(max:maxLen)
    for (size_t i = 0; i < allPhrases.size(); ++ i) {
        int parts = 1;
        const string &phrase = allPhrases[i];
        for (size_t j = 0; j < phrase.size(); ++ j) {
            parts += phrase[j] == ' ';
        }
        temp[i] = parts;
        maxLen = max(maxLen, parts);
    }
    for (size_t i = 0; i < allPhrases.size(); ++ i) {
        const string &phrase = allPhrases[i];
        phrase2id[phrase] = i;
        partsInPhrase[phrase] = temp[i];
    }
    cerr << "max #tokens in phrase = " << maxLen << endl;

    assert(!prob.count(""));
    assert(!logistic.count(""));

	// start adjust penalty, DP
	const double EPS = 1e-6;
	double lower = EPS, upper = 200;
	double penalty = EPS;
	unordered_map<string, double> backup = prob;
	DP(0, lower, false, false);
	unordered_map<string, double> large = prob;
	prob = backup;
	DP(0, upper, false, false);
	unordered_map<string, double> small = prob;

	prob = backup;
	FOR (iter, prob) {
        iter->second = (large[iter->first] + small[iter->first]) / 2.0;
	}
	backup = prob;

	for (int _ = 0; _ < 10; ++ _) {
//	    cerr << "[lower, upper] = [" << lower << ", " << upper << "]" << endl;
        penalty = (lower + upper) / 2;

        prob = backup;
        DP(-(_ + 1), penalty, false, false);

//        cerr << "penalty = "  << penalty << endl;
        if (getPenalty(argv[8], penalty)) {
            lower = penalty;
        } else {
            upper = penalty;
        }
	}

    if (true) {
        FILE* out = tryOpen(argv[9], "w");
        fprintf(out, "%.10f\n", penalty);
        fclose(out);
    }

	prob = backup;
    for (int round = 1; round <= maxIter; ++ round) {
        DP(round, penalty, round == maxIter);
    }
    DP(maxIter + 1, penalty, false, true);

    cerr << "adjust probability done." << endl;

	return 0;
}


