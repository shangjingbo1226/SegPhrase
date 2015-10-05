import sys

def main(argv):
    if len(argv) != 4: 
        print '[usage] <ranking list, e.g. unified.csv> <segmented corpus, _ joined> <support_threshold> <filtered output>'
        exit(-1)
    ranking_list_filename = argv[0]
    segmented_corpus_filename = argv[1]
    sigma = int(argv[2])
    filtered_output_filename = argv[3]
    
    support = {}
    for line in open(ranking_list_filename):
        lexicon = line.split(',')[0]
        key ='_'.join(lexicon.lower().split(' '))
        support[key] = 0
    for line in open(segmented_corpus_filename):
        tokens = line.split()
        for token in tokens:
            if token in support:
                support[token] += 1
    out = open(filtered_output_filename, 'w')
    filtered_cnt = 0
    keep_cnt = 0
    for line in open(ranking_list_filename):
        lexicon = line.split(',')[0]
        key ='_'.join(lexicon.lower().split(' '))
        if support[key] >= sigma:
            keep_cnt += 1
            out.write(line)
        else:
            filtered_cnt += 1
            #print 'filtered: ', lexicon, support[key]
    print 'done. filtered_cnt =', filtered_cnt, 'keep_cnt =', keep_cnt

if __name__ == '__main__':
    main(sys.argv[1:])
