#!/bin/bash

# Build
mkdir -p bin
rm -f bin/champsim
./config.sh lru_4core.json
make

# Sanity check
echo ""
if [ ! -f bin/champsim ]; then
    echo "${BOLD}ChampSim build FAILED!${NORMAL}"
    echo ""
    exit
fi

echo "${BOLD}ChampSim is successfully built"
echo "Branch Predictor: Perceptron"
echo "L1D Prefetcher: no"
echo "L2C Prefetcher: no"
echo "LLC Replacement: lru"
echo "Cores: 1"
BINARY_NAME="perceptron-no-no-lru-4core"
echo "Binary: bin/${BINARY_NAME}${NORMAL}"
echo ""
mv bin/champsim bin/${BINARY_NAME}
