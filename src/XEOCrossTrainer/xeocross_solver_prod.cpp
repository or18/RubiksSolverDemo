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


struct xeocross_search
{
    std::vector<int> sol;
    std::string scramble;
    std::string tmp;
    std::mt19937_64 generator;

    // Move tables
    std::vector<int> ep_move_table;
    std::vector<int> multi_move_table_5ep;
    std::vector<int> corner_move_table;
    std::vector<int> eo_move_table;

    // Pruning tables
    std::vector<unsigned char> prune_table_5ep;

    // Search database
    std::vector<std::vector<uint64_t>> index_pairs;
    std::vector<int> num_list;
    std::vector<bool> ma;

    // Goal states
    int goal_5ep;
    int goal_corner;
    int goal_eo;

    // State sizes
    static constexpr uint64_t SIZE_5EP = 95040ULL;       // 12 * 11 * 10 * 9 * 8
    static constexpr uint64_t SIZE_CORNER = 24ULL;       // 8 * 3
    static constexpr uint64_t SIZE_EO = 2048ULL;         // 2^11
    static constexpr uint64_t SIZE_C_EO = SIZE_CORNER * SIZE_EO; // 49152ULL
    
    // 4 EP move table for Cross EP * EO pruning
    std::vector<int> multi_move_table_4ep;

    // Dual pruning tables
    std::vector<unsigned char> prune_table_4ep_eo; // h1: Cross 4 EP * Full EO (23.2 MB)
    std::vector<unsigned char> prune_table_5ep_c;   // h2: 5 EP * Corner (2.18 MB)

    // Goal index for 4 EP
    int goal_4ep;

    static constexpr uint64_t SIZE_4EP = 11880ULL; // 12 * 11 * 10 * 9

    xeocross_search()
    {
        std::random_device rd;
        generator.seed(rd());

        // Create basic move tables
        ep_move_table = create_ep_move_table();
        corner_move_table = create_corner_move_table();
        eo_move_table = create_eo_move_table();

        // 4 EP move table (Cross: DF=8, DL=9, DB=10, DR=11)
        create_multi_move_table(4, 1, 12, SIZE_4EP, ep_move_table, multi_move_table_4ep);

        // 5 EP move table (Cross: DF=8, DL=9, DB=10, DR=11, BL=0)
        create_multi_move_table(5, 1, 12, SIZE_5EP, ep_move_table, multi_move_table_5ep);

        // Goal indices for BL slot
        std::vector<int> ep4_goal = {8, 9, 10, 11};
        std::vector<int> ep5_goal = {8, 9, 10, 11, 0};
        goal_4ep = array_to_index(ep4_goal, 4, 1, 12);
        goal_5ep = array_to_index(ep5_goal, 5, 1, 12);
        goal_corner = 12; // DLB corner solved (3 * 4 + 0)
        goal_eo = 0;      // All edges oriented

        // Construct dual pruning tables
        // h1: 4 EP * Full EO (depth 10 limit)
        create_prune_table2(goal_4ep, goal_eo, SIZE_4EP, SIZE_EO, 10,
                            multi_move_table_4ep, eo_move_table, prune_table_4ep_eo);

        // h2: 5 EP * Corner (depth 10 limit)
        create_prune_table2(goal_5ep, goal_corner, SIZE_5EP, SIZE_CORNER, 10,
                            multi_move_table_5ep, corner_move_table, prune_table_5ep_c);

        ma = create_ma_table();

        build_database();
    }


    void build_database()
    {
        index_pairs.resize(13);
        num_list.resize(13, 0);

        uint64_t goal_composite = static_cast<uint64_t>(goal_5ep) * SIZE_C_EO +
                                  static_cast<uint64_t>(goal_corner) * SIZE_EO +
                                  goal_eo;

        index_pairs[0].push_back(goal_composite);
        num_list[0] = 1;

        std::cout << "\n=== Phase 1: Full BFS (Depths 1-6) ===" << std::endl;

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
                int ep_idx = static_cast<int>(node / SIZE_C_EO);
                uint64_t rem = node % SIZE_C_EO;
                int c_idx = static_cast<int>(rem / SIZE_EO);
                int eo_idx = static_cast<int>(rem % SIZE_EO);

                int ep_stride = ep_idx * 18;
                int c_stride = c_idx * 18;
                int eo_stride = eo_idx * 18;

                for (int move = 0; move < 18; ++move)
                {
                    int next_ep = multi_move_table_5ep[ep_stride + move];
                    int next_c = corner_move_table[c_stride + move];
                    int next_eo = eo_move_table[eo_stride + move];

                    uint64_t next_node = static_cast<uint64_t>(next_ep) * SIZE_C_EO +
                                         static_cast<uint64_t>(next_c) * SIZE_EO +
                                         next_eo;

                    if (cur_set.find(next_node) == cur_set.end() &&
                        prev_set.find(next_node) == prev_set.end())
                    {
                        next_set.insert(next_node);
                    }
                }
            }

            index_pairs[d].assign(next_set.begin(), next_set.end());
            index_pairs[d].shrink_to_fit();
            num_list[d] = static_cast<int>(index_pairs[d].size());

            std::cout << "Depth " << d << ": " << num_list[d] << " nodes" << std::endl;

            prev_set = std::move(cur_set);
            cur_set = std::move(next_set);
        }

        // Release prev_set completely
        {
            tsl::robin_set<uint64_t> temp;
            prev_set.swap(temp);
        }

        std::cout << "\n=== Phase 2: Local Expansion (2M x 4, 1M x 2) ===" << std::endl;

        tsl::robin_set<uint64_t> depth6_set = std::move(cur_set);

        // Depth 7: 2M bucket
        expand_depth_partial(6, 7, depth6_set, index_pairs[6], nullptr, BUCKET_2M, TARGET_NODES_2M);

        // Depth 8: 2M bucket with Depth 6 backtrace check
        tsl::robin_set<uint64_t> depth7_set;
        depth7_set.max_load_factor(LOAD_FACTOR);
        depth7_set.insert(index_pairs[7].begin(), index_pairs[7].end());
        expand_depth_partial(7, 8, depth7_set, index_pairs[7], &depth6_set, BUCKET_2M, TARGET_NODES_2M);

        // Release depth6_set
        {
            tsl::robin_set<uint64_t> temp;
            depth6_set.swap(temp);
        }

        // Depth 9: 2M bucket with Depth 7 backtrace check
        tsl::robin_set<uint64_t> depth8_set;
        depth8_set.max_load_factor(LOAD_FACTOR);
        depth8_set.insert(index_pairs[8].begin(), index_pairs[8].end());
        expand_depth_partial(8, 9, depth8_set, index_pairs[8], &depth7_set, BUCKET_2M, TARGET_NODES_2M);

        // Release depth7_set
        {
            tsl::robin_set<uint64_t> temp;
            depth7_set.swap(temp);
        }

        // Depth 10: 2M bucket with Depth 8 backtrace check
        tsl::robin_set<uint64_t> depth9_set;
        depth9_set.max_load_factor(LOAD_FACTOR);
        depth9_set.insert(index_pairs[9].begin(), index_pairs[9].end());
        expand_depth_partial(9, 10, depth9_set, index_pairs[9], &depth8_set, BUCKET_2M, TARGET_NODES_2M);

        // Release depth8_set
        {
            tsl::robin_set<uint64_t> temp;
            depth8_set.swap(temp);
        }

        // Depth 11: 1M bucket with Depth 9 backtrace check
        tsl::robin_set<uint64_t> depth10_set;
        depth10_set.max_load_factor(LOAD_FACTOR);
        depth10_set.insert(index_pairs[10].begin(), index_pairs[10].end());
        expand_depth_partial(10, 11, depth10_set, index_pairs[10], &depth9_set, BUCKET_1M, TARGET_NODES_1M);

        // Release depth9_set
        {
            tsl::robin_set<uint64_t> temp;
            depth9_set.swap(temp);
        }

        // Depth 12: 1M bucket with Depth 10 backtrace check
        tsl::robin_set<uint64_t> depth11_set;
        depth11_set.max_load_factor(LOAD_FACTOR);
        depth11_set.insert(index_pairs[11].begin(), index_pairs[11].end());
        expand_depth_partial(11, 12, depth11_set, index_pairs[11], &depth10_set, BUCKET_1M, TARGET_NODES_1M);

        // Release depth10_set and depth11_set
        {
            tsl::robin_set<uint64_t> temp;
            depth10_set.swap(temp);
        }
        {
            tsl::robin_set<uint64_t> temp;
            depth11_set.swap(temp);
        }

        std::cout << "\n=== Complete Database Summary ===" << std::endl;
        for (int d = 0; d <= 12; ++d)
        {
            std::cout << "  depth=" << d << ": " << index_pairs[d].size() << " nodes" << std::endl;
        }
    }

    void expand_depth_partial(int parent_d, int next_d,
                              const tsl::robin_set<uint64_t> &parent_set,
                              const std::vector<uint64_t> &parent_vec,
                              const tsl::robin_set<uint64_t> *grandparent_set,
                              size_t bucket_size,
                              size_t target_nodes)
    {
        tsl::robin_set<uint64_t> next_set;
        next_set.max_load_factor(LOAD_FACTOR);
        next_set.rehash(bucket_size);

        index_pairs[next_d].clear();
        index_pairs[next_d].reserve(target_nodes);

        std::uniform_int_distribution<size_t> parent_dist(0, parent_vec.size() - 1);
        std::uniform_int_distribution<int> move_dist(0, 17);

        size_t max_attempts = target_nodes * 12;
        size_t attempts = 0;

        while (next_set.size() < target_nodes && attempts < max_attempts)
        {
            attempts++;
            uint64_t p_node = parent_vec[parent_dist(generator)];

            int ep_idx = static_cast<int>(p_node / SIZE_C_EO);
            uint64_t rem = p_node % SIZE_C_EO;
            int c_idx = static_cast<int>(rem / SIZE_EO);
            int eo_idx = static_cast<int>(rem % SIZE_EO);

            int move = move_dist(generator);

            int next_ep = multi_move_table_5ep[ep_idx * 18 + move];
            int next_c = corner_move_table[c_idx * 18 + move];
            int next_eo = eo_move_table[eo_idx * 18 + move];

            uint64_t next_node = static_cast<uint64_t>(next_ep) * SIZE_C_EO +
                                 static_cast<uint64_t>(next_c) * SIZE_EO +
                                 next_eo;

            if (parent_set.find(next_node) != parent_set.end() ||
                next_set.find(next_node) != next_set.end())
            {
                continue;
            }

            if (grandparent_set != nullptr)
            {
                bool back_to_grandparent = false;
                int b_ep_stride = next_ep * 18;
                int b_c_stride = next_c * 18;
                int b_eo_stride = next_eo * 18;

                for (int b_move = 0; b_move < 18; ++b_move)
                {
                    int prev_ep = multi_move_table_5ep[b_ep_stride + b_move];
                    int prev_c = corner_move_table[b_c_stride + b_move];
                    int prev_eo = eo_move_table[b_eo_stride + b_move];

                    uint64_t prev_node = static_cast<uint64_t>(prev_ep) * SIZE_C_EO +
                                         static_cast<uint64_t>(prev_c) * SIZE_EO +
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
            index_pairs[next_d].push_back(next_node);
        }

        index_pairs[next_d].shrink_to_fit();
        num_list[next_d] = static_cast<int>(index_pairs[next_d].size());
        std::cout << "Depth " << next_d << ": " << num_list[next_d]
                  << " nodes (Bucket: " << (bucket_size >> 20) << "M)" << std::endl;

        {
            tsl::robin_set<uint64_t> temp;
            next_set.swap(temp);
        }
    }

    bool depth_limited_search(int arg_ep5, int arg_ep4, int arg_c, int arg_eo, int depth, int prev)
    {
        for (int i = 0; i < 18; ++i)
        {
            if (ma[prev + i])
            {
                continue;
            }

            int next_ep4 = multi_move_table_4ep[arg_ep4 + i];
            int next_eo = eo_move_table[arg_eo + i];
            int p1 = prune_table_4ep_eo[next_ep4 * SIZE_EO + next_eo];
            int h1 = (p1 == 255) ? 11 : p1;
            if (h1 >= depth)
            {
                continue;
            }

            int next_ep5 = multi_move_table_5ep[arg_ep5 + i];
            int next_c = corner_move_table[arg_c + i];
            int p2 = prune_table_5ep_c[next_ep5 * SIZE_CORNER + next_c];
            int h2 = (p2 == 255) ? 11 : p2;
            if (h2 >= depth)
            {
                continue;
            }

            sol.emplace_back(i);

            if (depth == 1)
            {
                if (next_ep5 == goal_5ep && next_c == goal_corner && next_eo == goal_eo)
                {
                    tmp = AlgToString(sol);
                    return true;
                }
            }
            else if (depth_limited_search(next_ep5 * 18, next_ep4 * 18, next_c * 18, next_eo * 18, depth - 1, i * 18))
            {
                return true;
            }

            sol.pop_back();
        }
        return false;
    }

    std::string start_search(const std::string &arg_scramble, int max_depth = 14)
    {
        sol.clear();
        std::vector<int> alg = StringToAlg(arg_scramble);

        int ep5 = goal_5ep;
        int ep4 = goal_4ep;
        int c = goal_corner;
        int eo = goal_eo;

        for (int move : alg)
        {
            ep5 = multi_move_table_5ep[ep5 * 18 + move];
            ep4 = multi_move_table_4ep[ep4 * 18 + move];
            c = corner_move_table[c * 18 + move];
            eo = eo_move_table[eo * 18 + move];
        }

        if (ep5 == goal_5ep && c == goal_corner && eo == goal_eo)
        {
            return "";
        }

        int p1 = prune_table_4ep_eo[ep4 * SIZE_EO + eo];
        int p2 = prune_table_5ep_c[ep5 * SIZE_CORNER + c];
        int d_min = std::max((p1 == 255 ? 1 : p1), (p2 == 255 ? 1 : p2));
        if (d_min == 0)
        {
            d_min = 1;
        }

        for (int d = d_min; d <= max_depth; ++d)
        {
            if (depth_limited_search(ep5 * 18, ep4 * 18, c * 18, eo * 18, d, 18 * 18))
            {
                return tmp;
            }
        }
        return "";
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

    std::vector<int> generate_raw_walk(int len)
    {
        std::vector<int> walk;
        walk.reserve(len);

        int ep5 = goal_5ep;
        int ep4 = goal_4ep;
        int c = goal_corner;
        int eo = goal_eo;
        int prev = 18;

        for (int step = 0; step < len; ++step)
        {
            std::vector<int> candidate_moves;
            candidate_moves.reserve(18);

            int p1 = prune_table_4ep_eo[ep4 * SIZE_EO + eo];
            int p2 = prune_table_5ep_c[ep5 * SIZE_CORNER + c];
            int cur_dist = std::max((p1 == 255 ? 11 : p1), (p2 == 255 ? 11 : p2));

            for (int m = 0; m < 18; ++m)
            {
                if (ma[prev * 18 + m])
                {
                    continue;
                }

                int next_ep5 = multi_move_table_5ep[ep5 * 18 + m];
                int next_ep4 = multi_move_table_4ep[ep4 * 18 + m];
                int next_c = corner_move_table[c * 18 + m];
                int next_eo = eo_move_table[eo * 18 + m];

                int np1 = prune_table_4ep_eo[next_ep4 * SIZE_EO + next_eo];
                int np2 = prune_table_5ep_c[next_ep5 * SIZE_CORNER + next_c];
                int next_dist = std::max((np1 == 255 ? 11 : np1), (np2 == 255 ? 11 : np2));

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
            ep5 = multi_move_table_5ep[ep5 * 18 + chosen_move];
            ep4 = multi_move_table_4ep[ep4 * 18 + chosen_move];
            c = corner_move_table[c * 18 + chosen_move];
            eo = eo_move_table[eo * 18 + chosen_move];
            prev = chosen_move;
        }

        return walk;
    }

    std::string get_xeocross_scramble(int len)
    {
        if (len <= 0 || len >= static_cast<int>(index_pairs.size()) || index_pairs[len].empty())
        {
            return "";
        }

        // Database sample check with exact-depth verification
        const int max_attempts = 100;
        std::uniform_int_distribution<size_t> dist(0, index_pairs[len].size() - 1);

        for (int attempt = 0; attempt < max_attempts; ++attempt)
        {
            sol.clear();
            uint64_t node = index_pairs[len][dist(generator)];

            int ep5 = static_cast<int>(node / SIZE_C_EO);
            uint64_t rem = node % SIZE_C_EO;
            int c = static_cast<int>(rem / SIZE_EO);
            int eo = static_cast<int>(rem % SIZE_EO);

            // Decode Cross 4 EP from 5 EP index
            std::vector<int> ep5_arr(5);
            index_to_array(ep5_arr, ep5, 5, 1, 12);
            for (int i = 0; i < 5; ++i)
            {
                ep5_arr[i] = (ep5_arr[i] / 18);
            }
            std::vector<int> ep4_arr = {ep5_arr[0], ep5_arr[1], ep5_arr[2], ep5_arr[3]};
            int ep4 = array_to_index(ep4_arr, 4, 1, 12);

            int p1 = prune_table_4ep_eo[ep4 * SIZE_EO + eo];
            int p2 = prune_table_5ep_c[ep5 * SIZE_CORNER + c];
            int d_min = std::max((p1 == 255 ? 1 : p1), (p2 == 255 ? 1 : p2));
            if (d_min == 0)
            {
                d_min = 1;
            }

            int actual_depth = -1;
            for (int d = d_min; d <= len; ++d)
            {
                if (depth_limited_search(ep5 * 18, ep4 * 18, c * 18, eo * 18, d, 18 * 18))
                {
                    actual_depth = d;
                    break;
                }
            }

            if (actual_depth == len)
            {
                return AlgToString(sol);
            }
        }

        return "";
    }

    std::string func(const std::string &arg_scramble = "", const std::string &arg_length = "7")
    {
        int len = std::stoi(arg_length);

        // 1. Solve incoming random scramble R to origin O
        std::string sol_str = start_search(arg_scramble);

        // 2. Obtain exact-depth XEOCross solution W from precomputed database
        std::string w_str = get_xeocross_scramble(len);

        // Fallback: limited trial walk
        if (w_str.empty())
        {
            const int max_trials = (len >= 11) ? 5000 : 1000;
            for (int trial = 0; trial < max_trials; ++trial)
            {
                std::vector<int> candidate_walk = generate_raw_walk(len);
                if (candidate_walk.empty())
                {
                    continue;
                }

                std::string walk_scramble = AlgToString(candidate_walk);
                std::string verified_sol = start_search(walk_scramble, len);
                std::vector<int> sol_alg = StringToAlg(verified_sol);

                if (static_cast<int>(sol_alg.size()) == len)
                {
                    w_str = verified_sol;
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
            // Do NOT invert w_str here. HTML's min2phase solver handles the inversion.
            gen_str = w_str;
        }

        return arg_scramble + " " + sol_str + "," + gen_str;
    }
};

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_BINDINGS(xeocross_trainer_module)
{
    emscripten::class_<xeocross_search>("xeocross_search")
        .constructor<>()
        .function("func", &xeocross_search::func);
}
#endif
