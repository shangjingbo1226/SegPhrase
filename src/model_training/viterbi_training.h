#ifndef __VITERBI_TRAINING_H__
#define __VITERBI_TRAINING_H__

#include "../utils/helper.h"
#include <cassert>

namespace ViterbiTraining
{

    const double INF = 1e100;

    // multi-thread safe
    double train(const vector<string> &tokens, unordered_map<string, double> &logProb, int maxLen, unordered_map<string, int> &phrase2id, vector<int> &occur, bool needSegmentResult, string &parsed) {
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
                if (logProb.count(token)) {
                    double p = logProb[token];
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
        if (true) {
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
                ++ occur[phrase2id[token]];
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
                for (int _ = (int)segments.size() - 1; _ >= 0; -- _) {
                    if (segments[_] == "") {
                        continue;
                    }
                    if (parsed.size()) {
                        parsed += " ";
                    }
                    parsed += segments[_];
                }
                if (tokens.size() != 0) {
                    assert(parsed != "");
                }
            }
        }
        return f[tokens.size()];
    }
}

#endif
