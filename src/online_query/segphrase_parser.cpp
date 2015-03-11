#include "segphrase_parser.h"

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
    if (argc != 5 || sscanf(argv[3], "%d", &topN) != 1) {
        cerr << "[usage] <model-file> <rank-list> <top-n> <separator, _ or []>" << endl;
        return -1;
    }
    
    sep = argv[4];
    
    string model_path = (string)argv[1];
    SegPhraseParser* parser = new SegPhraseParser(model_path, 0);
    cerr << "parser built." << endl;
    
    loadRankList(argv[2], topN);
    parser->setDict(dict);
    
    vector<string> segments = parser->segment("data mining is an area");
    printVector(segments);
    
    while (getLine(stdin)) {
        segments = parser->segment(line);
        printVector(segments);
    }    
    cerr << "[done]" << endl;
    return 0;
}
