#!/bin/bash

# Build
mkdir -p bin
rm -f bin/champsim
./config.sh pythia_lru.json
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
echo "LLC Prefetcher: pythia"
echo "LLC Replacement: lru"
echo "Cores: 1"
BINARY_NAME="perceptron-no-pythia-lru-1core"
echo "Binary: bin/${BINARY_NAME}${NORMAL}"
echo ""
mv bin/champsim bin/${BINARY_NAME}

