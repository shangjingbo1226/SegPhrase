export PYTHON = pypy
export CC  = gcc
export CXX = g++
export CFLAGS = -std=c++11 -Wall -O3 -msse2  -fopenmp  -I..

BIN = ./bin/from_raw_to_binary ./bin/from_raw_to_binary_text ./bin/feature_extraction ./bin/predict_quality ./bin/adjust_probability ./bin/recompute_features ./bin/prune_and_combine ./bin/build_model ./bin/qualify_unigrams ./bin/segphrase_parser ./bin/generateNN ./bin/combine_phrases
.PHONY: clean all

all: ./bin $(BIN)

./bin/from_raw_to_binary: ./src/preprocessing/from_raw_to_binary.cpp ./src/utils/helper.h
./bin/from_raw_to_binary_text: ./src/preprocessing/from_raw_to_binary_text.cpp ./src/utils/helper.h
./bin/feature_extraction: ./src/classification/feature_extraction.cpp ./src/utils/helper.h ./src/classification/aho_corasick.h
./bin/predict_quality: ./src/classification/predict_quality.cpp ./src/utils/helper.h ./src/classification/random_forest.h
./bin/adjust_probability: ./src/model_training/adjust_probability.cpp ./src/utils/helper.h
./bin/recompute_features: ./src/model_training/recompute_features.cpp ./src/utils/helper.h
./bin/prune_and_combine: ./src/postprocessing/prune_and_combine.cpp ./src/utils/helper.h
./bin/build_model: ./src/postprocessing/build_model.cpp ./src/utils/helper.h
./bin/qualify_unigrams: ./src/postprocessing/qualify_unigrams.cpp ./src/utils/helper.h
./bin/segphrase_parser: ./src/online_query/segphrase_parser.cpp ./src/utils/helper.h ./src/online_query/segphrase_parser.h
./bin/generateNN: ./src/postprocessing/generateNN.cpp ./src/utils/helper.h ./src/postprocessing/kd_tree.h
./bin/combine_phrases: ./src/postprocessing/combine_phrases.cpp ./src/utils/helper.h

./bin:
	mkdir bin

export LDFLAGS= -pthread -lm -Wno-unused-result -Wno-sign-compare -Wno-unused-variable -Wno-parentheses -Wno-format
$(BIN) :
	$(CXX) $(CFLAGS) $(LDFLAGS) -o $@ $(filter %.cpp %.o %.c, $^)
$(OBJ) :
	$(CXX) -c $(CFLAGS) -o $@ $(firstword $(filter %.cpp %.c, $^) )

clean :
	rm -rf bin
