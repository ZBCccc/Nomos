# 实验运行指南 - Client Search Time Fixed W2

## 实验概述

本实验测量三个方案（Nomos、MC-ODXT、VQNomos）在固定 |Upd(w2)| 为最大关键词更新次数时的客户端搜索时间开销。

### 实验设置

- **固定参数**: |Upd(w2)| = 数据集中最大的关键词更新次数
- **变化参数**: |Upd(w1)| 从数据集最小更新次数递增到最大更新次数
- **测量指标**: 客户端搜索时间（毫秒）
- **重复次数**: 默认 1 次（可通过 `--repeat` 参数调整）

### 数据集

1. **Crime**: 63,659 个关键词，|Upd(w2)| = 16,644
2. **Enron**: 16,243 个关键词，|Upd(w2)| = 最大更新次数
3. **Wiki**: 10,001 个关键词，|Upd(w2)| = 最大更新次数

## 运行方法

### 方法 1: 运行所有数据集（默认）

```bash
cd /Users/cyan/code/paper/Nomos/build
./Nomos chapter4-client-search-fixed-w2
```

### 方法 2: 运行单个数据集

```bash
# Crime 数据集
./Nomos chapter4-client-search-fixed-w2 --dataset Crime

# Enron 数据集
./Nomos chapter4-client-search-fixed-w2 --dataset Enron

# Wiki 数据集
./Nomos chapter4-client-search-fixed-w2 --dataset Wiki
```

### 方法 3: 自定义重复次数

```bash
# 重复 3 次测量
./Nomos chapter4-client-search-fixed-w2 --repeat 3

# 单个数据集重复 5 次
./Nomos chapter4-client-search-fixed-w2 --dataset Crime --repeat 5
```

### 方法 4: 自定义输出目录

```bash
./Nomos chapter4-client-search-fixed-w2 --output-dir /path/to/output
```

## 输出文件

实验结果保存在 `results/ch4/client_search_time_fixed_w2/` 目录下：

- `Nomos_Crime.csv` - Nomos 方案在 Crime 数据集上的结果
- `MC-ODXT_Crime.csv` - MC-ODXT 方案在 Crime 数据集上的结果
- `VQNomos_Crime.csv` - VQNomos 方案在 Crime 数据集上的结果
- `Nomos_Enron.csv` - Nomos 方案在 Enron 数据集上的结果
- `MC-ODXT_Enron.csv` - MC-ODXT 方案在 Enron 数据集上的结果
- `VQNomos_Enron.csv` - VQNomos 方案在 Enron 数据集上的结果
- `Nomos_Wiki.csv` - Nomos 方案在 Wiki 数据集上的结果
- `MC-ODXT_Wiki.csv` - MC-ODXT 方案在 Wiki 数据集上的结果
- `VQNomos_Wiki.csv` - VQNomos 方案在 Wiki 数据集上的结果

### CSV 文件格式

```csv
dataset,scheme,upd_w1,upd_w2,client_search_time_ms,repeat
Crime,Nomos,1,16644,32.45,1
Crime,Nomos,2,16644,32.67,1
...
```

字段说明：
- `dataset`: 数据集名称
- `scheme`: 方案名称（Nomos/MC-ODXT/VQNomos）
- `upd_w1`: 关键词 w1 的更新次数（变化参数）
- `upd_w2`: 关键词 w2 的更新次数（固定为最大值）
- `client_search_time_ms`: 客户端搜索时间（毫秒）
- `repeat`: 重复测量次数

## 完整测试脚本

### 脚本 1: 快速测试（单次运行）

```bash
#!/bin/bash
cd /Users/cyan/code/paper/Nomos/build

echo "=== Client Search Time Fixed W2 Experiment ==="
echo "Running all datasets with repeat=1..."

./Nomos chapter4-client-search-fixed-w2 --repeat 1

echo "=== Experiment Complete ==="
echo "Results saved to: results/ch4/client_search_time_fixed_w2/"
```

### 脚本 2: 完整测试（多次重复）

```bash
#!/bin/bash
cd /Users/cyan/code/paper/Nomos/build

echo "=== Client Search Time Fixed W2 Experiment (Repeat=3) ==="

for dataset in Crime Enron Wiki; do
    echo "Running dataset: $dataset"
    ./Nomos chapter4-client-search-fixed-w2 --dataset $dataset --repeat 3
    echo "Completed: $dataset"
    echo "---"
done

echo "=== All Experiments Complete ==="
echo "Results saved to: results/ch4/client_search_time_fixed_w2/"
```

### 脚本 3: 分数据集测试

```bash
#!/bin/bash
cd /Users/cyan/code/paper/Nomos/build

# Crime 数据集
echo "=== Testing Crime Dataset ==="
./Nomos chapter4-client-search-fixed-w2 --dataset Crime --repeat 1

# Enron 数据集
echo "=== Testing Enron Dataset ==="
./Nomos chapter4-client-search-fixed-w2 --dataset Enron --repeat 1

# Wiki 数据集
echo "=== Testing Wiki Dataset ==="
./Nomos chapter4-client-search-fixed-w2 --dataset Wiki --repeat 1

echo "=== All Tests Complete ==="
```

### 脚本 4: 验证测试（检查输出）

```bash
#!/bin/bash
cd /Users/cyan/code/paper/Nomos

OUTPUT_DIR="results/ch4/client_search_time_fixed_w2"

echo "=== Running Experiment ==="
cd build
./Nomos chapter4-client-search-fixed-w2 --repeat 1

echo ""
echo "=== Checking Output Files ==="
cd ..

for dataset in Crime Enron Wiki; do
    for scheme in Nomos MC-ODXT VQNomos; do
        file="$OUTPUT_DIR/${scheme}_${dataset}.csv"
        if [ -f "$file" ]; then
            lines=$(wc -l < "$file")
            echo "✓ $file exists ($lines lines)"
            echo "  First 3 lines:"
            head -3 "$file" | sed 's/^/    /'
        else
            echo "✗ $file missing"
        fi
    done
    echo "---"
done
```

## 监控与调试

### 实时监控进度

```bash
# 在另一个终端窗口运行
watch -n 5 'ls -lh /Users/cyan/code/paper/Nomos/results/ch4/client_search_time_fixed_w2/'
```

### 检查实验进度

```bash
# 查看最新生成的文件
ls -lt /Users/cyan/code/paper/Nomos/results/ch4/client_search_time_fixed_w2/ | head -10

# 查看文件行数（估算进度）
wc -l /Users/cyan/code/paper/Nomos/results/ch4/client_search_time_fixed_w2/*.csv
```

### 故障排查

如果实验失败，检查以下内容：

1. **数据集文件是否存在**:
```bash
ls -lh /Users/cyan/code/paper/Nomos/pic/raw_data/*_filecnt_sorted.json
```

2. **输出目录权限**:
```bash
ls -ld /Users/cyan/code/paper/Nomos/results/ch4/client_search_time_fixed_w2/
```

3. **查看详细错误信息**:
```bash
./Nomos chapter4-client-search-fixed-w2 2>&1 | tee experiment.log
```

## 预期运行时间

基于 Fixed W1 实验的经验：

- **Crime 数据集**: 约 30-60 分钟（63,659 个数据点）
- **Enron 数据集**: 约 10-20 分钟（16,243 个数据点）
- **Wiki 数据集**: 约 5-10 分钟（10,001 个数据点）

总计：约 45-90 分钟（单次运行）

## 数据验证

### 验证 CSV 格式

```bash
# 检查 CSV 头部
head -1 /Users/cyan/code/paper/Nomos/results/ch4/client_search_time_fixed_w2/Nomos_Crime.csv

# 应该输出：
# dataset,scheme,upd_w1,upd_w2,client_search_time_ms,repeat
```

### 验证数据完整性

```bash
# 检查每个方案的数据点数量是否一致
for dataset in Crime Enron Wiki; do
    echo "Dataset: $dataset"
    for scheme in Nomos MC-ODXT VQNomos; do
        count=$(tail -n +2 "/Users/cyan/code/paper/Nomos/results/ch4/client_search_time_fixed_w2/${scheme}_${dataset}.csv" | wc -l)
        echo "  $scheme: $count points"
    done
done
```

### 验证 upd_w2 固定值

```bash
# 检查 upd_w2 是否为固定值（最大值）
for dataset in Crime Enron Wiki; do
    echo "Dataset: $dataset"
    max_w2=$(tail -n +2 "/Users/cyan/code/paper/Nomos/results/ch4/client_search_time_fixed_w2/Nomos_${dataset}.csv" | cut -d',' -f4 | sort -n | tail -1)
    echo "  Max upd_w2: $max_w2"
    unique_w2=$(tail -n +2 "/Users/cyan/code/paper/Nomos/results/ch4/client_search_time_fixed_w2/Nomos_${dataset}.csv" | cut -d',' -f4 | sort -u | wc -l)
    if [ "$unique_w2" -eq 1 ]; then
        echo "  ✓ upd_w2 is fixed"
    else
        echo "  ✗ upd_w2 has $unique_w2 different values (should be 1)"
    fi
done
```

## 注意事项

1. **实验时间**: Fixed W2 实验与 Fixed W1 实验运行时间相似，因为数据点数量相同
2. **内存使用**: VQNomos 方案由于 QTree 和验证机制会使用更多内存
3. **磁盘空间**: 确保有足够的磁盘空间存储结果文件（约 50-100 MB）
4. **中断恢复**: 如果实验中断，需要重新运行整个数据集（不支持断点续传）

## 与 Fixed W1 的区别

| 特性 | Fixed W1 | Fixed W2 |
|------|----------|----------|
| 固定参数 | \|Upd(w1)\| = 10 | \|Upd(w2)\| = max |
| 变化参数 | \|Upd(w2)\| 递增 | \|Upd(w1)\| 递增 |
| 数据点数量 | 与数据集关键词数相同 | 与数据集关键词数相同 |
| 输出目录 | `client_search_time_fixed_w1` | `client_search_time_fixed_w2` |
| 实验名称 | `chapter4-client-search-fixed-w1` | `chapter4-client-search-fixed-w2` |

---

**创建时间**: 2026-03-15
**实验版本**: 1.0
**对应代码**: `ClientSearchFixedW2Experiment.{hpp,cpp}`
