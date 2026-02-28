#include "core/CpABE.hpp"

#include <vector>

#include "core/Primitive.hpp"

namespace core {
namespace crypto {

// Helper to get group order
static void get_order(bn_t order) { ep_curve_get_ord(order); }

void CpABE::globalSetup() {
  if (core_get() == NULL) {
    core_init();
    pc_param_set_any();
  }
}

void CpABE::setup(MasterKey& mk, PublicKey& pk) {
  bn_t order;
  bn_new(order);
  get_order(order);

  bn_rand_mod(mk.alpha, order);
  bn_rand_mod(mk.a, order);

  ep_rand(pk.g);
  ep2_rand(pk.h);

  // e_gh_alpha = e(g,h)^alpha
  ep2_mul(mk.h_alpha, pk.h, mk.alpha);

  ep_norm(pk.g, pk.g);
  ep2_norm(mk.h_alpha, mk.h_alpha);
  pc_map(pk.e_gh_alpha, pk.g, mk.h_alpha);

  // g_a = g^a, h_a = h^a
  ep_mul(pk.g_a, pk.g, mk.a);
  ep2_mul(pk.h_a, pk.h, mk.a);

  bn_free(order);
}

void CpABE::keygen(SecretKey& sk, const MasterKey& mk, const PublicKey& pk,
                   const AttributeSet& userAttrs) {
  bn_t order, t;
  bn_new(order);
  bn_new(t);
  get_order(order);

  bn_rand_mod(t, order);

  // K = h^alpha * h^{at}
  ep2_t temp;
  ep2_new(temp);
  ep2_mul(temp, pk.h_a, t);
  ep2_add(sk.K, mk.h_alpha, temp);

  // L = h^t
  ep2_mul(sk.L, pk.h, t);

  sk.attributes = userAttrs;
  sk.components.clear();

  for (const auto& attr : userAttrs) {
    auto comp =
        std::unique_ptr<SecretKey::Component>(new SecretKey::Component());
    comp->attribute = attr;

    // K_x = H(x)^t (in G2)
    // H(x) = h^u where u = Hash_Zn(x)
    bn_t u;
    bn_new(u);
    Hash_Zn(u, attr);

    ep2_t h_u;
    ep2_new(h_u);
    ep2_mul(h_u, pk.h, u);

    ep2_mul(comp->K_x, h_u, t);

    sk.components.push_back(std::move(comp));
    ep2_free(h_u);
    bn_free(u);
  }

  bn_free(order);
  bn_free(t);
  ep2_free(temp);
}

bool CpABE::encrypt(Ciphertext& ct, const PublicKey& pk,
                    const AttributeSet& policyAttrs, const gt_t& message) {
  ct.policy = policyAttrs;
  ct.components.clear();

  bn_t order, s;
  bn_new(order);
  bn_new(s);
  get_order(order);

  // Global secret s
  bn_rand_mod(s, order);

  // C = M * e(g,h)^{alpha s}
  gt_t mask;
  gt_new(mask);
  gt_exp(mask, pk.e_gh_alpha, s);

  // DEBUG
  // std::cout << "DEBUG: Encryption mask hex: " << std::endl;
  // gt_print(mask);

  gt_mul(ct.C, message, mask);

  // C' = g^s
  ep_mul(ct.C_prime, pk.g, s);

  // Split s into shares s_i for each attribute in policy
  // s = sum(s_i)
  size_t n = policyAttrs.size();
  std::vector<bn_t*> shares(n);
  bn_t running_sum;
  bn_new(running_sum);
  bn_zero(running_sum);

  for (size_t i = 0; i < n; ++i) {
    shares[i] = new bn_t[1];
    bn_new(*shares[i]);
  }

  // Random shares for n-1
  for (size_t i = 0; i < n - 1; ++i) {
    bn_rand_mod(*shares[i], order);
    bn_add(running_sum, running_sum, *shares[i]);
    bn_mod(running_sum, running_sum, order);
  }
  // Last share
  bn_sub(*shares[n - 1], s, running_sum);
  bn_mod(*shares[n - 1], *shares[n - 1], order);
  if (bn_sign(*shares[n - 1]) == RLC_NEG) {
    bn_add(*shares[n - 1], *shares[n - 1], order);
  }

  size_t idx = 0;
  for (const auto& attr : policyAttrs) {
    auto comp =
        std::unique_ptr<Ciphertext::Component>(new Ciphertext::Component());
    comp->attribute = attr;

    bn_t r;
    bn_new(r);
    bn_rand_mod(r, order);

    // C_1 = g^{a s_i} H(w)^{-r}
    // H(w) = g^u where u = Hash_Zn(w)
    ep_t term1, term2, h_val;
    ep_new(term1);
    ep_new(term2);
    ep_new(h_val);

    ep_mul(term1, pk.g_a, *shares[idx]);

    bn_t u;
    bn_new(u);
    Hash_Zn(u, attr);

    ep_mul(h_val, pk.g, u);  // H(w) in G1

    ep_mul(term2, h_val, r);
    ep_neg(term2, term2);
    ep_add(comp->C_1, term1, term2);

    // C_2 = g^r
    ep_mul(comp->C_2, pk.g, r);

    ct.components.push_back(std::move(comp));

    bn_free(r);
    bn_free(u);
    ep_free(term1);
    ep_free(term2);
    ep_free(h_val);
    idx++;
  }

  // Cleanup
  for (size_t i = 0; i < n; ++i) {
    bn_free(*shares[i]);
    delete[] shares[i];
  }
  bn_free(running_sum);
  bn_free(order);
  bn_free(s);
  gt_free(mask);

  return true;
}

bool CpABE::decrypt(gt_t& recovered_message, const Ciphertext& ct,
                    const SecretKey& sk) {
  // Check Subset: Policy \subseteq UserAttributes
  for (const auto& reqAttr : ct.policy) {
    if (sk.attributes.find(reqAttr) == sk.attributes.end()) {
      return false;
    }
  }

  gt_t A, term, pair1, pair2;
  gt_new(A);
  gt_new(term);
  gt_new(pair1);
  gt_new(pair2);
  gt_set_unity(A);

  // Reconstruct e(g,g)^{a t s}
  // = Product [ e(C_{1,i}, L) * e(C_{2,i}, K_{w_i}) ]
  // = Product [ e(g^{a s_i} H^{-r}, g^t) * e(g^r, H^t) ]
  // = Product [ e(g,g)^{a t s_i} * e(H,g)^{-rt} * e(g,H)^{rt} ]
  // = Product [ e(g,g)^{a t s_i} ]  (cancellations)
  // = e(g,g)^{a t sum(s_i)} = e(g,g)^{a t s}

  for (const auto& ct_comp : ct.components) {
    // Find corresponding SK component
    // We know it exists from subset check
    const SecretKey::Component* sk_comp_ptr = nullptr;
    for (const auto& sc : sk.components) {
      if (sc->attribute == ct_comp->attribute) {
        sk_comp_ptr = sc.get();
        break;
      }
    }

    // e(C1, L)
    ep_t n_C1;
    ep2_t n_L;
    ep_new(n_C1);
    ep2_new(n_L);
    ep_norm(n_C1, ct_comp->C_1);
    ep2_norm(n_L, sk.L);
    pc_map(pair1, n_C1, n_L);

    // e(C2, K_x)
    ep_t n_C2;
    ep2_t n_Kx;
    ep_new(n_C2);
    ep2_new(n_Kx);
    ep_norm(n_C2, ct_comp->C_2);
    ep2_norm(n_Kx, sk_comp_ptr->K_x);
    pc_map(pair2, n_C2, n_Kx);

    gt_mul(term, pair1, pair2);
    gt_mul(A, A, term);

    ep_free(n_C1);
    ep2_free(n_L);
    ep_free(n_C2);
    ep2_free(n_Kx);
  }

  // Num = e(C', K) = e(g^s, g^\alpha g^{at}) = e(g,g)^{s alpha} * e(g,g)^{s a
  // t}
  gt_t Num;
  gt_new(Num);
  ep_t n_Cprime;
  ep2_t n_K;
  ep_new(n_Cprime);
  ep2_new(n_K);
  ep_norm(n_Cprime, ct.C_prime);
  ep2_norm(n_K, sk.K);
  pc_map(Num, n_Cprime, n_K);

  ep_free(n_Cprime);
  ep2_free(n_K);

  // Res = Num / A
  //     = (e(g,g)^{s alpha} * e(g,g)^{s a t}) / e(g,g)^{s a t}
  //     = e(g,g)^{s alpha}
  gt_t Res;
  gt_new(Res);

  gt_inv(A, A);
  gt_mul(Res, Num, A);

  // Recover M = C / Res
  gt_inv(Res, Res);
  gt_mul(recovered_message, ct.C, Res);

  gt_free(A);
  gt_free(term);
  gt_free(pair1);
  gt_free(pair2);
  gt_free(Num);
  gt_free(Res);

  return true;
}

}  // namespace crypto
}  // namespace core
