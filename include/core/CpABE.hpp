#pragma once

#include <vector>
#include <string>
#include <set>
#include <memory>
#include <iostream>

extern "C" {
#include <relic/relic.h>
}

namespace core::crypto {

    // Attribute set generic wrapper
    using AttributeSet = std::set<std::string>;
    
    // In this CP-ABE variants:
    // Key is associated with AttributeSet S_A
    // Ciphertext is associated with AttributeSet W (Policy is AND of all W)

    struct MasterKey {
        bn_t alpha; 
        bn_t a;     
        ep_t g_alpha; 
        ep_t g_a;     

        MasterKey() {
            bn_new(alpha); bn_new(a);
            ep_new(g_alpha); ep_new(g_a);
        }
        ~MasterKey() {
            bn_free(alpha); bn_free(a);
            ep_free(g_alpha); ep_free(g_a);
        }
        MasterKey(const MasterKey&) = delete;
        MasterKey& operator=(const MasterKey&) = delete;
    };

    struct PublicKey {
        ep_t g;       
        gt_t e_gg_alpha;
        ep_t g_a;     

        PublicKey() {
            ep_new(g); gt_new(e_gg_alpha); ep_new(g_a);
        }
        ~PublicKey() {
            ep_free(g); gt_free(e_gg_alpha); ep_free(g_a);
        }
        PublicKey(const PublicKey&) = delete;
        PublicKey& operator=(const PublicKey&) = delete;
    };

    struct SecretKey {
        ep_t K; // g^alpha * g^{at}
        ep_t L; // g^t
        
        // K_x = H(x)^t for each x in S_A
        struct Component {
            std::string attribute;
            ep_t K_x;
            Component() { ep_new(K_x); }
            ~Component() { ep_free(K_x); }
            Component(Component&&) = default; // Move only
        };
        std::vector<std::unique_ptr<Component>> components;
        AttributeSet attributes; // S_A

        SecretKey() {
            ep_new(K); ep_new(L);
        }
        ~SecretKey() {
            ep_free(K); ep_free(L);
        }
        SecretKey(const SecretKey&) = delete;
        SecretKey& operator=(const SecretKey&) = delete;
    };

    struct Ciphertext {
        AttributeSet policy; // The attribute set W
        gt_t C;  // M * e(g,g)^alpha*s
        ep_t C_prime; // g^s
        
        // For AND policy of W: 
        // We use simple n-out-of-n sharing of s.
        // s = sum(s_i)
        // Components for each w_i in W:
        // C_{1,i} = g^{a s_i} H(w_i)^{-r_i}
        // C_{2,i} = g^{r_i}
        
        struct Component {
            std::string attribute;
            ep_t C_1;
            ep_t C_2;
            Component() { ep_new(C_1); ep_new(C_2); }
            ~Component() { ep_free(C_1); ep_free(C_2); }
             Component(Component&&) = default;
        };
        std::vector<std::unique_ptr<Component>> components;

        Ciphertext() {
            gt_new(C); ep_new(C_prime);
        }
        ~Ciphertext() {
            gt_free(C); ep_free(C_prime);
        }
        Ciphertext(const Ciphertext&) = delete; 
        Ciphertext& operator=(const Ciphertext&) = delete;
    };

    class CpABE {
    public:
        static void globalSetup();
        
        static void setup(MasterKey& mk, PublicKey& pk);
        
        // Key is associated with User Attribute Set
        static void keygen(SecretKey& sk, const MasterKey& mk, const PublicKey& pk, const AttributeSet& userAttrs);
        
        // Encrypt under policy "AND of all attributes in policyAttrs"
        static bool encrypt(Ciphertext& ct, const PublicKey& pk, const AttributeSet& policyAttrs, const gt_t& message);
        
        static bool decrypt(gt_t& recovered_message, const Ciphertext& ct, const SecretKey& sk);
    };

} // namespace core::crypto
