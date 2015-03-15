./bin/segphrase_parser results/segmentation.model results/unified.csv 150000 ./data/test.txt ./results/parsed.txt
python ./src/online_query/compute_offset.py ./results/parsed.txt ./results/offset.txt
