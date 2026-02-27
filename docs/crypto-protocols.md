# Cryptographic Protocol Specifications

## Nomos Baseline Scheme

### Protocol Parameters
- ℓ = 3: Number of cross-tags generated per insertion
- k = 2: Number of cross-tags sampled during query
- λ = 256 bits: Security parameter

### Setup Phase
**Gatekeeper.Setup()**
1. Generate master keys:
   - Ks: Key for TSet address computation
   - Ky: Key for payload encryption
   - Km: Key for cross-tag generation
2. Initialize UpdateCnt = 0
3. Initialize keyword state tracking

### Update Protocol
**Client.Update(op, keyword, id)**
1. Compute TSet address: addr = F_Ks(keyword || UpdateCnt)
2. Compute encrypted payload: val = Enc_Ky(id || op)
3. Generate ℓ cross-tags: xtag_i = F_Km(keyword || id || i) for i ∈ [1, ℓ]
4. Send (addr, val, xtag_list) to Server
5. Server stores in TSet and XSet

### Search Protocol
**Phase 1: Token Generation (Gatekeeper)**
1. Generate stokens for candidate enumeration:
   - stoken_i = F_Ks(keyword || i) for i ∈ [0, UpdateCnt]
2. Generate xtokens for filtering:
   - Sample k cross-tag indices
   - xtoken_j = F_Km(keyword || "*" || j) for sampled indices

**Phase 2: Server Search**
1. Candidate enumeration using stokens
2. Cross-filtering using xtokens
3. Return matching encrypted payloads

**Phase 3: Client Decryption**
1. Decrypt payloads using Ky
2. Filter by operation type (add/delete)
3. Return final result set

## Verifiable Nomos Scheme

### Additional Components

#### QTree (Merkle Hash Tree)
- Capacity: 1024 leaf nodes
- Hash function: SHA-256
- Root commitment: R_X^(t)

**Operations:**
- `updateBit(index, value)`: Update leaf and recompute path to root
- `generateProof(index)`: Generate authentication path
- `verifyPath(proof, root)`: Verify proof against root

#### Address Commitment
**Commitment Scheme:**
- Cm_{w,id} = H_c(xtag_1 || xtag_2 || ... || xtag_ℓ)
- Opening: Reveal all xtags
- Verification: Recompute commitment and compare

**Subset Membership:**
- Check if queried xtags ⊆ committed xtags
- Used for verifiable filtering

### Verification Protocol
1. Server provides search results + proofs
2. Client verifies:
   - QTree proof against root R_X^(t)
   - Address commitment openings
   - Subset membership for cross-tags
3. Accept results only if all verifications pass

## Cryptographic Primitives

### Hash Functions
Implemented in `core/Primitive.{hpp,cpp}`:

**Hash_H1, Hash_H2**: SHA256/SHA384 → G1/G2 curve points
- Used for pairing-based operations

**Hash_G1, Hash_G2**: SHA512/SHA224/SHA384 → G1/G2 curve points
- Used for CP-ABE attribute mapping

**Hash_Zn**: SHA512 → Zn (modular big integers)
- Used for scalar operations

### Elliptic Curve Configuration
- Pairing type: Asymmetric (Type 3)
- Curve: BN254 or BLS12-381 (RELIC default)
- Security level: 128-bit

## Security Assumptions

### Nomos Baseline
- PRF security of F_Ks, F_Ky, F_Km
- IND-CPA security of encryption scheme
- Honest-but-curious server

### Verifiable Nomos
- Collision resistance of hash function H_c
- Binding property of commitment scheme
- Computational Diffie-Hellman assumption (for pairings)
