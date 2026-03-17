#!/bin/bash
# Generate Chapter 4 search-time data with |Upd(w2)| fixed to max keyword volume.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
NOMOS_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
NOMOS_BIN="${NOMOS_ROOT}/build/Nomos"
OUTPUT_DIR="${NOMOS_ROOT}/results/ch4/"
LOG_DIR="${OUTPUT_DIR}/logs-fixed-w2"

mkdir -p "${OUTPUT_DIR}/client_search_time_fixed_w2"
mkdir -p "${OUTPUT_DIR}/server_search_time_fixed_w2"
mkdir -p "${OUTPUT_DIR}/gatekeeper_search_time_fixed_w2"
mkdir -p "${LOG_DIR}"

if [ ! -f "${NOMOS_BIN}" ]; then
    echo "Missing executable: ${NOMOS_BIN}"
    echo "Build first with: cd ${NOMOS_ROOT}/build && cmake .. && cmake --build ."
    exit 1
fi

for dataset in Crime Enron Wiki; do
    echo "Running FixedW2 dataset: ${dataset}"
    "${NOMOS_BIN}" chapter4-client-search-fixed-w2 \
        --dataset "${dataset}" \
        --output-dir "${OUTPUT_DIR}" \
        > "${LOG_DIR}/${dataset}.log" 2>&1
done

echo "Data written to ${OUTPUT_DIR}"
