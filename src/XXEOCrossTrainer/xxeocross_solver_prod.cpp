#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>
#include <emscripten/heap.h>
#include <emscripten.h>
#endif
#include <iostream>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <string>
#include <sstream>
#include <cstdlib>
#include <random>
#include <cmath>
#include <cstdint>
#include <tsl/robin_set.h>

struct State
{
    std::vector<int> cp;
    std::vector<int> co;
    std::vector<int> ep;
    std::vector<int> eo;

    State(std::vector<int> arg_cp = {0, 1, 2, 3, 4, 5, 6, 7},
          std::vector<int> arg_co = {0, 0, 0, 0, 0, 0, 0, 0},
          std::vector<int> arg_ep = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11},
          std::vector<int> arg_eo = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0})
        : cp(arg_cp), co(arg_co), ep(arg_ep), eo(arg_eo) {}

    State apply_move(const State &move) const
    {
        std::vector<int> new_cp;
        std::vector<int> new_co;
        std::vector<int> new_ep;
        std::vector<int> new_eo;
        new_cp.reserve(8);
        new_co.reserve(8);
        new_ep.reserve(12);
        new_eo.reserve(12);
        for (int i = 0; i < 8; ++i)
        {
            int p = move.cp[i];
            new_cp.emplace_back(cp[p]);
            new_co.emplace_back((co[p] + move.co[i]) % 3);
        }
        for (int i = 0; i < 12; ++i)
        {
            int p = move.ep[i];
            new_ep.emplace_back(ep[p]);
            new_eo.emplace_back((eo[p] + move.eo[i]) % 2);
        }
        return State(new_cp, new_co, new_ep, new_eo);
    }

    State apply_move_edge(const State &move, int e) const
    {
        std::vector<int> new_ep(12, -1);
        std::vector<int> new_eo(12, -1);
        auto it = std::find(ep.begin(), ep.end(), e);
        int index = std::distance(ep.begin(), it);
        it = std::find(move.ep.begin(), move.ep.end(), e);
        int index_next = std::distance(move.ep.begin(), it);
        new_ep[index_next] = e;
        new_eo[index_next] = (eo[index] + move.eo[index_next]) % 2;
        return State(cp, co, new_ep, new_eo);
    }

    State apply_move_corner(const State &move, int c) const
    {
        std::vector<int> new_cp(8, -1);
        std::vector<int> new_co(8, -1);
        auto it = std::find(cp.begin(), cp.end(), c);
        int index = std::distance(cp.begin(), it);
        it = std::find(move.cp.begin(), move.cp.end(), c);
        int index_next = std::distance(move.cp.begin(), it);
        new_cp[index_next] = c;
        new_co[index_next] = (co[index] + move.co[index_next]) % 3;
        return State(new_cp, new_co, ep, eo);
    }
};

std::unordered_map<std::string, State> moves = {
    {"U", State({3, 0, 1, 2, 4, 5, 6, 7}, {0, 0, 0, 0, 0, 0, 0, 0}, {0, 1, 2, 3, 7, 4, 5, 6, 8, 9, 10, 11}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0})},
    {"U2", State({2, 3, 0, 1, 4, 5, 6, 7}, {0, 0, 0, 0, 0, 0, 0, 0}, {0, 1, 2, 3, 6, 7, 4, 5, 8, 9, 10, 11}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0})},
    {"U'", State({1, 2, 3, 0, 4, 5, 6, 7}, {0, 0, 0, 0, 0, 0, 0, 0}, {0, 1, 2, 3, 5, 6, 7, 4, 8, 9, 10, 11}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0})},
    {"D", State({0, 1, 2, 3, 5, 6, 7, 4}, {0, 0, 0, 0, 0, 0, 0, 0}, {0, 1, 2, 3, 4, 5, 6, 7, 9, 10, 11, 8}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0})},
    {"D2", State({0, 1, 2, 3, 6, 7, 4, 5}, {0, 0, 0, 0, 0, 0, 0, 0}, {0, 1, 2, 3, 4, 5, 6, 7, 10, 11, 8, 9}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0})},
    {"D'", State({0, 1, 2, 3, 7, 4, 5, 6}, {0, 0, 0, 0, 0, 0, 0, 0}, {0, 1, 2, 3, 4, 5, 6, 7, 11, 8, 9, 10}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0})},
    {"L", State({4, 1, 2, 0, 7, 5, 6, 3}, {2, 0, 0, 1, 1, 0, 0, 2}, {11, 1, 2, 7, 4, 5, 6, 0, 8, 9, 10, 3}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0})},
    {"L2", State({7, 1, 2, 4, 3, 5, 6, 0}, {0, 0, 0, 0, 0, 0, 0, 0}, {3, 1, 2, 0, 4, 5, 6, 11, 8, 9, 10, 7}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0})},
    {"L'", State({3, 1, 2, 7, 0, 5, 6, 4}, {2, 0, 0, 1, 1, 0, 0, 2}, {7, 1, 2, 11, 4, 5, 6, 3, 8, 9, 10, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0})},
    {"R", State({0, 2, 6, 3, 4, 1, 5, 7}, {0, 1, 2, 0, 0, 2, 1, 0}, {0, 5, 9, 3, 4, 2, 6, 7, 8, 1, 10, 11}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0})},
    {"R2", State({0, 6, 5, 3, 4, 2, 1, 7}, {0, 0, 0, 0, 0, 0, 0, 0}, {0, 2, 1, 3, 4, 9, 6, 7, 8, 5, 10, 11}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0})},
    {"R'", State({0, 5, 1, 3, 4, 6, 2, 7}, {0, 1, 2, 0, 0, 2, 1, 0}, {0, 9, 5, 3, 4, 1, 6, 7, 8, 2, 10, 11}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0})},
    {"F", State({0, 1, 3, 7, 4, 5, 2, 6}, {0, 0, 1, 2, 0, 0, 2, 1}, {0, 1, 6, 10, 4, 5, 3, 7, 8, 9, 2, 11}, {0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0})},
    {"F2", State({0, 1, 7, 6, 4, 5, 3, 2}, {0, 0, 0, 0, 0, 0, 0, 0}, {0, 1, 3, 2, 4, 5, 10, 7, 8, 9, 6, 11}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0})},
    {"F'", State({0, 1, 6, 2, 4, 5, 7, 3}, {0, 0, 1, 2, 0, 0, 2, 1}, {0, 1, 10, 6, 4, 5, 2, 7, 8, 9, 3, 11}, {0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0})},
    {"B", State({1, 5, 2, 3, 0, 4, 6, 7}, {1, 2, 0, 0, 2, 1, 0, 0}, {4, 8, 2, 3, 1, 5, 6, 7, 0, 9, 10, 11}, {1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0})},
    {"B2", State({5, 4, 2, 3, 1, 0, 6, 7}, {0, 0, 0, 0, 0, 0, 0, 0}, {1, 0, 2, 3, 8, 5, 6, 7, 4, 9, 10, 11}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0})},
    {"B'", State({4, 0, 2, 3, 5, 1, 6, 7}, {1, 2, 0, 0, 2, 1, 0, 0}, {8, 4, 2, 3, 0, 5, 6, 7, 1, 9, 10, 11}, {1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0})}};

std::vector<std::string> move_names = {"U", "U2", "U'", "D", "D2", "D'", "L", "L2", "L'", "R", "R2", "R'", "F", "F2", "F'", "B", "B2", "B'"};

std::string AlgToString(const std::vector<int> &alg)
{
    std::string result = "";
    for (int i : alg)
    {
        result += move_names[i] + " ";
    }
    return result;
}

std::vector<int> StringToAlg(const std::string &str)
{
    std::vector<int> alg;
    std::istringstream iss(str);
    std::string name;
    while (iss >> name)
    {
        if (!name.empty())
        {
            auto it = std::find(move_names.begin(), move_names.end(), name);
            if (it != move_names.end())
            {
                alg.emplace_back(std::distance(move_names.begin(), it));
            }
        }
    }
    return alg;
}

std::vector<std::vector<int>> c_array = {{0}, {1, 1, 1, 1, 1, 1}, {1, 2, 4, 8, 16, 32}, {1, 3, 9, 27, 81, 243}};
std::vector<std::vector<int>> base_array = {{0}, {0}, {1, 12, 12 * 11, 12 * 11 * 10, 12 * 11 * 10 * 9, 12 * 11 * 10 * 9 * 8}, {1, 8, 8 * 7, 8 * 7 * 6, 8 * 7 * 6 * 5}};

inline int array_to_index(std::vector<int> &a, int n, int c, int pn)
{
    int index_p = 0;
    int index_o = 0;
    int tmp;
    int tmp2 = 24 / pn;
    for (int i = 0; i < n; ++i)
    {
        index_o += (a[i] % c) * c_array[c][n - i - 1];
        a[i] /= c;
    }
    for (int i = 0; i < n; ++i)
    {
        tmp = 0;
        for (int j = 0; j < i; ++j)
        {
            if (a[j] < a[i])
            {
                tmp++;
            }
        }
        index_p += (a[i] - tmp) * base_array[tmp2][i];
    }
    return index_p * c_array[c][n] + index_o;
}

std::vector<int> sorted(6);
std::vector<std::vector<int>> base_array2 = {{0}, {0}, {12, 11, 10, 9, 8, 7}, {8, 7, 6, 5, 4}};

inline void index_to_array(std::vector<int> &p, int index, int n, int c, int pn)
{
    int tmp2 = 24 / pn;
    int p_index = index / c_array[c][n];
    int o_index = index % c_array[c][n];
    for (int i = 0; i < n; ++i)
    {
        p[i] = p_index % base_array2[tmp2][i];
        p_index /= base_array2[tmp2][i];
        std::sort(sorted.begin(), sorted.begin() + i);
        for (int j = 0; j < i; ++j)
        {
            if (sorted[j] <= p[i])
            {
                p[i] += 1;
            }
        }
        sorted[i] = p[i];
    }
    for (int i = 0; i < n; ++i)
    {
        p[n - i - 1] = 18 * (c * p[n - i - 1] + o_index % c);
        o_index /= c;
    }
}

std::vector<std::vector<int>> c_array2 = {{0}, {0}, {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048}, {1, 3, 9, 27, 81, 243, 729, 2187}};

inline int o_to_index(const std::vector<int> &o, int c, int pn)
{
    int o_index = 0;
    for (int i = 0; i < pn - 1; ++i)
    {
        o_index += o[i] * c_array2[c][pn - i - 2];
    }
    return o_index;
}

inline void index_to_o(std::vector<int> &o, int index, int c, int pn)
{
    int count = 0;
    for (int i = 0; i < pn - 1; ++i)
    {
        o[pn - i - 2] = index % c;
        count += o[pn - i - 2];
        index /= c;
    }
    o[pn - 1] = (c - count % c) % c;
}

std::vector<int> create_ep_move_table()
{
    std::vector<int> move_table(12 * 18, -1);
    for (int i = 0; i < 12; ++i)
    {
        std::vector<int> ep(12, -1);
        std::vector<int> eo(12, -1);
        std::vector<int> cp(8, 0);
        std::vector<int> co(8, 0);
        ep[i] = i;
        eo[i] = 0;
        State state(cp, co, ep, eo);
        for (int j = 0; j < 18; ++j)
        {
            State new_state = state.apply_move_edge(moves[move_names[j]], i);
            auto it = std::find(new_state.ep.begin(), new_state.ep.end(), i);
            move_table[18 * i + j] = std::distance(new_state.ep.begin(), it);
        }
    }
    return move_table;
}

std::vector<int> create_corner_move_table()
{
    std::vector<int> move_table(24 * 18, -1);
    int index;
    for (int i = 0; i < 24; ++i)
    {
        std::vector<int> ep(12, 0);
        std::vector<int> eo(12, 0);
        std::vector<int> cp(8, -1);
        std::vector<int> co(8, -1);
        cp[i / 3] = i / 3;
        co[i / 3] = i % 3;
        State state(cp, co, ep, eo);
        for (int j = 0; j < 18; ++j)
        {
            State new_state = state.apply_move_corner(moves[move_names[j]], i / 3);
            auto it = std::find(new_state.cp.begin(), new_state.cp.end(), i / 3);
            index = std::distance(new_state.cp.begin(), it);
            move_table[18 * i + j] = 3 * index + new_state.co[index];
        }
    }
    return move_table;
}

std::vector<int> create_eo_move_table()
{
    std::vector<int> move_table(2048 * 18, -1);
    for (int i = 0; i < 2048; ++i)
    {
        std::vector<int> ep(12, 0);
        std::vector<int> eo(12, 0);
        std::vector<int> cp(8, 0);
        std::vector<int> co(8, 0);
        index_to_o(eo, i, 2, 12);
        State state(cp, co, ep, eo);
        for (int j = 0; j < 18; ++j)
        {
            State new_state = state.apply_move(moves[move_names[j]]);
            move_table[18 * i + j] = o_to_index(new_state.eo, 2, 12);
        }
    }
    return move_table;
}

void create_multi_move_table(int n, int c, int pn, int size, const std::vector<int> &table, std::vector<int> &move_table)
{
    move_table = std::vector<int>(size * 18, -1);
    int tmp;
    int tmp_i;
    std::vector<int> a(n);
    std::vector<int> b(n);
    std::vector<int> inv_move = {2, 1, 0, 5, 4, 3, 8, 7, 6, 11, 10, 9, 14, 13, 12, 17, 16, 15};
    for (int i = 0; i < size; ++i)
    {
        index_to_array(a, i, n, c, pn);
        tmp_i = i * 18;
        for (int j = 0; j < 18; ++j)
        {
            if (move_table[tmp_i + j] == -1)
            {
                for (int k = 0; k < n; ++k)
                {
                    b[k] = table[a[k] + j];
                }
                tmp = array_to_index(b, n, c, pn);
                move_table[tmp_i + j] = tmp;
                move_table[tmp * 18 + inv_move[j]] = i;
            }
        }
    }
}

void create_prune_table(int index, int size, int depth, const std::vector<int> &table, std::vector<unsigned char> &prune_table)
{
    prune_table = std::vector<unsigned char>(size, 255);
    prune_table[index] = 0;
    int next_i;
    int index_tmp;
    int next_d;
    for (int d = 0; d < depth; ++d)
    {
        next_d = d + 1;
        for (int i = 0; i < size; ++i)
        {
            if (prune_table[i] == d)
            {
                index_tmp = i * 18;
                for (int j = 0; j < 18; ++j)
                {
                    next_i = table[index_tmp + j];
                    if (prune_table[next_i] == 255)
                    {
                        prune_table[next_i] = next_d;
                    }
                }
            }
        }
    }
}

void create_prune_table2(int index1, int index2, int size1, int size2, int depth,
                         const std::vector<int> &table1, const std::vector<int> &table2,
                         std::vector<unsigned char> &prune_table)
{
    int size = size1 * size2;
    prune_table = std::vector<unsigned char>(size, 255);
    prune_table[index1 * size2 + index2] = 0;
    int next_i;
    int index1_tmp;
    int index2_tmp;
    int next_d;
    int num_filled = 1;
    int num_old = 1;

    for (int d = 0; d < depth; ++d)
    {
        next_d = d + 1;
        for (int i = 0; i < size; ++i)
        {
            if (prune_table[i] == d)
            {
                index1_tmp = (i / size2) * 18;
                index2_tmp = (i % size2) * 18;
                for (int j = 0; j < 18; ++j)
                {
                    next_i = table1[index1_tmp + j] * size2 + table2[index2_tmp + j];
                    if (prune_table[next_i] == 255)
                    {
                        prune_table[next_i] = next_d;
                        num_filled++;
                    }
                }
            }
        }
        if (num_filled == num_old)
        {
            break;
        }
        num_old = num_filled;
    }
}

std::vector<bool> create_ma_table()
{
    std::vector<bool> ma;
    ma.reserve(19 * 18);
    bool condition;
    for (int prev = 0; prev < 19; ++prev)
    {
        for (int i = 0; i < 18; ++i)
        {
            condition = (prev < 18) &&
                        (i / 3 == prev / 3 ||
                         ((i / 3) / 2 == (prev / 3) / 2 && (prev / 3) % 2 > (i / 3) % 2));
            ma.emplace_back(condition);
        }
    }
    return ma;
}

static constexpr float LOAD_FACTOR = 0.90f;

// Bucket sizing configuration: 2M / 2M / 1M / 1M
static constexpr size_t BUCKET_2M = (1ULL << 21); // 2,097,152
static constexpr size_t BUCKET_1M = (1ULL << 20); // 1,048,576

static constexpr size_t TARGET_NODES_2M = static_cast<size_t>(BUCKET_2M * LOAD_FACTOR); // ~1,887,436 nodes
static constexpr size_t TARGET_NODES_1M = static_cast<size_t>(BUCKET_1M * LOAD_FACTOR); // ~943,718 nodes


struct xxeocross_search
{
    std::vector<int> sol;
    std::string scramble;
    std::string tmp;
    std::mt19937_64 generator;

    // Move tables
    std::vector<int> ep_move_table;
    std::vector<int> corner_move_table;
    std::vector<int> eo_move_table;

    std::vector<int> multi_move_table_4ep; // Cross 4 EP (size 11880)
    std::vector<int> multi_move_table_2ep; // Slot 2 EP (size 132)
    std::vector<int> multi_move_table_2c;  // Slot 2 Corners (size 504)

    // State sizes
    static constexpr uint64_t SIZE_4EP = 11880ULL; // 12 * 11 * 10 * 9
    static constexpr uint64_t SIZE_2EP = 132ULL;   // 12 * 11
    static constexpr uint64_t SIZE_2C = 504ULL;    // 8 * 7 * 9
    static constexpr uint64_t SIZE_EO = 2048ULL;   // 2^11

    static constexpr uint64_t SIZE_2C_EO = SIZE_2C * SIZE_EO;
    static constexpr uint64_t SIZE_2EP_2C_EO = SIZE_2EP * SIZE_2C_EO;

    // Search databases for 3 geometric configurations
    std::vector<std::vector<uint64_t>> index_pairs_para;  // BL BR (Parallel)
    std::vector<std::vector<uint64_t>> index_pairs_ortho; // BL FL (Orthogonal)
    std::vector<std::vector<uint64_t>> index_pairs_diag;  // BL FR (Diagonal)

    std::vector<int> num_list_para;
    std::vector<int> num_list_ortho;
    std::vector<int> num_list_diag;

    std::vector<bool> ma;

    // Goal states
    int goal_4ep;
    int goal_eo;

    // Parallel: BL (edge 0, c 12) & BR (edge 1, c 15)
    int goal_2ep_para;
    int goal_2c_para;

    // Orthogonal: BL (edge 0, c 12) & FL (edge 3, c 21)
    int goal_2ep_ortho;
    int goal_2c_ortho;

    // Diagonal: BL (edge 0, c 12) & FR (edge 2, c 18)
    int goal_2ep_diag;
    int goal_2c_diag;

    // Pruning tables
    std::vector<unsigned char> prune_table_4ep_eo;
    std::vector<unsigned char> prune_table_4ep_c_bl;
    std::vector<unsigned char> prune_table_4ep_c_br;
    std::vector<unsigned char> prune_table_4ep_c_fl;
    std::vector<unsigned char> prune_table_4ep_c_fr;

    xxeocross_search()
    {
        std::random_device rd;
        generator.seed(rd());

        // 1. Create basic move tables
        ep_move_table = create_ep_move_table();
        corner_move_table = create_corner_move_table();
        eo_move_table = create_eo_move_table();

        // 2. Create multi move tables
        // 4 EP (Cross: DF=8, DL=9, DB=10, DR=11, orientation=1, pn=12)
        create_multi_move_table(4, 1, 12, SIZE_4EP, ep_move_table, multi_move_table_4ep);

        // 2 EP (Slot 2 Edges: orientation=1, pn=12)
        create_multi_move_table(2, 1, 12, SIZE_2EP, ep_move_table, multi_move_table_2ep);

        // 2 Corners (Slot 2 Corners: orientation=3, pn=8)
        create_multi_move_table(2, 3, 8, SIZE_2C, corner_move_table, multi_move_table_2c);

        // 3. Goal configurations
        std::vector<int> ep4_goal = {8, 9, 10, 11};
        goal_4ep = array_to_index(ep4_goal, 4, 1, 12);
        goal_eo = 0;

        // Parallel (BL & BR)
        std::vector<int> ep2_para = {0, 1};
        std::vector<int> c2_para = {4 * 3 + 0, 5 * 3 + 0};
        goal_2ep_para = array_to_index(ep2_para, 2, 1, 12);
        goal_2c_para = array_to_index(c2_para, 2, 3, 8);

        // Orthogonal (BL & FL)
        std::vector<int> ep2_ortho = {0, 3};
        std::vector<int> c2_ortho = {4 * 3 + 0, 7 * 3 + 0};
        goal_2ep_ortho = array_to_index(ep2_ortho, 2, 1, 12);
        goal_2c_ortho = array_to_index(c2_ortho, 2, 3, 8);

        // Diagonal (BL & FR)
        std::vector<int> ep2_diag = {0, 2};
        std::vector<int> c2_diag = {4 * 3 + 0, 6 * 3 + 0};
        goal_2ep_diag = array_to_index(ep2_diag, 2, 1, 12);
        goal_2c_diag = array_to_index(c2_diag, 2, 3, 8);

        // h_eo: 4 EP * Full EO (depth 10 limit)
        create_prune_table2(goal_4ep, goal_eo, SIZE_4EP, SIZE_EO, 10,
                            multi_move_table_4ep, eo_move_table, prune_table_4ep_eo);

        // h_c: 4 EP * Corner (depth 10 limit)
        int goal_c_bl = 4 * 3 + 0; // DLB (12)
        int goal_c_br = 5 * 3 + 0; // DBR (15)
        int goal_c_fr = 6 * 3 + 0; // DFR (18)
        int goal_c_fl = 7 * 3 + 0; // DFL (21)

        create_prune_table2(goal_4ep, goal_c_bl, SIZE_4EP, 24, 10,
                            multi_move_table_4ep, corner_move_table, prune_table_4ep_c_bl);

        create_prune_table2(goal_4ep, goal_c_br, SIZE_4EP, 24, 10,
                            multi_move_table_4ep, corner_move_table, prune_table_4ep_c_br);

        create_prune_table2(goal_4ep, goal_c_fl, SIZE_4EP, 24, 10,
                            multi_move_table_4ep, corner_move_table, prune_table_4ep_c_fl);

        create_prune_table2(goal_4ep, goal_c_fr, SIZE_4EP, 24, 10,
                            multi_move_table_4ep, corner_move_table, prune_table_4ep_c_fr);

        ma = create_ma_table();

        build_database();


    }


    void expand_depth_partial(int parent_d, int next_d,
                              const tsl::robin_set<uint64_t> &parent_set,
                              const std::vector<uint64_t> &parent_vec,
                              const tsl::robin_set<uint64_t> *grandparent_set,
                              size_t bucket_size,
                              size_t target_nodes,
                              std::vector<std::vector<uint64_t>> &target_pairs,
                              std::vector<int> &target_num_list)
    {
        tsl::robin_set<uint64_t> next_set;
        next_set.max_load_factor(LOAD_FACTOR);
        next_set.rehash(bucket_size);

        target_pairs[next_d].clear();
        target_pairs[next_d].reserve(target_nodes);

        std::uniform_int_distribution<size_t> parent_dist(0, parent_vec.size() - 1);
        std::uniform_int_distribution<int> move_dist(0, 17);

        size_t max_attempts = target_nodes * 12;
        size_t attempts = 0;

        while (next_set.size() < target_nodes && attempts < max_attempts)
        {
            attempts++;
            uint64_t p_node = parent_vec[parent_dist(generator)];

            int idx_4ep = static_cast<int>(p_node / SIZE_2EP_2C_EO);
            uint64_t rem1 = p_node % SIZE_2EP_2C_EO;
            int idx_2ep = static_cast<int>(rem1 / SIZE_2C_EO);
            uint64_t rem2 = rem1 % SIZE_2C_EO;
            int idx_2c = static_cast<int>(rem2 / SIZE_EO);
            int idx_eo = static_cast<int>(rem2 % SIZE_EO);

            int move = move_dist(generator);

            int next_4ep = multi_move_table_4ep[idx_4ep * 18 + move];
            int next_2ep = multi_move_table_2ep[idx_2ep * 18 + move];
            int next_2c = multi_move_table_2c[idx_2c * 18 + move];
            int next_eo = eo_move_table[idx_eo * 18 + move];

            uint64_t next_node = static_cast<uint64_t>(next_4ep) * SIZE_2EP_2C_EO +
                                 static_cast<uint64_t>(next_2ep) * SIZE_2C_EO +
                                 static_cast<uint64_t>(next_2c) * SIZE_EO +
                                 next_eo;

            if (parent_set.find(next_node) != parent_set.end() ||
                next_set.find(next_node) != next_set.end())
            {
                continue;
            }

            if (grandparent_set != nullptr)
            {
                bool back_to_grandparent = false;
                int b_4ep_stride = next_4ep * 18;
                int b_2ep_stride = next_2ep * 18;
                int b_2c_stride = next_2c * 18;
                int b_eo_stride = next_eo * 18;

                for (int b_move = 0; b_move < 18; ++b_move)
                {
                    int prev_4ep = multi_move_table_4ep[b_4ep_stride + b_move];
                    int prev_2ep = multi_move_table_2ep[b_2ep_stride + b_move];
                    int prev_2c = multi_move_table_2c[b_2c_stride + b_move];
                    int prev_eo = eo_move_table[b_eo_stride + b_move];

                    uint64_t prev_node = static_cast<uint64_t>(prev_4ep) * SIZE_2EP_2C_EO +
                                         static_cast<uint64_t>(prev_2ep) * SIZE_2C_EO +
                                         static_cast<uint64_t>(prev_2c) * SIZE_EO +
                                         prev_eo;

                    if (grandparent_set->find(prev_node) != grandparent_set->end())
                    {
                        back_to_grandparent = true;
                        break;
                    }
                }

                if (back_to_grandparent)
                {
                    continue;
                }
            }

            if (next_set.will_rehash_on_next_insert())
            {
                break;
            }

            next_set.insert(next_node);
            target_pairs[next_d].push_back(next_node);
        }

        target_pairs[next_d].shrink_to_fit();
        target_num_list[next_d] = static_cast<int>(target_pairs[next_d].size());
        std::cout << "Depth " << next_d << ": " << target_num_list[next_d]
                  << " nodes (Bucket: " << (bucket_size >> 20) << "M)" << std::endl;

        {
            tsl::robin_set<uint64_t> temp;
            next_set.swap(temp);
        }
    }

    void build_single_database(uint64_t goal_composite,
                               std::vector<std::vector<uint64_t>> &target_pairs,
                               std::vector<int> &target_num_list,
                               const std::string &label)
    {
        target_pairs.resize(15);
        target_num_list.resize(15, 0);

        target_pairs[0].push_back(goal_composite);
        target_num_list[0] = 1;

        std::cout << "\n=== Phase 1: Full BFS (Depths 1-6) [" << label << "] ===" << std::endl;
        std::cout << "Depth 0: " << target_num_list[0] << " nodes" << std::endl;

        tsl::robin_set<uint64_t> prev_set, cur_set, next_set;
        prev_set.max_load_factor(LOAD_FACTOR);
        cur_set.max_load_factor(LOAD_FACTOR);
        next_set.max_load_factor(LOAD_FACTOR);

        cur_set.insert(goal_composite);

        for (int d = 1; d <= 6; ++d)
        {
            next_set.clear();
            for (uint64_t node : cur_set)
            {
                int idx_4ep = static_cast<int>(node / SIZE_2EP_2C_EO);
                uint64_t rem1 = node % SIZE_2EP_2C_EO;
                int idx_2ep = static_cast<int>(rem1 / SIZE_2C_EO);
                uint64_t rem2 = rem1 % SIZE_2C_EO;
                int idx_2c = static_cast<int>(rem2 / SIZE_EO);
                int idx_eo = static_cast<int>(rem2 % SIZE_EO);

                int stride_4ep = idx_4ep * 18;
                int stride_2ep = idx_2ep * 18;
                int stride_2c = idx_2c * 18;
                int stride_eo = idx_eo * 18;

                for (int move = 0; move < 18; ++move)
                {
                    int next_4ep = multi_move_table_4ep[stride_4ep + move];
                    int next_2ep = multi_move_table_2ep[stride_2ep + move];
                    int next_2c = multi_move_table_2c[stride_2c + move];
                    int next_eo = eo_move_table[stride_eo + move];

                    uint64_t next_node = static_cast<uint64_t>(next_4ep) * SIZE_2EP_2C_EO +
                                         static_cast<uint64_t>(next_2ep) * SIZE_2C_EO +
                                         static_cast<uint64_t>(next_2c) * SIZE_EO +
                                         next_eo;

                    if (cur_set.find(next_node) == cur_set.end() &&
                        prev_set.find(next_node) == prev_set.end())
                    {
                        next_set.insert(next_node);
                    }
                }
            }

            target_pairs[d].assign(next_set.begin(), next_set.end());
            target_pairs[d].shrink_to_fit();
            target_num_list[d] = static_cast<int>(target_pairs[d].size());

            std::cout << "Depth " << d << ": " << target_num_list[d] << " nodes" << std::endl;

            prev_set = std::move(cur_set);
            cur_set = std::move(next_set);
        }

        {
            tsl::robin_set<uint64_t> temp;
            prev_set.swap(temp);
        }

        std::cout << "\n=== Phase 2: Local Expansion [" << label << "] ===" << std::endl;

        tsl::robin_set<uint64_t> depth6_set = std::move(cur_set);

        // Depth 7: 2M bucket
        expand_depth_partial(6, 7, depth6_set, target_pairs[6], nullptr,
                             BUCKET_2M, TARGET_NODES_2M, target_pairs, target_num_list);

        // Depth 8: 2M bucket with Depth 6 backtrace check
        tsl::robin_set<uint64_t> depth7_set;
        depth7_set.max_load_factor(LOAD_FACTOR);
        depth7_set.insert(target_pairs[7].begin(), target_pairs[7].end());
        expand_depth_partial(7, 8, depth7_set, target_pairs[7], &depth6_set,
                             BUCKET_2M, TARGET_NODES_2M, target_pairs, target_num_list);

        {
            tsl::robin_set<uint64_t> temp;
            depth6_set.swap(temp);
        }

        // Depth 9: 2M bucket with Depth 7 backtrace check
        tsl::robin_set<uint64_t> depth8_set;
        depth8_set.max_load_factor(LOAD_FACTOR);
        depth8_set.insert(target_pairs[8].begin(), target_pairs[8].end());
        expand_depth_partial(8, 9, depth8_set, target_pairs[8], &depth7_set,
                             BUCKET_2M, TARGET_NODES_2M, target_pairs, target_num_list);

        {
            tsl::robin_set<uint64_t> temp;
            depth7_set.swap(temp);
        }

        // Depth 10: 2M bucket with Depth 8 backtrace check
        tsl::robin_set<uint64_t> depth9_set;
        depth9_set.max_load_factor(LOAD_FACTOR);
        depth9_set.insert(target_pairs[9].begin(), target_pairs[9].end());
        expand_depth_partial(9, 10, depth9_set, target_pairs[9], &depth8_set,
                             BUCKET_2M, TARGET_NODES_2M, target_pairs, target_num_list);

        {
            tsl::robin_set<uint64_t> temp;
            depth8_set.swap(temp);
        }

        // Depth 11: 2M bucket with Depth 9 backtrace check
        tsl::robin_set<uint64_t> depth10_set;
        depth10_set.max_load_factor(LOAD_FACTOR);
        depth10_set.insert(target_pairs[10].begin(), target_pairs[10].end());
        expand_depth_partial(10, 11, depth10_set, target_pairs[10], &depth9_set,
                             BUCKET_2M, TARGET_NODES_2M, target_pairs, target_num_list);

        {
            tsl::robin_set<uint64_t> temp;
            depth9_set.swap(temp);
        }

        // Depth 12: 2M bucket with Depth 10 backtrace check
        tsl::robin_set<uint64_t> depth11_set;
        depth11_set.max_load_factor(LOAD_FACTOR);
        depth11_set.insert(target_pairs[11].begin(), target_pairs[11].end());
        expand_depth_partial(11, 12, depth11_set, target_pairs[11], &depth10_set,
                             BUCKET_2M, TARGET_NODES_2M, target_pairs, target_num_list);

        {
            tsl::robin_set<uint64_t> temp;
            depth10_set.swap(temp);
        }

        // Depth 13: 1M bucket with Depth 11 backtrace check
        tsl::robin_set<uint64_t> depth12_set;
        depth12_set.max_load_factor(LOAD_FACTOR);
        depth12_set.insert(target_pairs[12].begin(), target_pairs[12].end());
        expand_depth_partial(12, 13, depth12_set, target_pairs[12], &depth11_set,
                             BUCKET_1M, TARGET_NODES_1M, target_pairs, target_num_list);

        {
            tsl::robin_set<uint64_t> temp;
            depth11_set.swap(temp);
        }

        // Depth 14: 1M bucket with Depth 12 backtrace check
        tsl::robin_set<uint64_t> depth13_set;
        depth13_set.max_load_factor(LOAD_FACTOR);
        depth13_set.insert(target_pairs[13].begin(), target_pairs[13].end());
        expand_depth_partial(13, 14, depth13_set, target_pairs[13], &depth12_set,
                             BUCKET_1M, TARGET_NODES_1M, target_pairs, target_num_list);

        {
            tsl::robin_set<uint64_t> temp;
            depth12_set.swap(temp);
        }
        {
            tsl::robin_set<uint64_t> temp;
            depth13_set.swap(temp);
        }

        std::cout << "Database Construction Complete [" << label << "] (Depths 0-14)." << std::endl;
    }

    void print_database_stats(const std::string &label,
                              const std::vector<std::vector<uint64_t>> &target_pairs,
                              const std::vector<int> &target_num_list)
    {
        std::cout << "\n--------------------------------------------------" << std::endl;
        std::cout << "  Database Stats: " << label << std::endl;
        std::cout << "--------------------------------------------------" << std::endl;
        size_t total_nodes = 0;
        for (size_t d = 0; d < target_pairs.size(); ++d)
        {
            size_t count = target_pairs[d].size();
            total_nodes += count;
            double mb = (count * sizeof(uint64_t)) / (1024.0 * 1024.0);
            std::cout << "  Depth " << (d < 10 ? " " : "") << d
                      << ": " << count << " nodes (" << mb << " MB)" << std::endl;
        }
        double total_mb = (total_nodes * sizeof(uint64_t)) / (1024.0 * 1024.0);
        std::cout << "  Total Nodes: " << total_nodes << " (" << total_mb << " MB)" << std::endl;
    }

    void build_database()
    {
        // 1. Build Parallel Database (BL-BR)
        uint64_t goal_para = static_cast<uint64_t>(goal_4ep) * SIZE_2EP_2C_EO +
                             static_cast<uint64_t>(goal_2ep_para) * SIZE_2C_EO +
                             static_cast<uint64_t>(goal_2c_para) * SIZE_EO +
                             goal_eo;
        build_single_database(goal_para, index_pairs_para, num_list_para, "Parallel (BL-BR)");

        // 2. Build Orthogonal Database (BL-FL)
        uint64_t goal_ortho = static_cast<uint64_t>(goal_4ep) * SIZE_2EP_2C_EO +
                              static_cast<uint64_t>(goal_2ep_ortho) * SIZE_2C_EO +
                              static_cast<uint64_t>(goal_2c_ortho) * SIZE_EO +
                              goal_eo;
        build_single_database(goal_ortho, index_pairs_ortho, num_list_ortho, "Orthogonal (BL-FL)");

        // 3. Build Diagonal Database (BL-FR)
        uint64_t goal_diag = static_cast<uint64_t>(goal_4ep) * SIZE_2EP_2C_EO +
                             static_cast<uint64_t>(goal_2ep_diag) * SIZE_2C_EO +
                             static_cast<uint64_t>(goal_2c_diag) * SIZE_EO +
                             goal_eo;
        build_single_database(goal_diag, index_pairs_diag, num_list_diag, "Diagonal (BL-FR)");

        std::cout << "\n==================================================" << std::endl;
        std::cout << "        XXEOCross Complete Distribution Log       " << std::endl;
        std::cout << "==================================================" << std::endl;
        print_database_stats("Parallel (BL-BR)", index_pairs_para, num_list_para);
        print_database_stats("Orthogonal (BL-FL)", index_pairs_ortho, num_list_ortho);
        print_database_stats("Diagonal (BL-FR)", index_pairs_diag, num_list_diag);

#ifdef __EMSCRIPTEN__
        size_t heap_size = emscripten_get_heap_size();
        std::cout << "\n[WASM Heap] Current Allocated Heap Size: "
                  << (heap_size / (1024.0 * 1024.0)) << " MB" << std::endl;
#endif
        std::cout << "==================================================\n" << std::endl;
    }

    bool depth_limited_search_eocross(int arg_4ep, int arg_eo, int depth, int prev)
    {
        for (int i = 0; i < 18; ++i)
        {
            if (ma[prev + i])
            {
                continue;
            }

            int next_4ep = multi_move_table_4ep[arg_4ep + i];
            int next_eo = eo_move_table[arg_eo + i];

            int p = prune_table_4ep_eo[next_4ep * SIZE_EO + next_eo];
            int h = (p == 255) ? 11 : p;
            if (h >= depth)
            {
                continue;
            }

            sol.emplace_back(i);

            if (depth == 1)
            {
                if (next_4ep == goal_4ep && next_eo == goal_eo)
                {
                    tmp = AlgToString(sol);
                    return true;
                }
            }
            else if (depth_limited_search_eocross(next_4ep * 18, next_eo * 18, depth - 1, i * 18))
            {
                return true;
            }

            sol.pop_back();
        }
        return false;
    }

    std::string start_search_eocross(const std::string &arg_scramble, int max_depth = 10)
    {
        sol.clear();
        std::vector<int> alg = StringToAlg(arg_scramble);

        int idx_4ep = goal_4ep;
        int idx_eo = goal_eo;

        // Trace state from solved origin under the incoming scramble
        for (int move : alg)
        {
            idx_4ep = multi_move_table_4ep[idx_4ep * 18 + move];
            idx_eo = eo_move_table[idx_eo * 18 + move];
        }

        if (idx_4ep == goal_4ep && idx_eo == goal_eo)
        {
            return "";
        }

        int p = prune_table_4ep_eo[idx_4ep * SIZE_EO + idx_eo];
        int d_min = (p == 255 ? 1 : p);
        if (d_min == 0)
        {
            d_min = 1;
        }

        for (int d = d_min; d <= max_depth; ++d)
        {
            if (depth_limited_search_eocross(idx_4ep * 18, idx_eo * 18, d, 18 * 18))
            {
                return tmp;
            }
        }
        return "";
    }

    bool depth_limited_search(int arg_4ep, int arg_2ep, int arg_2c, int arg_eo,
                              int depth, int prev,
                              int target_goal_2ep, int target_goal_2c,
                              int c1_val, int c2_val,
                              const std::vector<unsigned char> &prune_table_c1,
                              const std::vector<unsigned char> &prune_table_c2)
    {
        for (int i = 0; i < 18; ++i)
        {
            if (ma[prev + i])
            {
                continue;
            }

            int next_4ep = multi_move_table_4ep[arg_4ep + i];
            int next_eo = eo_move_table[arg_eo + i];

            // Pruning check 1: Cross 4EP * Full EO
            int p_eo = prune_table_4ep_eo[next_4ep * SIZE_EO + next_eo];
            int h_eo = (p_eo == 255) ? 11 : p_eo;
            if (h_eo >= depth)
            {
                continue;
            }

            // Pruning check 2: Cross 4EP * Slot Corner 1
            int next_c1 = corner_move_table[c1_val * 18 + i];
            int p_c1 = prune_table_c1[next_4ep * 24 + next_c1];
            int h_c1 = (p_c1 == 255) ? 11 : p_c1;
            if (h_c1 >= depth)
            {
                continue;
            }

            // Pruning check 3: Cross 4EP * Slot Corner 2
            int next_c2 = corner_move_table[c2_val * 18 + i];
            int p_c2 = prune_table_c2[next_4ep * 24 + next_c2];
            int h_c2 = (p_c2 == 255) ? 11 : p_c2;
            if (h_c2 >= depth)
            {
                continue;
            }

            int next_2ep = multi_move_table_2ep[arg_2ep + i];
            int next_2c = multi_move_table_2c[arg_2c + i];

            sol.emplace_back(i);

            if (depth == 1)
            {
                if (next_4ep == goal_4ep && next_eo == goal_eo &&
                    next_2ep == target_goal_2ep && next_2c == target_goal_2c)
                {
                    tmp = AlgToString(sol);
                    return true;
                }
            }
            else if (depth_limited_search(next_4ep * 18, next_2ep * 18, next_2c * 18, next_eo * 18,
                                          depth - 1, i * 18,
                                          target_goal_2ep, target_goal_2c,
                                          next_c1, next_c2,
                                          prune_table_c1, prune_table_c2))
            {
                return true;
            }

            sol.pop_back();
        }
        return false;
    }

    std::string start_search(const std::string &arg_scramble, int config_type, int max_depth = 12)
    {
        // Stage 1: Solve EOCross (Cross 4EP + Full EO)
        std::string eocross_sol = start_search_eocross(arg_scramble);

        std::string stage2_scramble = arg_scramble;
        if (!eocross_sol.empty())
        {
            if (!stage2_scramble.empty() && stage2_scramble.back() != ' ')
            {
                stage2_scramble += " ";
            }
            stage2_scramble += eocross_sol;
        }

        // Determine target goals and corner prune tables
        int target_goal_2ep;
        int target_goal_2c;
        int c1_goal = 4 * 3 + 0; // DLB (BL)
        int c2_goal;
        const std::vector<unsigned char> *prune_c1 = &prune_table_4ep_c_bl;
        const std::vector<unsigned char> *prune_c2 = nullptr;

        if (config_type == 0) // Parallel (BL & BR)
        {
            target_goal_2ep = goal_2ep_para;
            target_goal_2c = goal_2c_para;
            c2_goal = 5 * 3 + 0; // DBR (BR)
            prune_c2 = &prune_table_4ep_c_br;
        }
        else if (config_type == 1) // Orthogonal (BL & FL)
        {
            target_goal_2ep = goal_2ep_ortho;
            target_goal_2c = goal_2c_ortho;
            c2_goal = 7 * 3 + 0; // DFL (FL)
            prune_c2 = &prune_table_4ep_c_fl;
        }
        else // Diagonal (BL & FR)
        {
            target_goal_2ep = goal_2ep_diag;
            target_goal_2c = goal_2c_diag;
            c2_goal = 6 * 3 + 0; // DFR (FR)
            prune_c2 = &prune_table_4ep_c_fr;
        }

        // Stage 2: Solve remaining 2 slots
        sol.clear();
        std::vector<int> alg = StringToAlg(stage2_scramble);

        int idx_4ep = goal_4ep;
        int idx_2ep = target_goal_2ep;
        int idx_2c = target_goal_2c;
        int idx_eo = goal_eo;
        int c1 = c1_goal;
        int c2 = c2_goal;

        for (int move : alg)
        {
            idx_4ep = multi_move_table_4ep[idx_4ep * 18 + move];
            idx_2ep = multi_move_table_2ep[idx_2ep * 18 + move];
            idx_2c = multi_move_table_2c[idx_2c * 18 + move];
            idx_eo = eo_move_table[idx_eo * 18 + move];
            c1 = corner_move_table[c1 * 18 + move];
            c2 = corner_move_table[c2 * 18 + move];
        }

        if (idx_4ep == goal_4ep && idx_eo == goal_eo &&
            idx_2ep == target_goal_2ep && idx_2c == target_goal_2c)
        {
            return eocross_sol;
        }

        int p_eo = prune_table_4ep_eo[idx_4ep * SIZE_EO + idx_eo];
        int p_c1 = (*prune_c1)[idx_4ep * 24 + c1];
        int p_c2 = (*prune_c2)[idx_4ep * 24 + c2];

        int d_min = std::max({(p_eo == 255 ? 1 : p_eo),
                              (p_c1 == 255 ? 1 : p_c1),
                              (p_c2 == 255 ? 1 : p_c2)});
        if (d_min == 0)
        {
            d_min = 1;
        }

        for (int d = d_min; d <= max_depth; ++d)
        {
            if (depth_limited_search(idx_4ep * 18, idx_2ep * 18, idx_2c * 18, idx_eo * 18,
                                     d, 18 * 18,
                                     target_goal_2ep, target_goal_2c,
                                     c1, c2,
                                     *prune_c1, *prune_c2))
            {
                if (eocross_sol.empty())
                {
                    return tmp;
                }
                return eocross_sol + " " + tmp;
            }
        }

        return eocross_sol;
    }

    std::vector<int> invert_alg(const std::vector<int> &alg)
    {
        static const int inv_moves[18] = {
            2, 1, 0,   // U, U2, U' -> U', U2, U
            5, 4, 3,   // D, D2, D' -> D', D2, D
            8, 7, 6,   // L, L2, L' -> L', L2, L
            11, 10, 9, // R, R2, R' -> R', R2, R
            14, 13, 12,// F, F2, F' -> F', F2, F
            17, 16, 15 // B, B2, B' -> B', B2, B
        };
        std::vector<int> inv;
        inv.reserve(alg.size());
        for (auto it = alg.rbegin(); it != alg.rend(); ++it)
        {
            inv.push_back(inv_moves[*it]);
        }
        return inv;
    }

    std::vector<int> generate_raw_walk(int len, int config_type)
    {
        std::vector<int> walk;
        walk.reserve(len);

        int target_goal_2ep;
        int target_goal_2c;
        int c1 = 4 * 3 + 0; // DLB (BL)
        int c2;
        const std::vector<unsigned char> *prune_c1 = &prune_table_4ep_c_bl;
        const std::vector<unsigned char> *prune_c2 = nullptr;

        if (config_type == 0) // Parallel (BL & BR)
        {
            target_goal_2ep = goal_2ep_para;
            target_goal_2c = goal_2c_para;
            c2 = 5 * 3 + 0; // DBR (BR)
            prune_c2 = &prune_table_4ep_c_br;
        }
        else if (config_type == 1) // Orthogonal (BL & FL)
        {
            target_goal_2ep = goal_2ep_ortho;
            target_goal_2c = goal_2c_ortho;
            c2 = 7 * 3 + 0; // DFL (FL)
            prune_c2 = &prune_table_4ep_c_fl;
        }
        else // Diagonal (BL & FR)
        {
            target_goal_2ep = goal_2ep_diag;
            target_goal_2c = goal_2c_diag;
            c2 = 6 * 3 + 0; // DFR (FR)
            prune_c2 = &prune_table_4ep_c_fr;
        }

        int idx_4ep = goal_4ep;
        int idx_2ep = target_goal_2ep;
        int idx_2c = target_goal_2c;
        int idx_eo = goal_eo;
        int prev = 18;

        for (int step = 0; step < len; ++step)
        {
            std::vector<int> candidate_moves;
            candidate_moves.reserve(18);

            int p_eo = prune_table_4ep_eo[idx_4ep * SIZE_EO + idx_eo];
            int p_c1 = (*prune_c1)[idx_4ep * 24 + c1];
            int p_c2 = (*prune_c2)[idx_4ep * 24 + c2];
            int cur_dist = std::max({(p_eo == 255 ? 11 : p_eo),
                                     (p_c1 == 255 ? 11 : p_c1),
                                     (p_c2 == 255 ? 11 : p_c2)});

            for (int m = 0; m < 18; ++m)
            {
                if (ma[prev * 18 + m])
                {
                    continue;
                }

                int next_4ep = multi_move_table_4ep[idx_4ep * 18 + m];
                int next_eo = eo_move_table[idx_eo * 18 + m];
                int next_c1 = corner_move_table[c1 * 18 + m];
                int next_c2 = corner_move_table[c2 * 18 + m];

                int np_eo = prune_table_4ep_eo[next_4ep * SIZE_EO + next_eo];
                int np_c1 = (*prune_c1)[next_4ep * 24 + next_c1];
                int np_c2 = (*prune_c2)[next_4ep * 24 + next_c2];
                int next_dist = std::max({(np_eo == 255 ? 11 : np_eo),
                                          (np_c1 == 255 ? 11 : np_c1),
                                          (np_c2 == 255 ? 11 : np_c2)});

                // Suppress backtracking in early steps to spread nodes into deep space
                if (step < 7 && next_dist < cur_dist)
                {
                    continue;
                }
                candidate_moves.push_back(m);
            }

            if (candidate_moves.empty())
            {
                return {};
            }

            std::uniform_int_distribution<size_t> dist(0, candidate_moves.size() - 1);
            int chosen_move = candidate_moves[dist(generator)];

            walk.push_back(chosen_move);
            idx_4ep = multi_move_table_4ep[idx_4ep * 18 + chosen_move];
            idx_2ep = multi_move_table_2ep[idx_2ep * 18 + chosen_move];
            idx_2c = multi_move_table_2c[idx_2c * 18 + chosen_move];
            idx_eo = eo_move_table[idx_eo * 18 + chosen_move];
            c1 = corner_move_table[c1 * 18 + chosen_move];
            c2 = corner_move_table[c2 * 18 + chosen_move];
            prev = chosen_move;
        }

        return walk;
    }

    std::string get_xxeocross_scramble(int len, int config_type)
    {
        const auto &target_pairs = (config_type == 0) ? index_pairs_para :
                                   (config_type == 1) ? index_pairs_ortho :
                                                        index_pairs_diag;

        if (len <= 0 || len >= static_cast<int>(target_pairs.size()) || target_pairs[len].empty())
        {
            return "";
        }

        const int max_attempts = 50;
        std::uniform_int_distribution<size_t> dist(0, target_pairs[len].size() - 1);

        int target_goal_2ep;
        int target_goal_2c;
        const std::vector<unsigned char> *prune_c1 = &prune_table_4ep_c_bl;
        const std::vector<unsigned char> *prune_c2 = nullptr;

        if (config_type == 0) // Parallel (BL & BR)
        {
            target_goal_2ep = goal_2ep_para;
            target_goal_2c = goal_2c_para;
            prune_c2 = &prune_table_4ep_c_br;
        }
        else if (config_type == 1) // Orthogonal (BL & FL)
        {
            target_goal_2ep = goal_2ep_ortho;
            target_goal_2c = goal_2c_ortho;
            prune_c2 = &prune_table_4ep_c_fl;
        }
        else // Diagonal (BL & FR)
        {
            target_goal_2ep = goal_2ep_diag;
            target_goal_2c = goal_2c_diag;
            prune_c2 = &prune_table_4ep_c_fr;
        }

        std::vector<int> corners_arr(2);

        for (int attempt = 0; attempt < max_attempts; ++attempt)
        {
            sol.clear();
            uint64_t node = target_pairs[len][dist(generator)];

            int idx_4ep = static_cast<int>(node / SIZE_2EP_2C_EO);
            uint64_t rem1 = node % SIZE_2EP_2C_EO;
            int idx_2ep = static_cast<int>(rem1 / SIZE_2C_EO);
            uint64_t rem2 = rem1 % SIZE_2C_EO;
            int idx_2c = static_cast<int>(rem2 / SIZE_EO);
            int idx_eo = static_cast<int>(rem2 % SIZE_EO);

            index_to_array(corners_arr, idx_2c, 2, 3, 8);
            int c1 = corners_arr[0] / 18;
            int c2 = corners_arr[1] / 18;

            int p_eo = prune_table_4ep_eo[idx_4ep * SIZE_EO + idx_eo];
            int p_c1 = (*prune_c1)[idx_4ep * 24 + c1];
            int p_c2 = (*prune_c2)[idx_4ep * 24 + c2];

            int d_min = std::max({(p_eo == 255 ? 1 : p_eo),
                                  (p_c1 == 255 ? 1 : p_c1),
                                  (p_c2 == 255 ? 1 : p_c2)});
            if (d_min == 0)
            {
                d_min = 1;
            }

            int actual_depth = -1;
            for (int d = d_min; d <= len; ++d)
            {
                if (depth_limited_search(idx_4ep * 18, idx_2ep * 18, idx_2c * 18, idx_eo * 18,
                                         d, 18 * 18,
                                         target_goal_2ep, target_goal_2c,
                                         c1, c2,
                                         *prune_c1, *prune_c2))
                {
                    actual_depth = d;
                    break;
                }
            }

            if (actual_depth == len)
            {
                return tmp;
            }
        }

        return "";
    }

    std::string func(const std::string &arg_scramble = "",
                     const std::string &arg_length = "7",
                     const std::string &arg_slot = "BL BR")
    {
        int len = std::stoi(arg_length);

        int config_type = 0; // Default Parallel (BL-BR / FR-FL)

        if (arg_slot.find("BL FR") != std::string::npos ||
            arg_slot.find("FR BL") != std::string::npos ||
            arg_slot.find("BR FL") != std::string::npos ||
            arg_slot.find("FL BR") != std::string::npos ||
            arg_slot.find("diag") != std::string::npos ||
            arg_slot.find("DIAG") != std::string::npos)
        {
            config_type = 2; // Diagonal
        }
        else if (arg_slot.find("BL FL") != std::string::npos ||
                 arg_slot.find("FL BL") != std::string::npos ||
                 arg_slot.find("BR FR") != std::string::npos ||
                 arg_slot.find("FR BR") != std::string::npos ||
                 arg_slot.find("ortho") != std::string::npos ||
                 arg_slot.find("ORTHO") != std::string::npos)
        {
            config_type = 1; // Orthogonal
        }
        else
        {
            config_type = 0; // Parallel
        }

        // 1. Solve incoming random scramble R to fully solved origin state O via 2-stage solver
        std::string sol_str = start_search(arg_scramble, config_type);

        // 2. Sample candidate state from precomputed database
        std::string w_str = get_xxeocross_scramble(len, config_type);

        // 3. Fallback: trial random walk if database sampling yields nothing
        if (w_str.empty())
        {
            const int max_trials = 50;

            int target_goal_2ep;
            int target_goal_2c;
            int c1_goal = 4 * 3 + 0; // DLB (BL)
            int c2_goal;
            const std::vector<unsigned char> *prune_c1 = &prune_table_4ep_c_bl;
            const std::vector<unsigned char> *prune_c2 = nullptr;

            if (config_type == 0)
            {
                target_goal_2ep = goal_2ep_para;
                target_goal_2c = goal_2c_para;
                c2_goal = 5 * 3 + 0; // DBR (BR)
                prune_c2 = &prune_table_4ep_c_br;
            }
            else if (config_type == 1)
            {
                target_goal_2ep = goal_2ep_ortho;
                target_goal_2c = goal_2c_ortho;
                c2_goal = 7 * 3 + 0; // DFL (FL)
                prune_c2 = &prune_table_4ep_c_fl;
            }
            else
            {
                target_goal_2ep = goal_2ep_diag;
                target_goal_2c = goal_2c_diag;
                c2_goal = 6 * 3 + 0; // DFR (FR)
                prune_c2 = &prune_table_4ep_c_fr;
            }

            for (int trial = 0; trial < max_trials; ++trial)
            {
                std::vector<int> candidate_walk = generate_raw_walk(len, config_type);
                if (candidate_walk.empty())
                {
                    continue;
                }

                // Simulate walk from origin to locate terminal state
                int cur_4ep = goal_4ep;
                int cur_2ep = target_goal_2ep;
                int cur_2c = target_goal_2c;
                int cur_eo = goal_eo;
                int cur_c1 = c1_goal;
                int cur_c2 = c2_goal;

                for (int m : candidate_walk)
                {
                    cur_4ep = multi_move_table_4ep[cur_4ep * 18 + m];
                    cur_2ep = multi_move_table_2ep[cur_2ep * 18 + m];
                    cur_2c = multi_move_table_2c[cur_2c * 18 + m];
                    cur_eo = eo_move_table[cur_eo * 18 + m];
                    cur_c1 = corner_move_table[cur_c1 * 18 + m];
                    cur_c2 = corner_move_table[cur_c2 * 18 + m];
                }

                int p_eo = prune_table_4ep_eo[cur_4ep * SIZE_EO + cur_eo];
                int p_c1 = (*prune_c1)[cur_4ep * 24 + cur_c1];
                int p_c2 = (*prune_c2)[cur_4ep * 24 + cur_c2];

                int d_min = std::max({(p_eo == 255 ? 1 : p_eo),
                                      (p_c1 == 255 ? 1 : p_c1),
                                      (p_c2 == 255 ? 1 : p_c2)});
                if (d_min == 0)
                {
                    d_min = 1;
                }

                int actual_depth = -1;
                sol.clear();

                for (int d = d_min; d <= len; ++d)
                {
                    if (depth_limited_search(cur_4ep * 18, cur_2ep * 18, cur_2c * 18, cur_eo * 18,
                                             d, 18 * 18,
                                             target_goal_2ep, target_goal_2c,
                                             cur_c1, cur_c2,
                                             *prune_c1, *prune_c2))
                    {
                        actual_depth = d;
                        break;
                    }
                }

                if (actual_depth == len)
                {
                    w_str = tmp;
                    break;
                }
            }
        }

        std::string gen_str;
        if (w_str.empty())
        {
            gen_str = "RETRY_NEEDED";
        }
        else
        {
            gen_str = w_str;
        }

        return arg_scramble + " " + sol_str + "," + gen_str;
    }
};

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_BINDINGS(xxeocross_trainer_module)
{
    emscripten::class_<xxeocross_search>("xxeocross_search")
        .constructor<>()
        .function("func", &xxeocross_search::func);
}
#endif
