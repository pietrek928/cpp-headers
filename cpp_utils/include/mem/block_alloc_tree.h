#ifndef __BLOCK_ALLOC_H_
#define __BLOCK_ALLOC_H_

template<class T>
inline int bit_size(T v = 0) {
    return sizeof(T) << 3;
}

template<class T>
inline T full_mask(T v = 0) {
    return ~(T)0;
}

bool is_p2(auto n) {
    return !((n - 1) & n);
}

template<class Tn>
Tn lowest_p2(Tn v) {
    auto s = bit_size(v);
    Tn r = 1;
    while (s >>= 1) {
        auto nr = r << s;
        if (nr <= v) {
            r = nr;
        }
    }
    return r;
}

template<class Tn>
inline Tn div_by_const(Tn v, Tn d) {
    if (is_p2(d)) {
        return v >> lowest_p2(d);
    } else {
        return v / d;
    }
}

template<class Tn>
inline Tn mul_by_const(Tn v, Tn d) {
    if (is_p2(d)) {
        return v << lowest_p2(d);
    } else {
        return v * d;
    }
}

template<class Tn>
inline Tn set_bits(int n, Tn v = 0) {
    return full_mask(v) >> (bit_size(v) - n);
}

//template<class Tbit> bool has_free(Tbit v);
//template<class Tbit> bool has_used(Tbit v)
//template<class Tbit> int first_free(Tbit v);
//template<class Tbit> int last_used(Tbit v);

template<class Tbit>
bool val_has_free(Tbit v) {
    return ~v;
}

template<class Tbit>
bool val_has_used(Tbitv) {
    return v;
}

template<class Tbit>
auto val_set_nbit(Tbit v, int n) {
    return v | (((Tbit)1) << n);
}

template<class Tbit>
auto val_unset_nbit(Tbit v, int n) {
    return v & ~(((Tbit)1) << n);
}

template<class Tbit>
auto val_get_nbit(Tbit v, int n) {
    return (v>>n) & 1;
}

template<class Tbit>
void set_bit_at_pos(Tbit *data, size_t pos) {
    auto &p = data[elem_pos(pos)];
    p = val_set_nbit(p, bit_pos(pos));
}

template<class Tbit>
void unset_bit_at_pos(Tbit *data, size_t pos) {
    auto &p = data[elem_pos(pos)];
    p = val_unset_nbit(p, bit_pos(pos));
}

template<class Tbit>
int val_first_free(Tbit v) {
    v = ~v;
    int r = 0;

    auto l = bit_size(v);
    auto bb = full_mask(v);
    while (l >>= 1) {
        bb >>= l;
        if (!(bb & v)) {
            v >>= l;
            r += l;
        }
    }
    return r;
}

template<class Tbit>
int val_last_used(Tbit v) {
    int r = 0;

    auto l = bit_size(v);
    auto bb = full_mask(v);
    while (l >>= 1) {
        bb <<= l;
        if (set_bits(l) & v) {
            r += l;
        } else {
            v <<= l;
        }
    }
    return r;
}

template<class Tbit, class Tsize = int, class idx_levels = 2>
class BitTree {
    Tbit *data = NULL;
    Tsize data_size = 0;
    Tsize idx_size = 0;

    constexpr auto type_bit_size() {
        return bit_size<Tbit>();
    }

    inline auto reduce_ceil(size_t bit_pos) {
        return div_by_const(
            bit_pos + (type_bit_size() - 1), type_bit_size()
        );
    }

    inline auto nlevel_size(int n) {
        auto r = data_size;
        for (int i=0; i<n-1; i++) {
            r = reduce_ceil(r);
        }
        return r;
    }

    inline auto bit_pos(size_t bit_pos) {
        return bit_pos & set_bits(lowest_p2(type_bit_size()), bit_pos);
    }

    inline auto elem_pos(size_t bit_pos) {
        return div_by_const(bit_pos, type_bit_size());
    }

    bool get_bit_at_pos(size_t pos) {
        auto &p = data[elem_pos(pos)];
        return val_get_nbit(p, bit_pos(pos));
    }

    BitTree(Tbit *data, size_t bit_len) {
        data_size = reduce_ceil(bit_len);
        auto sz = data_size

        idx_size = 1;
        while ((sz = reduce_ceil(sz)) > 1) {
            idx_size += sz;
        }

        std::fill(data, data + (data_size + idx_size), 0);
        std::fill(
            data + (data_size + idx_size),
            data + (data_size + 2 * idx_size),
            full_mask(data[0]);
        );
    }

    inline void update_idx(size_t bit_pos) {
        Tbit *set_idx_pos = data + data_size;
        Tbit *unset_idx_pos = data + data_size + idx_size;
        while ((bit_pos = reduce_ceil(bit_pos)) > 1) {
        }
    }

    void set(size_t bit_pos) {
        set_bit_at_pos(bit_pos);
        update_free_idx(bit_pos);
        update_used_idx(bit_pos);
    }

    void unset(size_t bit_pos) {
        unset_bit_at_pos(bit_pos);
        update_free_idx(bit_pos);
        update_used_idx(bit_pos);
    }

    Tsize first_free() {
        auto idx_ptr = data + nlevel_size(0) + idx_size;

        size_t pos = 0;
        auto top_size = nlevel_size();
        while (pos < top_size && val_has_used(idx_ptr[pos])) {
            pos++;
        }
        // raise ??? !

        for (int i=0; i<idx_levels; i++) {
            auto bit_shift = val_first_free(idx_ptr[pos]);
            pos = mul_by_const(pos, type_bit_size()) | bit_shift;
            idx_ptr += nlevel_size();
        }
        auto bit_shift = val_first_free(data[pos]);
        return mul_by_const(pos, type_bit_size()) | bit_shift;
    }
};

template<class T, class Tid=int, class Tbit = int>
class BlockAlloc {
    T *objs = NULL;
    m128i *chk = NULL;

    BlockAlloc() {
    }
};

#endif /* __BLOCK_ALLOC_H_ */

