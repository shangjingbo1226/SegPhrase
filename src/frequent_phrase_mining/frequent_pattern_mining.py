from sets import Set

def frequentPatternMining(tokens, patternOutputFilename, threshold):
    dict = {}

    tokensNumber = len(tokens)
    for i in xrange(tokensNumber):
        token = tokens[i]
        if token == '$':
            continue
        if token in dict:
            dict[token].append(i)
        else:
            dict[token] = [i]
    print "# of distinct tokens = ", len(dict)

    patternOutput = open(patternOutputFilename, 'w')

    frequentPatterns = []
    patternLength = 1
    while (len(dict) > 0):
        if patternLength > 6:
            break
        #print "working on length = ", patternLength
        patternLength += 1
        newDict = {}
        for pattern, positions in dict.items():
            occurrence = len(positions)
            if occurrence >= threshold:
                frequentPatterns.append(pattern)
                
                patternOutput.write(pattern + "," + str(occurrence) + "\n")
                for i in positions:
                    if i + 1 < tokensNumber:
                        if tokens[i + 1] == '$':
                            continue
                        newPattern = pattern + " " + tokens[i + 1]
                        if newPattern in newDict:
                            newDict[newPattern].append(i + 1)
                        else:
                            newDict[newPattern] = [i + 1]
        dict.clear()
        dict = newDict
    patternOutput.close()
    return frequentPatterns
