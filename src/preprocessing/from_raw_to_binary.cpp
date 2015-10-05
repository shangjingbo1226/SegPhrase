#include "../utils/helper.h"
#include <cassert>

const string ENDINGS = ".!?,;:()\"[]";

int main(int argc, char* argv[])
{
	if (argc != 3) {
		cerr << "[Usage] <input-raw-text-file> <output-binary-file>" << endl;
		return -1;
	}
	
	FILE* in = tryOpen(argv[1], "r");
	vector<string> sentences;
	for (;getLine(in);) {
		string sentence = "";
		for (int i = 0; line[i]; ++ i) {
			char ch = tolower(line[i]);
			if (ENDINGS.find(ch) != -1) {
				if (sentence.size() > 0) {
					sentences.push_back(sentence);
				}
				sentence = "";
			} else {
				if (!isalpha(ch)) {
                    if (ch == '\'') {
                        sentence += ch;
                    } else if (sentence.size() > 0 && sentence[sentence.size() - 1] != ' ') {
						sentence += ' ';
					}
				} else {
					sentence += ch;
				}
			}
		}
		if (sentence.size() > 0) {
			sentences.push_back(sentence);
		}
	}
	fclose(in);
	
	cerr << "# Sentences = " << sentences.size() << endl;
	
	unordered_set<string> tokens;
	FOR (sentence, sentences) {
		vector<string> temp = splitBy(*sentence, ' ');
		FOR(iter, temp) {
			tokens.insert(*iter);
		}
        assert(tokens.size() != 0);
	}
	vector<string> unigrams(tokens.begin(), tokens.end());
	
	cerr << "# Unigrams = " << unigrams.size() << endl;
	
	FILE* out = tryOpen(argv[2], "wb");
	
	Binary::write(out, sentences.size());
	FOR (sentence, sentences) {
		Binary::write(out, *sentence);
	}
	
	Binary::write(out, unigrams.size());
	FOR (unigram, unigrams) {
		Binary::write(out, *unigram);
	}
	fclose(out);
	
	return 0;
}

