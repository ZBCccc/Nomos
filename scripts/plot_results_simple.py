#!/usr/bin/env python3
"""
plot_results_simple.py
简化版绘图脚本，不依赖 pandas
"""

import csv
import os
import glob

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
RESULTS_DIR = os.path.join(SCRIPT_DIR, '..', 'results')
OUTPUT_DIR = RESULTS_DIR

def load_csv(filename):
    """加载 CSV 文件"""
    data = []
    with open(filename, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            data.append(row)
    return data

def generate_summary():
    """生成实验数据摘要"""
    print("=== 实验数据摘要 ===")
    print()

    # 加载所有实验数据
    files = sorted(glob.glob(os.path.join(RESULTS_DIR, 'exp1_N*.csv')))
    if not files:
        print("错误: 未找到实验数据文件")
        return

    print("文件数量 (N) | 方案 | Setup (ms) | Update (ms) | Search (ms) | Storage (bytes)")
    print("-" * 90)

    for file in files:
        # 从文件名提取 N 值
        basename = os.path.basename(file)
        N = basename.split('N')[1].split('.')[0]

        data = load_csv(file)
        for row in data:
            print(f"{N:>12} | {row['Scheme']:>10} | {float(row['SetupTime']):>10.3f} | "
                  f"{float(row['UpdateTime']):>11.3f} | {float(row['SearchTime']):>11.3f} | "
                  f"{int(row['Storage']):>14}")

    print()
    print("=== 性能对比 (N=1000) ===")
    print()

    # 找到 N=1000 的数据
    target_file = os.path.join(RESULTS_DIR, 'exp1_N1000.csv')
    if os.path.exists(target_file):
        data = load_csv(target_file)

        print("方案对比:")
        for row in data:
            print(f"  {row['Scheme']:>10}: Update={float(row['UpdateTime']):.3f}ms, "
                  f"Search={float(row['SearchTime']):.3f}ms, "
                  f"Storage={int(row['Storage'])/1024:.2f}KB")

        # 计算相对性能
        print()
        print("相对性能 (以 Nomos 为基准):")
        nomos_update = float(data[0]['UpdateTime'])
        nomos_search = float(data[0]['SearchTime'])

        for row in data[1:]:
            update_ratio = float(row['UpdateTime']) / nomos_update
            search_ratio = float(row['SearchTime']) / nomos_search
            print(f"  {row['Scheme']:>10}: Update={update_ratio:.2f}x, Search={search_ratio:.2f}x")

    print()
    print("=== 数据文件 ===")
    for file in files:
        print(f"  {os.path.basename(file)}")

    print()
    print("注意: 图表生成需要 matplotlib 库")
    print("安装命令: pip3 install matplotlib pandas")

def main():
    print("=== 实验数据分析脚本 ===")
    print(f"结果目录: {RESULTS_DIR}")
    print()

    if not os.path.exists(RESULTS_DIR):
        print(f"错误: 结果目录不存在: {RESULTS_DIR}")
        return

    generate_summary()

if __name__ == '__main__':
    main()
