#include "../utils/helper.h"

int load(string filename, int window, double threshold, vector< pair<double, string> > &ret, bool det, int n)
{
    vector< pair<string, double> > order;
    FILE* in = tryOpen(filename.c_str(), "r");
    if (in != NULL) {
        for (;getLine(in);) {
            vector<string> tokens = splitBy(line, ',');
            myAssert(tokens.size() == 4, "wrong number of columns");
            string phrase = tokens[0];
            double quality;
            fromString(tokens[3], quality);
            order.push_back(make_pair(phrase, quality));
        }
    }
    vector<double> sum(order.size() + 1, 0);
    for (size_t i = 0; i < order.size(); ++ i) {
        sum[i + 1] = sum[i] + order[i].second;
    }
    for (size_t i = 0; i < order.size(); ++ i) {
        int l = i - window;
        int r = i + window;
        if (l < 0) {
            l = 0;
        }
        if (r >= order.size()) {
            r = (int)order.size() - 1;
        }
        if (det && (sum[r + 1] - sum[l]) / (r - l + 1) < threshold || !det && i >= n) {
            fprintf(stderr, "%d/%d phrases in %s\n", i, (int)order.size(), filename.c_str());
            return i;
        }
        ret.push_back(make_pair(order[i].second, order[i].first));
    }
    fprintf(stderr, "%d/%d phrases in %s\n", (int)order.size(), (int)order.size(), filename.c_str());
    return (int)order.size();
}

int main(int argc, char* argv[])
{
    int window;
    double threshold;
    if (argc != 7 || sscanf(argv[2], "%d", &window) != 1 || sscanf(argv[3], "%lf", &threshold) != 1) {
        fprintf(stderr, "[usage] <input prefix> <half slide window size> <avgerage quality> <output file> <load number> <save number>");
        return -1;
    }

    int numbers[10];
    if (strcmp(argv[5], "DET") != 0) {
        FILE* in = tryOpen(argv[5], "r");
        for (int l = 2; l <= 6; ++ l) {
            fscanf(in, "%d", &numbers[l]);
        }
        fclose(in);
    }
    
    vector< pair<double, string> > phrases;
    FILE* fn = tryOpen(argv[6], "w");
    for (int length = 2; length <= 6; ++ length) {
        char filename[256];
        sprintf(filename, "%s%d.csv", argv[1], length);
        int n = load(filename, window, threshold, phrases, strcmp(argv[5], "DET") == 0, numbers[length]);
        fprintf(fn, "%d\n", n);
    }
    fclose(fn);

    sort(phrases.rbegin(), phrases.rend());
    FILE* out = tryOpen(argv[4], "w");
    FOR (phrase, phrases) {
        fprintf(out, "%s,%.10f\n", phrase->second.c_str(), phrase->first);
    }
    fclose(out);    
    
    return 0;
}

