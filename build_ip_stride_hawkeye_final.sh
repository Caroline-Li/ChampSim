#!/bin/bash

# Build
mkdir -p bin
rm -f bin/champsim
./config.sh ip_stride_hawkeye_final.json
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
echo "L2C Prefetcher: ip_stride"
echo "LLC Replacement: hawkeye_final"
echo "Cores: 1"
BINARY_NAME="perceptron-no-ip_stride-hawkeye_final-1core"
echo "Binary: bin/${BINARY_NAME}${NORMAL}"
echo ""
mv bin/champsim bin/${BINARY_NAME}
