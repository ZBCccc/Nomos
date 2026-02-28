# Paper Reproduction Setup - 论文复现设置完成

## 完成的工作

### 1. 创建论文来源文档
**文件**: `docs/paper-sources.md`

**内容**:
- Bag et al. 2024 完整引用信息
- DOI 和本地路径
- 核心贡献总结
- 相关基础论文列表
- 三个方案的实现状态表

### 2. 创建强制复现规则
**文件**: `rules/论文复现规则.md`

**核心约束**:
1. **协议忠实性**: 必须严格按照论文描述实现,不得擅自修改
2. **参数一致性**: 所有密码学参数必须与论文设置保持一致
3. **算法映射**: 每个函数必须标注对应的论文算法编号
4. **安全性保证**: 不得削弱论文声称的安全性
5. **实验设置**: 性能评估必须使用论文的实验设置
6. **代码审查检查点**: 提交前的 6 项强制检查
7. **偏离记录**: 任何偏离必须记录在 `docs/parameter-deviations.md`

**禁止行为**:
- ❌ 使用论文未提及的密码学原语
- ❌ 修改协议流程
- ❌ 简化安全关键步骤
- ❌ 在日志中输出敏感信息

### 3. 创建参数偏离记录
**文件**: `docs/parameter-deviations.md`

**已记录的偏离**:
1. ℓ = 3 (交叉标签数量)
2. k = 2 (采样数量)
3. C++11 标准
4. QTree 容量 = 1024
5. 哈希函数选择 (SHA256)
6. 椭圆曲线选择 (BN254/BLS12-381)

**待确认项**:
- UpdateCnt 计数器逻辑
- TSet/XSet 存储结构
- 搜索结果解密逻辑
- 删除操作实现

### 4. 更新 CLAUDE.md
**修改**:
- 在顶部添加 "⚠️ CRITICAL: Paper Reproduction Project" 警告
- 标注主论文信息和路径
- 引用强制复现规则
- 更新 docs/ 目录说明,突出论文相关文档

### 5. 更新 rules/README.md
**修改**:
- 添加 `论文复现规则.md` 说明
- 在使用说明中强调论文复现约束的优先级

## 规则集成到开发流程

### Phase 0: 实施前辩论
- Codex 必须检查实现计划是否符合论文描述
- 协议偏离被标记为严重问题

### Phase 1: 内部验证
- 代码审查者验证算法映射正确性
- 检查参数设置与论文一致性

### Phase 2: Codex 审计
- 重点审计密码学协议忠实性
- 检查未记录的偏离

## 强制检查清单

在提交代码前必须确认:
- [ ] 是否有对应的论文算法引用?
- [ ] 参数设置是否与论文一致?
- [ ] 是否引入了论文未提及的密码学操作?
- [ ] 是否削弱了安全性保证?
- [ ] 变量命名是否与论文符号对应?
- [ ] 是否在注释中解释了与论文的偏离?

## 论文信息

**主论文**:
```
Arnab Bag, Sikhar Patranabis, and Debdeep Mukhopadhyay. 2024.
Tokenised Multi-client Provisioning for Dynamic Searchable Encryption
with Forward and Backward Privacy.
ACM ASIA CCS 2024.
DOI: 10.1145/3634737.3657018
```

**本地路径**: `/Users/cyan/code/paper/ref-thesis/Bag 等 - 2024 - Tokenised Multi-client Provisioning for Dynamic Searchable Encryption with Forward and Backward Priv.pdf`

## 实现状态

| 方案 | 论文章节 | 状态 | 代码位置 |
|------|----------|------|----------|
| Nomos Baseline | Section 4 | ✅ 已实现 | `nomos/` |
| Verifiable Nomos | 扩展方案 | ✅ 已实现 | `verifiable/` |
| MC-ODXT | 对比方案 | ⏳ 待实现 | `mc-odxt/` |

## 下一步行动

1. **代码审计**: 对现有代码进行论文符合性审计
   - 检查 `nomos/` 中的实现是否与论文 Section 4 一致
   - 验证 `verifiable/` 中的 QTree 和 AddressCommitment 实现
   - 标注所有函数的论文算法引用

2. **参数验证**: 确认所有密码学参数
   - 验证 ℓ 和 k 的选择是否合理
   - 检查哈希函数和椭圆曲线配置
   - 更新 `parameter-deviations.md` 中的待确认项

3. **注释补充**: 为核心函数添加论文引用
   - 格式: `// Paper: Algorithm X (Section Y.Z)`
   - 解释关键变量与论文符号的对应关系

4. **实验准备**: 准备论文中的实验设置
   - 获取 Enron Email Dataset
   - 实现论文中的性能评估指标
   - 准备对比实验框架

## 文件清单

**新增文件**:
- `docs/paper-sources.md` - 论文来源和引用信息
- `rules/论文复现规则.md` - 强制复现约束
- `docs/parameter-deviations.md` - 参数偏离记录

**修改文件**:
- `CLAUDE.md` - 添加论文复现警告
- `rules/README.md` - 更新规则说明

**总行数**: 约 400 行新增文档
