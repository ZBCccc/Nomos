#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

extern "C" {
#include <relic/relic.h>
}

namespace mcodxt {

// ============================================
// 数据结构定义
// ============================================

// 操作类型
enum class OpType : uint8_t {
    ADD = 0,
    DEL = 1
};

// 用户类型
enum class UserType : uint8_t {
    DATA_OWNER = 0,  // 数据所有者
    SEARCH_USER = 1  // 搜索用户
};

// 用户身份
struct UserID {
    std::string id;      // 用户唯一标识
    UserType type;       // 用户类型
    
    bool operator==(const UserID& other) const {
        return id == other.id && type == other.type;
    }
};

}  // namespace std

namespace std {
template<>
struct hash<mcodxt::UserID> {
    size_t operator()(const mcodxt::UserID& uid) const {
        return hash<string>()(uid.id) ^ static_cast<size_t>(uid.type);
    }
};
}  // namespace std

namespace mcodxt {

// 更新元数据
struct UpdateMetadata {
    std::string owner_id;          // 数据所有者ID
    ep_t addr;                     // TSet 地址
    std::vector<uint8_t> val;      // 加密的 (id||op)
    bn_t alpha;                    // Alpha 值
    std::vector<std::string> xtags; // ℓ xtags
    
    UpdateMetadata() {
        ep_null(addr);
        bn_null(alpha);
    }
    
    ~UpdateMetadata() {
        ep_free(addr);
        bn_free(alpha);
    }
};

// 搜索令牌
struct SearchToken {
    std::string owner_id;                    // 数据所有者ID
    std::string user_id;                     // 搜索用户ID
    ep_t strap;                              // H(w1)^Ks_owner
    std::vector<std::string> bstag;          // Blinded stags
    std::vector<std::string> delta;          // 解密掩码
    std::vector<std::vector<std::string>> bxtrap;  // [j][t]
    std::vector<uint8_t> env;                // 加密的 (rho, gamma)
    
    SearchToken() {
        ep_null(strap);
    }
    
    ~SearchToken() {
        ep_free(strap);
    }
};

// 搜索结果条目
struct SearchResultEntry {
    int j;
    std::vector<uint8_t> sval;  // 加密值
    int cnt;                    // 匹配计数
};

// TSet 条目
struct TSetEntry {
    std::vector<uint8_t> val;   // 加密的 (id||op)
    bn_t alpha;                 // Alpha 值
    std::string owner_id;       // 数据所有者ID
    
    TSetEntry() {
        bn_null(alpha);
    }
    
    ~TSetEntry() {
        bn_free(alpha);
    }
};

// 授权记录
struct Authorization {
    std::string owner_id;       // 数据所有者
    std::string user_id;        // 被授权用户
    std::vector<std::string> allowed_keywords;  // 允许搜索的关键词（空=全部）
    uint64_t expiry;            // 过期时间戳
    
    bool isValid() const {
        return true;  // TODO: 检查过期时间
    }
    
    bool canSearch(const std::string& keyword) const {
        if (allowed_keywords.empty()) return true;
        return std::find(allowed_keywords.begin(), 
                        allowed_keywords.end(), 
                        keyword) != allowed_keywords.end();
    }
};

// ============================================
// MC-ODXT 客户端 (搜索用户)
// ============================================

class McOdxtClient {
public:
    McOdxtClient(const std::string& user_id);
    ~McOdxtClient();
    
    /**
     * @brief Setup - 初始化用户密钥
     */
    int setup();
    
    /**
     * @brief 生成搜索令牌
     * @param query_keywords 查询关键词
     * @param owner_id 数据所有者ID
     * @param gatekeeper 网关引用
     * @return SearchToken
     */
    SearchToken genToken(
        const std::vector<std::string>& query_keywords,
        const std::string& owner_id,
        class McOdxtGatekeeper& gatekeeper);
    
    /**
     * @brief 准备搜索请求
     */
    struct SearchRequest {
        std::vector<std::string> stokenList;
        std::vector<std::vector<std::vector<std::string>>> xtokenList;
        std::vector<uint8_t> env;
        std::string owner_id;
        std::string user_id;
    };
    
    SearchRequest prepareSearch(
        const SearchToken& token,
        const std::vector<std::string>& query_keywords,
        const std::unordered_map<std::string, int>& updateCnt);
    
    /**
     * @brief 解密搜索结果
     */
    std::vector<std::string> decryptResults(
        const std::vector<SearchResultEntry>& results,
        const SearchToken& token);
    
    const std::string& getUserId() const { return m_user_id; }

private:
    std::string m_user_id;
    UserType m_user_type;
    
    // 用户密钥（对于搜索用户，这是从授权派生）
    bn_t m_Ks_user;           // 用户自己的 Ks
    std::vector<uint8_t> m_Km;  // 解密密钥
};

// ============================================
// MC-ODXT 数据所有者
// ============================================

class McOdxtDataOwner {
public:
    McOdxtDataOwner(const std::string& owner_id);
    ~McOdxtDataOwner();
    
    /**
     * @brief Setup - 初始化所有者密钥
     */
    int setup();
    
    /**
     * @brief 生成更新元数据（用于添加/删除文档）
     */
    UpdateMetadata update(
        OpType op,
        const std::string& doc_id,
        const std::string& keyword,
        class McOdxtGatekeeper& gatekeeper);
    
    /**
     * @brief 授权搜索用户
     */
    void authorize(
        const std::string& user_id,
        const std::vector<std::string>& allowed_keywords,
        class McOdxtGatekeeper& gatekeeper);
    
    const std::string& getOwnerId() const { return m_owner_id; }
    const bn_t& getKs() const { return m_Ks_owner; }

private:
    std::string m_owner_id;
    
    // 数据所有者密钥
    bn_t m_Ks_owner;           // OPRF 密钥 Ks_do
    bn_t* m_Kt;               // TSet 密钥数组
    bn_t* m_Kx;               // XSet 密钥数组
    bn_t m_Ky;                // PRF 密钥
    std::vector<uint8_t> m_Km; // 对称加密密钥
    
    int m_d;                  // 密钥数组大小
    
    int indexFunction(const std::string& keyword) const;
    void computeKz(bn_t kz, const std::string& keyword, const bn_t& Ks);
    void computeFp(bn_t result, bn_t key, const std::string& input);
};

// ============================================
// MC-ODXT 网关（密钥管理和令牌生成）
// ============================================

class McOdxtGatekeeper {
public:
    McOdxtGatekeeper();
    ~McOdxtGatekeeper();
    
    /**
     * @brief Setup - 初始化网关
     */
    int setup(int d = 10);
    
    /**
     * @brief 注册数据所有者
     */
    int registerDataOwner(const std::string& owner_id);
    
    /**
     * @brief 注册搜索用户
     */
    int registerSearchUser(const std::string& user_id);
    
    /**
     * @brief 授权搜索用户访问数据所有者
     */
    int grantAuthorization(
        const std::string& owner_id,
        const std::string& user_id,
        const std::vector<std::string>& allowed_keywords = {});
    
    /**
     * @brief 撤销授权
     */
    int revokeAuthorization(
        const std::string& owner_id,
        const std::string& user_id);
    
    /**
     * @brief 检查用户是否有权搜索
     */
    bool isAuthorized(const std::string& owner_id, const std::string& user_id) const;
    
    /**
     * @brief 生成搜索令牌（核心多用户逻辑）
     */
    SearchToken genToken(
        const std::vector<std::string>& query_keywords,
        const std::string& owner_id,
        const std::string& user_id);
    
    /**
     * @brief 获取数据所有者的更新计数
     */
    int getUpdateCount(const std::string& owner_id, const std::string& keyword) const;
    
    /**
     * @brief 获取所有更新计数
     */
    const std::unordered_map<std::string, int>& getUpdateCounts(
        const std::string& owner_id) const;
    
    // 获取密钥（供内部使用）
    const bn_t& getKsOwner(const std::string& owner_id) const;
    const bn_t* getKt(const std::string& owner_id) const;
    const bn_t* getKx(const std::string& owner_id) const;
    const bn_t& getKy(const std::string& owner_id) const;

private:
    int m_d;
    
    // 每个数据所有者的密钥
    struct OwnerKeys {
        bn_t Ks;           // OPRF 密钥
        bn_t* Kt;          // TSet 密钥数组
        bn_t* Kx;          // XSet 密钥数组
        bn_t Ky;           // PRF 密钥
        std::vector<uint8_t> Km;  // 对称加密密钥
        std::unordered_map<std::string, int> updateCnt;  // 关键词更新计数
        
        OwnerKeys() : Kt(nullptr), Kx(nullptr) {
            bn_null(Ks);
            bn_null(Ky);
        }
        
        ~OwnerKeys() {
            bn_free(Ks);
            bn_free(Ky);
            if (Kt) {
                for (int i = 0; i < 10; ++i) bn_free(Kt[i]);
                delete[] Kt;
            }
            if (Kx) {
                for (int i = 0; i < 10; ++i) bn_free(Kx[i]);
                delete[] Kx;
            }
        }
    };
    
    std::unordered_map<std::string, OwnerKeys> m_owner_keys;
    std::unordered_map<std::string, bn_t> m_user_keys;  // 搜索用户密钥
    
    // 授权表: owner_id -> (user_id -> Authorization)
    std::unordered_map<std::string, 
                       std::unordered_map<std::string, Authorization>> m_authorizations;
    
    // 辅助函数
    int indexFunction(const std::string& keyword) const;
    void computeKz(bn_t kz, const std::string& keyword, const bn_t& Ks);
    void computeFp(bn_t result, bn_t key, const std::string& input);
};

// ============================================
// MC-ODXT 服务器
// ============================================

class McOdxtServer {
public:
    McOdxtServer();
    ~McOdxtServer();
    
    /**
     * @brief 更新 - 存储数据
     */
    void update(const UpdateMetadata& meta);
    
    /**
     * @brief 搜索
     */
    std::vector<SearchResultEntry> search(const McOdxtClient::SearchRequest& req);
    
    size_t getTSetSize() const { return m_TSet.size(); }
    size_t getXSetSize() const { return m_XSet.size(); }

private:
    // TSet: owner_id + addr -> TSetEntry
    std::map<std::pair<std::string, std::string>, TSetEntry> m_TSet;
    
    // XSet: owner_id + xtag -> bool
    std::map<std::pair<std::string, std::string>, bool> m_XSet;
    
    std::string serializePoint(const ep_t point) const;
};

}  // namespace mcodxt
