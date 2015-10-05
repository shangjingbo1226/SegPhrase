#include "random_forest.h"
#include "../utils/helper.h"

using namespace RandomForestRelated;

vector< vector<double> > train, all;
vector<string> candidates;
vector<double> trainY;

map<string, int> labels;

void loadLabels(string filename)
{
	FILE* in = tryOpen(filename.c_str(), "r");
	for (;getLine(in);) {
		vector<string> tokens = splitBy(line, '\t');
		if (tokens.size() < 2) {
			continue;
		}
		string phrase = tolower(tokens[0]);
		int label;
		fromString(tokens[1], label);
		labels[phrase] = label;
	}
	fclose(in);
	fprintf(stderr, "%d labels loaded\n", labels.size());
}

vector<string> featureNames;

int loadFeatureMatrix(string filename, string forbid)
{
    vector<string> forbidFeat = splitBy(forbid, ',');
    unordered_set<string> forbidFeatSet(forbidFeat.begin(), forbidFeat.end());
    
	FILE* in = tryOpen(filename.c_str(), "r");
	getLine(in); // header
	vector<string> attributes = splitBy(line, ',');
	int dimension = 0;
	FOR (feat, attributes) {
	    if (*feat == "pattern") {
	        continue;
	    }
	    dimension += !forbidFeatSet.count(*feat);
	    if (!forbidFeatSet.count(*feat)) {
	        featureNames.push_back(*feat);
	    }
	}
	fprintf(stderr, "feature dimension = %d\n", dimension);
	for (;getLine(in);) {
		vector<string> tokens = splitBy(line, ',');
		string phrase = tokens[0];
		vector<double> features(dimension, 0);
		int ptr = 0;
		for (size_t i = 1; i < tokens.size(); ++ i) {
		    if (forbidFeatSet.count(attributes[i])) {
		        continue;
		    }
			fromString(tokens[i], features[ptr ++]);
		}
		myAssert(ptr == dimension, "ptr exceeds the dimension");
		if (labels.count(phrase)) {
			train.push_back(features);
			trainY.push_back(labels[phrase]);
		}
		candidates.push_back(phrase);
		all.push_back(features);
	}
	fclose(in);
	fprintf(stderr, "%d candidates loaded\n", candidates.size());
	fprintf(stderr, "%d are in labels\n", train.size());
    return dimension;
}

int main(int argc, char* argv[])
{
    double threshold;
	if (argc != 8 || sscanf(argv[5], "%lf", &threshold) != 1 || threshold < 0 || threshold > 1) {
		fprintf(stderr, "[usage] <feature file: final.csv> <label file: DBLP.label> <output: ranking.csv> <forbid features, comma separated> <prune-threshold : [0, 1]> <in-random-forest-model> <out-random-forest-model>\n");
		return -1;
	}
	loadLabels(argv[2]);
	int dimension = loadFeatureMatrix(argv[1], argv[4]);
	
	RandomForest *solver = new RandomForest();
	if (!strcmp(argv[6], "TRAIN")) {
	    fprintf(stderr, "start to train...\n");
	    rng.init();
	    RANDOM_FEATURES = 4;
	    RANDOM_POSITIONS = 8;
	    solver->train(train, trainY, 100, 1);
	} else {
	    fprintf(stderr, "start to load...\n");
	    solver->load(argv[6]);
	}
/*	fprintf(stderr, "=== feature importance ===\n");
    for (size_t i = 0; i < featureNames.size(); ++ i) {
        fprintf(stderr, "%s\t%.10f\n", featureNames[i].c_str(), featureImportance[i]);
    }
	
	fprintf(stderr, "start to dump...\n");*/
	solver->dump(argv[7]);
	
	vector< pair<double, string> > results;
	for (size_t i = 0; i < all.size(); ++ i) {
		double pred = solver->estimate(all[i]);
		results.push_back(make_pair(pred, candidates[i]));
	}
	sort(results.rbegin(), results.rend());
	
	FILE* out = tryOpen(argv[3], "w");
	for (size_t i = 0; i < results.size(); ++ i) {
	    if (results[i].first < threshold) {
	        break;
	    }
		fprintf(out, "%s,%.10f\n", results[i].second.c_str(), results[i].first);
	}
	fclose(out);
	
	return 0;
}

