#ifndef __MY_HELPER__
#define __MY_HELPER__

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <algorithm>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <cctype>
#include <sstream>
#include <cmath>
#include <unordered_map>
#include <unordered_set>
using namespace std;

#define FOR(i,a) for (__typeof((a).begin()) i = (a).begin(); i != (a).end(); ++ i)

const double EPS = 1e-8;

/*! \brief return a real numer uniform in (0,1) */
inline double next_double2(){
    return (static_cast<double>( rand() ) + 1.0 ) / (static_cast<double>(RAND_MAX) + 2.0);
}

/*! \brief return  x~N(0,1) */
inline double sample_normal(){
	double x,y,s;
	do{
		x = 2 * next_double2() - 1.0;
		y = 2 * next_double2() - 1.0;
		s = x*x + y*y;
	}while( s >= 1.0 || s == 0.0 );
	
	return x * sqrt( -2.0 * log(s) / s ) ;
}

bool myAssert(bool flg, string msg)
{
	if (!flg) {
		cerr << msg << endl;
		exit(-1);
	}
	return flg;
}

int sign(double x)
{
	return x < -EPS ? -1 : x > EPS;
}

string replaceAll(const string &s, const string &from, const string &to)
{
    string ret = "";
    for (size_t i = 0; i < s.size(); ++ i) {
        bool found = true;
        for (size_t offset = 0; offset < from.size() && found; ++ offset) {
            found &= i + offset < s.size() && s[i + offset] == from[offset];
        }
        if (found) {
            ret += to;
            i += from.size() - 1;
        } else {
            ret += s[i];
        }
    }
    return ret;
}

double sqr(double x)
{
    return x * x;
}

template<class T>
void fromString(const string &s, T &x)
{
	stringstream in(s);
	in >> x;
}

string tolower(const string &a)
{
	string ret = a;
	for (size_t i = 0; i < ret.size(); ++ i) {
		ret[i] = tolower(ret[i]);
	}
	return ret;
}

const int MAX_LENGTH = 100000000;

char line[MAX_LENGTH + 1];

bool getLine(FILE* in)
{
	bool hasNext = fgets(line, MAX_LENGTH, in);
	int length = strlen(line);
	while (length > 0 && (line[length - 1] == '\n' || line[length - 1] == '\r')) {
		-- length;
	}
	line[length] = 0;
	return hasNext;
}

FILE* tryOpen(const string &filename, const string &param)
{
	FILE* ret = fopen(filename.c_str(), param.c_str());
	if (ret == NULL) {
		cerr << "[Warning] failed to open " << filename  << " under parameters = " << param << endl;
	}
	return ret;
}

vector<string> splitBy(const string &line, char sep)
{
	vector<string> tokens;
	string token = "";
	for (size_t i = 0; i < line.size(); ++ i) {
		if (line[i] == sep) {
		    if (token != "") {
    			tokens.push_back(token);
			}
			token = "";
		} else {
			token += line[i];
		}
	}
	if (token != "") {
    	tokens.push_back(token);
	}
	return tokens;
}

namespace Binary
{
	void write(FILE* out, const size_t &size) {
		fwrite(&size, sizeof(size), 1, out);
	}
	
	void write(FILE* out, const string &s) {
		write(out, s.size());
		if (s.size() > 0) {
			fwrite(&s[0], sizeof(char), s.size(), out);
		}
	}
	
	void read(FILE* in, size_t &size) {
		fread(&size, sizeof(size), 1, in);
	}
	
	void read(FILE* in, string &s) {
		size_t size;
		read(in, size);
		s.resize(size);
		if (size > 0) {
			fread(&s[0], sizeof(char), size, in);
		}
	}
}

#endif

