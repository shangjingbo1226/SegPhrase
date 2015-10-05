#include "../utils/helper.h"

void dump(const unordered_map<string, double> &prob, FILE* out)
{
    vector<string> phrases;
    vector<double> probability;
    size_t size;
    size = prob.size();
    FOR (pairs, prob) {
        phrases.push_back(pairs->first);
        probability.push_back(pairs->second);
    }
    
    fwrite(&size, sizeof(size), 1, out);
    for (size_t i = 0; i < size; ++ i) {
        Binary::write(out, phrases[i]);
    }
    if (size > 0) {
        fwrite(&probability[0], sizeof(probability[0]), size, out);
    }
}

int main(int argc, char* argv[])
{
    int maxLen;
    if (argc != 5 || sscanf(argv[2], "%d", &maxLen) != 1) {
        cerr << "[usage] <length*.csv folder path> <maxLen> <penalty-file> <output-model-name>" << endl;
        return -1;
    }
    string folder = argv[1];
    
    FILE* in = tryOpen(argv[3], "r");
    double penalty;
    fscanf(in, "%lf", &penalty);
    fclose(in);
    
    string modelFilename = argv[4];
    
    FILE* out = tryOpen(modelFilename, "wb");
    fwrite(&penalty, sizeof(penalty), 1, out);
    
    unordered_map<string, double> unigrams, phrases;
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
                phrases[phrase] = prob;
            }
        }
        fclose(in);
    }
    cerr << "penalty = " << penalty << endl;
    cerr << "# unigrams = " << unigrams.size() << endl;
    cerr << "# phrases = " << phrases.size() << endl;
    
    dump(unigrams, out);
    dump(phrases, out);
    
    cerr << "segmetation model saved." << endl;
    
    fclose(out);
    
    return 0;
}
