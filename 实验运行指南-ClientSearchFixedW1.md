# Chapter 4 Fixed W1 实验运行指南

## 实验概述

本实验在双关键词搜索场景下，对 Nomos、MC-ODXT、VQNomos 三个方案分别测量客户端、服务器、网关三类搜索时间。

实验设置与代码实现一致：

- 固定参数：`|Upd(w1)| = 10`
- 变化参数：`|Upd(w2)|` 遍历 `pic/raw_data/<Dataset>_filecnt_sorted.json` 中所有关键词的更新次数
- 遍历方式：保留 JSON 文件顺序，保留重复值，不做去重
- 数据集：Crime、Enron、Wiki
- 重复次数：默认 `1`
- 输出根目录：默认 `results/ch4/`

## 运行方法

### 方法一：运行所有数据集

```bash
cd /Users/cyan/code/paper/Nomos/build
cmake --build . --target nomos_app -j4
./Nomos chapter4-client-search-fixed-w1 --dataset all
```

### 方法二：逐个数据集运行

```bash
cd /Users/cyan/code/paper/Nomos/build
cmake --build . --target nomos_app -j4

./Nomos chapter4-client-search-fixed-w1 --dataset Crime --repeat 1
./Nomos chapter4-client-search-fixed-w1 --dataset Enron --repeat 1
./Nomos chapter4-client-search-fixed-w1 --dataset Wiki --repeat 1
```

### 方法三：自定义输出根目录

`--output-dir` 传入的是根目录，不是某个 `client_search_time_*` 子目录。

```bash
cd /Users/cyan/code/paper/Nomos/build

DATASET="Crime"
REPEAT=1
OUTPUT_ROOT="/Users/cyan/code/paper/Nomos/results/ch4"

./Nomos chapter4-client-search-fixed-w1 \
    --dataset "$DATASET" \
    --repeat "$REPEAT" \
    --output-dir "$OUTPUT_ROOT"
```

### 方法四：后台运行

```bash
cd /Users/cyan/code/paper/Nomos/build

LOG_DIR="/Users/cyan/code/paper/Nomos/logs"
mkdir -p "$LOG_DIR"

TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
LOG_FILE="$LOG_DIR/fixed_w1_$TIMESTAMP.log"

nohup ./Nomos chapter4-client-search-fixed-w1 --dataset all --repeat 1 \
    > "$LOG_FILE" 2>&1 &

echo "日志文件: $LOG_FILE"
```

## 输出目录

结果默认写到 `results/ch4/` 下的三个子目录：

```text
/Users/cyan/code/paper/Nomos/results/ch4/
├── client_search_time_fixed_w1/
├── server_search_time_fixed_w1/
└── gatekeeper_search_time_fixed_w1/
```

每个子目录下都会生成按“方案 × 数据集”命名的 CSV，例如：

```text
client_search_time_fixed_w1/
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

`server_search_time_fixed_w1/` 和 `gatekeeper_search_time_fixed_w1/` 下的文件名完全相同，只是列名和数据口径不同。

总文件数是 `3 个目录 × 9 个 CSV = 27 个 CSV`。

## CSV 格式

三个目录的前四列一致，只有时间列不同。

客户端目录：

```csv
dataset,scheme,upd_w1,upd_w2,client_time_ms,repeat
Crime,Nomos,10,5,0.523,1
Crime,Nomos,10,7,0.687,1
```

服务器目录：

```csv
dataset,scheme,upd_w1,upd_w2,server_time_ms,repeat
Crime,Nomos,10,5,0.211,1
Crime,Nomos,10,7,0.245,1
```

网关目录：

```csv
dataset,scheme,upd_w1,upd_w2,gatekeeper_time_ms,repeat
Crime,Nomos,10,5,0.041,1
Crime,Nomos,10,7,0.044,1
```

字段说明：

- `dataset`：数据集名称
- `scheme`：方案名称，取值为 `Nomos`、`MC-ODXT`、`VQNomos`
- `upd_w1`：固定为 `10`
- `upd_w2`：当前数据点对应的 `|Upd(w2)|`
- `client_time_ms` / `server_time_ms` / `gatekeeper_time_ms`：对应角色的时间开销，单位毫秒
- `repeat`：第几次重复测量

## 数据点说明

代码当前实现不是遍历唯一频率值，而是遍历 JSON 中每个关键词对应的更新次数，因此数据点数量等于该 JSON 中的关键词条目数。

大致规模如下：

- Crime：约 `63,659` 个数据点
- Enron：约 `16,243` 个数据点
- Wiki：约 `10,001` 个数据点

## 监控实验进度

### 方法一：查看控制台输出

实验运行时会持续打印当前数据集与进度信息。

### 方法二：查看三个输出目录

```bash
ls -lh /Users/cyan/code/paper/Nomos/results/ch4/client_search_time_fixed_w1/
ls -lh /Users/cyan/code/paper/Nomos/results/ch4/server_search_time_fixed_w1/
ls -lh /Users/cyan/code/paper/Nomos/results/ch4/gatekeeper_search_time_fixed_w1/
```

### 方法三：检查某个结果文件的行数

```bash
wc -l /Users/cyan/code/paper/Nomos/results/ch4/client_search_time_fixed_w1/Nomos_Crime.csv
wc -l /Users/cyan/code/paper/Nomos/results/ch4/server_search_time_fixed_w1/Nomos_Crime.csv
wc -l /Users/cyan/code/paper/Nomos/results/ch4/gatekeeper_search_time_fixed_w1/Nomos_Crime.csv
```

### 方法四：后台日志

```bash
tail -f /Users/cyan/code/paper/Nomos/logs/fixed_w1_*.log
```

## 数据验证

```bash
#!/bin/bash
cd /Users/cyan/code/paper/Nomos

ROOT="results/ch4"

check_dir() {
    dir="$1"
    metric="$2"
    echo "=== Checking $dir ==="
    for scheme in Nomos MC-ODXT VQNomos; do
        for dataset in Crime Enron Wiki; do
            file="$ROOT/$dir/${scheme}_${dataset}.csv"
            if [ -f "$file" ]; then
                lines=$(wc -l < "$file")
                echo "✓ $file: $((lines-1)) data rows"
                head -1 "$file"
            else
                echo "✗ Missing: $file"
            fi
        done
    done
    echo ""
    awk -F',' -v metric="$metric" '
        NR==1 {
            if ($1!="dataset" || $2!="scheme" || $3!="upd_w1" || $4!="upd_w2" || $5!=metric || $6!="repeat") {
                print "Unexpected header in " FILENAME
                exit 1
            }
        }
        NR>1 && NF!=6 {
            print "Unexpected column count in " FILENAME " line " NR
            exit 1
        }
    ' "$ROOT/$dir"/*.csv
}

check_dir "client_search_time_fixed_w1" "client_time_ms"
check_dir "server_search_time_fixed_w1" "server_time_ms"
check_dir "gatekeeper_search_time_fixed_w1" "gatekeeper_time_ms"
```

## 数据分析示例

下面示例只画客户端曲线；如果要画服务器或网关曲线，只需要把目录名和时间列改成对应值。

```python
import pandas as pd
import matplotlib.pyplot as plt

nomos_crime = pd.read_csv("results/ch4/client_search_time_fixed_w1/Nomos_Crime.csv")
mcodxt_crime = pd.read_csv("results/ch4/client_search_time_fixed_w1/MC-ODXT_Crime.csv")
vqnomos_crime = pd.read_csv("results/ch4/client_search_time_fixed_w1/VQNomos_Crime.csv")

plt.figure(figsize=(12, 6))
plt.plot(nomos_crime["upd_w2"], nomos_crime["client_time_ms"], label="Nomos", marker="o", markersize=2)
plt.plot(mcodxt_crime["upd_w2"], mcodxt_crime["client_time_ms"], label="MC-ODXT", marker="s", markersize=2)
plt.plot(vqnomos_crime["upd_w2"], vqnomos_crime["client_time_ms"], label="VQNomos", marker="^", markersize=2)

plt.xlabel("|Upd(w2)|")
plt.ylabel("Client Time (ms)")
plt.title("Fixed W1: Client Time vs |Upd(w2)|")
plt.legend()
plt.grid(True, alpha=0.3)
plt.savefig("client_search_comparison_fixed_w1_crime.png", dpi=300, bbox_inches="tight")
plt.show()

print(nomos_crime["client_time_ms"].describe())
print(mcodxt_crime["client_time_ms"].describe())
print(vqnomos_crime["client_time_ms"].describe())
```

## 故障排查

### 编译失败

```bash
cd /Users/cyan/code/paper/Nomos/build
cmake ..
cmake --build . --target nomos_app -j4
```

### 数据文件不存在

```bash
ls -lh /Users/cyan/code/paper/Nomos/pic/raw_data/*_filecnt_sorted.json
```

### 查看详细错误

```bash
cd /Users/cyan/code/paper/Nomos/build
./Nomos chapter4-client-search-fixed-w1 --dataset Crime 2>&1 | tee fixed_w1.log
```

## 快速开始

```bash
cd /Users/cyan/code/paper/Nomos/build
cmake --build . --target nomos_app -j4
./Nomos chapter4-client-search-fixed-w1 --dataset all
```

---

**最后更新**: 2026-03-16
**对应代码**: `src/benchmark/ClientSearchFixedW1Experiment.cpp`
