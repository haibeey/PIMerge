


## compile with for benchmarking and analysis.


`g++-14 -O3 -mavx2 -mfma -I simde/ -pthread -fsanitize=address,leak  -g -o LendoMerge -I/Users/abrahamakerele/p-h/lendostuff/lendomerge/header-files/stitcher -I/usr/local/include -L/Users/abrahamakerele/p-h/lendostuff/lendomerge/libs/stitcher/mac/x86_64 -lNativeStitcher -L/usr/local/lib -lturbojpeg LendoMerge.cpp blur.cpp main.cpp && time ./LendoMerge`
