#include <emscripten/bind.h>
#include <emscripten.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <string>
#include <sstream>
#include <cstdlib>

EM_JS(void, update, (const char *str), {
    postMessage(UTF8ToString(str));
});

struct State
{
    std::vector<int> cp;
    std::vector<int> co;
    std::vector<int> ep;
    std::vector<int> eo;

    State(std::vector<int> arg_cp = {0, 1, 2, 3, 4, 5, 6, 7}, std::vector<int> arg_co = {0, 0, 0, 0, 0, 0, 0, 0}, std::vector<int> arg_ep = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}, std::vector<int> arg_eo = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}) : cp(arg_cp), co(arg_co), ep(arg_ep), eo(arg_eo) {}

    State apply_move(State move)
    {
        std::vector<int> new_cp;
        std::vector<int> new_co;
        std::vector<int> new_ep;
        std::vector<int> new_eo;
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

    State apply_move_corner(State move, int c)
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

std::string AlgToString(std::vector<int> alg)
{
    std::string result = "";
    for (int i : alg)
    {
        result += move_names[i] + " ";
    }
    return result;
}

std::vector<int> StringToAlg(std::string str)
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

std::vector<int> AlgConvertRotation(std::vector<int> alg, std::string rotation)
{
    if (rotation.empty())
    {
        return alg;
    }
    std::vector<int> face_list_0 = {0, 1, 2, 3, 4, 5};
    std::vector<int> face_list;
    if (rotation == "x")
    {
        face_list = {5, 4, 2, 3, 0, 1};
    }
    else if (rotation == "x2")
    {
        face_list = {1, 0, 2, 3, 5, 4};
    }
    else if (rotation == "x'")
    {
        face_list = {4, 5, 2, 3, 1, 0};
    }
    else if (rotation == "y")
    {
        face_list = {0, 1, 5, 4, 2, 3};
    }
    else if (rotation == "y2")
    {
        face_list = {0, 1, 3, 2, 5, 4};
    }
    else if (rotation == "y'")
    {
        face_list = {0, 1, 4, 5, 3, 2};
    }
    else if (rotation == "z")
    {
        face_list = {3, 2, 0, 1, 4, 5};
    }
    else if (rotation == "z2")
    {
        face_list = {1, 0, 3, 2, 4, 5};
    }
    else if (rotation == "z'")
    {
        face_list = {2, 3, 1, 0, 4, 5};
    }
    for (size_t i = 0; i < alg.size(); ++i)
    {
        alg[i] = 3 * face_list[alg[i] / 3] + alg[i] % 3;
    }
    return alg;
}

std::vector<int> AlgRotation(std::vector<int> alg, std::string rotation_algString)
{
    std::istringstream iss(rotation_algString);
    std::string rot;
    while (iss >> rot)
    {
        alg = AlgConvertRotation(alg, rot);
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

std::vector<std::vector<int>> c_array2 = {{0}, {0}, {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048}, {1, 3, 9, 27, 81, 243, 729, 2187}};

inline int o_to_index(std::vector<int> &o, int c, int pn)
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
            move_table[18 * i + j] = 18 * o_to_index(new_state.eo, 2, 12);
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

std::vector<int> create_cp_move_table()
{
    std::vector<int> move_table(8 * 18, 0);
    for (int i = 0; i < 8; ++i)
    {
        std::vector<int> ep(12, 0);
        std::vector<int> eo(12, 0);
        std::vector<int> cp(8, -1);
        std::vector<int> co(8, -1);
        cp[i] = i;
        co[i] = 0;
        State state(cp, co, ep, eo);
        for (int j = 0; j < 18; ++j)
        {
            State new_state = state.apply_move_corner(moves[move_names[j]], i);
            auto it = std::find(new_state.cp.begin(), new_state.cp.end(), i);
            move_table[18 * i + j] = std::distance(new_state.cp.begin(), it);
        }
    }
    return move_table;
}

std::vector<int> create_co_move_table()
{
    std::vector<int> move_table(2187 * 18, 0);
    for (int i = 0; i < 2187; ++i)
    {
        std::vector<int> ep(12, 0);
        std::vector<int> eo(12, 0);
        std::vector<int> cp(8, 0);
        std::vector<int> co(8, 0);
        index_to_o(co, i, 3, 8);
        State state(cp, co, ep, eo);
        for (int j = 0; j < 18; ++j)
        {
            State new_state = state.apply_move(moves[move_names[j]]);
            move_table[18 * i + j] = 18 * o_to_index(new_state.co, 3, 8);
        }
    }
    return move_table;
}

std::vector<int> create_multi_move_table(int n, int c, int pn, int size, const std::vector<int> &table)
{
    std::vector<int> move_table(size * 18, -1);
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
                move_table[18 * tmp + inv_move[j]] = i;
            }
        }
    }
    return move_table;
}

std::vector<int> create_multi_move_table2(int n, int c, int pn, int size, const std::vector<int> &table)
{
    std::vector<int> move_table(size * 24, -1);
    int tmp;
    int tmp_i;
    std::vector<int> a(n);
    std::vector<int> b(n);
    std::vector<int> inv_move = {2, 1, 0, 5, 4, 3, 8, 7, 6, 11, 10, 9, 14, 13, 12, 17, 16, 15};
    for (int i = 0; i < size; ++i)
    {
        index_to_array(a, i, n, c, pn);
        tmp_i = i * 24;
        for (int j = 0; j < 18; ++j)
        {
            if (move_table[tmp_i + j] == -1)
            {
                for (int k = 0; k < n; ++k)
                {
                    b[k] = table[a[k] + j];
                }
                tmp = 24 * array_to_index(b, n, c, pn);
                move_table[tmp_i + j] = tmp;
                move_table[tmp + inv_move[j]] = tmp_i;
            }
        }
    }
    return move_table;
}

std::vector<int> create_prune_table(int index1, int index2, int size1, int size2, int depth, const std::vector<int> &table1, const std::vector<int> &table2)
{
    int size = size1 * size2;
    std::vector<int> prune_table(size, -1);
    int start = index1 * size2 + index2;
    int next_i;
    int index1_tmp;
    int index2_tmp;
    int next_d;
    prune_table[start] = 0;
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
                    prune_table[next_i] = prune_table[next_i] == -1 ? next_d : prune_table[next_i];
                }
            }
        }
    }
    return prune_table;
}

void create_prune_table2(int index1, int index2, int size1, int size2, int depth, const std::vector<int> &table1, const std::vector<int> &table2, std::vector<int> &prune_table)
{
    int size = size1 * size2;
    int start = index1 * size2 + index2;
    int next_i;
    int index1_tmp;
    int index2_tmp;
    int next_d;
    prune_table[start] = 0;
    index1_tmp = index1 * 24;
    index2_tmp = index2 * 18;
    for (int j = 0; j < 18; ++j)
    {
        next_i = table1[index1_tmp + j] + table2[index2_tmp + j];
        prune_table[next_i] = prune_table[next_i] == -1 ? 1 : prune_table[next_i];
    }
    for (int d = 1; d < depth; ++d)
    {
        next_d = d + 1;
        for (int i = 0; i < size; ++i)
        {
            if (prune_table[i] == d)
            {
                index1_tmp = (i / size2) * 24;
                index2_tmp = (i % size2) * 18;
                for (int j = 0; j < 18; ++j)
                {
                    next_i = table1[index1_tmp + j] + table2[index2_tmp + j];
                    prune_table[next_i] = prune_table[next_i] == -1 ? next_d : prune_table[next_i];
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

struct cross_search
{
    std::vector<int> sol;
    std::string scramble;
    std::string rotation;
    int max_length;
    int sol_num;
    int count;
    std::vector<int> edge_move_table;
    std::vector<int> multi_move_table;
    std::vector<int> prune_table;
    std::vector<int> alg;
    std::vector<std::string> restrict;
    std::vector<int> move_restrict;
    std::vector<bool> ma;
    int index1;
    int index2;
    int index1_tmp;
    int index2_tmp;
    int prune_tmp;
    std::string tmp;

    cross_search()
    {
        edge_move_table = create_edge_move_table();
        multi_move_table = create_multi_move_table(2, 2, 12, 24 * 22, edge_move_table);
        index1 = 416;
        index2 = 520;
        prune_table = create_prune_table(index1, index2, 24 * 22, 24 * 22, 9, multi_move_table, multi_move_table);
        ma = create_ma_table();
    }

    bool depth_limited_search(int arg_index1, int arg_index2, int depth, int prev)
    {
        for (int i : move_restrict)
        {
            if (ma[prev + i])
            {
                continue;
            }
            index1_tmp = multi_move_table[arg_index1 + i];
            index2_tmp = multi_move_table[arg_index2 + i];
            prune_tmp = prune_table[index1_tmp * 528 + index2_tmp];
            if (prune_tmp >= depth)
            {
                continue;
            }
            sol.emplace_back(i);
            if (depth == 1)
            {
                if (prune_tmp == 0)
                {
                    bool valid = true;
                    int l = static_cast<int>(sol.size());
                    int c = 0;
                    int index1_tmp2 = index1;
                    int index2_tmp2 = index2;
                    for (int j : sol)
                    {
                        if (index1_tmp2 == multi_move_table[index1_tmp2 + j] * 18 && index2_tmp2 == multi_move_table[index2_tmp2 + j] * 18)
                        {
                            valid = false;
                            break;
                        }
                        else
                        {
                            c += 1;
                            index1_tmp2 = multi_move_table[index1_tmp2 + j];
                            index2_tmp2 = multi_move_table[index2_tmp2 + j];
                            if (c < l && prune_table[index1_tmp2 * 528 + index2_tmp2] == 0)
                            {
                                valid = false;
                                break;
                            }
                            index1_tmp2 *= 18;
                            index2_tmp2 *= 18;
                        }
                    }
                    if (valid)
                    {
                        count += 1;
                        if (rotation == "")
                        {
                            tmp = AlgToString(sol);
                        }
                        else
                        {
                            tmp = rotation + " " + AlgToString(sol);
                        }
                        update(tmp.c_str());
                        if (count == sol_num)
                        {
                            return true;
                        }
                    }
                }
            }
            else if (depth_limited_search(index1_tmp * 18, index2_tmp * 18, depth - 1, i * 18))
            {
                return true;
            }
            sol.pop_back();
        }
        return false;
    }

    void start_search(std::string arg_scramble = "", std::string arg_rotation = "", int arg_sol_num = 100, int arg_max_length = 8, std::vector<std::string> arg_restrict = move_names)
    {
        scramble = arg_scramble;
        rotation = arg_rotation;
        max_length = arg_max_length;
        sol_num = arg_sol_num;
        restrict = arg_restrict;
        std::vector<int> alg = AlgRotation(StringToAlg(scramble), rotation);
        index1 = 416;
        index2 = 520;
        count = 0;
        for (int m : alg)
        {
            index1 = multi_move_table[index1 * 18 + m];
            index2 = multi_move_table[index2 * 18 + m];
        }
        for (std::string name : restrict)
        {
            auto it = std::find(move_names.begin(), move_names.end(), name);
            move_restrict.emplace_back(std::distance(move_names.begin(), it));
        }
        prune_tmp = prune_table[index1 * 528 + index2];
        if (prune_tmp == 0)
        {
            update("Already solved.");
        }
        else
        {
            index1 *= 18;
            index2 *= 18;
            for (int d = prune_tmp; d <= max_length; d++)
            {
                if (depth_limited_search(index1, index2, d, 324))
                {
                    break;
                }
            }
            update("Search finished.");
        }
    }
};

struct xcross_search
{
    std::vector<int> sol;
    std::string scramble;
    std::string rotation;
    int slot1;
    int max_length;
    int sol_num;
    int count;
    std::vector<int> edge_move_table;
    std::vector<int> corner_move_table;
    std::vector<int> multi_move_table;
    std::vector<int> prune_table1;
    std::vector<int> alg;
    std::vector<std::string> restrict;
    std::vector<int> move_restrict;
    std::vector<bool> ma;
    int edge_solved1;
    int index1;
    int index2;
    int index3;
    int index1_tmp;
    int index2_tmp;
    int index3_tmp;
    int prune1_tmp;
    std::string tmp;

    xcross_search()
    {
        edge_move_table = create_edge_move_table();
        corner_move_table = create_corner_move_table();
        multi_move_table = create_multi_move_table2(4, 2, 12, 24 * 22 * 20 * 18, edge_move_table);
        ma = create_ma_table();
        prune_table1 = std::vector<int>(24 * 22 * 20 * 18 * 24, -1);
    }

    bool depth_limited_search(int arg_index1, int arg_index2, int arg_index3, int depth, int prev)
    {
        for (int i : move_restrict)
        {
            if (ma[prev + i])
            {
                continue;
            }
            index1_tmp = multi_move_table[arg_index1 + i];
            index2_tmp = corner_move_table[arg_index2 + i];
            index3_tmp = edge_move_table[arg_index3 + i];
            prune1_tmp = prune_table1[index1_tmp + index2_tmp];
            if (prune1_tmp >= depth)
            {
                continue;
            }
            sol.emplace_back(i);
            if (depth == 1)
            {
                if (prune1_tmp == 0 && index3_tmp == edge_solved1)
                {
                    bool valid = true;
                    int l = static_cast<int>(sol.size());
                    int c = 0;
                    int index1_tmp2 = index1;
                    int index2_tmp2 = index2;
                    int index3_tmp2 = index3;
                    for (int j : sol)
                    {
                        if (index1_tmp2 == multi_move_table[index1_tmp2 + j] && index2_tmp2 == corner_move_table[index2_tmp2 + j] * 18 && index3_tmp2 == edge_move_table[index3_tmp2 + j] * 18)
                        {
                            valid = false;
                            break;
                        }
                        else
                        {
                            c += 1;
                            index1_tmp2 = multi_move_table[index1_tmp2 + j];
                            index2_tmp2 = corner_move_table[index2_tmp2 + j];
                            index3_tmp2 = edge_move_table[index3_tmp2 + j];
                            if (c < l && (prune_table1[index1_tmp2 + index2_tmp2] == 0 && index3_tmp2 == edge_solved1))
                            {
                                valid = false;
                                break;
                            }
                            index2_tmp2 *= 18;
                            index3_tmp2 *= 18;
                        }
                    }
                    if (valid)
                    {
                        count += 1;
                        if (rotation == "")
                        {
                            tmp = AlgToString(sol);
                        }
                        else
                        {
                            tmp = rotation + " " + AlgToString(sol);
                        }
                        update(tmp.c_str());
                        if (count == sol_num)
                        {
                            return true;
                        }
                    }
                }
            }
            else if (depth_limited_search(index1_tmp, index2_tmp * 18, index3_tmp * 18, depth - 1, i * 18))
            {
                return true;
            }
            sol.pop_back();
        }
        return false;
    }

    void start_search(std::string arg_scramble = "", std::string arg_rotation = "", int arg_slot1 = 0, int arg_sol_num = 100, int arg_max_length = 10, std::vector<std::string> arg_restrict = move_names)
    {
        scramble = arg_scramble;
        rotation = arg_rotation;
        slot1 = arg_slot1;
        max_length = arg_max_length;
        sol_num = arg_sol_num;
        restrict = arg_restrict;
        std::vector<int> alg = AlgRotation(StringToAlg(scramble), rotation);
        std::vector<int> edge_index = {187520, 187520, 187520, 187520};
        std::vector<int> single_edge_index = {0, 2, 4, 6};
        std::vector<int> corner_index = {12, 15, 18, 21};
        index1 = edge_index[slot1];
        index2 = corner_index[slot1];
        index3 = single_edge_index[slot1];
        edge_solved1 = index3;
        create_prune_table2(index1, index2, 24 * 22 * 20 * 18, 24, 9, multi_move_table, corner_move_table, prune_table1);
        count = 0;
        for (std::string name : restrict)
        {
            auto it = std::find(move_names.begin(), move_names.end(), name);
            move_restrict.emplace_back(std::distance(move_names.begin(), it));
        }
        index1 *= 24;
        for (int m : alg)
        {
            index1 = multi_move_table[index1 + m];
            index2 = corner_move_table[index2 * 18 + m];
            index3 = edge_move_table[index3 * 18 + m];
        }
        prune1_tmp = prune_table1[index1 + index2];
        if (prune1_tmp == 0 && index3 == edge_solved1)
        {
            update("Already solved.");
        }
        else
        {
            index2 *= 18;
            index3 *= 18;
            for (int d = prune1_tmp; d <= max_length; d++)
            {
                if (depth_limited_search(index1, index2, index3, d, 324))
                {
                    break;
                }
            }
            update("Search finished.");
        }
    }
};

struct xxcross_search
{
    std::vector<int> sol;
    std::string scramble;
    std::string rotation;
    int slot1;
    int slot2;
    int max_length;
    int sol_num;
    int count;
    std::vector<int> edge_move_table;
    std::vector<int> corner_move_table;
    std::vector<int> multi_move_table;
    std::vector<int> prune_table1;
    std::vector<int> prune_table2;
    std::vector<int> alg;
    std::vector<std::string> restrict;
    std::vector<int> move_restrict;
    std::vector<bool> ma;
    int edge_solved1;
    int edge_solved2;
    int index1;
    int index2;
    int index3;
    int index4;
    int index5;
    int index6;
    int index1_tmp;
    int index2_tmp;
    int index3_tmp;
    int index4_tmp;
    int index5_tmp;
    int index6_tmp;
    int prune1_tmp;
    int prune2_tmp;
    std::string tmp;

    xxcross_search()
    {
        edge_move_table = create_edge_move_table();
        corner_move_table = create_corner_move_table();
        multi_move_table = create_multi_move_table2(4, 2, 12, 24 * 22 * 20 * 18, edge_move_table);
        ma = create_ma_table();
        prune_table1 = std::vector<int>(24 * 22 * 20 * 18 * 24, -1);
        prune_table2 = std::vector<int>(24 * 22 * 20 * 18 * 24, -1);
    }

    bool depth_limited_search(int arg_index1, int arg_index2, int arg_index3, int arg_index4, int arg_index5, int arg_index6, int depth, int prev)
    {
        for (int i : move_restrict)
        {
            if (ma[prev + i])
            {
                continue;
            }
            index1_tmp = multi_move_table[arg_index1 + i];
            index2_tmp = corner_move_table[arg_index2 + i];
            index5_tmp = edge_move_table[arg_index5 + i];
            prune1_tmp = prune_table1[index1_tmp + index2_tmp];
            if (prune1_tmp >= depth)
            {
                continue;
            }
            index3_tmp = multi_move_table[arg_index3 + i];
            index4_tmp = corner_move_table[arg_index4 + i];
            index6_tmp = edge_move_table[arg_index6 + i];
            prune2_tmp = prune_table2[index3_tmp + index4_tmp];
            if (prune2_tmp >= depth)
            {
                continue;
            }
            sol.emplace_back(i);
            if (depth == 1)
            {
                if (prune1_tmp == 0 && prune2_tmp == 0 && index5_tmp == edge_solved1 && index6_tmp == edge_solved2)
                {
                    bool valid = true;
                    int l = static_cast<int>(sol.size());
                    int c = 0;
                    int index1_tmp2 = index1;
                    int index2_tmp2 = index2;
                    int index3_tmp2 = index3;
                    int index4_tmp2 = index4;
                    int index5_tmp2 = index5;
                    int index6_tmp2 = index6;
                    for (int j : sol)
                    {
                        if (index1_tmp2 == multi_move_table[index1_tmp2 + j] && index2_tmp2 == corner_move_table[index2_tmp2 + j] * 18 && index3_tmp2 == multi_move_table[index3_tmp2 + j] && index4_tmp2 == corner_move_table[index4_tmp2 + j] * 18 && index5_tmp2 == edge_move_table[index5_tmp2 + j] * 18 && index6_tmp2 == edge_move_table[index6_tmp2 + j] * 18)
                        {
                            valid = false;
                            break;
                        }
                        else
                        {
                            c += 1;
                            index1_tmp2 = multi_move_table[index1_tmp2 + j];
                            index2_tmp2 = corner_move_table[index2_tmp2 + j];
                            index3_tmp2 = multi_move_table[index3_tmp2 + j];
                            index4_tmp2 = corner_move_table[index4_tmp2 + j];
                            index5_tmp2 = edge_move_table[index5_tmp2 + j];
                            index6_tmp2 = edge_move_table[index6_tmp2 + j];
                            if (c < l && (prune_table1[index1_tmp2 + index2_tmp2] == 0 && prune_table2[index3_tmp2 + index4_tmp2] == 0 && index5_tmp2 == edge_solved1 && index6_tmp2 == edge_solved2))
                            {
                                valid = false;
                                break;
                            }
                            index2_tmp2 *= 18;
                            index4_tmp2 *= 18;
                            index5_tmp2 *= 18;
                            index6_tmp2 *= 18;
                        }
                    }
                    if (valid)
                    {
                        count += 1;
                        if (rotation == "")
                        {
                            tmp = AlgToString(sol);
                        }
                        else
                        {
                            tmp = rotation + " " + AlgToString(sol);
                        }
                        update(tmp.c_str());
                        if (count == sol_num)
                        {
                            return true;
                        }
                    }
                }
            }
            else if (depth_limited_search(index1_tmp, index2_tmp * 18, index3_tmp, index4_tmp * 18, index5_tmp * 18, index6_tmp * 18, depth - 1, i * 18))
            {
                return true;
            }
            sol.pop_back();
        }
        return false;
    }

    void start_search(std::string arg_scramble = "", std::string arg_rotation = "", int arg_slot1 = 0, int arg_slot2 = 3, int arg_sol_num = 100, int arg_max_length = 12, std::vector<std::string> arg_restrict = move_names)
    {
        scramble = arg_scramble;
        rotation = arg_rotation;
        slot1 = arg_slot1;
        slot2 = arg_slot2;
        max_length = arg_max_length;
        sol_num = arg_sol_num;
        restrict = arg_restrict;
        std::vector<int> alg = AlgRotation(StringToAlg(scramble), rotation);
        std::vector<int> edge_index = {187520, 187520, 187520, 187520};
        std::vector<int> single_edge_index = {0, 2, 4, 6};
        std::vector<int> corner_index = {12, 15, 18, 21};
        index1 = edge_index[slot1];
        index2 = corner_index[slot1];
        index5 = single_edge_index[slot1];
        edge_solved1 = index5;
        create_prune_table2(index1, index2, 24 * 22 * 20 * 18, 24, 9, multi_move_table, corner_move_table, prune_table1);
        index3 = edge_index[slot2];
        index4 = corner_index[slot2];
        index6 = single_edge_index[slot2];
        edge_solved2 = index6;
        create_prune_table2(index3, index4, 24 * 22 * 20 * 18, 24, 9, multi_move_table, corner_move_table, prune_table2);
        count = 0;
        for (std::string name : restrict)
        {
            auto it = std::find(move_names.begin(), move_names.end(), name);
            move_restrict.emplace_back(std::distance(move_names.begin(), it));
        }
        index1 *= 24;
        index3 *= 24;
        for (int m : alg)
        {
            index1 = multi_move_table[index1 + m];
            index2 = corner_move_table[index2 * 18 + m];
            index3 = multi_move_table[index3 + m];
            index4 = corner_move_table[index4 * 18 + m];
            index5 = edge_move_table[index5 * 18 + m];
            index6 = edge_move_table[index6 * 18 + m];
        }
        prune1_tmp = prune_table1[index1 + index2];
        prune2_tmp = prune_table2[index3 + index4];
        if (prune1_tmp == 0 && prune2_tmp == 0 && index5 == edge_solved1 && index6 == edge_solved2)
        {
            update("Already solved.");
        }
        else
        {
            index2 *= 18;
            index4 *= 18;
            index5 *= 18;
            index6 *= 18;
            for (int d = std::max(prune1_tmp, prune2_tmp); d <= max_length; d++)
            {
                if (depth_limited_search(index1, index2, index3, index4, index5, index6, d, 324))
                {
                    break;
                }
            }
            update("Search finished.");
        }
    }
};

struct xxxcross_search
{
    std::vector<int> sol;
    std::string scramble;
    std::string rotation;
    int slot1;
    int slot2;
    int slot3;
    int max_length;
    int sol_num;
    int count;
    std::vector<int> edge_move_table;
    std::vector<int> corner_move_table;
    std::vector<int> multi_move_table;
    std::vector<int> prune_table1;
    std::vector<int> prune_table2;
    std::vector<int> prune_table3;
    std::vector<int> alg;
    std::vector<std::string> restrict;
    std::vector<int> move_restrict;
    std::vector<bool> ma;
    int edge_solved1;
    int edge_solved2;
    int edge_solved3;
    int index1;
    int index2;
    int index3;
    int index4;
    int index5;
    int index6;
    int index7;
    int index8;
    int index9;
    int index1_tmp;
    int index2_tmp;
    int index3_tmp;
    int index4_tmp;
    int index5_tmp;
    int index6_tmp;
    int index7_tmp;
    int index8_tmp;
    int index9_tmp;
    int prune1_tmp;
    int prune2_tmp;
    int prune3_tmp;
    std::string tmp;

    xxxcross_search()
    {
        edge_move_table = create_edge_move_table();
        corner_move_table = create_corner_move_table();
        multi_move_table = create_multi_move_table2(4, 2, 12, 24 * 22 * 20 * 18, edge_move_table);
        ma = create_ma_table();
        prune_table1 = std::vector<int>(24 * 22 * 20 * 18 * 24, -1);
        prune_table2 = std::vector<int>(24 * 22 * 20 * 18 * 24, -1);
        prune_table3 = std::vector<int>(24 * 22 * 20 * 18 * 24, -1);
    }

    bool depth_limited_search(int arg_index1, int arg_index2, int arg_index3, int arg_index4, int arg_index5, int arg_index6, int arg_index7, int arg_index8, int arg_index9, int depth, int prev)
    {
        for (int i : move_restrict)
        {
            if (ma[prev + i])
            {
                continue;
            }
            index1_tmp = multi_move_table[arg_index1 + i];
            index2_tmp = corner_move_table[arg_index2 + i];
            index7_tmp = edge_move_table[arg_index7 + i];
            prune1_tmp = prune_table1[index1_tmp + index2_tmp];
            if (prune1_tmp >= depth)
            {
                continue;
            }
            index3_tmp = multi_move_table[arg_index3 + i];
            index4_tmp = corner_move_table[arg_index4 + i];
            index8_tmp = edge_move_table[arg_index8 + i];
            prune2_tmp = prune_table2[index3_tmp + index4_tmp];
            if (prune2_tmp >= depth)
            {
                continue;
            }
            index5_tmp = multi_move_table[arg_index5 + i];
            index6_tmp = corner_move_table[arg_index6 + i];
            index9_tmp = edge_move_table[arg_index9 + i];
            prune3_tmp = prune_table3[index5_tmp + index6_tmp];
            if (prune3_tmp >= depth)
            {
                continue;
            }
            sol.emplace_back(i);
            if (depth == 1)
            {
                if (prune1_tmp == 0 && prune2_tmp == 0 && prune3_tmp == 0 && index7_tmp == edge_solved1 && index8_tmp == edge_solved2 && index9_tmp == edge_solved3)
                {
                    bool valid = true;
                    int l = static_cast<int>(sol.size());
                    int c = 0;
                    int index1_tmp2 = index1;
                    int index2_tmp2 = index2;
                    int index3_tmp2 = index3;
                    int index4_tmp2 = index4;
                    int index5_tmp2 = index5;
                    int index6_tmp2 = index6;
                    int index7_tmp2 = index7;
                    int index8_tmp2 = index8;
                    int index9_tmp2 = index9;
                    for (int j : sol)
                    {
                        if (index1_tmp2 == multi_move_table[index1_tmp2 + j] && index2_tmp2 == corner_move_table[index2_tmp2 + j] * 18 && index3_tmp2 == multi_move_table[index3_tmp2 + j] && index4_tmp2 == corner_move_table[index4_tmp2 + j] * 18 && index5_tmp2 == multi_move_table[index5_tmp2 + j] && index6_tmp2 == corner_move_table[index6_tmp2 + j] * 18 && index7_tmp2 == edge_move_table[index7_tmp2 + j] * 18 && index8_tmp2 == edge_move_table[index8_tmp2 + j] * 18 && index9_tmp2 == edge_move_table[index9_tmp2 + j] * 18)
                        {
                            valid = false;
                            break;
                        }
                        else
                        {
                            c += 1;
                            index1_tmp2 = multi_move_table[index1_tmp2 + j];
                            index2_tmp2 = corner_move_table[index2_tmp2 + j];
                            index3_tmp2 = multi_move_table[index3_tmp2 + j];
                            index4_tmp2 = corner_move_table[index4_tmp2 + j];
                            index5_tmp2 = multi_move_table[index5_tmp2 + j];
                            index6_tmp2 = corner_move_table[index6_tmp2 + j];
                            index7_tmp2 = edge_move_table[index7_tmp2 + j];
                            index8_tmp2 = edge_move_table[index8_tmp2 + j];
                            index9_tmp2 = edge_move_table[index9_tmp2 + j];
                            if (c < l && (prune_table1[index1_tmp2 + index2_tmp2] == 0 && prune_table2[index3_tmp2 + index4_tmp2] == 0 && prune_table3[index5_tmp2 + index6_tmp2] == 0 && index7_tmp2 == edge_solved1 && index8_tmp2 == edge_solved2 && index9_tmp2 == edge_solved3))
                            {
                                valid = false;
                                break;
                            }
                            index2_tmp2 *= 18;
                            index4_tmp2 *= 18;
                            index6_tmp2 *= 18;
                            index7_tmp2 *= 18;
                            index8_tmp2 *= 18;
                            index9_tmp2 *= 18;
                        }
                    }
                    if (valid)
                    {
                        count += 1;
                        if (rotation == "")
                        {
                            tmp = AlgToString(sol);
                        }
                        else
                        {
                            tmp = rotation + " " + AlgToString(sol);
                        }
                        update(tmp.c_str());
                        if (count == sol_num)
                        {
                            return true;
                        }
                    }
                }
            }
            else if (depth_limited_search(index1_tmp, index2_tmp * 18, index3_tmp, index4_tmp * 18, index5_tmp, index6_tmp * 18, index7_tmp * 18, index8_tmp * 18, index9_tmp * 18, depth - 1, i * 18))
            {
                return true;
            }
            sol.pop_back();
        }
        return false;
    }

    void start_search(std::string arg_scramble = "", std::string arg_rotation = "", int arg_slot1 = 0, int arg_slot2 = 3, int arg_slot3 = 1, int arg_sol_num = 100, int arg_max_length = 14, std::vector<std::string> arg_restrict = move_names)
    {
        scramble = arg_scramble;
        rotation = arg_rotation;
        slot1 = arg_slot1;
        slot2 = arg_slot2;
        slot3 = arg_slot3;
        max_length = arg_max_length;
        sol_num = arg_sol_num;
        restrict = arg_restrict;
        std::vector<int> alg = AlgRotation(StringToAlg(scramble), rotation);
        std::vector<int> edge_index = {187520, 187520, 187520, 187520};
        std::vector<int> single_edge_index = {0, 2, 4, 6};
        std::vector<int> corner_index = {12, 15, 18, 21};
        index1 = edge_index[slot1];
        index2 = corner_index[slot1];
        index7 = single_edge_index[slot1];
        edge_solved1 = index7;
        create_prune_table2(index1, index2, 24 * 22 * 20 * 18, 24, 9, multi_move_table, corner_move_table, prune_table1);
        index3 = edge_index[slot2];
        index4 = corner_index[slot2];
        index8 = single_edge_index[slot2];
        edge_solved2 = index8;
        create_prune_table2(index3, index4, 24 * 22 * 20 * 18, 24, 9, multi_move_table, corner_move_table, prune_table2);
        index5 = edge_index[slot3];
        index6 = corner_index[slot3];
        index9 = single_edge_index[slot3];
        edge_solved3 = index9;
        create_prune_table2(index5, index6, 24 * 22 * 20 * 18, 24, 9, multi_move_table, corner_move_table, prune_table3);
        count = 0;
        for (std::string name : restrict)
        {
            auto it = std::find(move_names.begin(), move_names.end(), name);
            move_restrict.emplace_back(std::distance(move_names.begin(), it));
        }
        index1 *= 24;
        index3 *= 24;
        index5 *= 24;
        for (int m : alg)
        {
            index1 = multi_move_table[index1 + m];
            index2 = corner_move_table[index2 * 18 + m];
            index3 = multi_move_table[index3 + m];
            index4 = corner_move_table[index4 * 18 + m];
            index5 = multi_move_table[index5 + m];
            index6 = corner_move_table[index6 * 18 + m];
            index7 = edge_move_table[index7 * 18 + m];
            index8 = edge_move_table[index8 * 18 + m];
            index9 = edge_move_table[index9 * 18 + m];
        }
        prune1_tmp = prune_table1[index1 + index2];
        prune2_tmp = prune_table2[index3 + index4];
        prune3_tmp = prune_table3[index5 + index6];
        if (prune1_tmp == 0 && prune2_tmp == 0 && prune3_tmp == 0 && index7 == edge_solved1 && index8 == edge_solved2 && index9 == edge_solved3)
        {
            update("Already solved.");
        }
        else
        {
            index2 *= 18;
            index4 *= 18;
            index6 *= 18;
            index7 *= 18;
            index8 *= 18;
            index9 *= 18;
            for (int d = std::max(prune1_tmp, std::max(prune2_tmp, prune3_tmp)); d <= max_length; d++)
            {
                if (depth_limited_search(index1, index2, index3, index4, index5, index6, index7, index8, index9, d, 324))
                {
                    break;
                }
            }
            update("Search finished.");
        }
    }
};

struct xxxxcross_search
{
    std::vector<int> sol;
    std::string scramble;
    std::string rotation;
    int max_length;
    int sol_num;
    int count;
    std::vector<int> edge_move_table;
    std::vector<int> corner_move_table;
    std::vector<int> multi_move_table;
    std::vector<int> prune_table1;
    std::vector<int> prune_table2;
    std::vector<int> prune_table3;
    std::vector<int> prune_table4;
    std::vector<int> alg;
    std::vector<std::string> restrict;
    std::vector<int> move_restrict;
    std::vector<bool> ma;
    int index1;
    int index2;
    int index3;
    int index4;
    int index5;
    int index6;
    int index7;
    int index8;
    int index9;
    int index10;
    int index11;
    int index12;
    int index1_tmp;
    int index2_tmp;
    int index3_tmp;
    int index4_tmp;
    int index5_tmp;
    int index6_tmp;
    int index7_tmp;
    int index8_tmp;
    int index9_tmp;
    int index10_tmp;
    int index11_tmp;
    int index12_tmp;
    int prune1_tmp;
    int prune2_tmp;
    int prune3_tmp;
    int prune4_tmp;
    std::string tmp;

    xxxxcross_search()
    {
        edge_move_table = create_edge_move_table();
        corner_move_table = create_corner_move_table();
        multi_move_table = create_multi_move_table2(4, 2, 12, 24 * 22 * 20 * 18, edge_move_table);
        ma = create_ma_table();
        prune_table1 = std::vector<int>(24 * 22 * 20 * 18 * 24, -1);
        prune_table2 = std::vector<int>(24 * 22 * 20 * 18 * 24, -1);
        prune_table3 = std::vector<int>(24 * 22 * 20 * 18 * 24, -1);
        prune_table4 = std::vector<int>(24 * 22 * 20 * 18 * 24, -1);
    }

    bool depth_limited_search(int arg_index1, int arg_index2, int arg_index3, int arg_index4, int arg_index5, int arg_index6, int arg_index7, int arg_index8, int arg_index9, int arg_index10, int arg_index11, int arg_index12, int depth, int prev)
    {
        for (int i : move_restrict)
        {
            if (ma[prev + i])
            {
                continue;
            }
            index1_tmp = multi_move_table[arg_index1 + i];
            index2_tmp = corner_move_table[arg_index2 + i];
            index9_tmp = edge_move_table[arg_index9 + i];
            prune1_tmp = prune_table1[index1_tmp + index2_tmp];
            if (prune1_tmp >= depth)
            {
                continue;
            }
            index3_tmp = multi_move_table[arg_index3 + i];
            index4_tmp = corner_move_table[arg_index4 + i];
            index10_tmp = edge_move_table[arg_index10 + i];
            prune2_tmp = prune_table2[index3_tmp + index4_tmp];
            if (prune2_tmp >= depth)
            {
                continue;
            }
            index5_tmp = multi_move_table[arg_index5 + i];
            index6_tmp = corner_move_table[arg_index6 + i];
            index11_tmp = edge_move_table[arg_index11 + i];
            prune3_tmp = prune_table3[index5_tmp + index6_tmp];
            if (prune3_tmp >= depth)
            {
                continue;
            }
            index7_tmp = multi_move_table[arg_index7 + i];
            index8_tmp = corner_move_table[arg_index8 + i];
            index12_tmp = edge_move_table[arg_index12 + i];
            prune4_tmp = prune_table4[index7_tmp + index8_tmp];
            if (prune4_tmp >= depth)
            {
                continue;
            }
            sol.emplace_back(i);
            if (depth == 1)
            {
                if (prune1_tmp == 0 && prune2_tmp == 0 && prune3_tmp == 0 && prune4_tmp == 0 && index9_tmp == 0 && index10_tmp == 2 && index11_tmp == 4 && index12_tmp == 6)
                {
                    bool valid = true;
                    int l = static_cast<int>(sol.size());
                    int c = 0;
                    int index1_tmp2 = index1;
                    int index2_tmp2 = index2;
                    int index3_tmp2 = index3;
                    int index4_tmp2 = index4;
                    int index5_tmp2 = index5;
                    int index6_tmp2 = index6;
                    int index7_tmp2 = index7;
                    int index8_tmp2 = index8;
                    int index9_tmp2 = index9;
                    int index10_tmp2 = index10;
                    int index11_tmp2 = index11;
                    int index12_tmp2 = index12;
                    for (int j : sol)
                    {
                        if (index1_tmp2 == multi_move_table[index1_tmp2 + j] && index2_tmp2 == corner_move_table[index2_tmp2 + j] * 18 && index3_tmp2 == multi_move_table[index3_tmp2 + j] && index4_tmp2 == corner_move_table[index4_tmp2 + j] * 18 && index5_tmp2 == multi_move_table[index5_tmp2 + j] && index6_tmp2 == corner_move_table[index6_tmp2 + j] * 18 && index7_tmp2 == multi_move_table[index7_tmp2 + j] && index8_tmp2 == corner_move_table[index8_tmp2 + j] * 18 && index9_tmp2 == edge_move_table[index9_tmp2 + j] * 18 && index10_tmp2 == edge_move_table[index10_tmp2 + j] * 18 && index11_tmp2 == edge_move_table[index11_tmp2 + j] * 18 && index12_tmp2 == edge_move_table[index12_tmp2 + j] * 18)
                        {
                            valid = false;
                            break;
                        }
                        else
                        {
                            c += 1;
                            index1_tmp2 = multi_move_table[index1_tmp2 + j];
                            index2_tmp2 = corner_move_table[index2_tmp2 + j];
                            index3_tmp2 = multi_move_table[index3_tmp2 + j];
                            index4_tmp2 = corner_move_table[index4_tmp2 + j];
                            index5_tmp2 = multi_move_table[index5_tmp2 + j];
                            index6_tmp2 = corner_move_table[index6_tmp2 + j];
                            index7_tmp2 = multi_move_table[index7_tmp2 + j];
                            index8_tmp2 = corner_move_table[index8_tmp2 + j];
                            index9_tmp2 = edge_move_table[index9_tmp2 + j];
                            index10_tmp2 = edge_move_table[index10_tmp2 + j];
                            index11_tmp2 = edge_move_table[index11_tmp2 + j];
                            index12_tmp2 = edge_move_table[index12_tmp2 + j];
                            if (c < l && (prune_table1[index1_tmp2 + index2_tmp2] == 0 && prune_table2[index3_tmp2 + index4_tmp2] == 0 && prune_table3[index5_tmp2 + index6_tmp2] == 0 && prune_table4[index7_tmp2 + index8_tmp2] == 0 && index9_tmp2 == 0 && index10_tmp2 == 2 && index11_tmp2 == 4 && index12_tmp2 == 6))
                            {
                                valid = false;
                                break;
                            }
                            index2_tmp2 *= 18;
                            index4_tmp2 *= 18;
                            index6_tmp2 *= 18;
                            index8_tmp2 *= 18;
                            index9_tmp2 *= 18;
                            index10_tmp2 *= 18;
                            index11_tmp2 *= 18;
                            index12_tmp2 *= 18;
                        }
                    }
                    if (valid)
                    {
                        count += 1;
                        if (rotation == "")
                        {
                            tmp = AlgToString(sol);
                        }
                        else
                        {
                            tmp = rotation + " " + AlgToString(sol);
                        }
                        update(tmp.c_str());
                        if (count == sol_num)
                        {
                            return true;
                        }
                    }
                }
            }
            else if (depth_limited_search(index1_tmp, index2_tmp * 18, index3_tmp, index4_tmp * 18, index5_tmp, index6_tmp * 18, index7_tmp, index8_tmp * 18, index9_tmp * 18, index10_tmp * 18, index11_tmp * 18, index12_tmp * 18, depth - 1, i * 18))
            {
                return true;
            }
            sol.pop_back();
        }
        return false;
    }

    void start_search(std::string arg_scramble = "", std::string arg_rotation = "", int arg_sol_num = 100, int arg_max_length = 16, std::vector<std::string> arg_restrict = move_names)
    {
        scramble = arg_scramble;
        rotation = arg_rotation;
        max_length = arg_max_length;
        sol_num = arg_sol_num;
        restrict = arg_restrict;
        std::vector<int> alg = AlgRotation(StringToAlg(scramble), rotation);
        std::vector<int> edge_index = {187520, 187520, 187520, 187520};
        std::vector<int> single_edge_index = {0, 2, 4, 6};
        std::vector<int> corner_index = {12, 15, 18, 21};
        index1 = edge_index[0];
        index2 = corner_index[0];
        index9 = single_edge_index[0];
        create_prune_table2(index1, index2, 24 * 22 * 20 * 18, 24, 9, multi_move_table, corner_move_table, prune_table1);
        index3 = edge_index[1];
        index4 = corner_index[1];
        index10 = single_edge_index[1];
        create_prune_table2(index3, index4, 24 * 22 * 20 * 18, 24, 9, multi_move_table, corner_move_table, prune_table2);
        index5 = edge_index[2];
        index6 = corner_index[2];
        index11 = single_edge_index[2];
        create_prune_table2(index5, index6, 24 * 22 * 20 * 18, 24, 9, multi_move_table, corner_move_table, prune_table3);
        index7 = edge_index[3];
        index8 = corner_index[3];
        index12 = single_edge_index[3];
        create_prune_table2(index7, index8, 24 * 22 * 20 * 18, 24, 9, multi_move_table, corner_move_table, prune_table4);
        count = 0;
        for (std::string name : restrict)
        {
            auto it = std::find(move_names.begin(), move_names.end(), name);
            move_restrict.emplace_back(std::distance(move_names.begin(), it));
        }
        index1 *= 24;
        index3 *= 24;
        index5 *= 24;
        index7 *= 24;
        for (int m : alg)
        {
            index1 = multi_move_table[index1 + m];
            index2 = corner_move_table[index2 * 18 + m];
            index3 = multi_move_table[index3 + m];
            index4 = corner_move_table[index4 * 18 + m];
            index5 = multi_move_table[index5 + m];
            index6 = corner_move_table[index6 * 18 + m];
            index7 = multi_move_table[index7 + m];
            index8 = corner_move_table[index8 * 18 + m];
            index9 = edge_move_table[index9 * 18 + m];
            index10 = edge_move_table[index10 * 18 + m];
            index11 = edge_move_table[index11 * 18 + m];
            index12 = edge_move_table[index12 * 18 + m];
        }
        prune1_tmp = prune_table1[index1 + index2];
        prune2_tmp = prune_table2[index3 + index4];
        prune3_tmp = prune_table3[index5 + index6];
        prune4_tmp = prune_table4[index7 + index8];
        if (prune1_tmp == 0 && prune2_tmp == 0 && prune3_tmp == 0 && prune4_tmp == 0 && index9 == 0 && index10 == 2 && index11 == 4 && index12 == 6)
        {
            update("Already solved.");
        }
        else
        {
            index2 *= 18;
            index4 *= 18;
            index6 *= 18;
            index8 *= 18;
            index9 *= 18;
            index10 *= 18;
            index11 *= 18;
            index12 *= 18;
            for (int d = std::max(prune1_tmp, std::max(prune2_tmp, std::max(prune3_tmp, prune4_tmp))); d <= max_length; d++)
            {
                if (depth_limited_search(index1, index2, index3, index4, index5, index6, index7, index8, index9, index10, index11, index12, d, 324))
                {
                    break;
                }
            }
            update("Search finished.");
        }
    }
};

struct LL_substeps_search
{
    std::vector<int> sol;
    std::string scramble;
    std::string rotation;
    bool solve_cp;
    bool solve_co;
    bool solve_ep;
    bool solve_eo;
    int max_length;
    int sol_num;
    int count;
    std::vector<int> edge_move_table;
    std::vector<int> single_ep_move_table;
    std::vector<int> ep_move_table;
    std::vector<int> eo_move_table;
    std::vector<int> corner_move_table;
    std::vector<int> single_cp_move_table;
    std::vector<int> cp_move_table;
    std::vector<int> co_move_table;
    std::vector<int> multi_move_table;
    std::vector<int> prune_table1;
    std::vector<int> prune_table2;
    std::vector<int> prune_table3;
    std::vector<int> prune_table4;
    std::vector<int> alg;
    std::vector<std::string> restrict;
    std::vector<int> move_restrict;
    std::vector<bool> ma;
    int index1;
    int index2;
    int index3;
    int index4;
    int index5;
    int index6;
    int index7;
    int index8;
    int index9;
    int index10;
    int index11;
    int index12;
    int index_cp;
    int index_co;
    int index_ep;
    int index_eo;
    int index1_tmp;
    int index2_tmp;
    int index3_tmp;
    int index4_tmp;
    int index5_tmp;
    int index6_tmp;
    int index7_tmp;
    int index8_tmp;
    int index9_tmp;
    int index10_tmp;
    int index11_tmp;
    int index12_tmp;
    int index_cp_tmp;
    int index_co_tmp;
    int index_ep_tmp;
    int index_eo_tmp;
    int prune1_tmp;
    int prune2_tmp;
    int prune3_tmp;
    int prune4_tmp;
    std::string tmp;

    LL_substeps_search()
    {
        edge_move_table = create_edge_move_table();
        corner_move_table = create_corner_move_table();
        multi_move_table = create_multi_move_table2(4, 2, 12, 24 * 22 * 20 * 18, edge_move_table);
        ma = create_ma_table();
        prune_table1 = std::vector<int>(24 * 22 * 20 * 18 * 24, -1);
        prune_table2 = std::vector<int>(24 * 22 * 20 * 18 * 24, -1);
        prune_table3 = std::vector<int>(24 * 22 * 20 * 18 * 24, -1);
        prune_table4 = std::vector<int>(24 * 22 * 20 * 18 * 24, -1);
        single_cp_move_table = create_cp_move_table();
        cp_move_table = create_multi_move_table(4, 1, 8, 8 * 7 * 6 * 5, single_cp_move_table);
        co_move_table = create_co_move_table();
        for (int i = 0; i < 8 * 7 * 6 * 5 * 18; ++i)
        {
            cp_move_table[i] *= 18;
        }
        single_ep_move_table = create_ep_move_table();
        ep_move_table = create_multi_move_table(4, 1, 12, 12 * 11 * 10 * 9, single_ep_move_table);
        eo_move_table = create_eo_move_table();
        for (int i = 0; i < 12 * 11 * 10 * 9 * 18; ++i)
        {
            ep_move_table[i] *= 18;
        }
    }

    bool depth_limited_search(int arg_index1, int arg_index2, int arg_index3, int arg_index4, int arg_index5, int arg_index6, int arg_index7, int arg_index8, int arg_index9, int arg_index10, int arg_index11, int arg_index12, int arg_index_cp, int arg_index_co, int arg_index_ep, int arg_index_eo, int depth, int prev)
    {
        for (int i : move_restrict)
        {
            if (ma[prev + i])
            {
                continue;
            }
            index1_tmp = multi_move_table[arg_index1 + i];
            index2_tmp = corner_move_table[arg_index2 + i];
            index9_tmp = edge_move_table[arg_index9 + i];
            prune1_tmp = prune_table1[index1_tmp + index2_tmp];
            if (prune1_tmp >= depth)
            {
                continue;
            }
            index3_tmp = multi_move_table[arg_index3 + i];
            index4_tmp = corner_move_table[arg_index4 + i];
            index10_tmp = edge_move_table[arg_index10 + i];
            prune2_tmp = prune_table2[index3_tmp + index4_tmp];
            if (prune2_tmp >= depth)
            {
                continue;
            }
            index5_tmp = multi_move_table[arg_index5 + i];
            index6_tmp = corner_move_table[arg_index6 + i];
            index11_tmp = edge_move_table[arg_index11 + i];
            prune3_tmp = prune_table3[index5_tmp + index6_tmp];
            if (prune3_tmp >= depth)
            {
                continue;
            }
            index7_tmp = multi_move_table[arg_index7 + i];
            index8_tmp = corner_move_table[arg_index8 + i];
            index12_tmp = edge_move_table[arg_index12 + i];
            prune4_tmp = prune_table4[index7_tmp + index8_tmp];
            if (prune4_tmp >= depth)
            {
                continue;
            }
            index_cp_tmp = cp_move_table[arg_index_cp + i];
            index_co_tmp = co_move_table[arg_index_co + i];
            index_ep_tmp = ep_move_table[arg_index_ep + i];
            index_eo_tmp = eo_move_table[arg_index_eo + i];
            sol.emplace_back(i);
            if (depth == 1)
            {
                if (prune1_tmp == 0 && prune2_tmp == 0 && prune3_tmp == 0 && prune4_tmp == 0 && index9_tmp == 0 && index10_tmp == 2 && index11_tmp == 4 && index12_tmp == 6 && (solve_ep || (index_ep_tmp == 105480 || index_ep_tmp == 105534 || index_ep_tmp == 105948 || index_ep_tmp == 108090)) && (solve_cp || (index_cp_tmp == 0 || index_cp_tmp == 54 || index_cp_tmp == 324 || index_cp_tmp == 1170)) && (solve_co || index_co_tmp == 0) && (solve_eo || index_eo_tmp == 0))
                {
                    bool valid = true;
                    int l = static_cast<int>(sol.size());
                    int c = 0;
                    int index1_tmp2 = index1;
                    int index2_tmp2 = index2;
                    int index3_tmp2 = index3;
                    int index4_tmp2 = index4;
                    int index5_tmp2 = index5;
                    int index6_tmp2 = index6;
                    int index7_tmp2 = index7;
                    int index8_tmp2 = index8;
                    int index9_tmp2 = index9;
                    int index10_tmp2 = index10;
                    int index11_tmp2 = index11;
                    int index12_tmp2 = index12;
                    int index_cp_tmp2 = index_cp;
                    int index_co_tmp2 = index_co;
                    int index_ep_tmp2 = index_ep;
                    int index_eo_tmp2 = index_eo;
                    for (int j : sol)
                    {
                        c += 1;
                        index1_tmp2 = multi_move_table[index1_tmp2 + j];
                        index2_tmp2 = corner_move_table[index2_tmp2 + j];
                        index3_tmp2 = multi_move_table[index3_tmp2 + j];
                        index4_tmp2 = corner_move_table[index4_tmp2 + j];
                        index5_tmp2 = multi_move_table[index5_tmp2 + j];
                        index6_tmp2 = corner_move_table[index6_tmp2 + j];
                        index7_tmp2 = multi_move_table[index7_tmp2 + j];
                        index8_tmp2 = corner_move_table[index8_tmp2 + j];
                        index9_tmp2 = edge_move_table[index9_tmp2 + j];
                        index10_tmp2 = edge_move_table[index10_tmp2 + j];
                        index11_tmp2 = edge_move_table[index11_tmp2 + j];
                        index12_tmp2 = edge_move_table[index12_tmp2 + j];
                        index_cp_tmp2 = cp_move_table[index_cp_tmp2 + j];
                        index_co_tmp2 = co_move_table[index_co_tmp2 + j];
                        index_ep_tmp2 = ep_move_table[index_ep_tmp2 + j];
                        index_eo_tmp2 = eo_move_table[index_eo_tmp2 + j];
                        if (c < l && (prune_table1[index1_tmp2 + index2_tmp2] == 0 && prune_table2[index3_tmp2 + index4_tmp2] == 0 && prune_table3[index5_tmp2 + index6_tmp2] == 0 && prune_table4[index7_tmp2 + index8_tmp2] == 0 && index9_tmp2 == 0 && index10_tmp2 == 2 && index11_tmp2 == 4 && index12_tmp2 == 6 && (solve_ep || (index_ep_tmp2 == 105480 || index_ep_tmp2 == 105534 || index_ep_tmp2 == 105948 || index_ep_tmp2 == 108090)) && (solve_cp || (index_cp_tmp2 == 0 || index_cp_tmp2 == 54 || index_cp_tmp2 == 324 || index_cp_tmp2 == 1170)) && (solve_co || index_co_tmp2 == 0) && (solve_eo || index_eo_tmp2 == 0)))
                        {
                            valid = false;
                            break;
                        }
                        index2_tmp2 *= 18;
                        index4_tmp2 *= 18;
                        index6_tmp2 *= 18;
                        index8_tmp2 *= 18;
                        index9_tmp2 *= 18;
                        index10_tmp2 *= 18;
                        index11_tmp2 *= 18;
                        index12_tmp2 *= 18;
                    }
                    if (valid)
                    {
                        count += 1;
                        if (rotation == "")
                        {
                            tmp = AlgToString(sol);
                        }
                        else
                        {
                            tmp = rotation + " " + AlgToString(sol);
                        }
                        update(tmp.c_str());
                        if (count == sol_num)
                        {
                            return true;
                        }
                    }
                }
            }
            else if (depth_limited_search(index1_tmp, index2_tmp * 18, index3_tmp, index4_tmp * 18, index5_tmp, index6_tmp * 18, index7_tmp, index8_tmp * 18, index9_tmp * 18, index10_tmp * 18, index11_tmp * 18, index12_tmp * 18, index_cp_tmp, index_co_tmp, index_ep_tmp, index_eo_tmp, depth - 1, i * 18))
            {
                return true;
            }
            sol.pop_back();
        }
        return false;
    }

    void start_search(std::string arg_scramble = "", bool arg_solve_cp = false, bool arg_solve_co = false, bool arg_solve_ep = true, bool arg_solve_eo = true, std::string arg_rotation = "", int arg_sol_num = 100, int arg_max_length = 16, std::vector<std::string> arg_restrict = move_names)
    {
        scramble = arg_scramble;
        solve_cp = !arg_solve_cp;
        solve_co = !arg_solve_co;
        solve_ep = !arg_solve_ep;
        solve_eo = !arg_solve_eo;
        rotation = arg_rotation;
        max_length = arg_max_length;
        sol_num = arg_sol_num;
        restrict = arg_restrict;
        std::vector<int> alg = AlgRotation(StringToAlg(scramble), rotation);
        std::vector<int> edge_index = {187520, 187520, 187520, 187520};
        std::vector<int> single_edge_index = {0, 2, 4, 6};
        std::vector<int> corner_index = {12, 15, 18, 21};
        index1 = edge_index[0];
        index2 = corner_index[0];
        index9 = single_edge_index[0];
        create_prune_table2(index1, index2, 24 * 22 * 20 * 18, 24, 9, multi_move_table, corner_move_table, prune_table1);
        index3 = edge_index[1];
        index4 = corner_index[1];
        index10 = single_edge_index[1];
        create_prune_table2(index3, index4, 24 * 22 * 20 * 18, 24, 9, multi_move_table, corner_move_table, prune_table2);
        index5 = edge_index[2];
        index6 = corner_index[2];
        index11 = single_edge_index[2];
        create_prune_table2(index5, index6, 24 * 22 * 20 * 18, 24, 9, multi_move_table, corner_move_table, prune_table3);
        index7 = edge_index[3];
        index8 = corner_index[3];
        index12 = single_edge_index[3];
        create_prune_table2(index7, index8, 24 * 22 * 20 * 18, 24, 9, multi_move_table, corner_move_table, prune_table4);
        count = 0;
        for (std::string name : restrict)
        {
            auto it = std::find(move_names.begin(), move_names.end(), name);
            move_restrict.emplace_back(std::distance(move_names.begin(), it));
        }
        index1 *= 24;
        index3 *= 24;
        index5 *= 24;
        index7 *= 24;
        index_cp = 0;
        index_co = 0;
        index_ep = 5860 * 18;
        index_eo = 0;
        for (int m : alg)
        {
            index1 = multi_move_table[index1 + m];
            index2 = corner_move_table[index2 * 18 + m];
            index3 = multi_move_table[index3 + m];
            index4 = corner_move_table[index4 * 18 + m];
            index5 = multi_move_table[index5 + m];
            index6 = corner_move_table[index6 * 18 + m];
            index7 = multi_move_table[index7 + m];
            index8 = corner_move_table[index8 * 18 + m];
            index_cp = cp_move_table[index_cp + m];
            index_co = co_move_table[index_co + m];
            index_ep = ep_move_table[index_ep + m];
            index_eo = eo_move_table[index_eo + m];
            index9 = edge_move_table[index9 * 18 + m];
            index10 = edge_move_table[index10 * 18 + m];
            index11 = edge_move_table[index11 * 18 + m];
            index12 = edge_move_table[index12 * 18 + m];
        }
        prune1_tmp = prune_table1[index1 + index2];
        prune2_tmp = prune_table2[index3 + index4];
        prune3_tmp = prune_table3[index5 + index6];
        prune4_tmp = prune_table4[index7 + index8];
        if (prune1_tmp == 0 && prune2_tmp == 0 && prune3_tmp == 0 && prune4_tmp == 0 && index9 == 0 && index10 == 2 && index11 == 4 && index12 == 6 && (solve_ep || (index_ep == 105480 || index_ep == 105534 || index_ep == 105948 || index_ep == 108090)) && (solve_cp || (index_cp == 0 || index_cp == 54 || index_cp == 324 || index_cp == 1170)) && (solve_co || index_co == 0) && (solve_eo || index_eo == 0))
        {
            update("Already solved.");
        }
        else
        {
            index2 *= 18;
            index4 *= 18;
            index6 *= 18;
            index8 *= 18;
            index9 *= 18;
            index10 *= 18;
            index11 *= 18;
            index12 *= 18;
            for (int d = std::max(prune1_tmp, std::max(prune2_tmp, std::max(prune3_tmp, prune4_tmp))); d <= max_length; d++)
            {
                if (depth_limited_search(index1, index2, index3, index4, index5, index6, index7, index8, index9, index10, index11, index12, index_cp, index_co, index_ep, index_eo, d, 324))
                {
                    break;
                }
            }
            update("Search finished.");
        }
    }
};

struct LL_search
{
    std::vector<int> sol;
    std::string scramble;
    std::string rotation;
    int max_length;
    int sol_num;
    int count;
    std::vector<int> edge_move_table;
    std::vector<int> single_ep_move_table;
    std::vector<int> ep_move_table;
    std::vector<int> eo_move_table;
    std::vector<int> corner_move_table;
    std::vector<int> single_cp_move_table;
    std::vector<int> cp_move_table;
    std::vector<int> co_move_table;
    std::vector<int> multi_move_table;
    std::vector<int> prune_table1;
    std::vector<int> prune_table2;
    std::vector<int> prune_table3;
    std::vector<int> prune_table4;
    std::vector<int> alg;
    std::vector<std::string> restrict;
    std::vector<int> move_restrict;
    std::vector<bool> ma;
    int index1;
    int index2;
    int index3;
    int index4;
    int index5;
    int index6;
    int index7;
    int index8;
    int index9;
    int index10;
    int index11;
    int index12;
    int index_cp;
    int index_co;
    int index_ep;
    int index_eo;
    int index1_tmp;
    int index2_tmp;
    int index3_tmp;
    int index4_tmp;
    int index5_tmp;
    int index6_tmp;
    int index7_tmp;
    int index8_tmp;
    int index9_tmp;
    int index10_tmp;
    int index11_tmp;
    int index12_tmp;
    int index_cp_tmp;
    int index_co_tmp;
    int index_ep_tmp;
    int index_eo_tmp;
    int prune1_tmp;
    int prune2_tmp;
    int prune3_tmp;
    int prune4_tmp;
    std::string tmp;

    LL_search()
    {
        edge_move_table = create_edge_move_table();
        corner_move_table = create_corner_move_table();
        multi_move_table = create_multi_move_table2(4, 2, 12, 24 * 22 * 20 * 18, edge_move_table);
        ma = create_ma_table();
        prune_table1 = std::vector<int>(24 * 22 * 20 * 18 * 24, -1);
        prune_table2 = std::vector<int>(24 * 22 * 20 * 18 * 24, -1);
        prune_table3 = std::vector<int>(24 * 22 * 20 * 18 * 24, -1);
        prune_table4 = std::vector<int>(24 * 22 * 20 * 18 * 24, -1);
        single_cp_move_table = create_cp_move_table();
        cp_move_table = create_multi_move_table(4, 1, 8, 8 * 7 * 6 * 5, single_cp_move_table);
        co_move_table = create_co_move_table();
        for (int i = 0; i < 8 * 7 * 6 * 5 * 18; ++i)
        {
            cp_move_table[i] *= 18;
        }
        single_ep_move_table = create_ep_move_table();
        ep_move_table = create_multi_move_table(4, 1, 12, 12 * 11 * 10 * 9, single_ep_move_table);
        eo_move_table = create_eo_move_table();
        for (int i = 0; i < 12 * 11 * 10 * 9 * 18; ++i)
        {
            ep_move_table[i] *= 18;
        }
    }

    bool depth_limited_search(int arg_index1, int arg_index2, int arg_index3, int arg_index4, int arg_index5, int arg_index6, int arg_index7, int arg_index8, int arg_index9, int arg_index10, int arg_index11, int arg_index12, int arg_index_cp, int arg_index_co, int arg_index_ep, int arg_index_eo, int depth, int prev)
    {
        for (int i : move_restrict)
        {
            if (ma[prev + i])
            {
                continue;
            }
            index1_tmp = multi_move_table[arg_index1 + i];
            index2_tmp = corner_move_table[arg_index2 + i];
            index9_tmp = edge_move_table[arg_index9 + i];
            prune1_tmp = prune_table1[index1_tmp + index2_tmp];
            if (prune1_tmp >= depth)
            {
                continue;
            }
            index3_tmp = multi_move_table[arg_index3 + i];
            index4_tmp = corner_move_table[arg_index4 + i];
            index10_tmp = edge_move_table[arg_index10 + i];
            prune2_tmp = prune_table2[index3_tmp + index4_tmp];
            if (prune2_tmp >= depth)
            {
                continue;
            }
            index5_tmp = multi_move_table[arg_index5 + i];
            index6_tmp = corner_move_table[arg_index6 + i];
            index11_tmp = edge_move_table[arg_index11 + i];
            prune3_tmp = prune_table3[index5_tmp + index6_tmp];
            if (prune3_tmp >= depth)
            {
                continue;
            }
            index7_tmp = multi_move_table[arg_index7 + i];
            index8_tmp = corner_move_table[arg_index8 + i];
            index12_tmp = edge_move_table[arg_index12 + i];
            prune4_tmp = prune_table4[index7_tmp + index8_tmp];
            if (prune4_tmp >= depth)
            {
                continue;
            }
            index_cp_tmp = cp_move_table[arg_index_cp + i];
            index_co_tmp = co_move_table[arg_index_co + i];
            index_ep_tmp = ep_move_table[arg_index_ep + i];
            index_eo_tmp = eo_move_table[arg_index_eo + i];
            sol.emplace_back(i);
            if (depth == 1)
            {
                if (prune1_tmp == 0 && prune2_tmp == 0 && prune3_tmp == 0 && prune4_tmp == 0 && index9_tmp == 0 && index10_tmp == 2 && index11_tmp == 4 && index12_tmp == 6 && ((index_ep_tmp == 105480 && index_cp_tmp == 0) || (index_ep_tmp == 105948 && index_cp_tmp == 324) || (index_ep_tmp == 108090 && index_cp_tmp == 1170) || (index_ep_tmp == 105534 && index_cp_tmp == 54)) && index_co_tmp == 0 && index_eo_tmp == 0)
                {
                    bool valid = true;
                    int l = static_cast<int>(sol.size());
                    int c = 0;
                    int index1_tmp2 = index1;
                    int index2_tmp2 = index2;
                    int index3_tmp2 = index3;
                    int index4_tmp2 = index4;
                    int index5_tmp2 = index5;
                    int index6_tmp2 = index6;
                    int index7_tmp2 = index7;
                    int index8_tmp2 = index8;
                    int index9_tmp2 = index9;
                    int index10_tmp2 = index10;
                    int index11_tmp2 = index11;
                    int index12_tmp2 = index12;
                    int index_cp_tmp2 = index_cp;
                    int index_co_tmp2 = index_co;
                    int index_ep_tmp2 = index_ep;
                    int index_eo_tmp2 = index_eo;
                    for (int j : sol)
                    {
                        c += 1;
                        index1_tmp2 = multi_move_table[index1_tmp2 + j];
                        index2_tmp2 = corner_move_table[index2_tmp2 + j];
                        index3_tmp2 = multi_move_table[index3_tmp2 + j];
                        index4_tmp2 = corner_move_table[index4_tmp2 + j];
                        index5_tmp2 = multi_move_table[index5_tmp2 + j];
                        index6_tmp2 = corner_move_table[index6_tmp2 + j];
                        index7_tmp2 = multi_move_table[index7_tmp2 + j];
                        index8_tmp2 = corner_move_table[index8_tmp2 + j];
                        index9_tmp2 = edge_move_table[index9_tmp2 + j];
                        index10_tmp2 = edge_move_table[index10_tmp2 + j];
                        index11_tmp2 = edge_move_table[index11_tmp2 + j];
                        index12_tmp2 = edge_move_table[index12_tmp2 + j];
                        index_cp_tmp2 = cp_move_table[index_cp_tmp2 + j];
                        index_co_tmp2 = co_move_table[index_co_tmp2 + j];
                        index_ep_tmp2 = ep_move_table[index_ep_tmp2 + j];
                        index_eo_tmp2 = eo_move_table[index_eo_tmp2 + j];
                        if (c < l && (prune_table1[index1_tmp2 + index2_tmp2] == 0 && prune_table2[index3_tmp2 + index4_tmp2] == 0 && prune_table3[index5_tmp2 + index6_tmp2] == 0 && prune_table4[index7_tmp2 + index8_tmp2] == 0 && index9_tmp2 == 0 && index10_tmp2 == 2 && index11_tmp2 == 4 && index12_tmp2 == 6 && ((index_ep_tmp2 == 105480 && index_cp_tmp2 == 0) || (index_ep_tmp2 == 105948 && index_cp_tmp2 == 324) || (index_ep_tmp2 == 108090 && index_cp_tmp2 == 1170) || (index_ep_tmp2 == 105534 && index_cp_tmp2 == 54)) && index_co_tmp2 == 0 && index_eo_tmp2 == 0))
                        {
                            valid = false;
                            break;
                        }
                        index2_tmp2 *= 18;
                        index4_tmp2 *= 18;
                        index6_tmp2 *= 18;
                        index8_tmp2 *= 18;
                        index9_tmp2 *= 18;
                        index10_tmp2 *= 18;
                        index11_tmp2 *= 18;
                        index12_tmp2 *= 18;
                    }
                    if (valid)
                    {
                        count += 1;
                        if (rotation == "")
                        {
                            tmp = AlgToString(sol);
                        }
                        else
                        {
                            tmp = rotation + " " + AlgToString(sol);
                        }
                        update(tmp.c_str());
                        if (count == sol_num)
                        {
                            return true;
                        }
                    }
                }
            }
            else if (depth_limited_search(index1_tmp, index2_tmp * 18, index3_tmp, index4_tmp * 18, index5_tmp, index6_tmp * 18, index7_tmp, index8_tmp * 18, index9_tmp * 18, index10_tmp * 18, index11_tmp * 18, index12_tmp * 18, index_cp_tmp, index_co_tmp, index_ep_tmp, index_eo_tmp, depth - 1, i * 18))
            {
                return true;
            }
            sol.pop_back();
        }
        return false;
    }

    void start_search(std::string arg_scramble = "", std::string arg_rotation = "", int arg_sol_num = 100, int arg_max_length = 18, std::vector<std::string> arg_restrict = move_names)
    {
        scramble = arg_scramble;
        rotation = arg_rotation;
        max_length = arg_max_length;
        sol_num = arg_sol_num;
        restrict = arg_restrict;
        std::vector<int> alg = AlgRotation(StringToAlg(scramble), rotation);
        std::vector<int> edge_index = {187520, 187520, 187520, 187520};
        std::vector<int> single_edge_index = {0, 2, 4, 6};
        std::vector<int> corner_index = {12, 15, 18, 21};
        index1 = edge_index[0];
        index2 = corner_index[0];
        index9 = single_edge_index[0];
        create_prune_table2(index1, index2, 24 * 22 * 20 * 18, 24, 9, multi_move_table, corner_move_table, prune_table1);
        index3 = edge_index[1];
        index4 = corner_index[1];
        index10 = single_edge_index[1];
        create_prune_table2(index3, index4, 24 * 22 * 20 * 18, 24, 9, multi_move_table, corner_move_table, prune_table2);
        index5 = edge_index[2];
        index6 = corner_index[2];
        index11 = single_edge_index[2];
        create_prune_table2(index5, index6, 24 * 22 * 20 * 18, 24, 9, multi_move_table, corner_move_table, prune_table3);
        index7 = edge_index[3];
        index8 = corner_index[3];
        index12 = single_edge_index[3];
        create_prune_table2(index7, index8, 24 * 22 * 20 * 18, 24, 9, multi_move_table, corner_move_table, prune_table4);
        count = 0;
        for (std::string name : restrict)
        {
            auto it = std::find(move_names.begin(), move_names.end(), name);
            move_restrict.emplace_back(std::distance(move_names.begin(), it));
        }
        index1 *= 24;
        index3 *= 24;
        index5 *= 24;
        index7 *= 24;
        index_cp = 0;
        index_co = 0;
        index_ep = 5860 * 18;
        index_eo = 0;
        for (int m : alg)
        {
            index1 = multi_move_table[index1 + m];
            index2 = corner_move_table[index2 * 18 + m];
            index3 = multi_move_table[index3 + m];
            index4 = corner_move_table[index4 * 18 + m];
            index5 = multi_move_table[index5 + m];
            index6 = corner_move_table[index6 * 18 + m];
            index7 = multi_move_table[index7 + m];
            index8 = corner_move_table[index8 * 18 + m];
            index_cp = cp_move_table[index_cp + m];
            index_co = co_move_table[index_co + m];
            index_ep = ep_move_table[index_ep + m];
            index_eo = eo_move_table[index_eo + m];
            index9 = edge_move_table[index9 * 18 + m];
            index10 = edge_move_table[index10 * 18 + m];
            index11 = edge_move_table[index11 * 18 + m];
            index12 = edge_move_table[index12 * 18 + m];
        }
        prune1_tmp = prune_table1[index1 + index2];
        prune2_tmp = prune_table2[index3 + index4];
        prune3_tmp = prune_table3[index5 + index6];
        prune4_tmp = prune_table4[index7 + index8];
        if (prune1_tmp == 0 && prune2_tmp == 0 && prune3_tmp == 0 && prune4_tmp == 0 && index9 == 0 && index10 == 2 && index11 == 4 && index12 == 6 && ((index_ep == 105480 && index_cp == 0) || (index_ep == 105948 && index_cp == 324) || (index_ep == 108090 && index_cp == 1170) || (index_ep == 105534 && index_cp == 54)) && index_co == 0 && index_eo == 0)
        {
            update("Already solved.");
        }
        else
        {
            index2 *= 18;
            index4 *= 18;
            index6 *= 18;
            index8 *= 18;
            index9 *= 18;
            index10 *= 18;
            index11 *= 18;
            index12 *= 18;
            for (int d = std::max(prune1_tmp, std::max(prune2_tmp, std::max(prune3_tmp, prune4_tmp))); d <= max_length; d++)
            {
                if (depth_limited_search(index1, index2, index3, index4, index5, index6, index7, index8, index9, index10, index11, index12, index_cp, index_co, index_ep, index_eo, d, 324))
                {
                    break;
                }
            }
            update("Search finished.");
        }
    }
};

struct LL_AUF_search
{
    std::vector<int> sol;
    std::string scramble;
    std::string rotation;
    int max_length;
    int sol_num;
    int count;
    std::vector<int> edge_move_table;
    std::vector<int> single_ep_move_table;
    std::vector<int> ep_move_table;
    std::vector<int> eo_move_table;
    std::vector<int> corner_move_table;
    std::vector<int> single_cp_move_table;
    std::vector<int> cp_move_table;
    std::vector<int> co_move_table;
    std::vector<int> multi_move_table;
    std::vector<int> prune_table1;
    std::vector<int> prune_table2;
    std::vector<int> prune_table3;
    std::vector<int> prune_table4;
    std::vector<int> alg;
    std::vector<std::string> restrict;
    std::vector<int> move_restrict;
    std::vector<bool> ma;
    int index1;
    int index2;
    int index3;
    int index4;
    int index5;
    int index6;
    int index7;
    int index8;
    int index9;
    int index10;
    int index11;
    int index12;
    int index_cp;
    int index_co;
    int index_ep;
    int index_eo;
    int index1_tmp;
    int index2_tmp;
    int index3_tmp;
    int index4_tmp;
    int index5_tmp;
    int index6_tmp;
    int index7_tmp;
    int index8_tmp;
    int index9_tmp;
    int index10_tmp;
    int index11_tmp;
    int index12_tmp;
    int index_cp_tmp;
    int index_co_tmp;
    int index_ep_tmp;
    int index_eo_tmp;
    int prune1_tmp;
    int prune2_tmp;
    int prune3_tmp;
    int prune4_tmp;
    std::string tmp;

    LL_AUF_search()
    {
        edge_move_table = create_edge_move_table();
        corner_move_table = create_corner_move_table();
        multi_move_table = create_multi_move_table2(4, 2, 12, 24 * 22 * 20 * 18, edge_move_table);
        ma = create_ma_table();
        prune_table1 = std::vector<int>(24 * 22 * 20 * 18 * 24, -1);
        prune_table2 = std::vector<int>(24 * 22 * 20 * 18 * 24, -1);
        prune_table3 = std::vector<int>(24 * 22 * 20 * 18 * 24, -1);
        prune_table4 = std::vector<int>(24 * 22 * 20 * 18 * 24, -1);
        single_cp_move_table = create_cp_move_table();
        cp_move_table = create_multi_move_table(4, 1, 8, 8 * 7 * 6 * 5, single_cp_move_table);
        co_move_table = create_co_move_table();
        for (int i = 0; i < 8 * 7 * 6 * 5 * 18; ++i)
        {
            cp_move_table[i] *= 18;
        }
        single_ep_move_table = create_ep_move_table();
        ep_move_table = create_multi_move_table(4, 1, 12, 12 * 11 * 10 * 9, single_ep_move_table);
        eo_move_table = create_eo_move_table();
        for (int i = 0; i < 12 * 11 * 10 * 9 * 18; ++i)
        {
            ep_move_table[i] *= 18;
        }
    }

    bool depth_limited_search(int arg_index1, int arg_index2, int arg_index3, int arg_index4, int arg_index5, int arg_index6, int arg_index7, int arg_index8, int arg_index9, int arg_index10, int arg_index11, int arg_index12, int arg_index_cp, int arg_index_co, int arg_index_ep, int arg_index_eo, int depth, int prev)
    {
        for (int i : move_restrict)
        {
            if (ma[prev + i])
            {
                continue;
            }
            index1_tmp = multi_move_table[arg_index1 + i];
            index2_tmp = corner_move_table[arg_index2 + i];
            index9_tmp = edge_move_table[arg_index9 + i];
            prune1_tmp = prune_table1[index1_tmp + index2_tmp];
            if (prune1_tmp >= depth)
            {
                continue;
            }
            index3_tmp = multi_move_table[arg_index3 + i];
            index4_tmp = corner_move_table[arg_index4 + i];
            index10_tmp = edge_move_table[arg_index10 + i];
            prune2_tmp = prune_table2[index3_tmp + index4_tmp];
            if (prune2_tmp >= depth)
            {
                continue;
            }
            index5_tmp = multi_move_table[arg_index5 + i];
            index6_tmp = corner_move_table[arg_index6 + i];
            index11_tmp = edge_move_table[arg_index11 + i];
            prune3_tmp = prune_table3[index5_tmp + index6_tmp];
            if (prune3_tmp >= depth)
            {
                continue;
            }
            index7_tmp = multi_move_table[arg_index7 + i];
            index8_tmp = corner_move_table[arg_index8 + i];
            index12_tmp = edge_move_table[arg_index12 + i];
            prune4_tmp = prune_table4[index7_tmp + index8_tmp];
            if (prune4_tmp >= depth)
            {
                continue;
            }
            index_cp_tmp = cp_move_table[arg_index_cp + i];
            index_co_tmp = co_move_table[arg_index_co + i];
            index_ep_tmp = ep_move_table[arg_index_ep + i];
            index_eo_tmp = eo_move_table[arg_index_eo + i];
            sol.emplace_back(i);
            if (depth == 1)
            {
                if (prune1_tmp == 0 && prune2_tmp == 0 && prune3_tmp == 0 && prune4_tmp == 0 && index9_tmp == 0 && index10_tmp == 2 && index11_tmp == 4 && index12_tmp == 6 && index_ep_tmp == 105480 && index_cp_tmp == 0 && index_co_tmp == 0 && index_eo_tmp == 0)
                {
                    count += 1;
                    if (rotation == "")
                    {
                        tmp = AlgToString(sol);
                    }
                    else
                    {
                        tmp = rotation + " " + AlgToString(sol);
                    }
                    update(tmp.c_str());
                    if (count == sol_num)
                    {
                        return true;
                    }
                }
            }
            else if (depth_limited_search(index1_tmp, index2_tmp * 18, index3_tmp, index4_tmp * 18, index5_tmp, index6_tmp * 18, index7_tmp, index8_tmp * 18, index9_tmp * 18, index10_tmp * 18, index11_tmp * 18, index12_tmp * 18, index_cp_tmp, index_co_tmp, index_ep_tmp, index_eo_tmp, depth - 1, i * 18))
            {
                return true;
            }
            sol.pop_back();
        }
        return false;
    }

    void start_search(std::string arg_scramble = "", std::string arg_rotation = "", int arg_sol_num = 100, int arg_max_length = 20, std::vector<std::string> arg_restrict = move_names)
    {
        scramble = arg_scramble;
        rotation = arg_rotation;
        max_length = arg_max_length;
        sol_num = arg_sol_num;
        restrict = arg_restrict;
        std::vector<int> alg = AlgRotation(StringToAlg(scramble), rotation);
        std::vector<int> edge_index = {187520, 187520, 187520, 187520};
        std::vector<int> single_edge_index = {0, 2, 4, 6};
        std::vector<int> corner_index = {12, 15, 18, 21};
        index1 = edge_index[0];
        index2 = corner_index[0];
        index9 = single_edge_index[0];
        create_prune_table2(index1, index2, 24 * 22 * 20 * 18, 24, 9, multi_move_table, corner_move_table, prune_table1);
        index3 = edge_index[1];
        index4 = corner_index[1];
        index10 = single_edge_index[1];
        create_prune_table2(index3, index4, 24 * 22 * 20 * 18, 24, 9, multi_move_table, corner_move_table, prune_table2);
        index5 = edge_index[2];
        index6 = corner_index[2];
        index11 = single_edge_index[2];
        create_prune_table2(index5, index6, 24 * 22 * 20 * 18, 24, 9, multi_move_table, corner_move_table, prune_table3);
        index7 = edge_index[3];
        index8 = corner_index[3];
        index12 = single_edge_index[3];
        create_prune_table2(index7, index8, 24 * 22 * 20 * 18, 24, 9, multi_move_table, corner_move_table, prune_table4);
        count = 0;
        for (std::string name : restrict)
        {
            auto it = std::find(move_names.begin(), move_names.end(), name);
            move_restrict.emplace_back(std::distance(move_names.begin(), it));
        }
        index1 *= 24;
        index3 *= 24;
        index5 *= 24;
        index7 *= 24;
        index_cp = 0;
        index_co = 0;
        index_ep = 5860 * 18;
        index_eo = 0;
        for (int m : alg)
        {
            index1 = multi_move_table[index1 + m];
            index2 = corner_move_table[index2 * 18 + m];
            index3 = multi_move_table[index3 + m];
            index4 = corner_move_table[index4 * 18 + m];
            index5 = multi_move_table[index5 + m];
            index6 = corner_move_table[index6 * 18 + m];
            index7 = multi_move_table[index7 + m];
            index8 = corner_move_table[index8 * 18 + m];
            index_cp = cp_move_table[index_cp + m];
            index_co = co_move_table[index_co + m];
            index_ep = ep_move_table[index_ep + m];
            index_eo = eo_move_table[index_eo + m];
            index9 = edge_move_table[index9 * 18 + m];
            index10 = edge_move_table[index10 * 18 + m];
            index11 = edge_move_table[index11 * 18 + m];
            index12 = edge_move_table[index12 * 18 + m];
        }
        prune1_tmp = prune_table1[index1 + index2];
        prune2_tmp = prune_table2[index3 + index4];
        prune3_tmp = prune_table3[index5 + index6];
        prune4_tmp = prune_table4[index7 + index8];
        if (prune1_tmp == 0 && prune2_tmp == 0 && prune3_tmp == 0 && prune4_tmp == 0 && index9 == 0 && index10 == 2 && index11 == 4 && index12 == 6 && index_ep == 105480 && index_cp == 0 && index_co == 0 && index_eo == 0)
        {
            update("Already solved.");
        }
        else
        {
            index2 *= 18;
            index4 *= 18;
            index6 *= 18;
            index8 *= 18;
            index9 *= 18;
            index10 *= 18;
            index11 *= 18;
            index12 *= 18;
            for (int d = std::max(prune1_tmp, std::max(prune2_tmp, std::max(prune3_tmp, prune4_tmp))); d <= max_length; d++)
            {
                if (depth_limited_search(index1, index2, index3, index4, index5, index6, index7, index8, index9, index10, index11, index12, index_cp, index_co, index_ep, index_eo, d, 324))
                {
                    break;
                }
            }
            update("Search finished.");
        }
    }
};

std::vector<bool> F2L_option_array(const std::string &input)
{
    std::vector<bool> result(4, false);
    std::istringstream iss(input);
    std::string token;

    while (iss >> token)
    {
        if (token == "BL")
        {
            result[0] = true;
        }
        else if (token == "BR")
        {
            result[1] = true;
        }
        else if (token == "FR")
        {
            result[2] = true;
        }
        else if (token == "FL")
        {
            result[3] = true;
        }
    }
    return result;
}

void solve_F2L(std::string scramble, std::string rotation, std::string option, std::string sol_num, std::string max_length, std::string restrict)
{
    std::vector<bool> option_list = F2L_option_array(option);
    int count = 0;
    int num = std::stoi(sol_num);
    int len = std::stoi(max_length);
    std::vector<int> slot;
    std::vector<std::string> move_restrict;
    for (char c : restrict)
    {
        std::string face(1, c);
        move_restrict.emplace_back(face);
        move_restrict.emplace_back(face + "2");
        move_restrict.emplace_back(face + "'");
    }
    if (option_list[0])
    {
        count += 1;
        slot.emplace_back(0);
    }
    if (option_list[1])
    {
        count += 1;
        slot.emplace_back(1);
    }
    if (option_list[2])
    {
        count += 1;
        slot.emplace_back(2);
    }
    if (option_list[3])
    {
        count += 1;
        slot.emplace_back(3);
    }

    if (count == 0)
    {
        cross_search search;
        search.start_search(scramble, rotation, num, len, move_restrict);
    }
    else if (count == 1)
    {
        xcross_search search;
        search.start_search(scramble, rotation, slot[0], num, len, move_restrict);
    }
    else if (count == 2)
    {
        xxcross_search search;
        search.start_search(scramble, rotation, slot[0], slot[1], num, len, move_restrict);
    }
    else if (count == 3)
    {
        xxxcross_search search;
        search.start_search(scramble, rotation, slot[0], slot[1], slot[2], num, len, move_restrict);
    }
    else if (count == 4)
    {
        xxxxcross_search search;
        search.start_search(scramble, rotation, num, len, move_restrict);
    }
}

std::vector<bool> LL_substeps_option_array(const std::string &input)
{
    std::vector<bool> result(4, false);
    std::istringstream iss(input);
    std::string token;

    while (iss >> token)
    {
        if (token == "CP")
        {
            result[0] = true;
        }
        else if (token == "CO")
        {
            result[1] = true;
        }
        else if (token == "EP")
        {
            result[2] = true;
        }
        else if (token == "EO")
        {
            result[3] = true;
        }
    }
    return result;
}

void solve_LL_substeps(std::string scramble, std::string rotation, std::string option, std::string sol_num, std::string max_length, std::string restrict)
{
    std::vector<bool> option_list = LL_substeps_option_array(option);
    int count = 0;
    int num = std::stoi(sol_num);
    int len = std::stoi(max_length);
    std::vector<int> slot;
    std::vector<std::string> move_restrict;
    for (char c : restrict)
    {
        std::string face(1, c);
        move_restrict.emplace_back(face);
        move_restrict.emplace_back(face + "2");
        move_restrict.emplace_back(face + "'");
    }
    LL_substeps_search search;
    search.start_search(scramble, option_list[0], option_list[1], option_list[2], option_list[3], rotation, num, len, move_restrict);
}

void solve_LL(std::string scramble, std::string rotation, std::string sol_num, std::string max_length, std::string restrict)
{
    int count = 0;
    int num = std::stoi(sol_num);
    int len = std::stoi(max_length);
    std::vector<int> slot;
    std::vector<std::string> move_restrict;
    for (char c : restrict)
    {
        std::string face(1, c);
        move_restrict.emplace_back(face);
        move_restrict.emplace_back(face + "2");
        move_restrict.emplace_back(face + "'");
    }
    LL_search search;
    search.start_search(scramble, rotation, num, len, move_restrict);
}

void solve_LL_AUF(std::string scramble, std::string rotation, std::string sol_num, std::string max_length, std::string restrict)
{
    int count = 0;
    int num = std::stoi(sol_num);
    int len = std::stoi(max_length);
    std::vector<int> slot;
    std::vector<std::string> move_restrict;
    for (char c : restrict)
    {
        std::string face(1, c);
        move_restrict.emplace_back(face);
        move_restrict.emplace_back(face + "2");
        move_restrict.emplace_back(face + "'");
    }
    LL_AUF_search search;
    search.start_search(scramble, rotation, num, len, move_restrict);
}

void controller(std::string solver, std::string scr, std::string rot, std::string slot, std::string ll, std::string num, std::string len, std::string restrict)
{
    if (solver == "F2L")
    {
        solve_F2L(scr, rot, slot, num, len, restrict);
    }
    else if (solver == "LS")
    {
        solve_LL_substeps(scr, rot, ll, num, len, restrict);
    }
    else if (solver == "LL")
    {
        solve_LL(scr, rot, num, len, restrict);
    }
    else if (solver == "LU")
    {
        solve_LL_AUF(scr, rot, num, len, restrict);
    }
}

EMSCRIPTEN_BINDINGS(my_module)
{
    emscripten::function("solve", &controller);
}