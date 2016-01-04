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

void loadRankList(string filename, double topNRatio)
{
    FILE* in = tryOpen(filename, "r");
    vector<pair<double, string>> order;
    if (topNRatio <= 0 || topNRatio > 1) {
        topNRatio = 0.5;
    }
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
        if (score > topNRatio) {
            order.push_back(make_pair(score, word));
        }
    }
    sort(order.rbegin(), order.rend());
    dict.clear();
    FOR (pair, order) {
        dict.insert(pair->second);
    }
}

string translate(vector<pair<string, bool>> &segments, bool clean_mode, string &origin, string &text, vector<string> &betweens, int &index)
{
    string answer = "";
    if (clean_mode) {
        for (size_t i = 0; i < segments.size(); ++ i) {
            if (segments[i].second) {
                answer += "[";
            }
            answer += segments[i].first;
            if (segments[i].second) {
                answer += "]";
            }
            answer += " ";
        }
        answer += "$ ";
    } else {
        size_t last = 0;
        if (segments.size() == 0) {
            answer += origin;
        } else {
            for (size_t i = 0; i < segments.size(); ++ i) {
                size_t st = last;
                while (text[st] != segments[i].first[0]) {
                    ++ st;
                }
                size_t ed = st;
                for (size_t j = 0; j < segments[i].first.size(); ++ j) {
                    while (text[ed] != segments[i].first[j]) {
                        ++ ed;
                    }
                    ++ ed;
                }

                for (size_t j = last; j < st; ++ j) {
                    answer += origin[j];
                }
                if (segments[i].second) {
                    answer += "[";
                }
                for (size_t j = st; j < ed; ++ j) {
                    answer += origin[j];
                }
                if (segments[i].second) {
                    answer += "]";
                }

                last = ed;
            }
            while (last < origin.size()) {
                answer += origin[last];
                ++ last;
            }
        }
        if (index < betweens.size()) {
            answer += betweens[index];
            ++ index;
        }
    }
    return answer;
}

int main(int argc, char* argv[])
{
    double topNRatio;
    if (argc != 7 || sscanf(argv[3], "%lf", &topNRatio) != 1) {
        cerr << "[usage] <model-file> <rank-list> <top-n> <corpus_in> <segmented_out> <clean_mode>" << endl;
        return -1;
    }


    string model_path = (string)argv[1];
    SegPhraseParser* parser = new SegPhraseParser(model_path, 0);
    cerr << "parser built." << endl;
    
    loadRankList(argv[2], topNRatio);
	parser->setDict(dict);

    FILE* in = tryOpen(argv[4], "r");
    FILE* out = tryOpen(argv[5], "w");

    bool clean_mode = (strcmp(argv[6], "0") != 0);
    for (;getLine(in);) {
        vector<string> sentences;
        vector<string> betweens;
        betweens.push_back("");

        string sentence = "";
        // if (line.size() == 0) continue;
        for (int i = 0; line[i]; ++ i) {
            char ch = line[i];
            if (ENDINGS.find(ch) != -1) {
                if (sentence.size() > 0) {
                    sentences.push_back(sentence);
                    betweens.push_back(string(1, ch));
                } else {
                    betweens.back() += ch;
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
        if (!clean_mode) {
            corpus += betweens[0];
        }
        int index = 1;
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
            vector<pair<string, bool>> segments = parser->segment(text);
            string answer = translate(segments, clean_mode, origin, text, betweens, index);
            corpus += answer;
        }
        fprintf(out, "%s\n", corpus.c_str());
    }

    cerr << "[done]" << endl;
    return 0;
}
