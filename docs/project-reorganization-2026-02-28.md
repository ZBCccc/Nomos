# 项目文件整理总结

## 整理时间
2026-02-28

## 整理内容

### 1. 文档合并与重组

#### 合并的文档
将以下三个重复的状态文档合并为一个：
- `IMPLEMENTATION_STATUS.md` (13K)
- `CORRECT_IMPLEMENTATION_PROGRESS.md` (4.9K)
- `CORRECT_IMPLEMENTATION_PLAN.md` (5.5K)

**合并后**：`docs/implementation-status.md` - 统一的实现状态文档

#### 移动到 docs/ 的文档
- `NOMOS_SUCCESS_REPORT.md` → `docs/success-report.md`
- `REORGANIZATION_SUMMARY.md` → `docs/reorganization-summary.md`
- `PAPER_REPRODUCTION_SETUP.md` → `docs/paper-reproduction-setup.md`
- `SCHEME_COMPARISON.md` → `docs/scheme-comparison.md`
- `guide.md` → `docs/quick-guide.md`
- `GEMINI.md` → `docs/gemini-instructions.md`

### 2. 旧实现文件归档

#### 归档位置
`archive/nomos-old-implementation/`

#### 归档的文件
**Headers**:
- `include/nomos/Client.hpp`
- `include/nomos/Server.hpp`
- `include/nomos/Gatekeeper.hpp`
- `include/nomos/types.hpp`
- `include/nomos/NomosExperiment.hpp`

**Implementation**:
- `src/nomos/Client.cpp`
- `src/nomos/Server.cpp`
- `src/nomos/Gatekeeper.cpp`

#### 归档原因
这些文件是早期实现版本，存在密码学正确性问题：
- 密钥结构错误（未使用密钥数组）
- Update 协议错误（使用哈希而非椭圆曲线点）
- 缺少 OPRF 协议
- 令牌结构错误
- 搜索协议错误

已被正确实现替代：
- `GatekeeperCorrect.{hpp,cpp}`
- `ServerCorrect.{hpp,cpp}`
- `ClientCorrect.{hpp,cpp}`
- `types_correct.hpp`
- `NomosSimplifiedExperiment.{hpp,cpp}`

### 3. 更新的配置文件

#### CLAUDE.md
- 更新架构指针，指向 Correct 版本的实现
- 更新文档列表，添加新的文档引用

#### docs/README.md
- 重新组织文档分类（核心文档、技术文档、指南、历史）
- 添加所有新文档的说明

## 整理后的项目结构

```
Nomos/
├── CLAUDE.md                    # 主配置文件（唯一根目录 .md）
├── CMakeLists.txt
├── CMakePresets.json
├── Makefile
├── .clang-format
├── .gitignore
│
├── docs/                        # 所有文档集中管理
│   ├── README.md
│   ├── implementation-status.md      # 合并后的状态文档
│   ├── paper-sources.md
│   ├── scheme-comparison.md
│   ├── architecture.md
│   ├── crypto-protocols.md
│   ├── parameter-deviations.md
│   ├── build-system.md
│   ├── known-issues.md
│   ├── quick-guide.md
│   ├── paper-reproduction-setup.md
│   ├── success-report.md
│   ├── reorganization-summary.md
│   └── gemini-instructions.md
│
├── rules/                       # 行为规则
│   ├── README.md
│   ├── 编码流程.md
│   └── 论文复现规则.md
│
├── include/                     # 头文件
│   ├── core/
│   ├── nomos/                   # 仅保留 Correct 版本
│   │   ├── GatekeeperCorrect.hpp
│   │   ├── ServerCorrect.hpp
│   │   ├── ClientCorrect.hpp
│   │   ├── types_correct.hpp
│   │   ├── NomosSimplifiedExperiment.hpp
│   │   └── GatekeeperState.hpp
│   ├── verifiable/
│   └── mc-odxt/
│
├── src/                         # 源文件
│   ├── core/
│   ├── nomos/                   # 仅保留 Correct 版本
│   │   ├── GatekeeperCorrect.cpp
│   │   ├── ServerCorrect.cpp
│   │   ├── ClientCorrect.cpp
│   │   ├── NomosSimplifiedExperiment.cpp
│   │   └── GatekeeperState.cpp
│   ├── verifiable/
│   ├── mc-odxt/
│   └── main.cpp
│
├── tests/
│
└── archive/                     # 归档的旧实现
    └── nomos-old-implementation/
        ├── README.md            # 归档说明
        ├── include/
        │   ├── Client.hpp
        │   ├── Server.hpp
        │   ├── Gatekeeper.hpp
        │   ├── types.hpp
        │   └── NomosExperiment.hpp
        └── src/
            ├── Client.cpp
            ├── Server.cpp
            └── Gatekeeper.cpp
```

## 整理效果

### 根目录清理
- **之前**：10 个 .md 文件混乱
- **之后**：仅 1 个 CLAUDE.md（配置文件）

### 文档组织
- **之前**：状态文档重复，分散在根目录
- **之后**：统一在 docs/ 目录，分类清晰

### 代码清理
- **之前**：新旧实现混杂，容易混淆
- **之后**：仅保留正确实现，旧实现已归档

## 后续建议

1. **更新 CMakeLists.txt**：确保不再引用旧的实现文件
2. **更新 .gitignore**：添加 archive/ 到版本控制（或排除）
3. **代码重命名**：考虑将 `*Correct.*` 重命名为标准名称（如果不再需要区分）
4. **文档维护**：定期更新 `docs/implementation-status.md`

## 验证清单

- [x] 根目录仅保留 CLAUDE.md
- [x] 所有文档移至 docs/
- [x] 旧实现已归档
- [x] CLAUDE.md 已更新
- [x] docs/README.md 已更新
- [x] 归档目录包含 README 说明
- [ ] CMakeLists.txt 需要验证（确保不引用旧文件）
- [ ] 编译测试（确保项目仍可正常编译）

---

**整理人员**: Claude Code
**整理日期**: 2026-02-28
