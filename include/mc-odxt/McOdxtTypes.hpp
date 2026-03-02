#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <map>
#include <utility>

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

    // Move constructor
    UpdateMetadata(UpdateMetadata&& other) noexcept 
        : owner_id(std::move(other.owner_id)), val(std::move(other.val)), 
          xtags(std::move(other.xtags)) {
        std::memcpy(addr, other.addr, sizeof(ep_t));
        std::memcpy(alpha, other.alpha, sizeof(bn_t));
        ep_null(other.addr);
        bn_null(other.alpha);
    }

    // Move assignment
    UpdateMetadata& operator=(UpdateMetadata&& other) noexcept {
        if (this != &other) {
            ep_free(addr);
            bn_free(alpha);
            owner_id = std::move(other.owner_id);
            val = std::move(other.val);
            xtags = std::move(other.xtags);
            std::memcpy(addr, other.addr, sizeof(ep_t));
            std::memcpy(alpha, other.alpha, sizeof(bn_t));
            ep_null(other.addr);
            bn_null(other.alpha);
        }
        return *this;
    }

    ~UpdateMetadata() {
        ep_free(addr);
        bn_free(alpha);
    }

    // Disable copy to enforce move
    UpdateMetadata(const UpdateMetadata&) = delete;
    UpdateMetadata& operator=(const UpdateMetadata&) = delete;
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

    // Move constructor
    SearchToken(SearchToken&& other) noexcept 
        : owner_id(std::move(other.owner_id)), user_id(std::move(other.user_id)),
          bstag(std::move(other.bstag)), delta(std::move(other.delta)),
          bxtrap(std::move(other.bxtrap)), env(std::move(other.env)) {
        std::memcpy(strap, other.strap, sizeof(ep_t));
        ep_null(other.strap);
    }

    // Move assignment
    SearchToken& operator=(SearchToken&& other) noexcept {
        if (this != &other) {
            ep_free(strap);
            owner_id = std::move(other.owner_id);
            user_id = std::move(other.user_id);
            bstag = std::move(other.bstag);
            delta = std::move(other.delta);
            bxtrap = std::move(other.bxtrap);
            env = std::move(other.env);
            std::memcpy(strap, other.strap, sizeof(ep_t));
            ep_null(other.strap);
        }
        return *this;
    }
    
    ~SearchToken() {
        ep_free(strap);
    }

    // Disable copy
    SearchToken(const SearchToken&) = delete;
    SearchToken& operator=(const SearchToken&) = delete;
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

    // Move constructor
    TSetEntry(TSetEntry&& other) noexcept 
        : val(std::move(other.val)), owner_id(std::move(other.owner_id)) {
        std::memcpy(alpha, other.alpha, sizeof(bn_t));
        bn_null(other.alpha);
    }

    // Move assignment
    TSetEntry& operator=(TSetEntry&& other) noexcept {
        if (this != &other) {
            bn_free(alpha);
            val = std::move(other.val);
            owner_id = std::move(other.owner_id);
            std::memcpy(alpha, other.alpha, sizeof(bn_t));
            bn_null(other.alpha);
        }
        return *this;
    }
    
    ~TSetEntry() {
        bn_free(alpha);
    }

    // Disable copy
    TSetEntry(const TSetEntry&) = delete;
    TSetEntry& operator=(const TSetEntry&) = delete;
};

// 授权记录
struct Authorization {
    std::string owner_id;       // 数据所有者
    std::string user_id;        // 被授权用户
    std::vector<std::string> allowed_keywords;  // 允许搜索的关键词（空=全部）
    uint64_t expiry;            // 过期时间戳
    
    bool isValid() const {
        // Check if authorization has expired
        if (expiry == 0) {
            return true;  // Never expires
        }
        auto now = std::chrono::system_clock::now();
        auto now_time_t = std::chrono::system_clock::to_time_t(now);
        return static_cast<uint64_t>(now_time_t) < expiry;
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
        int m;  // 搜索计数 (w1 的更新次数)
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
        const SearchToken& token,
        class McOdxtGatekeeper& gatekeeper);
    
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
     * @brief 注册更新 (doc_id <-> keywords)
     */
    void registerUpdate(const std::string& owner_id, const std::string& doc_id, const std::string& keyword);

    /**
     * @brief 获取并递增更新计数
     */
    int getAndIncrementUpdateCount(const std::string& owner_id, const std::string& keyword);
    
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
    const std::vector<uint8_t>& getKm(const std::string& owner_id) const;

private:
    int m_d;
    
    // 每个数据所有者的密钥
    struct OwnerKeys {
        bn_t Ks;           // OPRF 密钥
        bn_t* Kt;          // TSet 密钥数组
        bn_t* Kx;          // XSet 密钥数组
        bn_t Ky;           // PRF 密钥
        int d;             // 密钥数组大小
        std::vector<uint8_t> Km;  // 对称加密密钥
        std::unordered_map<std::string, int> updateCnt;  // 关键词更新计数
        std::unordered_map<std::string, std::vector<std::string>> doc_keywords; // doc_id -> keywords
        std::unordered_map<std::string, std::vector<std::string>> keyword_docs; // keyword -> doc_ids
        
        OwnerKeys() : Kt(nullptr), Kx(nullptr), d(0) {
            bn_null(Ks);
            bn_null(Ky);
        }

        // Move constructor
        OwnerKeys(OwnerKeys&& other) noexcept 
            : Kt(other.Kt), Kx(other.Kx), d(other.d),
              Km(std::move(other.Km)), updateCnt(std::move(other.updateCnt)) {
            std::memcpy(Ks, other.Ks, sizeof(bn_t));
            std::memcpy(Ky, other.Ky, sizeof(bn_t));
            bn_null(other.Ks);
            bn_null(other.Ky);
            other.Kt = nullptr;
            other.Kx = nullptr;
            other.d = 0;
        }

        // Move assignment
        OwnerKeys& operator=(OwnerKeys&& other) noexcept {
            if (this != &other) {
                this->cleanup();
                std::memcpy(Ks, other.Ks, sizeof(bn_t));
                std::memcpy(Ky, other.Ky, sizeof(bn_t));
                Kt = other.Kt;
                Kx = other.Kx;
                d = other.d;
                Km = std::move(other.Km);
                updateCnt = std::move(other.updateCnt);
                bn_null(other.Ks);
                bn_null(other.Ky);
                other.Kt = nullptr;
                other.Kx = nullptr;
                other.d = 0;
            }
            return *this;
        }

        void cleanup() {
            if (Kt) {
                for (int i = 0; i < d; ++i) bn_free(Kt[i]);
                delete[] Kt;
                Kt = nullptr;
            }
            if (Kx) {
                for (int i = 0; i < d; ++i) bn_free(Kx[i]);
                delete[] Kx;
                Kx = nullptr;
            }
            // Only free if they were initialized (not null)
            // In RELIC, bn_free is safe on bn_null'd values usually, 
            // but let's be extra careful if we suspect it causes hangs.
            // Actually, we should check if they were initialized.
            bn_free(Ks);
            bn_free(Ky);
            bn_null(Ks);
            bn_null(Ky);
        }
        
        ~OwnerKeys() {
            cleanup();
        }

        // Disable copy
        OwnerKeys(const OwnerKeys&) = delete;
        OwnerKeys& operator=(const OwnerKeys&) = delete;
    };
    
    std::unordered_map<std::string, OwnerKeys> m_owner_keys;
    std::unordered_map<std::string, bn_t*> m_user_keys;  // 搜索用户密钥 (pointers)
    
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
