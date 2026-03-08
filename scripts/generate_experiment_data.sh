#!/bin/bash
# generate_experiment_data.sh
# 为硕士论文第四章生成实验数据

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
NOMOS_BIN="${SCRIPT_DIR}/../build/Nomos"
OUTPUT_DIR="${SCRIPT_DIR}/../results"

# 创建输出目录
mkdir -p "${OUTPUT_DIR}"

echo "=== 实验数据生成脚本 ==="
echo "Nomos 可执行文件: ${NOMOS_BIN}"
echo "输出目录: ${OUTPUT_DIR}"
echo ""

# 检查 Nomos 可执行文件
if [ ! -f "${NOMOS_BIN}" ]; then
    echo "错误: 找不到 Nomos 可执行文件"
    echo "请先运行: cd build && cmake .. && cmake --build ."
    exit 1
fi

# 实验 1: 可扩展性测试 (变化 N - 文件数量)
echo "=== 实验 1: 可扩展性 (变化 N) ==="
for N in 100 500 1000 5000 10000; do
    echo "  运行 N=${N}..."
    "${NOMOS_BIN}" comparative-benchmark > "${OUTPUT_DIR}/exp1_N${N}.log" 2>&1
    # 复制生成的 CSV 文件
    if [ -f "comparative_results.csv" ]; then
        mv comparative_results.csv "${OUTPUT_DIR}/exp1_N${N}.csv"
    fi
done

echo ""
echo "=== 实验完成 ==="
echo "结果保存在: ${OUTPUT_DIR}"
echo ""
echo "下一步: 运行绘图脚本"
echo "  python3 ${SCRIPT_DIR}/plot_results.py"
