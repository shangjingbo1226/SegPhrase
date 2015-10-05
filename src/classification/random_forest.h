#ifndef __RANDOM_FOREST_H__
#define __RANDOM_FOREST_H__

#include "../utils/helper.h"

#include <iostream>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <sstream>
#include <ctime>
using namespace std;

vector<double> featureImportance;

int RANDOM_FEATURES = 4;
int RANDOM_POSITIONS = 8;
const double PI = acos(-1.0);

int decayPoint = 12;

namespace RandomNumbers
{
struct RandomNumberGenerator
{
    unsigned int MT[624];
    int index;
    
    void init(int seed = 1) {
        MT[0] = seed;
		for (int i = 1; i < 624; ++ i) {
			MT[i] = (1812433253UL * (MT[i-1] ^ (MT[i-1] >> 30)) + i);
		}
        index = 0;
    }
    
    void generate() {
        const unsigned int MULT[] = {0, 2567483615UL};
		for (int i = 0; i < 227; ++ i) {
            unsigned int y = (MT[i] & 0x8000000UL) + (MT[i+1] & 0x7FFFFFFFUL);
            MT[i] = MT[i+397] ^ (y >> 1);
            MT[i] ^= MULT[y&1];
        }
		for (int i = 227; i < 623; ++ i) {
            unsigned int y = (MT[i] & 0x8000000UL) + (MT[i+1] & 0x7FFFFFFFUL);
            MT[i] = MT[i-227] ^ (y >> 1);
            MT[i] ^= MULT[y&1];
        }
        unsigned int y = (MT[623] & 0x8000000UL) + (MT[0] & 0x7FFFFFFFUL);
        MT[623] = MT[623-227] ^ (y >> 1);
        MT[623] ^= MULT[y&1];
    }
    
    unsigned int rand() {
        if (index == 0) {
            generate();
        }
        
        unsigned int y = MT[index];
        y ^= y >> 11;
        y ^= y << 7  & 2636928640UL;
        y ^= y << 15 & 4022730752UL;
        y ^= y >> 18;
        index = index == 623 ? 0 : index + 1;
        return y;
    }
    
    int next(int x) { // [0, x)
        return rand() % x;
    }
    
    int next(int a, int b) { // [a, b)
        return a + (rand() % (b - a));
    }
    
    double nextDouble() { // (0, 1)
        return (rand() + 0.5) * (1.0 / 4294967296.0);
    }
};

static RandomNumberGenerator rng;
}
using namespace RandomNumbers;

namespace RandomForestRelated
{
struct TreeNode {
	bool leaf;
	int level, feature;
	double value, result;
	int left, right;

	TreeNode() {
		leaf = false;
		level = feature = left = right = -1;
		value = result = 0;
	}
};

class DecisionTree
{
public:
	vector<TreeNode> nodes;
	vector<int> oobSamples;
	
		
	void dump(FILE* out) {
	    size_t size = nodes.size();
	    fwrite(&size, sizeof(size), 1, out);
	    if (size > 0) {
    	    fwrite(&nodes[0], sizeof(nodes[0]), size, out);
	    }
	}
	
	void load(FILE* in) {
	    size_t size;
	    fread(&size, sizeof(size), 1, in);
	    nodes.resize(size);
	    if (size > 0) {
    	    fread(&nodes[0], sizeof(nodes[0]), size, in);
	    }
    }

	DecisionTree() {}

	DecisionTree(vector< vector<double> > &features, vector<double> &results, int minNodeSize, int maxLevel = 18) {
//fprintf(stderr, "%d, %d\n", features.size(), features[0].size());
		TreeNode root;
		root.level = 0;
		nodes.push_back(root);

		vector<int> index[2];
		for (int i = 0; i < (int)results.size(); ++ i) {
			index[(int)results[i]].push_back(i);
		}
		vector<int> rootBag, used(features.size(), 0);
		for (int type = 0; type < 2; ++ type) {
			int selected = (int)(min(index[0].size(), index[1].size())) * 16;
//fprintf(stderr, "selected = %d\n", selected);
			for (int i = 0; i < selected; ++ i) {
				int id = index[type][rng.next(index[type].size())];
				rootBag.push_back(id);
				used[id] = 1;
			}
		}

		for (int i = 0; i < (int)results.size(); ++ i) {
			if (!used[i]) {
				oobSamples.push_back(i);
			}
		}
		vector< vector<int> > nodeBags;
		nodeBags.push_back(rootBag);

		for (int curNode = 0; curNode < (int)nodes.size(); ++ curNode) {
			TreeNode &node = nodes[curNode];
			vector<int> &bag = nodeBags[curNode];

			myAssert((int)bag.size() > 0, "[ERROR] empty node in decision tree!");

			bool equal = true;

			double first = results[bag[0]];
			for (int i = 1; i < (int)bag.size(); ++ i) {
				if (sign(results[bag[i]] - first)) {
					equal = false;
					break;
				}
			}
//fprintf(stderr, "equal = %d, minNodeSize = %d, maxLevel = %d\n", equal, minNodeSize, maxLevel);

			if (equal || (int)bag.size() <= minNodeSize || node.level >= maxLevel) {
				// leaf
				node.leaf = true;
				for (int i = 0; i < (int)bag.size(); ++ i) {
					node.result += results[bag[i]];
				}
				node.result /= bag.size();
				continue;
			}

            double bag_sum = 0;
            for (int i = 0; i < (int)bag.size(); ++ i) {
                bag_sum += results[bag[i]];
            }
            double bag_mean = bag_sum / bag.size();
            double bag_mse = 0;
            for (int i = 0; i < (int)bag.size(); ++ i) {
                double x = bag_mean - results[bag[i]];
                bag_mse += x * x;
            }

			int bestFeature = -1;
			int bestLeft = 0, bestRight = 0;
			double bestValue = 0;
			double bestMSE = 1e100;
			for (int _ = 0; _ < RANDOM_FEATURES; ++ _) {
				int featureID = rng.next(features[0].size());
                if (featureID >= decayPoint) {
                    int number = features[0].size() - decayPoint;
                    if (rand() % number != 0) {
                        -- _;
                        continue;
                    }
                }
				for (int __ = 0; __ < RANDOM_POSITIONS; ++ __) {
					int instanceID = bag[rng.next(bag.size())];
					double splitValue = features[instanceID][featureID];
//fprintf(stderr, "attribute %d, value = %.6f\n", featureID, splitValue);
					double sum[2] = {0, 0};
					int total[2] = {0, 0};
					for (int i = 0; i < (int)bag.size(); ++ i) {
						int id = bag[i];
						int side = features[id][featureID] < splitValue;
						sum[side] += results[id];
						++ total[side];
					}

					if (total[0] == 0 || total[1] == 0) {
						continue;
					}

					double mean[2] = {sum[0] / total[0], sum[1] / total[1]};

					double mse = 0;
					for (int i = 0; i < (int)bag.size(); ++ i) {
						int id = bag[i];
						int side = features[id][featureID] < splitValue;
						
						mse += sqr(results[id] - mean[side]);
					}

					if (mse < bestMSE) {
						bestMSE = mse;
						bestValue = splitValue;
						bestFeature = featureID;
						bestLeft = total[1];
						bestRight = total[0];
					}
				}
			}
//fprintf(stderr, "bset mse = %.10f\n", bestMSE);
			if (bestLeft == 0 || bestRight == 0) {
				// leaf
				node.leaf = true;
				for (int i = 0; i < (int)bag.size(); ++ i) {
					node.result += results[bag[i]];
				}
				node.result /= bag.size();
				continue;
			}
			
			featureImportance[bestFeature] += bag_mse - bestMSE;

			double nextValue = -1e100;
			for (int i = 0; i < (int)bag.size(); ++ i) {
				int id = bag[i];
				if (features[id][bestFeature] < bestValue) {
					nextValue = max(nextValue, features[id][bestFeature]);
				}
			}

			TreeNode left, right;

			left.level = right.level = node.level + 1;
			node.feature = bestFeature;
			node.value = (bestValue + nextValue) / 2;
			node.left = nodes.size();
			node.right = nodes.size() + 1;

			vector<int> leftBag(bestLeft, 0), rightBag(bestRight, 0);
			for (int i = 0; i < (int)bag.size(); ++ i) {
				int id = bag[i];
				if (features[id][bestFeature] < bestValue) {
					leftBag[-- bestLeft] = id;
				} else {
					rightBag[-- bestRight] = id;
				}
			}

			myAssert(bestLeft == 0, "Left Space Remained!");
			myAssert(bestRight == 0, "Right Space Remained!");

			nodes.push_back(left);
			nodes.push_back(right);

			nodeBags.push_back(leftBag);
			nodeBags.push_back(rightBag);
		}
	}

	double estimate(vector<double> &features) {
		TreeNode *current = &nodes[0];
		while (!current->leaf) {
			if (features[current->feature] < current->value) {
				current = &nodes[current->left];
			} else {
				current = &nodes[current->right];
			}
		}
		return current->result;
	}
};

class RandomForest
{
	vector<DecisionTree> trees;
	vector< vector<double> > features;
	vector<double> results;
public:
    void dump(string filename) {
        FILE* out = fopen(filename.c_str(), "wb");
        size_t size = trees.size();
        fwrite(&size, sizeof(size), 1, out);
        for (size_t i = 0; i < trees.size(); ++ i) {
            trees[i].dump(out);
        }
        fclose(out);
    }
    
    void load(string filename) {
        FILE* in = fopen(filename.c_str(), "rb");
        size_t size;
        fread(&size, sizeof(size), 1, in);
        trees.resize(size);
        for (size_t i = 0; i < trees.size(); ++ i) {
            trees[i].load(in);
        }
        fclose(in);
    }

	void clear() {
		features.clear();
		results.clear();
		trees.clear();
	}

	void train(vector< vector<double> > &_features, vector<double> _results, int treesNo = 100, int minNodeSize = 100) {
		if (features.size() == 0) {
			features = _features;
			results = _results;
			if (features.size() > 0) {
    			featureImportance.resize(features[0].size(), 0);
			}
		}
		myAssert(features.size() == results.size(), "[ERROR] wrong training data!");

		for (int i = 0; i < treesNo; ++ i) {
			trees.push_back(DecisionTree(_features, _results, minNodeSize));
//fprintf(stderr, "treeNo = %d, node size = %d\n", i, trees.back().nodes.size());
		}
	}

	double estimate(vector<double> &features) {
		if (trees.size() == 0) {
			return 0.0;
		}

		double sum = 0;
		for (int i = 0; i < (int)trees.size(); ++ i) {
			sum += trees[i].estimate(features);
		}
		return sum / trees.size();
	}

	vector<double> calcOOB() {
		vector<double> w(features.size(), 0);
		vector<double> v(features.size(), 0);
		vector<double> rv(features.size(), 0);

		for (int i = 0; i < (int)trees.size(); ++ i) {
			DecisionTree &t = trees[i];
			for (int j = 0; j < (int)t.oobSamples.size(); ++ j) {
				int id = t.oobSamples[j];
				v[id] += t.estimate(features[i]);
				w[id] += 1;
			}
		}

		for (int i = 0; i < (int)rv.size(); ++ i) {
			rv[i] = v[i] / w[i];
		}
		return rv;
	}
};
};

#endif
