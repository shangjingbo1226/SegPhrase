#include "../utils/helper.h"

unordered_map<string, double> phrases;

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
    if (argc != 3) {
        printf("[usage] <length*.csv folder path> <output: unified-rank>\n");
        return 0;
    }
    loadPatterns(argv[1]);

    vector<pair<double, string>> order;
    FOR (w, phrases) {
        order.push_back(make_pair(w->second, w->first));
    }
    sort(order.rbegin(), order.rend());

    FILE* out = tryOpen(argv[2], "w");
    FOR (word, order) {
        fprintf(out, "%s,%.10f\n", word->second.c_str(), word->first);
    }
    fclose(out);

    return 0;
}
