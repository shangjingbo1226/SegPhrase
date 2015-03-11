#include "segphrase_parser.h"

const string ENDINGS = ".!?,;:[]";

string sep = "[]";

void printVector(vector<string> a) {
    for (size_t i = 0; i < a.size(); ++ i) {
        if (sep == "_") {
            for (size_t j = 0; j < a[i].size(); ++ j) {
                if (a[i][j] == ' ') {
                    a[i][j] = '_';
                }
            }
        } else {
            a[i] = "[" + a[i] + "]";
        }
        cout << a[i];
        if (i + 1 == a.size()) {
            cout << endl;
        } else {
            cout << " ";
        }
    }
}

unordered_set<string> dict;

void loadRankList(string filename, int topN)
{
    FILE* in = tryOpen(filename, "r");
    vector<pair<double, string>> order;
    while (getLine(in)) {
        vector<string> tokens = splitBy(line, ',');
        string word = tokens[0];
        double score;
        fromString(tokens[1], score);
        for (size_t i = 0; i < word.size(); ++ i) {
            if (word[i] == '_') {
                word[i] = ' ';
            }
        }
        order.push_back(make_pair(score, word));
    }
    sort(order.rbegin(), order.rend());
    
    if (topN < 0) {
        topN = order.size();
    }
    if (topN < order.size()) {
        order.resize(topN);
    }
    dict.clear();
    FOR (pair, order) {
        dict.insert(pair->second);
    }
}

int main(int argc, char* argv[])
{
    int topN;
    if (argc != 7 || sscanf(argv[3], "%d", &topN) != 1) {
        cerr << "[usage] <model-file> <rank-list> <top-n> <separator, _ or []> <corpus_in> <segmented_out>" << endl;
        return -1;
    }
    
    sep = argv[4];
    
    string model_path = (string)argv[1];
    SegPhraseParser* parser = new SegPhraseParser(model_path, 0);
    cerr << "parser built." << endl;
    
    loadRankList(argv[2], topN);
    parser->setDict(dict);
    
    FILE* in = tryOpen(argv[5], "r");
    FILE* out = tryOpen(argv[6], "w");
    vector<string> sentences;
	for (;getLine(in);) {
		string sentence = "";
		for (int i = 0; line[i]; ++ i) {
			char ch = line[i];
			if (ENDINGS.find(ch) != -1) {
				if (sentence.size() > 0) {
					sentences.push_back(sentence);
				}
				sentence = "";
			} else {
				sentence += ch;
			}
		}
		if (sentence.size() > 0) {
			sentences.push_back(sentence);
		}
		
		string corpus = "";
		FOR (sentence, sentences) {
    		string origin = *sentence;
		    string text = *sentence;
		    for (size_t i = 0; i < text.size(); ++ i) {
		        if (isalpha(text[i])) {
		            text[i] = tolower(text[i]);
		        } else if (text[i] != '\'') {
		            text[i] = ' ';
		        }
		    }
		    vector<string> segments = parser->segment(text);
		    size_t last = 0;
		    string answer = "";
		    for (size_t i = 0; i < segments.size(); ++ i) {
		        size_t st = last;
		        while (text[st] != segments[i][0]) {
		            ++ st;
		        }
		        size_t ed = st;
		        for (size_t j = 0; j < segments[i].size(); ++ j) {
		            while (text[ed] != segments[i][j]) {
		                ++ ed;
		            }
		            ++ ed;
		        }
		        
		        if (st > 0 && origin[st - 1] == '"' && ed < origin.size() && origin[ed] == '"') {
		            -- st;
		            ++ ed;
		        }
		        
		        for (size_t j = last; j < st; ++ j) {
		            answer += origin[j];
		        }
		        answer += "[";
		        for (size_t j = st; j < ed; ++ j) {
		            answer += origin[j];
		        }
		        answer += "]";
		        
		        last = ed;
		    }
		    if (corpus != "") {
		        corpus += "$";
		    }
		    corpus += answer;
		}
		fprintf(out, "%s\n", corpus.c_str());
	}
    
    cerr << "[done]" << endl;
    return 0;
}
