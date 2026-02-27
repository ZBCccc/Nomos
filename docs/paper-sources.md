# Paper Sources - 论文来源

本项目复现以下学术论文中的方案:

## 主要论文

### Nomos (2024)

**完整引用**:
```
Arnab Bag, Sikhar Patranabis, and Debdeep Mukhopadhyay. 2024.
Tokenised Multi-client Provisioning for Dynamic Searchable Encryption
with Forward and Backward Privacy.
In ACM Asia Conference on Computer and Communications Security (ASIA CCS '24),
July 1–5, 2024, Singapore, Singapore. ACM, New York, NY, USA, 17 pages.
https://doi.org/10.1145/3634737.3657018
```

**作者**: Arnab Bag, Sikhar Patranabis, Debdeep Mukhopadhyay
**机构**: Indian Institute of Technology Kharagpur, IBM Research India
**会议**: ACM ASIA CCS 2024
**DOI**: 10.1145/3634737.3657018
**本地路径**: `/Users/cyan/code/paper/ref-thesis/Bag 等 - 2024 - Tokenised Multi-client Provisioning for Dynamic Searchable Encryption with Forward and Backward Priv.pdf`

**核心贡献**:
1. 首个支持多客户端动态更新的 SSE 方案 (Multi-client SSE)
2. 同时实现前向隐私和 Type-II 后向隐私
3. 基于 ODXT 的多客户端扩展,引入 gate-keeper 角色
4. 使用 OPRF 机制实现客户端间的令牌生成协调
5. 支持高效的多关键词布尔查询 (conjunctive Boolean queries)

**方案设置**:
- Multi-Reader-Single-Writer (MRSW)
- 三方架构: Clients, Gate-keeper, Server
- 前向隐私 + Type-II 后向隐私
- 线性存储开销,次线性搜索时间

## 相关基础论文

### ODXT (2021)
**引用**: Patranabis et al., "Dynamic Searchable Encryption with Small Client Storage", NDSS 2021
**说明**: Nomos 基于 ODXT 进行多客户端扩展

### Σoφoς (2016)
**引用**: Bost, Raphael. "Σoφoς: Forward secure searchable encryption", CCS 2016
**DOI**: 10.1145/2976749.2978428
**说明**: 首次提出前向隐私概念

### Forward and Backward Private SSE (2017)
**引用**: Bost, Raphaël, Brice Minaud, and Olga Ohrimenko. "Forward and Backward Private Searchable Encryption from Constrained Cryptographic Primitives", CCS 2017
**DOI**: 10.1145/3133956.3133980
**说明**: 首次同时实现前后向隐私

## 本项目实现的方案

### 1. Nomos Baseline (已实现)
- 论文 Section 4 的基础 Nomos 构造
- 多用户多关键词动态 SSE
- 三方架构: Gatekeeper, Server, Client

### 2. Verifiable Nomos (已实现)
- 论文中提到的可验证扩展
- 使用 QTree (Merkle Hash Tree) 提供完整性证明
- 使用嵌入式承诺 (embedded commitment) 绑定地址

### 3. MC-ODXT (待实现)
- 对比方案,用于性能评估
- 基于 ODXT 的多客户端改造

## 复现状态

| 方案 | 论文章节 | 实现状态 | 代码位置 |
|------|----------|----------|----------|
| Nomos Baseline | Section 4 | ✅ 已实现 | `nomos/` |
| Verifiable Nomos | 扩展方案 | ✅ 已实现 | `verifiable/` |
| MC-ODXT | 对比方案 | ⏳ 待实现 | `mc-odxt/` |

## 参考资料

- 论文 PDF: `/Users/cyan/code/paper/ref-thesis/Bag 等 - 2024 - Tokenised Multi-client Provisioning for Dynamic Searchable Encryption with Forward and Backward Priv.pdf`
- 研究笔记: `/Users/cyan/code/paper/docs/`
- 协议重构记录: `/Users/cyan/code/paper/docs/memory/Nomos协议重构与MHT过渡记录-2026-02-21.md`
