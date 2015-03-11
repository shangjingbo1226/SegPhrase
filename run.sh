#!/bin/bash

export PYTHON=python

RAW_TEXT=../DBLPLexiconCreation/data/DBLP.txt
AUTO_LABEL=1
DATA_LABEL=data/DBLP.label.auto
KNOWLEDGE_BASE=data/wiki_labels_quality.txt

STOPWORD_LIST=data/stopwords.txt
SUPPORT_THRESHOLD=30

OMP_NUM_THREADS=10
DISCARD_RATIO=0.05
MAX_ITERATION=5

ALPHA=0.5

SLIDING_WINDOW=10
SLIDING_THRES=0.5

# clearance
rm -rf tmp
rm -rf results

mkdir tmp
mkdir results

# preprocessing
./bin/from_raw_to_binary_text ${RAW_TEXT} tmp/sentencesWithPunc.buf

# frequent phrase mining for phrase candidates
${PYTHON} ./src/frequent_phrase_mining/main.py -thres ${SUPPORT_THRESHOLD} -o ./results/patterns.csv -raw ${RAW_TEXT}
${PYTHON} ./src/preprocessing/compute_idf.py -raw ${RAW_TEXT} -o results/wordIDF.txt

# feature extraction
./bin/feature_extraction tmp/sentencesWithPunc.buf results/patterns.csv ${STOPWORD_LIST} results/wordIDF.txt results/feature_table_0.csv

if [ ${AUTO_LABEL} -eq 1 ];
then
    ${PYTHON} src/classification/auto_label_generation.py ${KNOWLEDGE_BASE} results/feature_table_0.csv ${DATA_LABEL}
fi

# classifier training
./bin/predict_quality results/feature_table_0.csv ${DATA_LABEL} results/ranking.csv outsideSentence,log_occur_feature,constant,frequency 0 TRAIN results/random_forest_0.model

MAX_ITERATION_1=$(expr $MAX_ITERATION + 1)

# 1-st round
./bin/from_raw_to_binary ${RAW_TEXT} tmp/sentences.buf
./bin/adjust_probability tmp/sentences.buf ${OMP_NUM_THREADS} results/ranking.csv results/patterns.csv ${DISCARD_RATIO} ${MAX_ITERATION} ./results/ ${DATA_LABEL} ./results/penalty.1

# 2-nd round
./bin/recompute_features results/iter${MAX_ITERATION_1}_discard${DISCARD_RATIO}/length results/feature_table_0.csv results/patterns.csv tmp/sentencesWithPunc.buf results/feature_table_1.csv ./results/penalty.1 1
./bin/predict_quality results/feature_table_1.csv ${DATA_LABEL} results/ranking_1.csv outsideSentence,log_occur_feature,constant,frequency 0 TRAIN results/random_forest_1.model
./bin/adjust_probability tmp/sentences.buf ${OMP_NUM_THREADS} results/ranking_1.csv results/patterns.csv ${DISCARD_RATIO} ${MAX_ITERATION} ./results/1. ${DATA_LABEL} ./results/penalty.2

# phrase list & segmentation model
./bin/prune_and_combine results/1.iter${MAX_ITERATION_1}_discard${DISCARD_RATIO}/length ${SLIDING_WINDOW} ${SLIDING_THRES} results/phrase_list.txt DET results/phrase_list.stat
./bin/build_model results/1.iter${MAX_ITERATION_1}_discard${DISCARD_RATIO}/ 6 ./results/penalty.2 results/segmentation.model

# unigrams
normalize_text() {
  awk '{print tolower($0);}' | sed -e "s/’/'/g" -e "s/′/'/g" -e "s/''/ /g" -e "s/'/ ' /g" -e "s/“/\"/g" -e "s/”/\"/g" \
  -e 's/"/ " /g' -e 's/\./ \. /g' -e 's/<br \/>/ /g' -e 's/, / , /g' -e 's/(/ ( /g' -e 's/)/ ) /g' -e 's/\!/ \! /g' \
  -e 's/\?/ \? /g' -e 's/\;/ /g' -e 's/\:/ /g' -e 's/-/ - /g' -e 's/=/ /g' -e 's/=/ /g' -e 's/*/ /g' -e 's/|/ /g' \
  -e 's/«/ /g' | tr 0-9 " "
}
normalize_text < results/1.iter${MAX_ITERATION}_discard${DISCARD_RATIO}/segmented.txt > tmp/normalized.txt

cd word2vec_tool
make
cd ..
./word2vec_tool/word2vec -train tmp/normalized.txt -output ./results/vectors.bin -cbow 2 -size 300 -window 6 -negative 25 -hs 0 -sample 1e-4 -threads 10 -binary 1 -iter  15
./bin/qualify_unigrams results/vectors.bin results/1.iter${MAX_ITERATION_1}_discard${DISCARD_RATIO}/ results/unigrams.csv 0 ${ALPHA}

