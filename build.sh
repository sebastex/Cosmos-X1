#!/usr/bin/env bash
# Builds Cosmos X1 with g++ on Linux (used by the GitHub Actions workflow).
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p build
g++ -std=c++20 -O3 -march=native -fopenmp -Wall -Wextra -I include src/*.cpp -o "build/${1:-cosmos_x1}"
echo "Built build/${1:-cosmos_x1}"
