#include "../utils/helper.h"

const string ENDINGS = ".!?,;:()\"[]";

vector< vector<string> > process(string line)
{
    vector<string> sentences;
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
    vector< vector<string> > ret;
	FOR (sentence, sentences) {
		vector<string> temp = splitBy(*sentence, ' ');
		ret.push_back(temp);
	}
	return ret;
}

void selfTest(string text, string pos)
{
    vector< vector<string> > sentencesTokens = process(text);
    vector< vector<string> > posTokens = process(pos);

    /*cerr << text << endl;
    cerr << "--------------------" << endl;
    cerr << pos << endl;

    cerr << "# sentence = " << sentencesTokens.size() << endl;
    cerr << "# pos = " << posTokens.size() << endl;*/
    myAssert(sentencesTokens.size() == posTokens.size(), "[ERROR] Self checking ERROR 1");
    for (int i = 0; i < sentencesTokens.size(); ++ i) {
        /*cerr << "i = " << i << endl;
        cerr << sentencesTokens[i].size() << endl;
        cerr << posTokens[i].size() << endl;
        for (int j = 0; j < posTokens[i].size(); ++ j) {
            cerr << posTokens[i][j] << " ";
        }
        cerr << endl;
        for (int j = 0; j < sentencesTokens[i].size(); ++ j) {
            cerr << sentencesTokens[i][j] << " ";
        }
        cerr << endl;*/
        myAssert(sentencesTokens[i].size() == posTokens[i].size(), "[ERROR] Self checking ERROR 2");
    }
}

string normalize(string text) {
    string ret = "";
    for (int i = 0; i < text.size(); ++ i) {
        char ch = tolower(text[i]);
        if (ENDINGS.find(ch) != -1) {
            ret += ch;
        } else {
            if (!isalpha(ch)) {
                if (ch == '\'') {
                    ret += ch;
                } else if (ret.size() > 0 && ret[ret.size() - 1] != ' ') {
                    ret += ' ';
                }
            } else {
                ret += text[i];
            }
        }
    }
    return ret;
}

string clean(string s)
{
    string ret = "";
    for (int i = 0; i < s.size(); ++ i) {
        char ch = tolower(s[i]);

        if (!isalpha(ch)) {
            if (ch == '\'') {
                ret += ch;
            }
        } else {
            ret += s[i];
        }
    }
    return ret;
}

pair<string, string> align(string text, string pos)
{
    text = normalize(text);
    stringstream in(pos);
    string keep = "";
    pos = "";
    for (string pos_token;in >> pos_token;) {
        vector<string> pairs = splitBy(pos_token, ':');
        int index = text.find(pairs[0]);
        if (index != -1) {
            for (int i = 0; i < index; ++ i) {
                if (!isalpha(text[i]) && !isdigit(text[i]) && text[i] != '\'') {
                    if (keep.size() > 0 && keep.back() == ' ' && text[i] == ' ') {
                        continue;
                    }
                    if (keep.size() == 0 && text[i] == ' ') {
                        continue;
                    }
                    pos += text[i];
                    keep += text[i];
                }
            }

            text = text.substr(index + pairs[0].size());

            string term = clean(pairs[0]);
            string pos_tag = clean(pairs[1]);
            if (term.size() > 0 && pos_tag.size() > 0) {
                keep += term;
                pos += pos_tag;
            }
        }
    }
    for (int i = 0; i < text.size(); ++ i) {
        if (!isalpha(text[i]) && !isdigit(text[i]) && text[i] != '\'') {
            if (keep.size() > 0 && keep.back() == ' ' && text[i] == ' ') {
                continue;
            }
            if (keep.size() == 0 && text[i] == ' ') {
                continue;
            }
            pos += text[i];
            keep += text[i];
        }
    }

    //selfTest(keep, pos);

    //myAssert(splitBy(pos, ' ').size() == splitBy(keep, ' ').size(), "[ERROR] pos and text are not matched correctly");

    return make_pair(keep, pos);
}

int main(int argc, char* argv[])
{
    if (argc != 5) {
        fprintf(stderr, "[usage] <text-tsv> <pos-tsv> <raw-text-output> <pos-output>\n");
        return -1;
    }

    FILE* in_text = tryOpen(argv[1], "r");
    FILE* in_pos = tryOpen(argv[2], "r");

    FILE* out_text = tryOpen(argv[3], "w");
    FILE* out_pos = tryOpen(argv[4], "w");
    while (getLine(in_text)) {
        string text = line;
        if (text == "PMID	title	abstract") {
            // skip header
            continue;
        }
        myAssert(getLine(in_pos), "[ERROR] lines are not enough in POS");
        string pos = line;
        vector<string> text_tokens = splitBy(text, '\t');
        vector<string> pos_tokens = splitBy(pos, '\t');

        myAssert(text_tokens[0] == pos_tokens[0], "[ERROR] ID mismatched!");

        // title
        pair<string, string> ret = align(text_tokens[1], pos_tokens[1]);
        fprintf(out_text, "%s\n", ret.first.c_str());
        fprintf(out_pos, "%s\n", ret.second.c_str());

        // abstract
        ret = align(text_tokens[2], pos_tokens[2]);
        fprintf(out_text, "%s\n", ret.first.c_str());
        fprintf(out_pos, "%s\n", ret.second.c_str());
    }
    fclose(out_text);
    fclose(out_pos);
    return 0;
}
