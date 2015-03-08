#ifndef __AHO_CORASICK_H__
#define __AHO_CORASICK_H__

#include "../utils/helper.h"
#include <queue>
using namespace std;

class AhoCorasick
{
    vector< unordered_map<char, int> > next;
    vector<int> failed, depth;
    vector<bool> isEnd;
    int nodes;
    
    int addNewNode() {
        next.push_back(unordered_map<char, int>());
        isEnd.push_back(false);
        depth.push_back(0);
        return nodes ++;
    }
    
public:
    AhoCorasick() {
        nodes = 0;
        addNewNode();
    }
    
    void add(const string &s) {
        int u = 0;
        for (size_t i = 0; i < s.size(); ++ i) {
            char ch = s[i];
            int v;
            if (!next[u].count(ch)) {
                v = addNewNode();
                depth[v] = depth[u] + 1;
                next[u][ch] = v;
            } else {
                v = next[u][ch];
            }
            u = v;
        }
        isEnd[u] = true;
    }
    
    void make() {
        queue<int> q;
        failed.resize(nodes, -1);
        q.push(0);
        while (q.size()) {
            int u = q.front();
            q.pop();
            FOR (edge, next[u]) {
                char ch = edge->first;
                int v = edge->second;
                if (u == 0) {
                    failed[v] = 0;
                } else {
                    failed[v] = 0;
                    for (int p = failed[u];p != -1;p = failed[p]) {
                        if (next[p].count(ch)) {
                            failed[v] = next[p][ch];
                            break;
                        }
                    }
                }
                q.push(v);
            }
        }
    }
    
    void search(const string &text, vector< pair<int, int> > &ret) {
        for (int i = 0, p = 0; i < (int)text.size(); ++ i) {
            char ch = text[i];
            while (!next[p].count(ch) && p != 0) {
                p = failed[p];
            }
            if (next[p].count(ch)) {
                p = next[p][ch];
            }
            int temp = p;
            while (temp != 0 && isEnd[temp]) {
                ret.push_back(make_pair(i - depth[temp] + 1, i + 1));
                temp = failed[temp];
            }
        }
    }
};

#endif
