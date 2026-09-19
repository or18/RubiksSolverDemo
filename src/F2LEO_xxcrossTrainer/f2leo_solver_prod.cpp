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

std::vector<std::vector<int>> c_array = {{0}, {1, 1, 1, 1, 1}, {1, 2, 4, 8, 16, 32}, {1, 3, 9, 27, 81, 243}};
std::vector<std::vector<int>> base_array = {{0}, {0}, {1, 12, 12 * 11, 12 * 11 * 10, 12 * 11 * 10 * 9}, {1, 8, 8 * 7, 8 * 7 * 6, 8 * 7 * 6 * 5}};

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

std::vector<int> create_edge_move_table()
{
    std::vector<int> move_table(24 * 18, -1);
    int index;
    for (int i = 0; i < 24; ++i)
    {
        std::vector<int> ep(12, -1);
        std::vector<int> eo(12, -1);
        std::vector<int> cp(8, 0);
        std::vector<int> co(8, 0);
        ep[i / 2] = i / 2;
        eo[i / 2] = i % 2;
        State state(cp, co, ep, eo);
        for (int j = 0; j < 18; ++j)
        {
            State new_state = state.apply_move_edge(moves[move_names[j]], i / 2);
            auto it = std::find(new_state.ep.begin(), new_state.ep.end(), i / 2);
            index = std::distance(new_state.ep.begin(), it);
            move_table[18 * i + j] = 2 * index + new_state.eo[index];
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

// Bucket sizing configuration: 2M / 2M / 2M / 2M / 1M / 1M
static constexpr size_t BUCKET_2M = (1ULL << 21); // 2,097,152
static constexpr size_t BUCKET_1M = (1ULL << 20); // 1,048,576

static constexpr size_t TARGET_NODES_2M = static_cast<size_t>(BUCKET_2M * LOAD_FACTOR); // ~1,887,436 nodes
static constexpr size_t TARGET_NODES_1M = static_cast<size_t>(BUCKET_1M * LOAD_FACTOR); // ~943,718 nodes

struct f2leo_xxcross_search
{
    std::vector<int> sol;
    std::string scramble;
    std::string tmp;
    std::mt19937_64 generator;

    // Movement tables
    std::vector<int> edge_move_table;    // 24 * 18 (Oriented edge movement)
    std::vector<int> corner_move_table;  // 24 * 18 (Oriented corner movement)

    std::vector<int> multi_move_table_4e; // SIZE_4E * 18 (Cross 4 Edges, oriented)
    std::vector<int> multi_move_table_2e; // SIZE_2E * 18 (Shared for Slot 2E and Remaining 2E)
    std::vector<int> multi_move_table_2c; // SIZE_2C * 18 (Slot 2 Corners)

    // State space dimensions
    static constexpr uint64_t SIZE_4E = 190080ULL; // 12*11*10*9 * 2^4
    static constexpr uint64_t SIZE_2E = 528ULL;    // 12*11 * 2^2
    static constexpr uint64_t SIZE_2C = 504ULL;    // 8*7 * 3^2

    // uint64_t packing strides
    static constexpr uint64_t STRIDE_REM2E  = SIZE_2E;
    static constexpr uint64_t STRIDE_2C     = SIZE_2C * STRIDE_REM2E;     // 504 * 528 = 266,112
    static constexpr uint64_t STRIDE_SLOT2E = SIZE_2E * STRIDE_2C;        // 528 * 266,112 = 140,507,136

    // Search databases for 3 geometric configurations
    std::vector<std::vector<uint64_t>> index_pairs_para;  // BL BR (Parallel)
    std::vector<std::vector<uint64_t>> index_pairs_ortho; // BL FL (Orthogonal)
    std::vector<std::vector<uint64_t>> index_pairs_diag;  // BL FR (Diagonal)

    std::vector<int> num_list_para;
    std::vector<int> num_list_ortho;
    std::vector<int> num_list_diag;

    std::vector<bool> ma;

    // Goal definitions
    int goal_4e; // Cross 4E: DF=16, DL=18, DB=20, DR=22

    // Parallel (BL & BR)
    int goal_slot2e_para; // BL(0), BR(2)
    int goal_slot2c_para; // DBL(12), DBR(15)
    int goal_rem2e_para;  // FR(4), FL(6)

    // Orthogonal (BL & FL)
    int goal_slot2e_ortho; // BL(0), FL(6)
    int goal_slot2c_ortho; // DBL(12), DFL(21)
    int goal_rem2e_ortho;  // BR(2), FR(4)

    // Diagonal (BL & FR)
    int goal_slot2e_diag; // BL(0), FR(4)
    int goal_slot2c_diag; // DBL(12), DFR(18)
    int goal_rem2e_diag;  // BR(2), FL(6)

    // Pruning tables
    std::vector<unsigned char> prune_table_cross;
    std::vector<unsigned char> prune_table_cross_c_bl;
    std::vector<unsigned char> prune_table_cross_c_br;
    std::vector<unsigned char> prune_table_cross_c_fr;
    std::vector<unsigned char> prune_table_cross_c_fl;
    std::vector<unsigned char> prune_table_cross_e_bl;
    std::vector<unsigned char> prune_table_cross_e_br;
    std::vector<unsigned char> prune_table_cross_e_fr;
    std::vector<unsigned char> prune_table_cross_e_fl;

    f2leo_xxcross_search()
    {
        std::random_device rd;
        generator.seed(rd());

        // 1. Primitive single-piece move tables
        edge_move_table = create_edge_move_table();
        corner_move_table = create_corner_move_table();

        // 2. Multi-piece move tables
        // Cross 4E (DF=8, DL=9, DB=10, DR=11 -> orient=2, pn=12)
        create_multi_move_table(4, 2, 12, SIZE_4E, edge_move_table, multi_move_table_4e);

        // 2E (Slot 2E & Rem 2E shared -> orient=2, pn=12)
        create_multi_move_table(2, 2, 12, SIZE_2E, edge_move_table, multi_move_table_2e);

        // 2C (Slot 2 Corners -> orient=3, pn=8)
        create_multi_move_table(2, 3, 8, SIZE_2C, corner_move_table, multi_move_table_2c);

        // 3. Goal configuration definitions
        // Cross 4E: DF(16), DL(18), DB(20), DR(22)
        std::vector<int> cg4 = {16, 18, 20, 22};
        goal_4e = array_to_index(cg4, 4, 2, 12);

        // Parallel (BL & BR)
        std::vector<int> s2e_para = {0, 2};             // BL(0), BR(2)
        std::vector<int> s2c_para = {4 * 3 + 0, 5 * 3 + 0}; // DBL(12), DBR(15)
        std::vector<int> r2e_para = {4, 6};             // FR(4), FL(6)
        goal_slot2e_para = array_to_index(s2e_para, 2, 2, 12);
        goal_slot2c_para = array_to_index(s2c_para, 2, 3, 8);
        goal_rem2e_para  = array_to_index(r2e_para, 2, 2, 12);

        // Orthogonal (BL & FL)
        std::vector<int> s2e_ortho = {0, 6};             // BL(0), FL(6)
        std::vector<int> s2c_ortho = {4 * 3 + 0, 7 * 3 + 0}; // DBL(12), DFL(21)
        std::vector<int> r2e_ortho = {2, 4};             // BR(2), FR(4)
        goal_slot2e_ortho = array_to_index(s2e_ortho, 2, 2, 12);
        goal_slot2c_ortho = array_to_index(s2c_ortho, 2, 3, 8);
        goal_rem2e_ortho  = array_to_index(r2e_ortho, 2, 2, 12);

        // Diagonal (BL & FR)
        std::vector<int> s2e_diag = {0, 4};             // BL(0), FR(4)
        std::vector<int> s2c_diag = {4 * 3 + 0, 6 * 3 + 0}; // DBL(12), DFR(18)
        std::vector<int> r2e_diag = {2, 6};             // BR(2), FL(6)
        goal_slot2e_diag = array_to_index(s2e_diag, 2, 2, 12);
        goal_slot2c_diag = array_to_index(s2c_diag, 2, 3, 8);
        goal_rem2e_diag  = array_to_index(r2e_diag, 2, 2, 12);

        // 4. Pruning tables (All lower bounds safely set to depth 10)
        create_prune_table(goal_4e, SIZE_4E, 10, multi_move_table_4e, prune_table_cross);

        int goal_c_bl = 4 * 3 + 0; // 12
        int goal_c_br = 5 * 3 + 0; // 15
        int goal_c_fr = 6 * 3 + 0; // 18
        int goal_c_fl = 7 * 3 + 0; // 21

        create_prune_table2(goal_4e, goal_c_bl, SIZE_4E, 24, 10,
                            multi_move_table_4e, corner_move_table, prune_table_cross_c_bl);
        create_prune_table2(goal_4e, goal_c_br, SIZE_4E, 24, 10,
                            multi_move_table_4e, corner_move_table, prune_table_cross_c_br);
        create_prune_table2(goal_4e, goal_c_fr, SIZE_4E, 24, 10,
                            multi_move_table_4e, corner_move_table, prune_table_cross_c_fr);
        create_prune_table2(goal_4e, goal_c_fl, SIZE_4E, 24, 10,
                            multi_move_table_4e, corner_move_table, prune_table_cross_c_fl);

        int goal_e_bl = 0;
        int goal_e_br = 2;
        int goal_e_fr = 4;
        int goal_e_fl = 6;

        create_prune_table2(goal_4e, goal_e_bl, SIZE_4E, 24, 10,
                            multi_move_table_4e, edge_move_table, prune_table_cross_e_bl);
        create_prune_table2(goal_4e, goal_e_br, SIZE_4E, 24, 10,
                            multi_move_table_4e, edge_move_table, prune_table_cross_e_br);
        create_prune_table2(goal_4e, goal_e_fr, SIZE_4E, 24, 10,
                            multi_move_table_4e, edge_move_table, prune_table_cross_e_fr);
        create_prune_table2(goal_4e, goal_e_fl, SIZE_4E, 24, 10,
                            multi_move_table_4e, edge_move_table, prune_table_cross_e_fl);

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

            int idx_4e     = static_cast<int>(p_node / STRIDE_SLOT2E);
            uint64_t rem1  = p_node % STRIDE_SLOT2E;
            int idx_slot2e = static_cast<int>(rem1 / STRIDE_2C);
            uint64_t rem2  = rem1 % STRIDE_2C;
            int idx_2c     = static_cast<int>(rem2 / STRIDE_REM2E);
            int idx_rem2e  = static_cast<int>(rem2 % STRIDE_REM2E);

            int move = move_dist(generator);

            int next_4e     = multi_move_table_4e[idx_4e * 18 + move];
            int next_slot2e = multi_move_table_2e[idx_slot2e * 18 + move];
            int next_2c     = multi_move_table_2c[idx_2c * 18 + move];
            int next_rem2e  = multi_move_table_2e[idx_rem2e * 18 + move];

            uint64_t next_node = static_cast<uint64_t>(next_4e) * STRIDE_SLOT2E +
                                 static_cast<uint64_t>(next_slot2e) * STRIDE_2C +
                                 static_cast<uint64_t>(next_2c) * STRIDE_REM2E +
                                 next_rem2e;

            if (parent_set.find(next_node) != parent_set.end() ||
                next_set.find(next_node) != next_set.end())
            {
                continue;
            }

            if (grandparent_set != nullptr)
            {
                bool back_to_grandparent = false;
                int b_4e_stride     = next_4e * 18;
                int b_slot2e_stride = next_slot2e * 18;
                int b_2c_stride     = next_2c * 18;
                int b_rem2e_stride  = next_rem2e * 18;

                for (int b_move = 0; b_move < 18; ++b_move)
                {
                    int prev_4e     = multi_move_table_4e[b_4e_stride + b_move];
                    int prev_slot2e = multi_move_table_2e[b_slot2e_stride + b_move];
                    int prev_2c     = multi_move_table_2c[b_2c_stride + b_move];
                    int prev_rem2e  = multi_move_table_2e[b_rem2e_stride + b_move];

                    uint64_t prev_node = static_cast<uint64_t>(prev_4e) * STRIDE_SLOT2E +
                                         static_cast<uint64_t>(prev_slot2e) * STRIDE_2C +
                                         static_cast<uint64_t>(prev_2c) * STRIDE_REM2E +
                                         prev_rem2e;

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
        target_pairs.resize(13);
        target_num_list.resize(13, 0);

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
                int idx_4e     = static_cast<int>(node / STRIDE_SLOT2E);
                uint64_t rem1  = node % STRIDE_SLOT2E;
                int idx_slot2e = static_cast<int>(rem1 / STRIDE_2C);
                uint64_t rem2  = rem1 % STRIDE_2C;
                int idx_2c     = static_cast<int>(rem2 / STRIDE_REM2E);
                int idx_rem2e  = static_cast<int>(rem2 % STRIDE_REM2E);

                int stride_4e     = idx_4e * 18;
                int stride_slot2e = idx_slot2e * 18;
                int stride_2c     = idx_2c * 18;
                int stride_rem2e  = idx_rem2e * 18;

                for (int move = 0; move < 18; ++move)
                {
                    int next_4e     = multi_move_table_4e[stride_4e + move];
                    int next_slot2e = multi_move_table_2e[stride_slot2e + move];
                    int next_2c     = multi_move_table_2c[stride_2c + move];
                    int next_rem2e  = multi_move_table_2e[stride_rem2e + move];

                    uint64_t next_node = static_cast<uint64_t>(next_4e) * STRIDE_SLOT2E +
                                         static_cast<uint64_t>(next_slot2e) * STRIDE_2C +
                                         static_cast<uint64_t>(next_2c) * STRIDE_REM2E +
                                         next_rem2e;

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

        // Depth 9: 1M bucket with Depth 7 backtrace check
        tsl::robin_set<uint64_t> depth8_set;
        depth8_set.max_load_factor(LOAD_FACTOR);
        depth8_set.insert(target_pairs[8].begin(), target_pairs[8].end());
        expand_depth_partial(8, 9, depth8_set, target_pairs[8], &depth7_set,
                             BUCKET_2M, TARGET_NODES_2M, target_pairs, target_num_list);

        {
            tsl::robin_set<uint64_t> temp;
            depth7_set.swap(temp);
        }

        // Depth 10: 1M bucket with Depth 8 backtrace check
        tsl::robin_set<uint64_t> depth9_set;
        depth9_set.max_load_factor(LOAD_FACTOR);
        depth9_set.insert(target_pairs[9].begin(), target_pairs[9].end());
        expand_depth_partial(9, 10, depth9_set, target_pairs[9], &depth8_set,
                             BUCKET_2M, TARGET_NODES_2M, target_pairs, target_num_list);

        {
            tsl::robin_set<uint64_t> temp;
            depth8_set.swap(temp);
        }

        // Depth 11: 1M bucket with Depth 9 backtrace check
        tsl::robin_set<uint64_t> depth10_set;
        depth10_set.max_load_factor(LOAD_FACTOR);
        depth10_set.insert(target_pairs[10].begin(), target_pairs[10].end());
        expand_depth_partial(10, 11, depth10_set, target_pairs[10], &depth9_set,
                             BUCKET_1M, TARGET_NODES_1M, target_pairs, target_num_list);

        {
            tsl::robin_set<uint64_t> temp;
            depth9_set.swap(temp);
        }

        // Depth 12: 1M bucket with Depth 10 backtrace check (Frontier)
        tsl::robin_set<uint64_t> depth11_set;
        depth11_set.max_load_factor(LOAD_FACTOR);
        depth11_set.insert(target_pairs[11].begin(), target_pairs[11].end());
        expand_depth_partial(11, 12, depth11_set, target_pairs[11], &depth10_set,
                             BUCKET_1M, TARGET_NODES_1M, target_pairs, target_num_list);

        {
            tsl::robin_set<uint64_t> temp;
            depth10_set.swap(temp);
        }
        {
            tsl::robin_set<uint64_t> temp;
            depth11_set.swap(temp);
        }

        std::cout << "Database Construction Complete [" << label << "] (Depths 0-12)." << std::endl;
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
        uint64_t goal_para = static_cast<uint64_t>(goal_4e) * STRIDE_SLOT2E +
                             static_cast<uint64_t>(goal_slot2e_para) * STRIDE_2C +
                             static_cast<uint64_t>(goal_slot2c_para) * STRIDE_REM2E +
                             goal_rem2e_para;
        build_single_database(goal_para, index_pairs_para, num_list_para, "Parallel (BL-BR)");

        // 2. Build Orthogonal Database (BL-FL)
        uint64_t goal_ortho = static_cast<uint64_t>(goal_4e) * STRIDE_SLOT2E +
                              static_cast<uint64_t>(goal_slot2e_ortho) * STRIDE_2C +
                              static_cast<uint64_t>(goal_slot2c_ortho) * STRIDE_REM2E +
                              goal_rem2e_ortho;
        build_single_database(goal_ortho, index_pairs_ortho, num_list_ortho, "Orthogonal (BL-FL)");

        // 3. Build Diagonal Database (BL-FR)
        uint64_t goal_diag = static_cast<uint64_t>(goal_4e) * STRIDE_SLOT2E +
                             static_cast<uint64_t>(goal_slot2e_diag) * STRIDE_2C +
                             static_cast<uint64_t>(goal_slot2c_diag) * STRIDE_REM2E +
                             goal_rem2e_diag;
        build_single_database(goal_diag, index_pairs_diag, num_list_diag, "Diagonal (BL-FR)");

        std::cout << "\n==================================================" << std::endl;
        std::cout << "        F2LEO XXCross Complete Distribution Log   " << std::endl;
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


    // -------------------------------------------------------------------------
    // DLS for F2LEO XXCross Condition
    // Cross 4E + Slot 2E + Slot 2C fully solved, Remaining 2 middle edges EO=0
    // -------------------------------------------------------------------------
    bool depth_limited_search_f2leo_xxcross(int arg_4e, int arg_slot2e, int arg_slot2c,
                                            int rem1_val, int rem2_val,
                                            int depth, int prev,
                                            int target_goal_slot2e, int target_goal_slot2c,
                                            int c1_val, int c2_val, int e1_val, int e2_val,
                                            const std::vector<unsigned char> &prune_c2,
                                            const std::vector<unsigned char> &prune_e2)
    {
        for (int i = 0; i < 18; ++i)
        {
            if (ma[prev + i])
            {
                continue;
            }

            int next_4e = multi_move_table_4e[arg_4e + i];

            // 1. Cross 4E Pruning Lower-bound
            int p_cross = prune_table_cross[next_4e];
            int h_cross = (p_cross == 255) ? 11 : p_cross;
            if (h_cross >= depth)
            {
                continue;
            }

            // 2. Corner 1 (BL Corner) Pruning
            int next_c1 = corner_move_table[c1_val * 18 + i];
            int p_c1 = prune_table_cross_c_bl[next_4e * 24 + next_c1];
            int h_c1 = (p_c1 == 255) ? 11 : p_c1;
            if (h_c1 >= depth)
            {
                continue;
            }

            // 3. Corner 2 (Slot 2 Corner) Pruning
            int next_c2 = corner_move_table[c2_val * 18 + i];
            int p_c2 = prune_c2[next_4e * 24 + next_c2];
            int h_c2 = (p_c2 == 255) ? 11 : p_c2;
            if (h_c2 >= depth)
            {
                continue;
            }

            // 4. Edge 1 (BL Edge) Pruning
            int next_e1 = edge_move_table[e1_val * 18 + i];
            int p_e1 = prune_table_cross_e_bl[next_4e * 24 + next_e1];
            int h_e1 = (p_e1 == 255) ? 11 : p_e1;
            if (h_e1 >= depth)
            {
                continue;
            }

            // 5. Edge 2 (Slot 2 Edge) Pruning
            int next_e2 = edge_move_table[e2_val * 18 + i];
            int p_e2 = prune_e2[next_4e * 24 + next_e2];
            int h_e2 = (p_e2 == 255) ? 11 : p_e2;
            if (h_e2 >= depth)
            {
                continue;
            }

            int next_slot2e = multi_move_table_2e[arg_slot2e + i];
            int next_slot2c = multi_move_table_2c[arg_slot2c + i];

            int next_rem1 = edge_move_table[rem1_val * 18 + i];
            int next_rem2 = edge_move_table[rem2_val * 18 + i];

            sol.emplace_back(i);

            if (depth == 1)
            {
                // Goal check: Cross, 2 slots fully solved, remaining 2 edges EO solved (even parity)
                if (next_4e == goal_4e &&
                    next_slot2e == target_goal_slot2e &&
                    next_slot2c == target_goal_slot2c &&
                    (next_rem1 % 2 == 0) && (next_rem2 % 2 == 0))
                {
                    tmp = AlgToString(sol);
                    return true;
                }
            }
            else if (depth_limited_search_f2leo_xxcross(next_4e * 18, next_slot2e * 18, next_slot2c * 18,
                                                       next_rem1, next_rem2,
                                                       depth - 1, i * 18,
                                                       target_goal_slot2e, target_goal_slot2c,
                                                       next_c1, next_c2, next_e1, next_e2,
                                                       prune_c2, prune_e2))
            {
                return true;
            }

            sol.pop_back();
        }
        return false;
    }

    // -------------------------------------------------------------------------
    // Strictly evaluate the true minimal F2LEO XXCross depth
    // -------------------------------------------------------------------------
    int evaluate_scramble_depth(const std::vector<int> &scramble_alg, int target_len, int config_type)
    {
        int target_goal_slot2e;
        int target_goal_slot2c;
        int c1_goal = 4 * 3 + 0; // DBL (12)
        int c2_goal;
        int e1_goal = 0;         // BL (0)
        int e2_goal;
        int rem1_init;
        int rem2_init;

        const std::vector<unsigned char> *prune_c2 = nullptr;
        const std::vector<unsigned char> *prune_e2 = nullptr;

        if (config_type == 0) // Parallel (BL & BR)
        {
            target_goal_slot2e = goal_slot2e_para;
            target_goal_slot2c = goal_slot2c_para;
            c2_goal = 5 * 3 + 0; // DBR (15)
            e2_goal = 2;         // BR (2)
            rem1_init = 4;       // FR (4)
            rem2_init = 6;       // FL (6)
            prune_c2 = &prune_table_cross_c_br;
            prune_e2 = &prune_table_cross_e_br;
        }
        else if (config_type == 1) // Orthogonal (BL & FL)
        {
            target_goal_slot2e = goal_slot2e_ortho;
            target_goal_slot2c = goal_slot2c_ortho;
            c2_goal = 7 * 3 + 0; // DFL (21)
            e2_goal = 6;         // FL (6)
            rem1_init = 2;       // BR (2)
            rem2_init = 4;       // FR (4)
            prune_c2 = &prune_table_cross_c_fl;
            prune_e2 = &prune_table_cross_e_fl;
        }
        else // Diagonal (BL & FR)
        {
            target_goal_slot2e = goal_slot2e_diag;
            target_goal_slot2c = goal_slot2c_diag;
            c2_goal = 6 * 3 + 0; // DFR (18)
            e2_goal = 4;         // FR (4)
            rem1_init = 2;       // BR (2)
            rem2_init = 6;       // FL (6)
            prune_c2 = &prune_table_cross_c_fr;
            prune_e2 = &prune_table_cross_e_fr;
        }

        // Trace state from goal origin under the scramble
        int cur_4e     = goal_4e;
        int cur_slot2e = target_goal_slot2e;
        int cur_slot2c = target_goal_slot2c;
        int cur_rem1   = rem1_init;
        int cur_rem2   = rem2_init;
        int cur_c1     = c1_goal;
        int cur_c2     = c2_goal;
        int cur_e1     = e1_goal;
        int cur_e2     = e2_goal;

        for (int m : scramble_alg)
        {
            cur_4e     = multi_move_table_4e[cur_4e * 18 + m];
            cur_slot2e = multi_move_table_2e[cur_slot2e * 18 + m];
            cur_slot2c = multi_move_table_2c[cur_slot2c * 18 + m];
            cur_rem1   = edge_move_table[cur_rem1 * 18 + m];
            cur_rem2   = edge_move_table[cur_rem2 * 18 + m];
            cur_c1     = corner_move_table[cur_c1 * 18 + m];
            cur_c2     = corner_move_table[cur_c2 * 18 + m];
            cur_e1     = edge_move_table[cur_e1 * 18 + m];
            cur_e2     = edge_move_table[cur_e2 * 18 + m];
        }

        // Check if already solved at depth 0
        if (cur_4e == goal_4e &&
            cur_slot2e == target_goal_slot2e &&
            cur_slot2c == target_goal_slot2c &&
            (cur_rem1 % 2 == 0) && (cur_rem2 % 2 == 0))
        {
            return 0;
        }

        // Compute lower bound for starting search depth
        int p_cross = prune_table_cross[cur_4e];
        int p_c1 = prune_table_cross_c_bl[cur_4e * 24 + cur_c1];
        int p_c2 = (*prune_c2)[cur_4e * 24 + cur_c2];
        int p_e1 = prune_table_cross_e_bl[cur_4e * 24 + cur_e1];
        int p_e2 = (*prune_e2)[cur_4e * 24 + cur_e2];

        int d_min = std::max({(p_cross == 255 ? 1 : p_cross),
                              (p_c1 == 255 ? 1 : p_c1),
                              (p_c2 == 255 ? 1 : p_c2),
                              (p_e1 == 255 ? 1 : p_e1),
                              (p_e2 == 255 ? 1 : p_e2)});
        if (d_min == 0)
        {
            d_min = 1;
        }

        if (d_min > target_len)
        {
            return target_len + 1;
        }

        // Iterative deepening search up to target_len
        for (int d = d_min; d <= target_len; ++d)
        {
            sol.clear();
            if (depth_limited_search_f2leo_xxcross(cur_4e * 18, cur_slot2e * 18, cur_slot2c * 18,
                                                   cur_rem1, cur_rem2,
                                                   d, 18 * 18,
                                                   target_goal_slot2e, target_goal_slot2c,
                                                   cur_c1, cur_c2, cur_e1, cur_e2,
                                                   *prune_c2, *prune_e2))
            {
                return d;
            }
        }

        return target_len + 1;
    }


    // -------------------------------------------------------------------------
    // DLS for Origin O Convergence
    // Cross 4E + Slot 2E + Slot 2C + Remaining 2 Middle Edges (ALL Fully Solved)
    // -------------------------------------------------------------------------
    bool depth_limited_search_origin(int arg_4e, int arg_slot2e, int arg_slot2c, int arg_rem2e,
                                     int depth, int prev,
                                     int target_goal_slot2e, int target_goal_slot2c, int target_goal_rem2e,
                                     int c1_val, int c2_val, int e1_val, int e2_val, int r1_val, int r2_val,
                                     const std::vector<unsigned char> &prune_c2,
                                     const std::vector<unsigned char> &prune_e2,
                                     const std::vector<unsigned char> &prune_r1,
                                     const std::vector<unsigned char> &prune_r2)
    {
        for (int i = 0; i < 18; ++i)
        {
            if (ma[prev + i])
            {
                continue;
            }

            int next_4e = multi_move_table_4e[arg_4e + i];

            // 1. Cross 4E Pruning Lower-bound
            int p_cross = prune_table_cross[next_4e];
            int h_cross = (p_cross == 255) ? 11 : p_cross;
            if (h_cross >= depth)
            {
                continue;
            }

            // 2. Corner 1 (BL) & Corner 2 (Slot 2) Pruning
            int next_c1 = corner_move_table[c1_val * 18 + i];
            int p_c1 = prune_table_cross_c_bl[next_4e * 24 + next_c1];
            int h_c1 = (p_c1 == 255) ? 11 : p_c1;
            if (h_c1 >= depth)
            {
                continue;
            }

            int next_c2 = corner_move_table[c2_val * 18 + i];
            int p_c2 = prune_c2[next_4e * 24 + next_c2];
            int h_c2 = (p_c2 == 255) ? 11 : p_c2;
            if (h_c2 >= depth)
            {
                continue;
            }

            // 3. Edge 1 (BL) & Edge 2 (Slot 2) Pruning
            int next_e1 = edge_move_table[e1_val * 18 + i];
            int p_e1 = prune_table_cross_e_bl[next_4e * 24 + next_e1];
            int h_e1 = (p_e1 == 255) ? 11 : p_e1;
            if (h_e1 >= depth)
            {
                continue;
            }

            int next_e2 = edge_move_table[e2_val * 18 + i];
            int p_e2 = prune_e2[next_4e * 24 + next_e2];
            int h_e2 = (p_e2 == 255) ? 11 : p_e2;
            if (h_e2 >= depth)
            {
                continue;
            }

            // 4. Remaining Middle Edge 1 & Edge 2 Pruning
            int next_r1 = edge_move_table[r1_val * 18 + i];
            int p_r1 = prune_r1[next_4e * 24 + next_r1];
            int h_r1 = (p_r1 == 255) ? 11 : p_r1;
            if (h_r1 >= depth)
            {
                continue;
            }

            int next_r2 = edge_move_table[r2_val * 18 + i];
            int p_r2 = prune_r2[next_4e * 24 + next_r2];
            int h_r2 = (p_r2 == 255) ? 11 : p_r2;
            if (h_r2 >= depth)
            {
                continue;
            }

            int next_slot2e = multi_move_table_2e[arg_slot2e + i];
            int next_slot2c = multi_move_table_2c[arg_slot2c + i];
            int next_rem2e  = multi_move_table_2e[arg_rem2e + i];

            sol.emplace_back(i);

            if (depth == 1)
            {
                if (next_4e == goal_4e &&
                    next_slot2e == target_goal_slot2e &&
                    next_slot2c == target_goal_slot2c &&
                    next_rem2e == target_goal_rem2e)
                {
                    tmp = AlgToString(sol);
                    return true;
                }
            }
            else if (depth_limited_search_origin(next_4e * 18, next_slot2e * 18, next_slot2c * 18, next_rem2e * 18,
                                                depth - 1, i * 18,
                                                target_goal_slot2e, target_goal_slot2c, target_goal_rem2e,
                                                next_c1, next_c2, next_e1, next_e2, next_r1, next_r2,
                                                prune_c2, prune_e2, prune_r1, prune_r2))
            {
                return true;
            }

            sol.pop_back();
        }
        return false;
    }

    // -------------------------------------------------------------------------
    // Solve incoming random scramble R strictly to fully solved Origin O
    // -------------------------------------------------------------------------
    std::string start_search_origin(const std::string &arg_scramble, int config_type, int max_depth = 14)
    {
        sol.clear();
        std::vector<int> alg = StringToAlg(arg_scramble);

        int target_goal_slot2e;
        int target_goal_slot2c;
        int target_goal_rem2e;
        int c1_goal = 4 * 3 + 0; // DBL (12)
        int c2_goal;
        int e1_goal = 0;         // BL (0)
        int e2_goal;
        int r1_goal;
        int r2_goal;

        const std::vector<unsigned char> *prune_c2 = nullptr;
        const std::vector<unsigned char> *prune_e2 = nullptr;
        const std::vector<unsigned char> *prune_r1 = nullptr;
        const std::vector<unsigned char> *prune_r2 = nullptr;

        if (config_type == 0) // Parallel (BL & BR)
        {
            target_goal_slot2e = goal_slot2e_para;
            target_goal_slot2c = goal_slot2c_para;
            target_goal_rem2e  = goal_rem2e_para;
            c2_goal = 5 * 3 + 0; // DBR (15)
            e2_goal = 2;         // BR (2)
            r1_goal = 4;         // FR (4)
            r2_goal = 6;         // FL (6)
            prune_c2 = &prune_table_cross_c_br;
            prune_e2 = &prune_table_cross_e_br;
            prune_r1 = &prune_table_cross_e_fr;
            prune_r2 = &prune_table_cross_e_fl;
        }
        else if (config_type == 1) // Orthogonal (BL & FL)
        {
            target_goal_slot2e = goal_slot2e_ortho;
            target_goal_slot2c = goal_slot2c_ortho;
            target_goal_rem2e  = goal_rem2e_ortho;
            c2_goal = 7 * 3 + 0; // DFL (21)
            e2_goal = 6;         // FL (6)
            r1_goal = 2;         // BR (2)
            r2_goal = 4;         // FR (4)
            prune_c2 = &prune_table_cross_c_fl;
            prune_e2 = &prune_table_cross_e_fl;
            prune_r1 = &prune_table_cross_e_br;
            prune_r2 = &prune_table_cross_e_fr;
        }
        else // Diagonal (BL & FR)
        {
            target_goal_slot2e = goal_slot2e_diag;
            target_goal_slot2c = goal_slot2c_diag;
            target_goal_rem2e  = goal_rem2e_diag;
            c2_goal = 6 * 3 + 0; // DFR (18)
            e2_goal = 4;         // FR (4)
            r1_goal = 2;         // BR (2)
            r2_goal = 6;         // FL (6)
            prune_c2 = &prune_table_cross_c_fr;
            prune_e2 = &prune_table_cross_e_fr;
            prune_r1 = &prune_table_cross_e_br;
            prune_r2 = &prune_table_cross_e_fl;
        }

        // Trace state from origin under scramble R
        int cur_4e     = goal_4e;
        int cur_slot2e = target_goal_slot2e;
        int cur_slot2c = target_goal_slot2c;
        int cur_rem2e  = target_goal_rem2e;
        int cur_c1     = c1_goal;
        int cur_c2     = c2_goal;
        int cur_e1     = e1_goal;
        int cur_e2     = e2_goal;
        int cur_r1     = r1_goal;
        int cur_r2     = r2_goal;

        for (int m : alg)
        {
            cur_4e     = multi_move_table_4e[cur_4e * 18 + m];
            cur_slot2e = multi_move_table_2e[cur_slot2e * 18 + m];
            cur_slot2c = multi_move_table_2c[cur_slot2c * 18 + m];
            cur_rem2e  = multi_move_table_2e[cur_rem2e * 18 + m];
            cur_c1     = corner_move_table[cur_c1 * 18 + m];
            cur_c2     = corner_move_table[cur_c2 * 18 + m];
            cur_e1     = edge_move_table[cur_e1 * 18 + m];
            cur_e2     = edge_move_table[cur_e2 * 18 + m];
            cur_r1     = edge_move_table[cur_r1 * 18 + m];
            cur_r2     = edge_move_table[cur_r2 * 18 + m];
        }

        // Already at Origin
        if (cur_4e == goal_4e &&
            cur_slot2e == target_goal_slot2e &&
            cur_slot2c == target_goal_slot2c &&
            cur_rem2e == target_goal_rem2e)
        {
            return "";
        }

        int p_cross = prune_table_cross[cur_4e];
        int p_c1 = prune_table_cross_c_bl[cur_4e * 24 + cur_c1];
        int p_c2 = (*prune_c2)[cur_4e * 24 + cur_c2];
        int p_e1 = prune_table_cross_e_bl[cur_4e * 24 + cur_e1];
        int p_e2 = (*prune_e2)[cur_4e * 24 + cur_e2];
        int p_r1 = (*prune_r1)[cur_4e * 24 + cur_r1];
        int p_r2 = (*prune_r2)[cur_4e * 24 + cur_r2];

        int d_min = std::max({(p_cross == 255 ? 1 : p_cross),
                              (p_c1 == 255 ? 1 : p_c1),
                              (p_c2 == 255 ? 1 : p_c2),
                              (p_e1 == 255 ? 1 : p_e1),
                              (p_e2 == 255 ? 1 : p_e2),
                              (p_r1 == 255 ? 1 : p_r1),
                              (p_r2 == 255 ? 1 : p_r2)});
        if (d_min == 0)
        {
            d_min = 1;
        }

        for (int d = d_min; d <= max_depth; ++d)
        {
            if (depth_limited_search_origin(cur_4e * 18, cur_slot2e * 18, cur_slot2c * 18, cur_rem2e * 18,
                                           d, 18 * 18,
                                           target_goal_slot2e, target_goal_slot2c, target_goal_rem2e,
                                           cur_c1, cur_c2, cur_e1, cur_e2, cur_r1, cur_r2,
                                           *prune_c2, *prune_e2, *prune_r1, *prune_r2))
            {
                return tmp;
            }
        }

        return "";
    }

    // Invert numerical algorithm sequence
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

    // -------------------------------------------------------------------------
    // Candidate Generator: Guided random walk from Origin O
    // -------------------------------------------------------------------------
    std::vector<int> generate_raw_walk(int len, int config_type)
    {
        std::vector<int> walk;
        walk.reserve(len);

        int target_goal_slot2e;
        int target_goal_slot2c;
        int target_goal_rem2e;
        int c1 = 4 * 3 + 0; // DBL (12)
        int c2;
        int e1 = 0;         // BL (0)
        int e2;
        int r1;
        int r2;

        const std::vector<unsigned char> *prune_c2 = nullptr;
        const std::vector<unsigned char> *prune_e2 = nullptr;
        const std::vector<unsigned char> *prune_r1 = nullptr;
        const std::vector<unsigned char> *prune_r2 = nullptr;

        if (config_type == 0) // Parallel (BL & BR)
        {
            target_goal_slot2e = goal_slot2e_para;
            target_goal_slot2c = goal_slot2c_para;
            target_goal_rem2e  = goal_rem2e_para;
            c2 = 5 * 3 + 0; // DBR (15)
            e2 = 2;         // BR (2)
            r1 = 4;         // FR (4)
            r2 = 6;         // FL (6)
            prune_c2 = &prune_table_cross_c_br;
            prune_e2 = &prune_table_cross_e_br;
            prune_r1 = &prune_table_cross_e_fr;
            prune_r2 = &prune_table_cross_e_fl;
        }
        else if (config_type == 1) // Orthogonal (BL & FL)
        {
            target_goal_slot2e = goal_slot2e_ortho;
            target_goal_slot2c = goal_slot2c_ortho;
            target_goal_rem2e  = goal_rem2e_ortho;
            c2 = 7 * 3 + 0; // DFL (21)
            e2 = 6;         // FL (6)
            r1 = 2;         // BR (2)
            r2 = 4;         // FR (4)
            prune_c2 = &prune_table_cross_c_fl;
            prune_e2 = &prune_table_cross_e_fl;
            prune_r1 = &prune_table_cross_e_br;
            prune_r2 = &prune_table_cross_e_fr;
        }
        else // Diagonal (BL & FR)
        {
            target_goal_slot2e = goal_slot2e_diag;
            target_goal_slot2c = goal_slot2c_diag;
            target_goal_rem2e  = goal_rem2e_diag;
            c2 = 6 * 3 + 0; // DFR (18)
            e2 = 4;         // FR (4)
            r1 = 2;         // BR (2)
            r2 = 6;         // FL (6)
            prune_c2 = &prune_table_cross_c_fr;
            prune_e2 = &prune_table_cross_e_fr;
            prune_r1 = &prune_table_cross_e_br;
            prune_r2 = &prune_table_cross_e_fl;
        }

        int idx_4e     = goal_4e;
        int idx_slot2e = target_goal_slot2e;
        int idx_slot2c = target_goal_slot2c;
        int idx_rem2e  = target_goal_rem2e;
        int prev = 18;

        for (int step = 0; step < len; ++step)
        {
            std::vector<int> candidate_moves;
            candidate_moves.reserve(18);

            // Compute composite lower bound at current state
            int p_cross = prune_table_cross[idx_4e];
            int p_c1 = prune_table_cross_c_bl[idx_4e * 24 + c1];
            int p_c2 = (*prune_c2)[idx_4e * 24 + c2];
            int p_e1 = prune_table_cross_e_bl[idx_4e * 24 + e1];
            int p_e2 = (*prune_e2)[idx_4e * 24 + e2];
            int p_r1 = (*prune_r1)[idx_4e * 24 + r1];
            int p_r2 = (*prune_r2)[idx_4e * 24 + r2];

            int cur_dist = std::max({(p_cross == 255 ? 11 : p_cross),
                                     (p_c1 == 255 ? 11 : p_c1),
                                     (p_c2 == 255 ? 11 : p_c2),
                                     (p_e1 == 255 ? 11 : p_e1),
                                     (p_e2 == 255 ? 11 : p_e2),
                                     (p_r1 == 255 ? 11 : p_r1),
                                     (p_r2 == 255 ? 11 : p_r2)});

            for (int m = 0; m < 18; ++m)
            {
                if (ma[prev * 18 + m])
                {
                    continue;
                }

                int next_4e = multi_move_table_4e[idx_4e * 18 + m];
                int next_c1 = corner_move_table[c1 * 18 + m];
                int next_c2 = corner_move_table[c2 * 18 + m];
                int next_e1 = edge_move_table[e1 * 18 + m];
                int next_e2 = edge_move_table[e2 * 18 + m];
                int next_r1 = edge_move_table[r1 * 18 + m];
                int next_r2 = edge_move_table[r2 * 18 + m];

                int np_cross = prune_table_cross[next_4e];
                int np_c1 = prune_table_cross_c_bl[next_4e * 24 + next_c1];
                int np_c2 = (*prune_c2)[next_4e * 24 + next_c2];
                int np_e1 = prune_table_cross_e_bl[next_4e * 24 + next_e1];
                int np_e2 = (*prune_e2)[next_4e * 24 + next_e2];
                int np_r1 = (*prune_r1)[next_4e * 24 + next_r1];
                int np_r2 = (*prune_r2)[next_4e * 24 + next_r2];

                int next_dist = std::max({(np_cross == 255 ? 11 : np_cross),
                                          (np_c1 == 255 ? 11 : np_c1),
                                          (np_c2 == 255 ? 11 : np_c2),
                                          (np_e1 == 255 ? 11 : np_e1),
                                          (np_e2 == 255 ? 11 : np_e2),
                                          (np_r1 == 255 ? 11 : np_r1),
                                          (np_r2 == 255 ? 11 : np_r2)});

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
            idx_4e     = multi_move_table_4e[idx_4e * 18 + chosen_move];
            idx_slot2e = multi_move_table_2e[idx_slot2e * 18 + chosen_move];
            idx_slot2c = multi_move_table_2c[idx_slot2c * 18 + chosen_move];
            idx_rem2e  = multi_move_table_2e[idx_rem2e * 18 + chosen_move];
            c1 = corner_move_table[c1 * 18 + chosen_move];
            c2 = corner_move_table[c2 * 18 + chosen_move];
            e1 = edge_move_table[e1 * 18 + chosen_move];
            e2 = edge_move_table[e2 * 18 + chosen_move];
            r1 = edge_move_table[r1 * 18 + chosen_move];
            r2 = edge_move_table[r2 * 18 + chosen_move];
            prev = chosen_move;
        }

        return walk;
    }


    // -------------------------------------------------------------------------
    // Sample candidate algorithm directly from precomputed database
    // -------------------------------------------------------------------------
    std::vector<int> get_xxcross_scramble(int len, int config_type)
    {
        const auto &target_pairs = (config_type == 0) ? index_pairs_para :
                                   (config_type == 1) ? index_pairs_ortho :
                                                        index_pairs_diag;

        if (len <= 0 || len >= static_cast<int>(target_pairs.size()) || target_pairs[len].empty())
        {
            return {};
        }

        int target_goal_slot2e;
        int target_goal_slot2c;
        int target_goal_rem2e;
        int c1_goal = 4 * 3 + 0; // DBL (12)
        int c2_goal;
        int e1_goal = 0;         // BL (0)
        int e2_goal;
        int r1_goal;
        int r2_goal;

        const std::vector<unsigned char> *prune_c2 = nullptr;
        const std::vector<unsigned char> *prune_e2 = nullptr;
        const std::vector<unsigned char> *prune_r1 = nullptr;
        const std::vector<unsigned char> *prune_r2 = nullptr;

        if (config_type == 0) // Parallel (BL & BR)
        {
            target_goal_slot2e = goal_slot2e_para;
            target_goal_slot2c = goal_slot2c_para;
            target_goal_rem2e  = goal_rem2e_para;
            c2_goal = 5 * 3 + 0;
            e2_goal = 2;
            r1_goal = 4;
            r2_goal = 6;
            prune_c2 = &prune_table_cross_c_br;
            prune_e2 = &prune_table_cross_e_br;
            prune_r1 = &prune_table_cross_e_fr;
            prune_r2 = &prune_table_cross_e_fl;
        }
        else if (config_type == 1) // Orthogonal (BL & FL)
        {
            target_goal_slot2e = goal_slot2e_ortho;
            target_goal_slot2c = goal_slot2c_ortho;
            target_goal_rem2e  = goal_rem2e_ortho;
            c2_goal = 7 * 3 + 0;
            e2_goal = 6;
            r1_goal = 2;
            r2_goal = 4;
            prune_c2 = &prune_table_cross_c_fl;
            prune_e2 = &prune_table_cross_e_fl;
            prune_r1 = &prune_table_cross_e_br;
            prune_r2 = &prune_table_cross_e_fr;
        }
        else // Diagonal (BL & FR)
        {
            target_goal_slot2e = goal_slot2e_diag;
            target_goal_slot2c = goal_slot2c_diag;
            target_goal_rem2e  = goal_rem2e_diag;
            c2_goal = 6 * 3 + 0;
            e2_goal = 4;
            r1_goal = 2;
            r2_goal = 6;
            prune_c2 = &prune_table_cross_c_fr;
            prune_e2 = &prune_table_cross_e_fr;
            prune_r1 = &prune_table_cross_e_br;
            prune_r2 = &prune_table_cross_e_fl;
        }

        std::vector<int> corners_arr(2);
        std::vector<int> slot_edges_arr(2);
        std::vector<int> rem_edges_arr(2);

        // Try sampling up to 30 times from precomputed database nodes
        const int max_db_samples = 30;
        std::uniform_int_distribution<size_t> dist(0, target_pairs[len].size() - 1);

        for (int attempt = 0; attempt < max_db_samples; ++attempt)
        {
            uint64_t node = target_pairs[len][dist(generator)];

            int idx_4e     = static_cast<int>(node / STRIDE_SLOT2E);
            uint64_t rem1  = node % STRIDE_SLOT2E;
            int idx_slot2e = static_cast<int>(rem1 / STRIDE_2C);
            uint64_t rem2  = rem1 % STRIDE_2C;
            int idx_2c     = static_cast<int>(rem2 / STRIDE_REM2E);
            int idx_rem2e  = static_cast<int>(rem2 % STRIDE_REM2E);

            index_to_array(corners_arr, idx_2c, 2, 3, 8);
            int c1 = corners_arr[0] / 18;
            int c2 = corners_arr[1] / 18;

            index_to_array(slot_edges_arr, idx_slot2e, 2, 2, 12);
            int e1 = slot_edges_arr[0] / 18;
            int e2 = slot_edges_arr[1] / 18;

            index_to_array(rem_edges_arr, idx_rem2e, 2, 2, 12);
            int r1 = rem_edges_arr[0] / 18;
            int r2 = rem_edges_arr[1] / 18;

            int p_cross = prune_table_cross[idx_4e];
            int p_c1 = prune_table_cross_c_bl[idx_4e * 24 + c1];
            int p_c2 = (*prune_c2)[idx_4e * 24 + c2];
            int p_e1 = prune_table_cross_e_bl[idx_4e * 24 + e1];
            int p_e2 = (*prune_e2)[idx_4e * 24 + e2];
            int p_r1 = (*prune_r1)[idx_4e * 24 + r1];
            int p_r2 = (*prune_r2)[idx_4e * 24 + r2];

            int d_min = std::max({(p_cross == 255 ? 1 : p_cross),
                                  (p_c1 == 255 ? 1 : p_c1),
                                  (p_c2 == 255 ? 1 : p_c2),
                                  (p_e1 == 255 ? 1 : p_e1),
                                  (p_e2 == 255 ? 1 : p_e2),
                                  (p_r1 == 255 ? 1 : p_r1),
                                  (p_r2 == 255 ? 1 : p_r2)});
            if (d_min == 0) d_min = 1;

            sol.clear();
            for (int d = d_min; d <= len; ++d)
            {
                if (depth_limited_search_origin(idx_4e * 18, idx_slot2e * 18, idx_2c * 18, idx_rem2e * 18,
                                                d, 18 * 18,
                                                target_goal_slot2e, target_goal_slot2c, target_goal_rem2e,
                                                c1, c2, e1, e2, r1, r2,
                                                *prune_c2, *prune_e2, *prune_r1, *prune_r2))
                {
                    if (d == len)
                    {
                        // Path from node to origin is sol. Invert to get walk from origin to node.
                        return invert_alg(sol);
                    }
                    break;
                }
            }
        }

        return {};
    }

    // -------------------------------------------------------------------------
    // Single-Loop Production Generator for F2LEO XXCross
    // -------------------------------------------------------------------------
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

        // 1. Solve incoming random scramble R strictly to Origin O
        std::string sol_str = start_search_origin(arg_scramble, config_type);

        std::vector<int> r_alg = StringToAlg(arg_scramble);
        std::vector<int> s_alg = StringToAlg(sol_str);

        // Base sequence: R * S (strictly at Origin O)
        std::vector<int> rs_alg = r_alg;
        rs_alg.insert(rs_alg.end(), s_alg.begin(), s_alg.end());

        // Fixed trial budget of 50 for fast UI response
        const int max_trials = 50;
        std::vector<int> chosen_walk;

        for (int trial = 0; trial < max_trials; ++trial)
        {
            std::vector<int> candidate_walk;

            // Alternate between DB sampling (first 30 trials) and Guided Raw Walk
            if (trial < 30)
            {
                candidate_walk = get_xxcross_scramble(len, config_type);
            }
            if (candidate_walk.empty())
            {
                candidate_walk = generate_raw_walk(len, config_type);
            }
            if (candidate_walk.empty())
            {
                continue;
            }

            // HTML applies: mapped_seq based on ret[1] + reverse(ret[0])
            // Inverted by min2phase, the actual scramble applied to the cube is:
            // X = (candidate_walk * (R * S)^-1)^-1 = (R * S) * candidate_walk^-1
            std::vector<int> inv_w = invert_alg(candidate_walk);

            std::vector<int> test_composed = rs_alg;
            test_composed.insert(test_composed.end(), inv_w.begin(), inv_w.end());

            // Directly evaluate true minimal F2LEO XXCross depth on composed state
            int composed_depth = evaluate_scramble_depth(test_composed, len, config_type);

            if (composed_depth == len)
            {
                chosen_walk = candidate_walk;
                break;
            }
        }

        std::string gen_str;
        if (chosen_walk.empty())
        {
            gen_str = "RETRY_NEEDED";
        }
        else
        {
            gen_str = AlgToString(chosen_walk);
        }

        // ret[0]: R * S (Origin sequence), ret[1]: candidate_walk (length len)
        std::string ret = arg_scramble + " " + sol_str + "," + gen_str;
        return ret;
    }
};

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_BINDINGS(f2leo_xxcross_trainer_module)
{
    emscripten::class_<f2leo_xxcross_search>("f2leo_xxcross_search")
        .constructor<>()
        .function("func", &f2leo_xxcross_search::func);
}
#endif
