# Chapter 4 客户端搜索时间实验运行指南

## 实验概述

本实验测量三个方案（Nomos、MC-ODXT、VQNomos）在固定 |Upd(w1)| = 10 条件下，随 |Upd(w2)| 变化的客户端搜索时间。

### 实验参数

- **固定参数**: |Upd(w1)| = 10
- **变化参数**: |Upd(w2)| 遍历数据集中所有唯一频率值
- **测试方案**: Nomos、MC-ODXT、VQNomos（均为实际测量）
- **数据集**: Crime、Enron、Wiki
- **重复次数**: 1 次（默认）
- **QTree 容量**: 1024

### 测量内容

每个方案测量客户端搜索的三个阶段：

1. **Token 生成**: `client.genToken()` + `gatekeeper.genToken()`
2. **搜索准备**: `client.prepareSearch()`
3. **解密/验证**:
   - Nomos/MC-ODXT: `client.decryptResults()`
   - VQNomos: `client.decryptAndVerify()` (包含 QTree 验证)

## 完整测试脚本

### 方式一：运行所有数据集

```bash
#!/bin/bash
# 运行所有三个数据集的完整实验

cd /Users/cyan/code/paper/Nomos/build

# 确保代码是最新编译的
cmake --build .

# 运行实验（默认 repeat=1）
./Nomos chapter4-client-search-fixed-w1 --dataset all

echo "实验完成！"
echo "结果保存在: /Users/cyan/code/paper/Nomos/results/ch4/client_search_time_fixed_w1/"
```

### 方式二：逐个数据集运行

```bash
#!/bin/bash
# 分别运行每个数据集，便于监控进度

cd /Users/cyan/code/paper/Nomos/build

# 确保代码是最新编译的
cmake --build .

# Crime 数据集 (63,659 关键词, 1,624 个数据点)
echo "=== 开始 Crime 数据集 ==="
./Nomos chapter4-client-search-fixed-w1 --dataset Crime --repeat 1
echo "Crime 数据集完成"

# Enron 数据集 (16,243 关键词)
echo "=== 开始 Enron 数据集 ==="
./Nomos chapter4-client-search-fixed-w1 --dataset Enron --repeat 1
echo "Enron 数据集完成"

# Wiki 数据集 (10,001 关键词)
echo "=== 开始 Wiki 数据集 ==="
./Nomos chapter4-client-search-fixed-w1 --dataset Wiki --repeat 1
echo "Wiki 数据集完成"

echo "所有实验完成！"
echo "结果保存在: /Users/cyan/code/paper/Nomos/results/ch4/client_search_time_fixed_w1/"
```

### 方式三：自定义参数运行

```bash
#!/bin/bash
# 自定义输出目录和重复次数

cd /Users/cyan/code/paper/Nomos/build

# 自定义参数
DATASET="Crime"           # 可选: Crime, Enron, Wiki, all
REPEAT=1                  # 重复次数
OUTPUT_DIR="/Users/cyan/code/paper/Nomos/results/ch4/client_search_time_fixed_w1"

# 运行实验
./Nomos chapter4-client-search-fixed-w1 \
    --dataset "$DATASET" \
    --repeat "$REPEAT" \
    --output-dir "$OUTPUT_DIR"

echo "实验完成！结果保存在: $OUTPUT_DIR"
```

### 方式四：后台运行（推荐用于长时间实验）

```bash
#!/bin/bash
# 后台运行实验，输出重定向到日志文件

cd /Users/cyan/code/paper/Nomos/build

# 创建日志目录
LOG_DIR="/Users/cyan/code/paper/Nomos/logs"
mkdir -p "$LOG_DIR"

# 获取时间戳
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
LOG_FILE="$LOG_DIR/experiment_$TIMESTAMP.log"

# 后台运行
nohup ./Nomos chapter4-client-search-fixed-w1 --dataset all --repeat 1 \
    > "$LOG_FILE" 2>&1 &

PID=$!
echo "实验已在后台启动，PID: $PID"
echo "日志文件: $LOG_FILE"
echo ""
echo "查看实时日志: tail -f $LOG_FILE"
echo "检查进程状态: ps aux | grep $PID"
echo "停止实验: kill $PID"
```

## 输出文件

实验会在输出目录生成 9 个 CSV 文件：

```
/Users/cyan/code/paper/Nomos/results/ch4/client_search_time_fixed_w1/
├── Nomos_Crime.csv
├── Nomos_Enron.csv
├── Nomos_Wiki.csv
├── MC-ODXT_Crime.csv
├── MC-ODXT_Enron.csv
├── MC-ODXT_Wiki.csv
├── VQNomos_Crime.csv
├── VQNomos_Enron.csv
└── VQNomos_Wiki.csv
```

### CSV 文件格式

每个 CSV 文件包含以下列：

```csv
dataset,scheme,upd_w1,upd_w2,client_search_time_ms,repeat
Crime,Nomos,10,1,0.523,1
Crime,Nomos,10,2,0.687,1
Crime,Nomos,10,3,0.845,1
...
```

**列说明**：
- `dataset`: 数据集名称 (Crime/Enron/Wiki)
- `scheme`: 方案名称 (Nomos/MC-ODXT/VQNomos)
- `upd_w1`: 固定为 10
- `upd_w2`: 当前测试的 |Upd(w2)| 值
- `client_search_time_ms`: 客户端搜索时间（毫秒）
- `repeat`: 重复次数（默认为 1）

## 预期运行时间

基于每个数据集的数据点数量和方案数量：

| 数据集 | 数据点数 | 方案数 | 预估时间 (repeat=1) |
|--------|---------|--------|-------------------|
| Crime  | ~1,624  | 3      | ~2-3 小时         |
| Enron  | ~800    | 3      | ~1-1.5 小时       |
| Wiki   | ~600    | 3      | ~45-60 分钟       |
| **总计** | ~3,024  | 3      | **~4-5 小时**     |

**注意**：
- VQNomos 包含 QTree 验证，比 Nomos 慢约 20-30%
- MC-ODXT 性能与 Nomos 相近
- 实际时间取决于硬件性能

## 监控实验进度

### 方法一：查看控制台输出

实验会每处理 250 个数据点输出一次进度：

```
[ClientSearchFixedW1] Dataset 1/3: Crime (1624 points)
  Processed 250/1624 points
  Processed 500/1624 points
  Processed 750/1624 points
  ...
```

### 方法二：检查输出文件

```bash
# 查看已生成的 CSV 文件
ls -lh /Users/cyan/code/paper/Nomos/results/ch4/client_search_time_fixed_w1/

# 查看某个 CSV 文件的行数（数据点数）
wc -l /Users/cyan/code/paper/Nomos/results/ch4/client_search_time_fixed_w1/Nomos_Crime.csv
```

### 方法三：实时监控日志（后台运行时）

```bash
# 实时查看日志
tail -f /Users/cyan/code/paper/Nomos/logs/experiment_*.log

# 查看最近的进度信息
tail -20 /Users/cyan/code/paper/Nomos/logs/experiment_*.log | grep "Processed"
```

## 故障排查

### 问题 1：编译失败

```bash
cd /Users/cyan/code/paper/Nomos/build
rm -rf *
cmake ..
cmake --build .
```

### 问题 2：RELIC 初始化失败

确保 RELIC 库已正确安装：

```bash
brew install relic
```

### 问题 3：内存不足

如果遇到内存问题，可以分别运行每个数据集：

```bash
./Nomos chapter4-client-search-fixed-w1 --dataset Crime --repeat 1
# 等待完成后再运行下一个
./Nomos chapter4-client-search-fixed-w1 --dataset Enron --repeat 1
./Nomos chapter4-client-search-fixed-w1 --dataset Wiki --repeat 1
```

### 问题 4：进程意外终止

检查日志文件中的错误信息：

```bash
tail -50 /Users/cyan/code/paper/Nomos/logs/experiment_*.log
```

## 数据验证

实验完成后，验证输出数据：

```bash
#!/bin/bash
# 验证脚本

RESULT_DIR="/Users/cyan/code/paper/Nomos/results/ch4/client_search_time_fixed_w1"

echo "=== 检查输出文件 ==="
for scheme in Nomos MC-ODXT VQNomos; do
    for dataset in Crime Enron Wiki; do
        file="$RESULT_DIR/${scheme}_${dataset}.csv"
        if [ -f "$file" ]; then
            lines=$(wc -l < "$file")
            echo "✓ $file: $((lines-1)) 数据点"
        else
            echo "✗ 缺失: $file"
        fi
    done
done

echo ""
echo "=== 检查数据完整性 ==="
for file in "$RESULT_DIR"/*.csv; do
    if [ -f "$file" ]; then
        # 检查是否有空行或格式错误
        if grep -q "^$" "$file"; then
            echo "⚠ 警告: $file 包含空行"
        fi
        # 检查列数是否一致
        awk -F',' 'NR>1 && NF!=6 {print "⚠ 警告: " FILENAME " 第 " NR " 行列数不正确"; exit}' "$file"
    fi
done

echo ""
echo "验证完成！"
```

## 数据分析示例

使用 Python 快速查看结果：

```python
import pandas as pd
import matplotlib.pyplot as plt

# 读取数据
nomos_crime = pd.read_csv('results/ch4/client_search_time_fixed_w1/Nomos_Crime.csv')
mcodxt_crime = pd.read_csv('results/ch4/client_search_time_fixed_w1/MC-ODXT_Crime.csv')
vqnomos_crime = pd.read_csv('results/ch4/client_search_time_fixed_w1/VQNomos_Crime.csv')

# 绘制对比图
plt.figure(figsize=(12, 6))
plt.plot(nomos_crime['upd_w2'], nomos_crime['client_search_time_ms'],
         label='Nomos', marker='o', markersize=2)
plt.plot(mcodxt_crime['upd_w2'], mcodxt_crime['client_search_time_ms'],
         label='MC-ODXT', marker='s', markersize=2)
plt.plot(vqnomos_crime['upd_w2'], vqnomos_crime['client_search_time_ms'],
         label='VQNomos', marker='^', markersize=2)

plt.xlabel('|Upd(w2)|')
plt.ylabel('Client Search Time (ms)')
plt.title('Client Search Time vs |Upd(w2)| (Crime Dataset, |Upd(w1)|=10)')
plt.legend()
plt.grid(True, alpha=0.3)
plt.savefig('client_search_comparison_crime.png', dpi=300, bbox_inches='tight')
plt.show()

# 统计摘要
print("=== Nomos ===")
print(nomos_crime['client_search_time_ms'].describe())
print("\n=== MC-ODXT ===")
print(mcodxt_crime['client_search_time_ms'].describe())
print("\n=== VQNomos ===")
print(vqnomos_crime['client_search_time_ms'].describe())
```

## 重要提示

1. **首次运行前**：确保已编译最新代码 `cmake --build .`
2. **长时间实验**：建议使用后台运行方式（方式四）
3. **磁盘空间**：确保有足够空间存储结果（约 10-20 MB）
4. **系统资源**：实验期间避免运行其他高负载任务
5. **数据备份**：实验完成后及时备份结果文件

## 快速开始

最简单的运行方式：

```bash
cd /Users/cyan/code/paper/Nomos/build
cmake --build .
./Nomos chapter4-client-search-fixed-w1 --dataset all
```

---

**最后更新**: 2026-03-15
**实验版本**: v1.0 (真实测量模式，repeat=1)
