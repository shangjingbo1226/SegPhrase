#include "segphrase_parser.h"

template<class T>
void printVector(vector<T> a) {
    for (size_t i = 0; i < a.size(); ++ i) {
        cerr << a[i];
        if (i + 1 == a.size()) {
            cerr << endl;
        } else {
            cerr << ", ";
        }
    }
}

int main(int argc, char* argv[])
{
    if (argc != 2) {
        cerr << "[usage] <model-file>" << endl;
        return -1;
    }
    
    string model_path = (string)argv[1];
    SegPhraseParser* parser = new SegPhraseParser(model_path, 0);
    cerr << "parser built." << endl;
    
    vector<string> segments = parser->segment("data mining is an area");
    printVector(segments);
    
    cerr << "Please type in a sentence in a single line (or exit()):" << endl;
    while (getLine(stdin)) {
        if (strcmp(line, "exit()") == 0) {
            break;
        }
        segments = parser->segment(line);
        cerr << "[Segmentation Result]" << endl;
        printVector(segments);
        cerr << "\nPlease type in a sentence in a single line (or exit()):" << endl;
    }
    
    cerr << "[done]" << endl;
    return 0;
}
