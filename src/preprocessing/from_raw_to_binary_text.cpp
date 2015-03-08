#include "../utils/helper.h"

const string ENDINGS = ".!?,;:'[]";

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
			char ch = line[i];
			if (ENDINGS.find(ch) != -1) {
				if (sentence.size() > 0) {
					sentences.push_back(sentence);
				}
				sentence = "";
			} else {
				sentence += ch;
			}
		}
		if (sentence.size() > 0) {
			sentences.push_back(sentence);
		}
	}
	fclose(in);
	
	cerr << "# Sentences = " << sentences.size() << endl;
	
	FILE* out = tryOpen(argv[2], "wb");
	
	Binary::write(out, sentences.size());
	FOR (sentence, sentences) {
		Binary::write(out, *sentence);
	}
		
	return 0;
}

