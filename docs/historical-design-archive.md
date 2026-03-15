# 历史设计归档

本文件用于集中保留 `Nomos/` 项目中已经退出主线的设计稿、阶段性计划和实现总结，避免这些材料继续和当前实现说明混在一起。

## 归档范围

本次归档合并了以下过期文档：

- `docs/Benchmark-Plan.md`
- `docs/MC-ODXT-Design.md`
- `docs/OPRF-Implementation.md`
- `docs/OPRF实现总结.md`
- `docs/reorganization-summary.md`

这些文件的共性是：

- 描述的是某个历史阶段的设计设想或实现阶段总结
- 已经和当前代码主线不一致
- 继续单独保留会增加误读成本

## 历史阶段摘要

### 1. 早期基准测试规划

最初的 benchmark 计划把 `Nomos` 与 `MC-ODXT` 都按“功能测试 + 性能测试 + 规模测试”拆成多阶段任务，并假设：

- `MC-ODXT` 保留多用户授权模型
- 有单独的授权时间、用户数量扩展实验
- 命令行会提供多种 benchmark 参数入口

当前状态：

- 这套计划已经被 Chapter 4 的数据驱动实验入口替代
- 当前主线实验以 `chapter4-client-search-fixed-w1` 等实际论文图需求为准
- 不再维护那份泛化 benchmark 规划

### 2. OPRF 盲化协议阶段

项目中曾完整实现过一条 OPRF 盲化搜索令牌路径，包含：

- `Client` 盲化请求
- `Gatekeeper` 处理盲化请求
- `Client` 去盲化
- `Server` 解密 `env` 并执行去盲化搜索

这批工作当时是为了贴近论文中的完整安全模型，并生成了两份详细文档：

- 一份偏实现说明
- 一份偏阶段总结

当前状态：

- 当前代码已明确面向实验用途
- 主线只保留 `Client::genToken() -> Gatekeeper::genToken()` 简化搜索路径
- OPRF 盲化不再属于当前运行主线

因此，旧 OPRF 文档只保留为“曾实现过、后来主动收缩”的历史记录，不再作为当前协议说明。

### 3. MC-ODXT 多用户设计阶段

项目曾有一版 `MC-ODXT` 设计稿，假设：

- 存在 `Data Owner / Search User / Gatekeeper / Server` 四方
- 有授权、撤销授权、用户隔离
- `XSet` 可能改造成 Bloom Filter
- 对比方案将保留独立的多客户端语义

这份设计和之后的原型代码一度一致，但已经不符合当前主线。

当前状态：

- `MC-ODXT` 已重构为与 `Nomos` 同构的单租户简化接口
- 当前只保留 `McOdxtGatekeeper / McOdxtClient / McOdxtServer`
- 与 `Nomos` 的唯一区别收敛为：`MC-ODXT` 只使用 map-backed 索引实现
- 不再维护 owner/user/authorization 那条旧协议线

因此，旧 `MC-ODXT` 设计稿只保留其“为何曾出现多用户外壳”的背景价值。

### 4. 文档体系重组阶段

项目曾单独保留一份文档结构重组总结，用来说明：

- `CLAUDE.md / rules / docs` 三层结构
- 哪些信息该放主入口，哪些该放 `docs/`

这份总结在当时有用，但它描述的是一次已经完成的整理动作。

当前状态：

- 文档组织以当前的 `CLAUDE.md` 和 `docs/README.md` 为准
- 不再需要单独保留一次历史整理总结作为主线入口文档

## 当前信息源

如果要看当前主线，请优先读这些文档：

- `docs/implementation-status.md`
- `docs/architecture.md`
- `docs/crypto-protocols.md`
- `docs/parameter-deviations.md`
- `docs/experiment-validation.md`

## 清理原则

本次文档精简采用以下原则：

- 当前实现说明保留
- 当前实验入口说明保留
- 历史设计材料只保留一份合并归档
- 不再保留多个彼此覆盖、且结论已过期的设计稿

## 备注

如果后续还出现新的阶段性设计稿或迁移总结，默认不要再单独长期保留多份；优先追加到本归档文件，或者直接写入当前主线文档中。
