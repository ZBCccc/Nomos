# 实验运行指南 - Fixed W2

## 实验概述

本实验在双关键词搜索场景下，对 Nomos、MC-ODXT、VQNomos 三个方案分别测量客户端、服务器、网关三类搜索时间。

实验设置与代码实现一致：

- 固定参数：`|Upd(w2)|` 固定为当前数据集中更新次数最大的关键词
- 变化参数：`w1` 依次遍历当前数据集中的所有关键词
- 数据点含义：每个关键词对应一个数据点，因此点数等于关键词总数
- 数据集：Crime、Enron、Wiki
- 输出根目录：默认 `results/ch4/`

## 运行方法

### 方法一：运行所有数据集

```bash
cd /Users/cyan/code/paper/Nomos/build
cmake --build . --target Nomos -j4
./Nomos chapter4-client-search-fixed-w2
```

### 方法二：运行单个数据集

```bash
cd /Users/cyan/code/paper/Nomos/build
cmake --build . --target Nomos -j4

./Nomos chapter4-client-search-fixed-w2 --dataset Crime
./Nomos chapter4-client-search-fixed-w2 --dataset Enron
./Nomos chapter4-client-search-fixed-w2 --dataset Wiki
```

### 方法三：自定义输出根目录

`--output-dir` 传入的是根目录，不是某个 `client_search_time_*` 子目录。

```bash
cd /Users/cyan/code/paper/Nomos/build

OUTPUT_ROOT="/Users/cyan/code/paper/Nomos/results/ch4"
./Nomos chapter4-client-search-fixed-w2 --output-dir "$OUTPUT_ROOT"
```

### 方法四：只运行指定方案

```bash
cd /Users/cyan/code/paper/Nomos/build
./Nomos chapter4-client-search-fixed-w2 --scheme Nomos
./Nomos chapter4-client-search-fixed-w2 --scheme MC-ODXT
./Nomos chapter4-client-search-fixed-w2 --scheme VQNomos
```

## 支持的命令行参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `--dataset <name>` | `all` | 数据集名称：`Crime`、`Enron`、`Wiki`，或 `all` |
| `--output-dir <path>` | `results/ch4/` | 输出根目录 |
| `--scheme <name>` | `all` | 只运行指定方案：`Nomos`、`MC-ODXT`、`VQNomos`，或 `all` |

## 输出目录

结果默认写到 `results/ch4/` 下的三个子目录：

```text
/Users/cyan/code/paper/Nomos/results/ch4/
├── client_search_time_fixed_w2/
├── server_search_time_fixed_w2/
└── gatekeeper_search_time_fixed_w2/
```

每个子目录下都会生成按"方案 × 数据集"命名的 CSV，例如：

```text
client_search_time_fixed_w2/
├── Nomos_Crime.csv
├── MC-ODXT_Crime.csv
├── VQNomos_Crime.csv
├── Nomos_Enron.csv
├── MC-ODXT_Enron.csv
├── VQNomos_Enron.csv
├── Nomos_Wiki.csv
├── MC-ODXT_Wiki.csv
└── VQNomos_Wiki.csv
```

`server_search_time_fixed_w2/` 和 `gatekeeper_search_time_fixed_w2/` 下的文件名完全相同，只是列名和数据口径不同。

总文件数是 `3 个目录 × 9 个 CSV = 27 个 CSV`。

## CSV 格式

客户端目录：

```csv
dataset,scheme,upd_w1,upd_w2,client_time_ms
Crime,Nomos,1,16644,32.45
Crime,Nomos,2,16644,32.67
```

服务器目录：

```csv
dataset,scheme,upd_w1,upd_w2,server_time_ms
Crime,Nomos,1,16644,1.21
Crime,Nomos,2,16644,1.24
```

网关目录：

```csv
dataset,scheme,upd_w1,upd_w2,gatekeeper_time_ms
Crime,Nomos,1,16644,0.17
Crime,Nomos,2,16644,0.18
```

字段说明：

- `dataset`：数据集名称
- `scheme`：方案名称，取值为 `Nomos`、`MC-ODXT`、`VQNomos`
- `upd_w1`：当前数据点对应的 `|Upd(w1)|`
- `upd_w2`：固定的最大 `|Upd(w2)|`
- `client_time_ms` / `server_time_ms` / `gatekeeper_time_ms`：对应角色的时间开销，单位毫秒

## 数据点说明

Fixed W2 当前实现是"固定一个最大频次关键词作为 `w2`，再遍历数据集中所有关键词作为 `w1`"，因此数据点数量等于关键词总数。

大致规模如下：

- Crime：约 `63,659` 个数据点
- Enron：约 `16,243` 个数据点
- Wiki：约 `10,001` 个数据点

## 完整测试脚本

### 脚本一：默认全量运行

```bash
#!/bin/bash
cd /Users/cyan/code/paper/Nomos/build
cmake --build . --target Nomos -j4
./Nomos chapter4-client-search-fixed-w2
```

### 脚本二：逐数据集运行

```bash
#!/bin/bash
cd /Users/cyan/code/paper/Nomos/build
cmake --build . --target Nomos -j4

for dataset in Crime Enron Wiki; do
    ./Nomos chapter4-client-search-fixed-w2 --dataset "$dataset"
done
```

### 脚本三：验证输出

```bash
#!/bin/bash
cd /Users/cyan/code/paper/Nomos

ROOT="results/ch4"

check_dir() {
    dir="$1"
    metric="$2"
    echo "=== Checking $dir ==="
    for dataset in Crime Enron Wiki; do
        for scheme in Nomos MC-ODXT VQNomos; do
            file="$ROOT/$dir/${scheme}_${dataset}.csv"
            if [ -f "$file" ]; then
                lines=$(wc -l < "$file")
                echo "✓ $file exists ($((lines-1)) rows)"
            else
                echo "✗ $file missing"
            fi
        done
    done
    awk -F',' -v metric="$metric" '
        NR==1 {
            if ($1!="dataset" || $2!="scheme" || $3!="upd_w1" || $4!="upd_w2" || $5!=metric) {
                print "Unexpected header in " FILENAME
                exit 1
            }
        }
        NR>1 && NF!=5 {
            print "Unexpected column count in " FILENAME " line " NR
            exit 1
        }
    ' "$ROOT/$dir"/*.csv
    echo ""
}

check_dir "client_search_time_fixed_w2" "client_time_ms"
check_dir "server_search_time_fixed_w2" "server_time_ms"
check_dir "gatekeeper_search_time_fixed_w2" "gatekeeper_time_ms"
```

## 监控与调试

### 查看三个输出目录

```bash
ls -lh /Users/cyan/code/paper/Nomos/results/ch4/client_search_time_fixed_w2/
ls -lh /Users/cyan/code/paper/Nomos/results/ch4/server_search_time_fixed_w2/
ls -lh /Users/cyan/code/paper/Nomos/results/ch4/gatekeeper_search_time_fixed_w2/
```

### 查看最近生成的文件

```bash
ls -lt /Users/cyan/code/paper/Nomos/results/ch4/client_search_time_fixed_w2/ | head -10
ls -lt /Users/cyan/code/paper/Nomos/results/ch4/server_search_time_fixed_w2/ | head -10
ls -lt /Users/cyan/code/paper/Nomos/results/ch4/gatekeeper_search_time_fixed_w2/ | head -10
```

### 检查 CSV 头部

```bash
head -1 /Users/cyan/code/paper/Nomos/results/ch4/client_search_time_fixed_w2/Nomos_Crime.csv
head -1 /Users/cyan/code/paper/Nomos/results/ch4/server_search_time_fixed_w2/Nomos_Crime.csv
head -1 /Users/cyan/code/paper/Nomos/results/ch4/gatekeeper_search_time_fixed_w2/Nomos_Crime.csv
```

### 验证 `upd_w2` 为固定值

```bash
for dataset in Crime Enron Wiki; do
    echo "Dataset: $dataset"
    unique_w2=$(tail -n +2 "/Users/cyan/code/paper/Nomos/results/ch4/client_search_time_fixed_w2/Nomos_${dataset}.csv" | cut -d',' -f4 | sort -u | wc -l)
    if [ "$unique_w2" -eq 1 ]; then
        echo "  ✓ upd_w2 is fixed"
    else
        echo "  ✗ upd_w2 has $unique_w2 different values"
    fi
done
```

## 数据分析示例

下面示例只画客户端曲线；如果要画服务器或网关曲线，只需要把目录名和时间列改成对应值。

```python
import pandas as pd
import matplotlib.pyplot as plt

nomos_crime = pd.read_csv("results/ch4/client_search_time_fixed_w2/Nomos_Crime.csv")
mcodxt_crime = pd.read_csv("results/ch4/client_search_time_fixed_w2/MC-ODXT_Crime.csv")
vqnomos_crime = pd.read_csv("results/ch4/client_search_time_fixed_w2/VQNomos_Crime.csv")

plt.figure(figsize=(12, 6))
plt.plot(nomos_crime["upd_w1"], nomos_crime["client_time_ms"], label="Nomos", marker="o", markersize=2)
plt.plot(mcodxt_crime["upd_w1"], mcodxt_crime["client_time_ms"], label="MC-ODXT", marker="s", markersize=2)
plt.plot(vqnomos_crime["upd_w1"], vqnomos_crime["client_time_ms"], label="VQNomos", marker="^", markersize=2)

plt.xlabel("|Upd(w1)|")
plt.ylabel("Client Time (ms)")
plt.title("Fixed W2: Client Time vs |Upd(w1)|")
plt.legend()
plt.grid(True, alpha=0.3)
plt.savefig("client_search_comparison_fixed_w2_crime.png", dpi=300, bbox_inches="tight")
plt.show()
```

## 与 Fixed W1 的区别

| 特性 | Fixed W1 | Fixed W2 |
|------|----------|----------|
| 固定参数 | `\|Upd(w1)\| = 10` | `\|Upd(w2)\| = 当前数据集最大值` |
| 变化参数 | JSON 中所有 `\|Upd(w2)\|` 值，保留重复 | 当前数据集所有关键词对应的 `\|Upd(w1)\|` |
| 点数来源 | JSON 条目数 | 关键词总数 |
| 默认输出根目录 | `results/ch4/` | `results/ch4/` |

## 快速开始

```bash
cd /Users/cyan/code/paper/Nomos/build
cmake --build . --target Nomos -j4
./Nomos chapter4-client-search-fixed-w2
```

---

**最后更新**: 2026-03-16
**对应代码**: `src/benchmark/ClientSearchFixedW2Experiment.cpp`
