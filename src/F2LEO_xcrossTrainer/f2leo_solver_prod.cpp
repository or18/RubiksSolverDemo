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

    State apply_move_edge(State move, int e)
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

std::vector<int> sorted(5);
std::vector<std::vector<int>> base_array2 = {{0}, {0}, {12, 11, 10, 9, 8}, {8, 7, 6, 5, 4}};

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

std::vector<bool> create_ma_table()
{
    std::vector<bool> ma;
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

// =========================================================================
// Constants & Sizing for F2LEO XCross (Target Slot: BL)
// =========================================================================
static constexpr uint64_t SIZE_5E = 3041280ULL; // Cross 4 edges + BL edge (5 edges: P(12,5)*2^5)
static constexpr uint64_t SIZE_4E = 190080ULL;  // Middle 4 edges (P(12,4)*2^4)
static constexpr int SIZE_1C = 24;              // DBL corner (8 pos * 3 ori)


struct f2leo_xcross_search
{
    std::vector<int> sol;
    std::string scramble;
    std::string tmp;
    std::mt19937_64 generator;

    // Movement tables
    std::vector<int> edge_move_table;      // 24 * 18
    std::vector<int> corner_move_table;    // 24 * 18
    std::vector<int> multi_move_table_5e;  // SIZE_5E * 18 (Cross 4 + BL edge)
    std::vector<int> multi_move_table_4e;  // SIZE_4E * 18 (Middle 4 edges)

    // Pruning tables (Lower-bounds for IDA*)
    std::vector<unsigned char> prune_table_5e;        // SIZE_5E (Cross + BL edge)
    std::vector<unsigned char> prune_table_middle;    // SIZE_4E (Middle edges EO bound)
    std::vector<unsigned char> prune_table_cross_c;   // SIZE_4E * 24 (Cross 4 + DBL corner)

    // Database & Pruning helpers
    std::vector<std::vector<uint64_t>> index_pairs;
    std::vector<int> num_list;
    std::vector<bool> ma;

    // Goal states (BL slotting target)
    int goal_5e;      // DF, DL, DB, DR + BL
    int goal_middle;  // FR, FL, BL, BR
    int goal_corner;  // DBL (position 4, orientation 0 -> index 4*3+0 = 12)

    f2leo_xcross_search()
    {
        std::random_device rd;
        generator.seed(rd());

        // 1. Primitive single-piece move tables
        edge_move_table = create_edge_move_table();
        corner_move_table = create_corner_move_table();

        // 2. Multi-piece permutation/orientation move tables
        create_multi_move_table(5, 2, 12, SIZE_5E, edge_move_table, multi_move_table_5e);
        create_multi_move_table(4, 2, 12, SIZE_4E, edge_move_table, multi_move_table_4e);

        // 3. Goal state definitions
        // Cross edges: DF=8(16), DL=9(18), DB=10(20), DR=11(22), BL edge: BL=2(4)
        std::vector<int> cg5 = {16, 18, 20, 22, 4};
        goal_5e = array_to_index(cg5, 5, 2, 12);

        // Middle edges: FR=0(0), FL=1(2), BL=2(4), BR=3(6)
        std::vector<int> mg = {0, 2, 4, 6};
        goal_middle = array_to_index(mg, 4, 2, 12);

        // DBL corner: pos=4, ori=0
        goal_corner = 4 * 3 + 0;

        // 4. Heuristic Pruning tables
        create_prune_table(goal_5e, SIZE_5E, 10, multi_move_table_5e, prune_table_5e);
        create_prune_table(goal_middle, SIZE_4E, 10, multi_move_table_4e, prune_table_middle);

        ma = create_ma_table();

        // 5. Database generation
        build_database();
    }

    void build_database()
    {
        index_pairs.resize(12);
        num_list.resize(12, 0);

        // Composite goal key: ((5e * 24 + 1c) * SIZE_4E + 4e)
        uint64_t goal_composite = ((static_cast<uint64_t>(goal_5e) * 24 + goal_corner) * SIZE_4E) + goal_middle;
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
                uint64_t rem = node / SIZE_4E;
                int m_idx = static_cast<int>(node % SIZE_4E);
                int c_idx = static_cast<int>(rem % 24);
                int e5_idx = static_cast<int>(rem / 24);

                int e5_stride = e5_idx * 18;
                int c_stride = c_idx * 18;
                int m_stride = m_idx * 18;

                for (int move = 0; move < 18; ++move)
                {
                    int next_e5 = multi_move_table_5e[e5_stride + move];
                    int next_c = corner_move_table[c_stride + move];
                    int next_m = multi_move_table_4e[m_stride + move];

                    uint64_t next_node = ((static_cast<uint64_t>(next_e5) * 24 + next_c) * SIZE_4E) + next_m;

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

        // Release prev_set
        {
            tsl::robin_set<uint64_t> temp;
            prev_set.swap(temp);
        }

        std::cout << "\n=== Phase 2: Local Expansion (2M / 2M / 1M / 1M / 1M) ===" << std::endl;

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

        // Depth 9: 1M bucket with Depth 7 backtrace check
        tsl::robin_set<uint64_t> depth8_set;
        depth8_set.max_load_factor(LOAD_FACTOR);
        depth8_set.insert(index_pairs[8].begin(), index_pairs[8].end());
        expand_depth_partial(8, 9, depth8_set, index_pairs[8], &depth7_set, BUCKET_1M, TARGET_NODES_1M);

        // Release depth7_set
        {
            tsl::robin_set<uint64_t> temp;
            depth7_set.swap(temp);
        }

        // Depth 10: 1M bucket with Depth 8 backtrace check
        tsl::robin_set<uint64_t> depth9_set;
        depth9_set.max_load_factor(LOAD_FACTOR);
        depth9_set.insert(index_pairs[9].begin(), index_pairs[9].end());
        expand_depth_partial(9, 10, depth9_set, index_pairs[9], &depth8_set, BUCKET_1M, TARGET_NODES_1M);

        // Release depth8_set
        {
            tsl::robin_set<uint64_t> temp;
            depth8_set.swap(temp);
        }

        // Depth 11: 1M bucket with Depth 9 backtrace check (Frontier / Rare)
        tsl::robin_set<uint64_t> depth10_set;
        depth10_set.max_load_factor(LOAD_FACTOR);
        depth10_set.insert(index_pairs[10].begin(), index_pairs[10].end());
        expand_depth_partial(10, 11, depth10_set, index_pairs[10], &depth9_set, BUCKET_1M, TARGET_NODES_1M);

        // Release depth9_set and depth10_set
        {
            tsl::robin_set<uint64_t> temp;
            depth9_set.swap(temp);
        }
        {
            tsl::robin_set<uint64_t> temp;
            depth10_set.swap(temp);
        }

        std::cout << "\n=== Complete Database Summary ===" << std::endl;
        for (int d = 0; d <= 11; ++d)
        {
            std::cout << "  depth=" << d << ": " << index_pairs[d].size() << " nodes" << std::endl;
        }
    }

    void expand_depth_partial(int parent_d, int next_d,
                             const tsl::robin_set<uint64_t>& parent_set,
                             const std::vector<uint64_t>& parent_vec,
                             const tsl::robin_set<uint64_t>* grandparent_set,
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

            uint64_t rem = p_node / SIZE_4E;
            int m_idx = static_cast<int>(p_node % SIZE_4E);
            int c_idx = static_cast<int>(rem % 24);
            int e5_idx = static_cast<int>(rem / 24);

            int move = move_dist(generator);

            int next_e5 = multi_move_table_5e[e5_idx * 18 + move];
            int next_c = corner_move_table[c_idx * 18 + move];
            int next_m = multi_move_table_4e[m_idx * 18 + move];

            // Heuristic lower-bound check: prune if 5-edge distance itself is too small
            int p_5e = prune_table_5e[next_e5];
            if (p_5e != 255 && p_5e < parent_d - 1)
            {
                continue;
            }

            uint64_t next_node = ((static_cast<uint64_t>(next_e5) * 24 + next_c) * SIZE_4E) + next_m;

            if (parent_set.find(next_node) != parent_set.end() ||
                next_set.find(next_node) != next_set.end())
            {
                continue;
            }

            // Grandparent backtrace check: prevent stepping backward to depth - 2
            if (grandparent_set != nullptr)
            {
                bool back_to_grandparent = false;
                int b_e5_stride = next_e5 * 18;
                int b_c_stride = next_c * 18;
                int b_m_stride = next_m * 18;

                for (int b_move = 0; b_move < 18; ++b_move)
                {
                    int prev_e5 = multi_move_table_5e[b_e5_stride + b_move];
                    int prev_c = corner_move_table[b_c_stride + b_move];
                    int prev_m = multi_move_table_4e[b_m_stride + b_move];

                    uint64_t prev_node = ((static_cast<uint64_t>(prev_e5) * 24 + prev_c) * SIZE_4E) + prev_m;

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

    // Invert numerical algorithm sequence
    std::vector<int> invert_alg(const std::vector<int>& alg)
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

    // Invert string algorithm
    std::string invert_alg_string(const std::string& str)
    {
        std::vector<int> alg = StringToAlg(str);
        std::vector<int> inv = invert_alg(alg);
        return AlgToString(inv);
    }

    // -------------------------------------------------------------------------
    // Solver: F2LEO XCross condition
    // (Cross 4 edges + BL edge + DBL corner solved, remaining 3 middle edges EO solved)
    // -------------------------------------------------------------------------
    bool depth_limited_search_f2leo_xcross(int arg_5e, int arg_c, 
                                           int eo_fr, int eo_fl, int eo_br, 
                                           int depth, int prev)
    {
        for (int i = 0; i < 18; ++i)
        {
            if (ma[prev + i])
            {
                continue;
            }

            int next_5e = multi_move_table_5e[arg_5e + i];

            // Pruning lower-bound via 5-edge table
            int p_5e = prune_table_5e[next_5e];
            if (p_5e >= depth)
            {
                continue;
            }

            int next_c  = corner_move_table[arg_c + i];
            int next_fr = edge_move_table[eo_fr + i];
            int next_fl = edge_move_table[eo_fl + i];
            int next_br = edge_move_table[eo_br + i];

            sol.emplace_back(i);

            if (depth == 1)
            {
                if (next_5e == goal_5e && next_c == goal_corner &&
                    (next_fr % 2 == 0) && (next_fl % 2 == 0) && (next_br % 2 == 0))
                {
                    tmp = AlgToString(sol);
                    return true;
                }
            }
            else if (depth_limited_search_f2leo_xcross(next_5e * 18, next_c * 18,
                                                       next_fr * 18, next_fl * 18, next_br * 18,
                                                       depth - 1, i * 18))
            {
                return true;
            }

            sol.pop_back();
        }
        return false;
    }

    // -------------------------------------------------------------------------
    // Strictly evaluate the true minimal F2LEO XCross depth
    // -------------------------------------------------------------------------
    int evaluate_scramble_depth(const std::vector<int>& scramble_alg, int target_len)
    {
        int cur_5e = goal_5e;
        int cur_c  = goal_corner;

        // Middle edge initial states (FR=0, FL=2, BR=6)
        int cur_fr = 0;
        int cur_fl = 2;
        int cur_br = 6;

        for (int m : scramble_alg)
        {
            cur_5e = multi_move_table_5e[cur_5e * 18 + m];
            cur_c  = corner_move_table[cur_c * 18 + m];
            cur_fr = edge_move_table[cur_fr * 18 + m];
            cur_fl = edge_move_table[cur_fl * 18 + m];
            cur_br = edge_move_table[cur_br * 18 + m];
        }

        if (cur_5e == goal_5e && cur_c == goal_corner &&
            (cur_fr % 2 == 0) && (cur_fl % 2 == 0) && (cur_br % 2 == 0))
        {
            return 0;
        }

        int d_min = prune_table_5e[cur_5e];
        if (d_min == 0) d_min = 1;

        if (d_min > target_len)
        {
            return target_len + 1;
        }

        for (int d = d_min; d <= target_len; ++d)
        {
            sol.clear();
            if (depth_limited_search_f2leo_xcross(cur_5e * 18, cur_c * 18,
                                                  cur_fr * 18, cur_fl * 18, cur_br * 18,
                                                  d, 324))
            {
                return d;
            }
        }

        return target_len + 1;
    }

    // -------------------------------------------------------------------------
    // Fast Candidate Generator: Single walk guided by 5-edge distance
    // -------------------------------------------------------------------------
    std::vector<int> generate_raw_walk(int len)
    {
        std::vector<int> walk;
        walk.reserve(len);

        int cur_5e = goal_5e;
        int prev = 18;

        for (int step = 0; step < len; ++step)
        {
            std::vector<int> candidate_moves;
            candidate_moves.reserve(18);
            int current_dist = prune_table_5e[cur_5e];

            for (int m = 0; m < 18; ++m)
            {
                if (ma[prev * 18 + m]) continue;

                int next_5e = multi_move_table_5e[cur_5e * 18 + m];
                int next_dist = prune_table_5e[next_5e];

                // Guide away from origin during the early steps
                if (step < 7 && next_dist < current_dist) continue;
                candidate_moves.push_back(m);
            }

            if (candidate_moves.empty())
            {
                return {};
            }

            std::uniform_int_distribution<size_t> c_dist(0, candidate_moves.size() - 1);
            int chosen_move = candidate_moves[c_dist(generator)];

            walk.push_back(chosen_move);
            cur_5e = multi_move_table_5e[cur_5e * 18 + chosen_move];
            prev = chosen_move;
        }

        return walk;
    }


    // -------------------------------------------------------------------------
    // Origin Solver: Solves state fully to Origin O
    // (5 edges: Cross+BL, 4 middle edges, DBL corner)
    // -------------------------------------------------------------------------
    bool depth_limited_search_origin(int arg_5e, int arg_m, int arg_c, int depth, int prev)
    {
        for (int i = 0; i < 18; ++i)
        {
            if (ma[prev + i])
            {
                continue;
            }

            int next_5e = multi_move_table_5e[arg_5e + i];
            int next_m  = multi_move_table_4e[arg_m + i];
            int next_c  = corner_move_table[arg_c + i];

            int p_5e = prune_table_5e[next_5e];
            int p_m  = prune_table_middle[next_m];

            int h = std::max((p_5e == 255 ? 0 : p_5e), (p_m == 255 ? 0 : p_m));
            if (h >= depth)
            {
                continue;
            }

            sol.emplace_back(i);

            if (depth == 1)
            {
                if (next_5e == goal_5e && next_m == goal_middle && next_c == goal_corner)
                {
                    tmp = AlgToString(sol);
                    return true;
                }
            }
            else if (depth_limited_search_origin(next_5e * 18, next_m * 18, next_c * 18, depth - 1, i * 18))
            {
                return true;
            }

            sol.pop_back();
        }
        return false;
    }

    // Solve incoming random scramble R completely to Origin O
    std::string start_search_origin(const std::string& arg_scramble)
    {
        sol.clear();
        std::vector<int> alg = StringToAlg(arg_scramble);

        int cur_5e = goal_5e;
        int cur_m  = goal_middle;
        int cur_c  = goal_corner;

        for (int move : alg)
        {
            cur_5e = multi_move_table_5e[cur_5e * 18 + move];
            cur_m  = multi_move_table_4e[cur_m * 18 + move];
            cur_c  = corner_move_table[cur_c * 18 + move];
        }

        if (cur_5e == goal_5e && cur_m == goal_middle && cur_c == goal_corner)
        {
            return "";
        }

        int p_5e = prune_table_5e[cur_5e];
        int p_m  = prune_table_middle[cur_m];
        int d_min = std::max((p_5e == 255 ? 1 : p_5e), (p_m == 255 ? 1 : p_m));
        if (d_min == 0) d_min = 1;

        // Origin O requires solving corner as well, search up to depth 14-15
        for (int d = d_min; d <= 15; ++d)
        {
            if (depth_limited_search_origin(cur_5e * 18, cur_m * 18, cur_c * 18, d, 324))
            {
                return tmp;
            }
        }
        return "";
    }

    // -------------------------------------------------------------------------
    // Single-Loop Production Generator for F2LEO XCross (BL Slot)
    // Directly checks final composed cube state to guarantee exact depth
    // -------------------------------------------------------------------------
    std::string func(std::string arg_scramble = "", std::string arg_length = "7")
    {
        int len = std::stoi(arg_length);

        // 1. Solve incoming random scramble R strictly to Origin O
        std::string sol_str = start_search_origin(arg_scramble);

        std::vector<int> r_alg = StringToAlg(arg_scramble);
        std::vector<int> s_alg = StringToAlg(sol_str);

        // Base sequence: R * S (strictly at Origin O)
        std::vector<int> rs_alg = r_alg;
        rs_alg.insert(rs_alg.end(), s_alg.begin(), s_alg.end());

        // Search budget: Expand trials for high depths (9, 10, 11)
        const int max_trials = 100;
        std::vector<int> chosen_walk;

        for (int trial = 0; trial < max_trials; ++trial)
        {
            std::vector<int> candidate_walk = generate_raw_walk(len);
            if (candidate_walk.empty()) continue;

            // HTML applies: mapped_seq based on ret[1] + reverse(ret[0])
            // Inverted by min2phase, the actual scramble applied to the cube is:
            // X = (candidate_walk * (R * S)^-1)^-1 = (R * S) * candidate_walk^-1
            std::vector<int> inv_w = invert_alg(candidate_walk);

            std::vector<int> test_composed = rs_alg;
            test_composed.insert(test_composed.end(), inv_w.begin(), inv_w.end());

            // Directly evaluate true minimal F2LEO XCross depth on composed state
            int composed_depth = evaluate_scramble_depth(test_composed, len);

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
EMSCRIPTEN_BINDINGS(f2leo_xcross_trainer_module)
{
    emscripten::class_<f2leo_xcross_search>("f2leo_xcross_search")
        .constructor<>()
        .function("func", &f2leo_xcross_search::func);
}
#endif
