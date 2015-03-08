from frequent_pattern_mining import *
import re
import sys

def main(argv):
    ENDINGS = ".!?,;:\"[]"
    
    threshold = 1000
    rawTextInput = 'rawText.txt'
    patternOutputFilename = 'patterns.csv'
    argc = len(argv)
    for i in xrange(argc):
        if argv[i] == "-raw" and i + 1 < argc:
            rawTextInput = argv[i + 1]
        elif argv[i] == "-thres" and i + 1 < argc:
            threshold = int(argv[i + 1])
        elif argv[i] == "-o" and i + 1 < argc:
            patternOutputFilename = argv[i + 1]
    
    raw = open(rawTextInput, 'r');
    tokens = []
    for line in raw:
        inside = 0
        chars = []
        for ch in line:
            if ch == '(':
                inside += 1
            elif ch == ')':
                inside -= 1
            elif inside == 0:
                if ch.isalpha():
                    chars.append(ch.lower())
                elif ch == '\'':
                    chars.append(ch)
                else:
                    if len(chars) > 0:
                        tokens.append(''.join(chars))
                    chars = []
            if ch in ENDINGS:
                tokens.append('$')
        if len(chars) > 0:
            tokens.append(''.join(chars))
            chars = []
        
    print "# tokens = ", len(tokens)

    frequentPatterns = frequentPatternMining(tokens, patternOutputFilename, threshold)

    print "# of frequent pattern = ", len(frequentPatterns)
    
if __name__ == "__main__":
    main(sys.argv[1 : ])
