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
    int tmp2 = 24 / pn;
    for (int i = 0; i < n; ++i)
    {
        index_o += (a[i] % c) * c_array[c][n - i - 1];
        a[i] /= c;
    }
    for (int i = 0; i < n; ++i)
    {
        int tmp = 0;
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
            int index = std::distance(new_state.ep.begin(), it);
            move_table[18 * i + j] = 2 * index + new_state.eo[index];
        }
    }
    return move_table;
}

std::vector<int> create_corner_move_table()
{
    std::vector<int> move_table(24 * 18, -1);
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
            int index = std::distance(new_state.cp.begin(), it);
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


struct xxxcross_search
{
    std::vector<int> sol;
    std::string scramble;
    std::string tmp;
    std::mt19937_64 generator;

    // Common Move tables
    std::vector<int> edge_move_table;
    std::vector<int> corner_move_table;
    std::vector<int> multi_move_table_4e; // Cross 4 Edges
    std::vector<int> multi_move_table_3e; // Slot 3 Edges (BL, BR, FR)
    std::vector<int> multi_move_table_3c; // Slot 3 Corners (DLB, DBR, DFR)

    // Dual/Triple Pruning tables: Cross 4E * Single Corner
    // c1: DLB (BL slot)
    // c2: DBR (BR slot)
    // c3: DFR (FR slot)
    std::vector<unsigned char> prune_table_4e_c1;
    std::vector<unsigned char> prune_table_4e_c2;
    std::vector<unsigned char> prune_table_4e_c3;

    // Search database (Single instance for FL-empty base state)
    std::vector<std::vector<uint64_t>> index_pairs;
    std::vector<int> num_list;

    std::vector<bool> ma;

    // State sizes
    static constexpr uint64_t SIZE_4E = 190080ULL; // 24 * 22 * 20 * 18
    static constexpr uint64_t SIZE_3E = 10560ULL;  // 12 * 11 * 10 * 8
    static constexpr uint64_t SIZE_3C = 9072ULL;   // 8 * 7 * 6 * 27
    static constexpr uint64_t SIZE_C = 24ULL;      // 8 * 3
    static constexpr uint64_t SIZE_3E_3C = SIZE_3E * SIZE_3C; // 95,800,320ULL

    // Goal states (Origin: Cross + BL, BR, FR solved; FL open)
    int goal_4e;
    int goal_3e;
    int goal_3c;
    uint64_t goal_node;

    static constexpr int GOAL_C1 = 4 * 3 + 0; // DLB (BL)
    static constexpr int GOAL_C2 = 5 * 3 + 0; // DBR (BR)
    static constexpr int GOAL_C3 = 6 * 3 + 0; // DFR (FR)

    // Pruning table for XCross (Cross 4E + BL Edge)
    std::vector<unsigned char> prune_table_4e_e1;

    static constexpr int GOAL_E1 = 0 * 2 + 0; // BL edge (pos 0, ori 0)
    static constexpr uint64_t SIZE_E = 24ULL; // 12 * 2
                                              //
    xxxcross_search()
    {
        std::random_device rd;
        generator.seed(rd());

        // Basic move tables
        edge_move_table = create_edge_move_table();
        corner_move_table = create_corner_move_table();

        // Cross 4 Edges: DF(8), DL(9), DB(10), DR(11) with orientation 0
        create_multi_move_table(4, 2, 12, SIZE_4E, edge_move_table, multi_move_table_4e);

        // Slot 3 Edges: BL(0), BR(1), FR(2) with orientation 0 (pn=12, size=10560)
        create_multi_move_table(3, 2, 12, SIZE_3E, edge_move_table, multi_move_table_3e);

        // Slot 3 Corners: DLB(4), DBR(5), DFR(6) with orientation 0 (pn=8, size=9072)
        create_multi_move_table(3, 3, 8, SIZE_3C, corner_move_table, multi_move_table_3c);

        // Set Cross 4 Edges goal
        std::vector<int> cross_edges = {8 * 2 + 0, 9 * 2 + 0, 10 * 2 + 0, 11 * 2 + 0};
        goal_4e = array_to_index(cross_edges, 4, 2, 12);

        // Set Slot 3 Edges goal: BL(0), BR(1), FR(2)
        std::vector<int> slot_edges = {0 * 2 + 0, 1 * 2 + 0, 2 * 2 + 0};
        goal_3e = array_to_index(slot_edges, 3, 2, 12);

        // Set Slot 3 Corners goal: DLB(4), DBR(5), DFR(6)
        std::vector<int> slot_corners = {4 * 3 + 0, 5 * 3 + 0, 6 * 3 + 0};
        goal_3c = array_to_index(slot_corners, 3, 3, 8);

        // Composite goal node
        goal_node = static_cast<uint64_t>(goal_4e) * SIZE_3E_3C +
                    static_cast<uint64_t>(goal_3e) * SIZE_3C +
                    goal_3c;

        // Pruning tables: Cross 4E * Single Corner (depth 10)
        create_prune_table2(goal_4e, GOAL_C1, SIZE_4E, SIZE_C, 10,
                            multi_move_table_4e, corner_move_table, prune_table_4e_c1);

        create_prune_table2(goal_4e, GOAL_C2, SIZE_4E, SIZE_C, 10,
                            multi_move_table_4e, corner_move_table, prune_table_4e_c2);

        create_prune_table2(goal_4e, GOAL_C3, SIZE_4E, SIZE_C, 10,
                            multi_move_table_4e, corner_move_table, prune_table_4e_c3);

        // Prune table for XCross step: Cross 4E * BL Edge (depth 10)
        create_prune_table2(goal_4e, GOAL_E1, SIZE_4E, SIZE_E, 10,
                            multi_move_table_4e, edge_move_table, prune_table_4e_e1);

        ma = create_ma_table();

        build_database();
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

            int idx_4e = static_cast<int>(p_node / SIZE_3E_3C);
            uint64_t rem = p_node % SIZE_3E_3C;
            int idx_3e = static_cast<int>(rem / SIZE_3C);
            int idx_3c = static_cast<int>(rem % SIZE_3C);

            int move = move_dist(generator);

            int next_4e = multi_move_table_4e[idx_4e * 18 + move];
            int next_3e = multi_move_table_3e[idx_3e * 18 + move];
            int next_3c = multi_move_table_3c[idx_3c * 18 + move];

            uint64_t next_node = static_cast<uint64_t>(next_4e) * SIZE_3E_3C +
                                 static_cast<uint64_t>(next_3e) * SIZE_3C +
                                 next_3c;

            if (parent_set.find(next_node) != parent_set.end() ||
                next_set.find(next_node) != next_set.end())
            {
                continue;
            }

            if (grandparent_set != nullptr)
            {
                bool back_to_grandparent = false;
                int b_4e_stride = next_4e * 18;
                int b_3e_stride = next_3e * 18;
                int b_3c_stride = next_3c * 18;

                for (int b_move = 0; b_move < 18; ++b_move)
                {
                    int prev_4e = multi_move_table_4e[b_4e_stride + b_move];
                    int prev_3e = multi_move_table_3e[b_3e_stride + b_move];
                    int prev_3c = multi_move_table_3c[b_3c_stride + b_move];

                    uint64_t prev_node = static_cast<uint64_t>(prev_4e) * SIZE_3E_3C +
                                         static_cast<uint64_t>(prev_3e) * SIZE_3C +
                                         prev_3c;

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

    void build_database()
    {
        // Allocate depths 0 to 14
        index_pairs.resize(15);
        num_list.resize(15, 0);

        index_pairs[0].push_back(goal_node);
        num_list[0] = 1;

        std::cout << "\n=== Phase 1: Full BFS (Depths 1-6) [XXXCross FL-Open] ===" << std::endl;
        std::cout << "Depth 0: " << num_list[0] << " nodes" << std::endl;

        tsl::robin_set<uint64_t> prev_set, cur_set, next_set;
        prev_set.max_load_factor(LOAD_FACTOR);
        cur_set.max_load_factor(LOAD_FACTOR);
        next_set.max_load_factor(LOAD_FACTOR);

        cur_set.insert(goal_node);

        for (int d = 1; d <= 6; ++d)
        {
            next_set.clear();
            for (uint64_t node : cur_set)
            {
                int idx_4e = static_cast<int>(node / SIZE_3E_3C);
                uint64_t rem = node % SIZE_3E_3C;
                int idx_3e = static_cast<int>(rem / SIZE_3C);
                int idx_3c = static_cast<int>(rem % SIZE_3C);

                int stride_4e = idx_4e * 18;
                int stride_3e = idx_3e * 18;
                int stride_3c = idx_3c * 18;

                for (int move = 0; move < 18; ++move)
                {
                    int next_4e = multi_move_table_4e[stride_4e + move];
                    int next_3e = multi_move_table_3e[stride_3e + move];
                    int next_3c = multi_move_table_3c[stride_3c + move];

                    uint64_t next_node = static_cast<uint64_t>(next_4e) * SIZE_3E_3C +
                                         static_cast<uint64_t>(next_3e) * SIZE_3C +
                                         next_3c;

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

        {
            tsl::robin_set<uint64_t> temp;
            prev_set.swap(temp);
        }

        std::cout << "\n=== Phase 2: Local Expansion [XXXCross FL-Open] ===" << std::endl;

        tsl::robin_set<uint64_t> depth6_set = std::move(cur_set);

        // Depth 7: 2M bucket from Depth 6
        expand_depth_partial(6, 7, depth6_set, index_pairs[6], nullptr,
                             BUCKET_2M, TARGET_NODES_2M);

        // Depth 8: 2M bucket with Depth 6 backtrace check
        tsl::robin_set<uint64_t> depth7_set;
        depth7_set.max_load_factor(LOAD_FACTOR);
        depth7_set.insert(index_pairs[7].begin(), index_pairs[7].end());
        expand_depth_partial(7, 8, depth7_set, index_pairs[7], &depth6_set,
                             BUCKET_2M, TARGET_NODES_2M);

        {
            tsl::robin_set<uint64_t> temp;
            depth6_set.swap(temp);
        }

        // Depth 9: 2M bucket with Depth 7 backtrace check
        tsl::robin_set<uint64_t> depth8_set;
        depth8_set.max_load_factor(LOAD_FACTOR);
        depth8_set.insert(index_pairs[8].begin(), index_pairs[8].end());
        expand_depth_partial(8, 9, depth8_set, index_pairs[8], &depth7_set,
                             BUCKET_2M, TARGET_NODES_2M);

        {
            tsl::robin_set<uint64_t> temp;
            depth7_set.swap(temp);
        }

        // Depth 10: 2M bucket with Depth 8 backtrace check
        tsl::robin_set<uint64_t> depth9_set;
        depth9_set.max_load_factor(LOAD_FACTOR);
        depth9_set.insert(index_pairs[9].begin(), index_pairs[9].end());
        expand_depth_partial(9, 10, depth9_set, index_pairs[9], &depth8_set,
                             BUCKET_2M, TARGET_NODES_2M);

        {
            tsl::robin_set<uint64_t> temp;
            depth8_set.swap(temp);
        }

        // Depth 11: 2M bucket with Depth 9 backtrace check
        tsl::robin_set<uint64_t> depth10_set;
        depth10_set.max_load_factor(LOAD_FACTOR);
        depth10_set.insert(index_pairs[10].begin(), index_pairs[10].end());
        expand_depth_partial(10, 11, depth10_set, index_pairs[10], &depth9_set,
                             BUCKET_2M, TARGET_NODES_2M);

        {
            tsl::robin_set<uint64_t> temp;
            depth9_set.swap(temp);
        }

        // Depth 12: 2M bucket with Depth 10 backtrace check
        tsl::robin_set<uint64_t> depth11_set;
        depth11_set.max_load_factor(LOAD_FACTOR);
        depth11_set.insert(index_pairs[11].begin(), index_pairs[11].end());
        expand_depth_partial(11, 12, depth11_set, index_pairs[11], &depth10_set,
                             BUCKET_2M, TARGET_NODES_2M);

        {
            tsl::robin_set<uint64_t> temp;
            depth10_set.swap(temp);
        }

        // Depth 13: 1M bucket with Depth 11 backtrace check
        tsl::robin_set<uint64_t> depth12_set;
        depth12_set.max_load_factor(LOAD_FACTOR);
        depth12_set.insert(index_pairs[12].begin(), index_pairs[12].end());
        expand_depth_partial(12, 13, depth12_set, index_pairs[12], &depth11_set,
                             BUCKET_1M, TARGET_NODES_1M);

        {
            tsl::robin_set<uint64_t> temp;
            depth11_set.swap(temp);
        }

        // Depth 14: 1M bucket with Depth 12 backtrace check
        tsl::robin_set<uint64_t> depth13_set;
        depth13_set.max_load_factor(LOAD_FACTOR);
        depth13_set.insert(index_pairs[13].begin(), index_pairs[13].end());
        expand_depth_partial(13, 14, depth13_set, index_pairs[13], &depth12_set,
                             BUCKET_1M, TARGET_NODES_1M);

        {
            tsl::robin_set<uint64_t> temp;
            depth12_set.swap(temp);
        }
        {
            tsl::robin_set<uint64_t> temp;
            depth13_set.swap(temp);
        }

        std::cout << "XXXCross Database Construction Complete (Depths 0-14)." << std::endl;
    }

    bool depth_limited_search_xcross(int arg_4e, int arg_e1, int arg_c1,
                                     int depth, int prev)
    {
        for (int i = 0; i < 18; ++i)
        {
            if (ma[prev + i])
            {
                continue;
            }

            int next_4e = multi_move_table_4e[arg_4e + i];

            // Prune check 1: Cross 4E + BL Corner (c1)
            int next_c1 = corner_move_table[arg_c1 + i];
            int p1 = prune_table_4e_c1[next_4e * SIZE_C + next_c1];
            int h1 = (p1 == 255) ? 11 : p1;
            if (h1 >= depth)
            {
                continue;
            }

            // Prune check 2: Cross 4E + BL Edge (e1)
            int next_e1 = edge_move_table[arg_e1 + i];
            int pe1 = prune_table_4e_e1[next_4e * SIZE_E + next_e1];
            int he1 = (pe1 == 255) ? 11 : pe1;
            if (he1 >= depth)
            {
                continue;
            }

            sol.emplace_back(i);

            if (depth == 1)
            {
                if (next_4e == goal_4e && next_e1 == GOAL_E1 && next_c1 == GOAL_C1)
                {
                    tmp = AlgToString(sol);
                    return true;
                }
            }
            else if (depth_limited_search_xcross(next_4e * 18, next_e1 * 18, next_c1 * 18,
                                                depth - 1, i * 18))
            {
                return true;
            }

            sol.pop_back();
        }
        return false;
    }

    std::string start_search_xcross(const std::string &arg_scramble, int max_depth = 10)
    {
        sol.clear();
        std::vector<int> alg = StringToAlg(arg_scramble);

        int idx_4e = goal_4e;
        int e1 = GOAL_E1;
        int c1 = GOAL_C1;

        // Trace state from solved origin under the incoming scramble
        for (int move : alg)
        {
            idx_4e = multi_move_table_4e[idx_4e * 18 + move];
            e1 = edge_move_table[e1 * 18 + move];
            c1 = corner_move_table[c1 * 18 + move];
        }

        if (idx_4e == goal_4e && e1 == GOAL_E1 && c1 == GOAL_C1)
        {
            return "";
        }

        int p1 = prune_table_4e_c1[idx_4e * SIZE_C + c1];
        int pe1 = prune_table_4e_e1[idx_4e * SIZE_E + e1];
        int d_min = std::max((p1 == 255 ? 1 : p1), (pe1 == 255 ? 1 : pe1));
        if (d_min == 0)
        {
            d_min = 1;
        }

        for (int d = d_min; d <= max_depth; ++d)
        {
            if (depth_limited_search_xcross(idx_4e * 18, e1 * 18, c1 * 18,
                                           d, 18 * 18))
            {
                return tmp;
            }
        }
        return "";
    }

    bool depth_limited_search(int arg_4e, int arg_3e, int arg_c1, int arg_c2, int arg_c3,
                              int depth, int prev)
    {
        for (int i = 0; i < 18; ++i)
        {
            if (ma[prev + i])
            {
                continue;
            }

            int next_4e = multi_move_table_4e[arg_4e + i];

            // 1. Dual prune check: Cross 4E + Corner 1 (DLB)
            int next_c1 = corner_move_table[arg_c1 + i];
            int p1 = prune_table_4e_c1[next_4e * SIZE_C + next_c1];
            int h1 = (p1 == 255) ? 11 : p1;
            if (h1 >= depth)
            {
                continue;
            }

            // 2. Dual prune check: Cross 4E + Corner 2 (DBR)
            int next_c2 = corner_move_table[arg_c2 + i];
            int p2 = prune_table_4e_c2[next_4e * SIZE_C + next_c2];
            int h2 = (p2 == 255) ? 11 : p2;
            if (h2 >= depth)
            {
                continue;
            }

            // 3. Dual prune check: Cross 4E + Corner 3 (DFR)
            int next_c3 = corner_move_table[arg_c3 + i];
            int p3 = prune_table_4e_c3[next_4e * SIZE_C + next_c3];
            int h3 = (p3 == 255) ? 11 : p3;
            if (h3 >= depth)
            {
                continue;
            }

            // Look up 3 slot edges only after passing all 3 corner tests
            int next_3e = multi_move_table_3e[arg_3e + i];

            sol.emplace_back(i);

            if (depth == 1)
            {
                if (next_4e == goal_4e && next_3e == goal_3e &&
                    next_c1 == GOAL_C1 && next_c2 == GOAL_C2 && next_c3 == GOAL_C3)
                {
                    tmp = AlgToString(sol);
                    return true;
                }
            }
            else if (depth_limited_search(next_4e * 18, next_3e * 18,
                                          next_c1 * 18, next_c2 * 18, next_c3 * 18,
                                          depth - 1, i * 18))
            {
                return true;
            }

            sol.pop_back();
        }
        return false;
    }

    std::string start_search(const std::string &arg_scramble, int max_depth = 12)
    {
        // Stage 1: Solve XCross (Cross 4E + BL Slot)
        std::string xcross_sol = start_search_xcross(arg_scramble);

        // Concatenate incoming scramble with XCross solution
        std::string stage2_scramble = arg_scramble;
        if (!xcross_sol.empty())
        {
            if (!stage2_scramble.empty() && stage2_scramble.back() != ' ')
            {
                stage2_scramble += " ";
            }
            stage2_scramble += xcross_sol;
        }

        // Stage 2: Solve remaining 2 slots towards the complete XXXCross
        sol.clear();
        std::vector<int> alg = StringToAlg(stage2_scramble);

        int idx_4e = goal_4e;
        int idx_3e = goal_3e;
        int c1 = GOAL_C1;
        int c2 = GOAL_C2;
        int c3 = GOAL_C3;

        // Apply concatenated sequence to solved base state
        for (int move : alg)
        {
            idx_4e = multi_move_table_4e[idx_4e * 18 + move];
            idx_3e = multi_move_table_3e[idx_3e * 18 + move];
            c1 = corner_move_table[c1 * 18 + move];
            c2 = corner_move_table[c2 * 18 + move];
            c3 = corner_move_table[c3 * 18 + move];
        }

        if (idx_4e == goal_4e && idx_3e == goal_3e &&
            c1 == GOAL_C1 && c2 == GOAL_C2 && c3 == GOAL_C3)
        {
            return xcross_sol;
        }

        int p1 = prune_table_4e_c1[idx_4e * SIZE_C + c1];
        int p2 = prune_table_4e_c2[idx_4e * SIZE_C + c2];
        int p3 = prune_table_4e_c3[idx_4e * SIZE_C + c3];

        int d_min = std::max({(p1 == 255 ? 1 : p1),
                              (p2 == 255 ? 1 : p2),
                              (p3 == 255 ? 1 : p3)});
        if (d_min == 0)
        {
            d_min = 1;
        }

        for (int d = d_min; d <= max_depth; ++d)
        {
            if (depth_limited_search(idx_4e * 18, idx_3e * 18,
                                     c1 * 18, c2 * 18, c3 * 18,
                                     d, 18 * 18))
            {
                if (xcross_sol.empty())
                {
                    return tmp;
                }
                return xcross_sol + " " + tmp;
            }
        }

        return xcross_sol;
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

        int idx_4e = goal_4e;
        int idx_3e = goal_3e;
        int c1 = GOAL_C1;
        int c2 = GOAL_C2;
        int c3 = GOAL_C3;
        int prev = 18;

        for (int step = 0; step < len; ++step)
        {
            std::vector<int> candidate_moves;
            candidate_moves.reserve(18);

            int p1 = prune_table_4e_c1[idx_4e * SIZE_C + c1];
            int p2 = prune_table_4e_c2[idx_4e * SIZE_C + c2];
            int p3 = prune_table_4e_c3[idx_4e * SIZE_C + c3];
            int cur_dist = std::max({(p1 == 255 ? 11 : p1),
                                     (p2 == 255 ? 11 : p2),
                                     (p3 == 255 ? 11 : p3)});

            for (int m = 0; m < 18; ++m)
            {
                if (ma[prev * 18 + m])
                {
                    continue;
                }

                int next_4e = multi_move_table_4e[idx_4e * 18 + m];
                int next_c1 = corner_move_table[c1 * 18 + m];
                int next_c2 = corner_move_table[c2 * 18 + m];
                int next_c3 = corner_move_table[c3 * 18 + m];

                int np1 = prune_table_4e_c1[next_4e * SIZE_C + next_c1];
                int np2 = prune_table_4e_c2[next_4e * SIZE_C + next_c2];
                int np3 = prune_table_4e_c3[next_4e * SIZE_C + next_c3];
                int next_dist = std::max({(np1 == 255 ? 11 : np1),
                                          (np2 == 255 ? 11 : np2),
                                          (np3 == 255 ? 11 : np3)});

                // Suppress backtracking towards the goal in early steps
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
            idx_4e = multi_move_table_4e[idx_4e * 18 + chosen_move];
            idx_3e = multi_move_table_3e[idx_3e * 18 + chosen_move];
            c1 = corner_move_table[c1 * 18 + chosen_move];
            c2 = corner_move_table[c2 * 18 + chosen_move];
            c3 = corner_move_table[c3 * 18 + chosen_move];
            prev = chosen_move;
        }

        return walk;
    }

    std::string get_xxxcross_scramble(int len)
    {
        if (len <= 0 || len >= static_cast<int>(index_pairs.size()) || index_pairs[len].empty())
        {
            return "";
        }

        const int max_attempts = (len >= 11) ? 60 : (len >= 10 ? 120 : 300);
        std::uniform_int_distribution<size_t> dist(0, index_pairs[len].size() - 1);

        std::vector<int> corners_arr(3);

        for (int attempt = 0; attempt < max_attempts; ++attempt)
        {
            sol.clear();
            uint64_t node = index_pairs[len][dist(generator)];

            int idx_4e = static_cast<int>(node / SIZE_3E_3C);
            uint64_t rem = node % SIZE_3E_3C;
            int idx_3e = static_cast<int>(rem / SIZE_3C);
            int idx_3c = static_cast<int>(rem % SIZE_3C);

            index_to_array(corners_arr, idx_3c, 3, 3, 8);

           // corners_arr contains already 18-multiplied strides (0 .. 23 * 18)
            int c1_stride = corners_arr[0];
            int c2_stride = corners_arr[1];
            int c3_stride = corners_arr[2];

            int c1 = c1_stride / 18;
            int c2 = c2_stride / 18;
            int c3 = c3_stride / 18; 

            int p1 = prune_table_4e_c1[idx_4e * SIZE_C + c1];
            int p2 = prune_table_4e_c2[idx_4e * SIZE_C + c2];
            int p3 = prune_table_4e_c3[idx_4e * SIZE_C + c3];

            int d_min = std::max({(p1 == 255 ? 1 : p1),
                                  (p2 == 255 ? 1 : p2),
                                  (p3 == 255 ? 1 : p3)});
            if (d_min == 0)
            {
                d_min = 1;
            }

            int actual_depth = -1;
            for (int d = d_min; d <= len; ++d)
            {
                // Pass c1_stride, c2_stride, c3_stride directly (do NOT multiply by 18 again)
                if (depth_limited_search(idx_4e * 18, idx_3e * 18,
                                         c1_stride, c2_stride, c3_stride,
                                         d, 18 * 18))
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
                     const std::string &arg_slot = "FL")
    {
        int len = std::stoi(arg_length);

        // 1. Solve incoming random scramble R to fully solved origin state O
        std::string sol_str = start_search(arg_scramble);

        // 2. Sample candidate state from precomputed database
        std::string w_str = get_xxxcross_scramble(len);

        // 3. Fallback: trial random walk if database sampling yields nothing
        if (w_str.empty())
        {
            const int max_trials = 50;

            for (int trial = 0; trial < max_trials; ++trial)
            {
                std::vector<int> candidate_walk = generate_raw_walk(len);
                if (candidate_walk.empty())
                {
                    continue;
                }

                // Simulate walk from origin to locate terminal state
                int cur_4e = goal_4e;
                int cur_3e = goal_3e;
                int cur_c1 = GOAL_C1;
                int cur_c2 = GOAL_C2;
                int cur_c3 = GOAL_C3;

                for (int m : candidate_walk)
                {
                    cur_4e = multi_move_table_4e[cur_4e * 18 + m];
                    cur_3e = multi_move_table_3e[cur_3e * 18 + m];
                    cur_c1 = corner_move_table[cur_c1 * 18 + m];
                    cur_c2 = corner_move_table[cur_c2 * 18 + m];
                    cur_c3 = corner_move_table[cur_c3 * 18 + m];
                }

                int p1 = prune_table_4e_c1[cur_4e * SIZE_C + cur_c1];
                int p2 = prune_table_4e_c2[cur_4e * SIZE_C + cur_c2];
                int p3 = prune_table_4e_c3[cur_4e * SIZE_C + cur_c3];

                int d_min = std::max({(p1 == 255 ? 1 : p1),
                                      (p2 == 255 ? 1 : p2),
                                      (p3 == 255 ? 1 : p3)});
                if (d_min == 0)
                {
                    d_min = 1;
                }

                int actual_depth = -1;
                sol.clear();

                for (int d = d_min; d <= len; ++d)
                {
                    if (depth_limited_search(cur_4e * 18, cur_3e * 18,
                                             cur_c1 * 18, cur_c2 * 18, cur_c3 * 18,
                                             d, 18 * 18))
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
EMSCRIPTEN_BINDINGS(xxxcross_trainer_module)
{
    emscripten::class_<xxxcross_search>("xxxcross_search")
        .constructor<>()
        .function("func", &xxxcross_search::func);
}
#endif

#ifndef __EMSCRIPTEN__
#include <chrono>
#include <iomanip>

int main()
{
    std::cout << "==========================================" << std::endl;
    std::cout << "  XXXCross Full BFS Node Distribution Test" << std::endl;
    std::cout << "  Target: Max Depth 6 (Safety capped)" << std::endl;
    std::cout << "==========================================" << std::endl;

    auto t_start = std::chrono::high_resolution_clock::now();

    std::cout << "Generating base move tables..." << std::endl;
    std::vector<int> edge_move_table = create_edge_move_table(); 
    std::vector<int> corner_move_table = create_corner_move_table(); 

    std::vector<int> multi_move_table_4e;
    std::vector<int> multi_move_table_3e;
    std::vector<int> multi_move_table_3c;

    static constexpr uint64_t SIZE_4E = 190080ULL; 
    static constexpr uint64_t SIZE_3E = 10560ULL;
    static constexpr uint64_t SIZE_3C = 9072ULL;
    static constexpr uint64_t SIZE_3E_3C = SIZE_3E * SIZE_3C;

    create_multi_move_table(4, 2, 12, SIZE_4E, edge_move_table, multi_move_table_4e); 
    create_multi_move_table(3, 2, 12, SIZE_3E, edge_move_table, multi_move_table_3e); 
    create_multi_move_table(3, 3, 8, SIZE_3C, corner_move_table, multi_move_table_3c); 

    std::vector<int> cross_edges = {8 * 2 + 0, 9 * 2 + 0, 10 * 2 + 0, 11 * 2 + 0}; 
    int goal_4e = array_to_index(cross_edges, 4, 2, 12); 

    std::vector<int> slot_edges = {0 * 2 + 0, 1 * 2 + 0, 2 * 2 + 0};
    int goal_3e = array_to_index(slot_edges, 3, 2, 12); 

    std::vector<int> slot_corners = {4 * 3 + 0, 5 * 3 + 0, 6 * 3 + 0};
    int goal_3c = array_to_index(slot_corners, 3, 3, 8); 

    uint64_t goal_node = static_cast<uint64_t>(goal_4e) * SIZE_3E_3C +
                         static_cast<uint64_t>(goal_3e) * SIZE_3C +
                         goal_3c;

    std::cout << "Tables generated. Starting BFS from Origin..." << std::endl;

    tsl::robin_set<uint64_t> prev_set, cur_set, next_set;
    prev_set.max_load_factor(0.90f); 
    cur_set.max_load_factor(0.90f); 
    next_set.max_load_factor(0.90f); 

    cur_set.insert(goal_node);

    std::vector<size_t> counts = {1};
    std::cout << "Depth 0: 1 nodes (Initial)" << std::endl;

    static constexpr size_t SAFETY_NODE_LIMIT = 12000000ULL;
    static constexpr int MAX_TEST_DEPTH = 6; 

    for (int d = 1; d <= MAX_TEST_DEPTH; ++d)
    {
        auto d_start = std::chrono::high_resolution_clock::now();

        size_t prev_count = cur_set.size();
        size_t predicted_next = static_cast<size_t>(prev_count * 13.0);

        if (d == 6 && predicted_next > SAFETY_NODE_LIMIT)
        {
            std::cout << "\n[Safety Guard] Predicted nodes for Depth " << d
                      << " (~" << (predicted_next / 1000000.0) << "M) exceeds limit ("
                      << (SAFETY_NODE_LIMIT / 1000000.0) << "M)." << std::endl;
            std::cout << "*** Aborting before Depth " << d << " to maintain safe memory limit ***" << std::endl;
            break;
        }

        next_set.clear();

        for (uint64_t node : cur_set) 
        {
            int idx_4e = static_cast<int>(node / SIZE_3E_3C);
            uint64_t rem = node % SIZE_3E_3C;
            int idx_3e = static_cast<int>(rem / SIZE_3C);
            int idx_3c = static_cast<int>(rem % SIZE_3C);

            int stride_4e = idx_4e * 18;
            int stride_3e = idx_3e * 18;
            int stride_3c = idx_3c * 18;

            for (int move = 0; move < 18; ++move)
            {
                int next_4e = multi_move_table_4e[stride_4e + move];
                int next_3e = multi_move_table_3e[stride_3e + move];
                int next_3c = multi_move_table_3c[stride_3c + move];

                uint64_t next_node = static_cast<uint64_t>(next_4e) * SIZE_3E_3C +
                                     static_cast<uint64_t>(next_3e) * SIZE_3C +
                                     next_3c;

                if (cur_set.find(next_node) == cur_set.end() &&
                    prev_set.find(next_node) == prev_set.end()) 
                {
                    next_set.insert(next_node); 
                }
            }

            if (next_set.size() > SAFETY_NODE_LIMIT)
            {
                std::cout << "\n[Safety Guard] Hit node cap during Depth " << d << " expansion!" << std::endl;
                break;
            }
        }

        auto d_end = std::chrono::high_resolution_clock::now();
        double d_sec = std::chrono::duration<double>(d_end - d_start).count();

        size_t current_count = next_set.size();
        counts.push_back(current_count);

        double factor = (prev_count > 0) ? (static_cast<double>(current_count) / prev_count) : 0.0;
        double est_mem_mb = (current_count * 24) / (1024.0 * 1024.0);

        std::cout << "Depth " << d << ": " << current_count << " nodes "
                  << "(Factor: " << std::fixed << std::setprecision(2) << factor
                  << ", Time: " << d_sec << "s"
                  << ", Set Mem: ~" << est_mem_mb << " MB)" << std::endl;

        prev_set = std::move(cur_set);
        cur_set = std::move(next_set);
    }

    auto t_end = std::chrono::high_resolution_clock::now();
    double total_sec = std::chrono::duration<double>(t_end - t_start).count();

    std::cout << "==========================================" << std::endl;
    std::cout << "Test Finished in " << total_sec << "s" << std::endl;
    std::cout << "==========================================" << std::endl;

    return 0;
}
#endif
