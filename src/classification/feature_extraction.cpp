#include "../utils/helper.h"
#include "aho_corasick.h"
#include <cassert>

typedef unordered_map<string, double> MAP_S_D;

MAP_S_D word2idf, stopwords;

void loadStopwords(string stopFile, string idfFile)
{
    FILE* in = tryOpen(idfFile, "r");
    for (;getLine(in);) {
        vector<string> tokens = splitBy(line, ',');
        if (tokens.size() == 2) {
            string word = tokens[0];
            double idf;
            fromString(tokens[1], idf);
            word2idf[word] = idf;
        } else {
            cerr << line << endl;
        }
    }
    in = tryOpen(stopFile, "r");
    for (;getLine(in);) {
        stopwords[line] = 1;
    }
}

vector<string> printPunc(MAP_S_D &patterns, MAP_S_D &quote, MAP_S_D &dash, MAP_S_D &parenthesis, MAP_S_D &capital, MAP_S_D &total)
{
    vector<string> ret(1, "quote,dash,parenthesis,capital");
    char temp[1000];
    FOR (iter, patterns) {
        const string &pattern = iter->first;
        if (pattern.find(' ') == string::npos) {
            continue;
        }
        if (total[pattern] == 0) {
            ret.push_back("0,0,0,0");
        } else {
            sprintf(temp, "%.10f,%.10f,%.10f,%.10f",
                            quote[pattern] / total[pattern],
                            dash[pattern] / total[pattern],
                            parenthesis[pattern] / total[pattern],
                            capital[pattern] / total[pattern]
                   );
            ret.push_back(temp);
        }
    }
    return ret;
}

vector<string> printStat(MAP_S_D &patterns, MAP_S_D &prob, unordered_map<string, vector<double> > &f, MAP_S_D &total, unordered_map<string, vector<double> > &sumOutside)
{
    vector<string> ret(1, "prob_feature,occur_feature,log_occur_feature,prob_log_occur,constant,outsideSentence,new_outside,frequency");
    char temp[1000];
    FOR (iter, patterns) {
        const string &pattern = iter->first;
        if (pattern.find(' ') == string::npos) {
            continue;
        }
        string AB = "";
        string CD = "";
        double best = -1;
        for (size_t i = 0; i < pattern.size(); ++ i) {
            if (pattern[i] == ' ') {
                string left = pattern.substr(0, i);
                string right = pattern.substr(i + 1);
                double current = prob[left] * prob[right];
                if (current > best) {
                    best = current;
                    AB = left;
                    CD = right;
                }
            }
        }
        double f1 = prob[pattern] / (prob[AB] * prob[CD]);
        double f2 = iter->second / sqrt(patterns[AB] * patterns[CD]);
        double f3 = sqrt(iter->second) * log(f1);
        double f4 = prob[pattern] * log(f1);
        
        vector<string> tokens = splitBy(pattern, ' ');
        double sum = 0, norm = 0;
        for (size_t i = 0; i < tokens.size(); ++ i) {
            if (total[pattern] > 0) {
                f[pattern][i] /= total[pattern];
            }
            double wi = log(prob[tokens[i]]);
            sum += sqr(f[pattern][i]) * sqr(wi);
            norm += sqr(wi);
        }
        double outside = sqrt(sum / norm);
        
        sum = norm = 0;
        for (size_t i = 0; i < tokens.size(); ++ i) {
            sum += sumOutside[pattern][i] * word2idf[tokens[i]];
            norm += word2idf[tokens[i]];
        }
        if (total[pattern] > 0) {
            sum /= total[pattern];
        }
        double newOutside = sum / norm;
        
        sprintf(temp, "%.10f,%.10f,%.10f,%.10f,1,%.10f,%.10f,%.0f", f1, f2, f3, f4, outside, newOutside, iter->second);
        ret.push_back(temp);
    }
    return ret;
}

vector<string> printStopwords(MAP_S_D &patterns)
{
    vector<string> ret(1, "avg_idf,stop_ratio,first_stop,last_stop");
    char temp[1000];
    FOR (iter, patterns) {
        const string &pattern = iter->first;
        if (pattern.find(' ') == string::npos) {
            continue;
        }
        vector<string> tokens = splitBy(pattern, ' ');
        double sum = 0;
        int cnt = 0, stopCnt = 0;
        FOR (token, tokens) {
            if (word2idf.count(*token)) {
                ++ cnt;
                sum += word2idf[*token];
            }
            if (stopwords.count(*token)) {
                ++ stopCnt;
            }
        }
        if (cnt > 0) {
            sum /= cnt;
        }
        sprintf(temp, "%.10f,%.10f,%d,%d", sum, (double)stopCnt / tokens.size(), stopwords.count(tokens[0]), stopwords.count(tokens[1]));
        ret.push_back(temp);
    }
    return ret;
}

void selftest()
{
    // self test
    AhoCorasick tree;
    tree.add("a");
    tree.add("aa");
    tree.add("ab");
    tree.make();
    //                                                0123456789
    vector< pair<int, int> > positions;
    tree.search("aa ab baa", positions);
    /*FOR (iter, positions) {
        cerr << iter->first << " " << iter->second << endl;
    }*/
    assert(positions.size() == 8);
    
    //cerr << "self test on AC automaton passed" << endl;
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

AhoCorasick tree;
MAP_S_D patterns, prob, quote, parenthesis, dash, capital, total;
unordered_map<string, vector<double> > f, sumOutside;

void loadPattern(const string &filename)
{
	FILE* in = tryOpen(filename, "r");
	for (;getLine(in);) {
		vector<string> tokens = splitBy(line, ',');
		string pattern = tolower(tokens[0]);
		int occurrence;
		fromString(tokens[1], occurrence);
		
		patterns[pattern] = occurrence;
		prob[pattern] = occurrence;
		
		quote[pattern] = parenthesis[pattern] = dash[pattern] = capital[pattern] = total[pattern] = 0;
		
		size_t tokensN = splitBy(pattern, ' ').size();
		f[pattern].resize(tokensN, 0);
		sumOutside[pattern].resize(tokensN, 0);
		
		tree.add(" " + pattern + " ");
	}
	fclose(in);
	//cerr << "# Pattern = " << prob.size() << endl;
	
	tree.make();
	//cerr << "Tree is built" << endl;
}

int main(int argc, char* argv[])
{
    if (argc != 6) {
        cerr << "[usage] <sentencesText.buf> <patterns.csv> <stopwords.txt> <stopwordsFromText.txt> <final.csv>" << endl;
        return -1;
    }
    selftest();
    
    loadSentences(argv[1]);
    loadPattern(argv[2]);
    loadStopwords(argv[3], argv[4]);
    
    int corpusTokensN = 0;
    for (size_t sentenceID = 0; sentenceID < sentences.size(); ++ sentenceID) {
        const string &text = sentences[sentenceID];
        string alpha = text;
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
        
        string outsideText = alpha;
        if (sentenceID > 0) {
            outsideText += " " + sentences[sentenceID - 1];
        }
        if (sentenceID + 1 < sentences.size()) {
            outsideText += " " + sentences[sentenceID + 1];
        }
        for (size_t i = 0; i < outsideText.size(); ++ i) {
            if (isalpha(outsideText[i])) {
                outsideText[i] = tolower(outsideText[i]);
            } else {
                outsideText[i] = ' ';
            }
        }
        
        vector<string> outside = splitBy(outsideText, ' ');
        unordered_map<string, int> outsideCnt;
        FOR (token, outside) {
            ++ outsideCnt[*token];
        }
        
        vector< pair<int, int> > positions;
        tree.search(" " + alpha + " ", positions);
        
        unordered_map<string, int> patternCnt;
        FOR (pos, positions) {
            int st = pos->first;
            int ed = pos->second - 2;
            string pattern = alpha.substr(st, ed - st);
            ++ patternCnt[pattern];
        }
        FOR (pos, positions) {
            int st = pos->first;
            int ed = pos->second - 2;
            string pattern = alpha.substr(st, ed - st);
            
            vector<string> tokens = splitBy(pattern, ' ');
            unordered_map<string, int> tokenCnt;
            int delta = patternCnt[pattern];
            for (size_t i = 0; i < tokens.size(); ++ i) {
                tokenCnt[tokens[i]] += delta;
            }
            for (size_t i = 0; i < tokens.size(); ++ i) {
                if (outsideCnt[tokens[i]] > tokenCnt[tokens[i]]) {
                    f[pattern][i] += 1;
                    sumOutside[pattern][i] += outsideCnt[tokens[i]] - tokenCnt[tokens[i]];
                }
            }
            
            total[pattern] += 1;
            
            if (st > 0 && ed < (int)text.size()) {
                if (text[st - 1] == '(' && text[ed] == ')') {
                    parenthesis[pattern] += 1;
                }
                if (text[st - 1] == '"' && text[ed] == '"') {
                    quote[pattern] += 1;
                }
            }
            
            bool found = false;
            for (int i = st; i < ed && !found; ++ i) {
                found |= text[i] == '-';
            }
            dash[pattern] += found;
            
            bool valid = true;
            for (int i = st; i < ed && valid; ++ i) {
                if (isalpha(alpha[i]) && (i == st || alpha[i - 1] == ' ')) {
                    if (text[i] < 'A' && text[i] > 'Z') {
                        valid = false;
                    }
                }
            }
            capital[pattern] += valid;
        }
    }
    FOR (iter, prob) {
        iter->second /= (double)corpusTokensN / splitBy(iter->first, ' ').size();
    }
    //cerr << "prepared" << endl;
    
    vector<string> stat = printStat(patterns, prob, f, total, sumOutside);
    //cerr << "stat features ready" << endl;
    vector<string> punc = printPunc(patterns, quote, dash, parenthesis, capital, total);
    //cerr << "punc features ready" << endl;
    vector<string> stop = printStopwords(patterns);
    //cerr << "stop features ready" << endl;
    
    vector<string> compounds(1, "pattern");
    FOR (iter, patterns) {
        if (iter->first.find(' ') != string::npos) {
            compounds.push_back(iter->first);
        }
    }
    //cerr << "compound ready" << endl;
    
    FILE* out = tryOpen(argv[5], "w");
    for (size_t i = 0; i < compounds.size(); ++ i) {
        fprintf(out, "%s,%s,%s,%s\n", compounds[i].c_str(), stat[i].c_str(), punc[i].c_str(), stop[i].c_str());
    }
    fclose(out);
    cerr << "feature extraction done." << endl;
}

