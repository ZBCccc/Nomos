#!/usr/bin/env python3
"""
plot_results.py
为硕士论文第四章生成实验图表
"""

import pandas as pd
import matplotlib.pyplot as plt
import matplotlib
import os
import glob

# 设置中文字体支持
matplotlib.rcParams['font.sans-serif'] = ['Arial Unicode MS', 'SimHei', 'DejaVu Sans']
matplotlib.rcParams['axes.unicode_minus'] = False

# 设置论文风格
plt.style.use('seaborn-v0_8-paper')
matplotlib.rcParams['figure.figsize'] = (8, 6)
matplotlib.rcParams['font.size'] = 12

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
RESULTS_DIR = os.path.join(SCRIPT_DIR, '..', 'results')
OUTPUT_DIR = RESULTS_DIR

def load_experiment_data(pattern):
    """加载实验数据"""
    files = glob.glob(os.path.join(RESULTS_DIR, pattern))
    if not files:
        print(f"警告: 未找到匹配 {pattern} 的文件")
        return None

    dfs = []
    for file in sorted(files):
        df = pd.read_csv(file)
        dfs.append(df)

    return pd.concat(dfs, ignore_index=True)

def plot_scalability():
    """绘制可扩展性图表 (变化 N)"""
    print("绘制可扩展性图表...")

    df = load_experiment_data('exp1_N*.csv')
    if df is None:
        return

    # 按方案分组
    schemes = df['Scheme'].unique()

    # 绘制 Update 时间
    plt.figure(figsize=(10, 6))
    for scheme in schemes:
        scheme_data = df[df['Scheme'] == scheme]
        plt.plot(scheme_data['N'], scheme_data['UpdateTime'],
                marker='o', label=scheme, linewidth=2)

    plt.xlabel('Number of Files (N)', fontsize=14)
    plt.ylabel('Update Time (ms)', fontsize=14)
    plt.title('Update Performance vs. Dataset Size', fontsize=16)
    plt.legend(fontsize=12)
    plt.grid(True, alpha=0.3)
    plt.tight_layout()
    plt.savefig(os.path.join(OUTPUT_DIR, 'update-time.pdf'), dpi=300)
    plt.close()
    print(f"  保存: {os.path.join(OUTPUT_DIR, 'update-time.pdf')}")

    # 绘制 Search 时间
    plt.figure(figsize=(10, 6))
    for scheme in schemes:
        scheme_data = df[df['Scheme'] == scheme]
        plt.plot(scheme_data['N'], scheme_data['SearchTime'],
                marker='s', label=scheme, linewidth=2)

    plt.xlabel('Number of Files (N)', fontsize=14)
    plt.ylabel('Search Time (ms)', fontsize=14)
    plt.title('Search Performance vs. Dataset Size', fontsize=16)
    plt.legend(fontsize=12)
    plt.grid(True, alpha=0.3)
    plt.tight_layout()
    plt.savefig(os.path.join(OUTPUT_DIR, 'search-time.pdf'), dpi=300)
    plt.close()
    print(f"  保存: {os.path.join(OUTPUT_DIR, 'search-time.pdf')}")

    # 绘制存储开销
    plt.figure(figsize=(10, 6))
    for scheme in schemes:
        scheme_data = df[df['Scheme'] == scheme]
        plt.plot(scheme_data['N'], scheme_data['Storage'] / 1024,
                marker='^', label=scheme, linewidth=2)

    plt.xlabel('Number of Files (N)', fontsize=14)
    plt.ylabel('Storage (KB)', fontsize=14)
    plt.title('Storage Overhead vs. Dataset Size', fontsize=16)
    plt.legend(fontsize=12)
    plt.grid(True, alpha=0.3)
    plt.tight_layout()
    plt.savefig(os.path.join(OUTPUT_DIR, 'storage.pdf'), dpi=300)
    plt.close()
    print(f"  保存: {os.path.join(OUTPUT_DIR, 'storage.pdf')}")

def plot_comparison_bar():
    """绘制三方案对比柱状图"""
    print("绘制对比柱状图...")

    df = load_experiment_data('exp1_N1000.csv')
    if df is None:
        return

    schemes = df['Scheme'].tolist()
    update_times = df['UpdateTime'].tolist()
    search_times = df['SearchTime'].tolist()

    x = range(len(schemes))
    width = 0.35

    fig, ax = plt.subplots(figsize=(10, 6))
    ax.bar([i - width/2 for i in x], update_times, width, label='Update', alpha=0.8)
    ax.bar([i + width/2 for i in x], search_times, width, label='Search', alpha=0.8)

    ax.set_xlabel('Scheme', fontsize=14)
    ax.set_ylabel('Time (ms)', fontsize=14)
    ax.set_title('Performance Comparison (N=1000)', fontsize=16)
    ax.set_xticks(x)
    ax.set_xticklabels(schemes)
    ax.legend(fontsize=12)
    ax.grid(True, alpha=0.3, axis='y')

    plt.tight_layout()
    plt.savefig(os.path.join(OUTPUT_DIR, 'comparison-bar.pdf'), dpi=300)
    plt.close()
    print(f"  保存: {os.path.join(OUTPUT_DIR, 'comparison-bar.pdf')}")

def main():
    print("=== 实验数据绘图脚本 ===")
    print(f"结果目录: {RESULTS_DIR}")
    print(f"输出目录: {OUTPUT_DIR}")
    print()

    # 检查结果目录
    if not os.path.exists(RESULTS_DIR):
        print(f"错误: 结果目录不存在: {RESULTS_DIR}")
        print("请先运行实验脚本: bash scripts/generate_experiment_data.sh")
        return

    # 绘制图表
    plot_scalability()
    plot_comparison_bar()

    print()
    print("=== 绘图完成 ===")
    print(f"图表保存在: {OUTPUT_DIR}")

if __name__ == '__main__':
    main()
