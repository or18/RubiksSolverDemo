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

std::string AlgToString(std::vector<int> &alg)
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
    std::vector<int> eo_move_table;
    std::vector<int> multi_move_table;
    std::vector<int> prune_table;
    std::vector<int> alg;
    std::vector<std::string> restrict;
    std::vector<int> move_restrict;
    std::vector<bool> ma;
    int index1;
    int index2;
    int index_eo;
    int index1_tmp;
    int index2_tmp;
    int index_eo_tmp;
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
        eo_move_table = create_eo_move_table();
    }

    bool depth_limited_search(int arg_index1, int arg_index2, int arg_index_eo, int depth, int prev)
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
            index_eo_tmp = eo_move_table[arg_index_eo + i];
            sol.emplace_back(i);
            if (depth == 1)
            {
                if (prune_tmp == 0 && index_eo_tmp == 0)
                {
                    bool valid = true;
                    int l = static_cast<int>(sol.size());
                    int c = 0;
                    int index1_tmp2 = index1;
                    int index2_tmp2 = index2;
                    int index_eo_tmp2 = index_eo;
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
                            index_eo_tmp2 = eo_move_table[index_eo_tmp2 + j];
                            if (c < l && prune_table[index1_tmp2 * 528 + index2_tmp2] == 0 && index_eo_tmp2 == 0)
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
            else if (depth_limited_search(index1_tmp * 18, index2_tmp * 18, index_eo_tmp, depth - 1, i * 18))
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
        index_eo = 0;
        count = 0;
        for (int m : alg)
        {
            index1 = multi_move_table[index1 * 18 + m];
            index2 = multi_move_table[index2 * 18 + m];
            index_eo = eo_move_table[index_eo + m];
        }
        for (std::string name : restrict)
        {
            auto it = std::find(move_names.begin(), move_names.end(), name);
            move_restrict.emplace_back(std::distance(move_names.begin(), it));
        }
        prune_tmp = prune_table[index1 * 528 + index2];
        if (prune_tmp == 0 && index_eo == 0)
        {
            update("Already solved.");
        }
        else
        {
            index1 *= 18;
            index2 *= 18;
            for (int d = prune_tmp; d <= max_length; d++)
            {
                if (depth_limited_search(index1, index2, index_eo, d, 324))
                {
                    break;
                }
            }
            update("Search finished.");
        }
    }
};

void solve_F2L(std::string scramble, std::string rotation, std::string sol_num, std::string max_length, std::string restrict)
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
    cross_search search;
    search.start_search(scramble, rotation, num, len, move_restrict);
}

void controller(std::string scr, std::string rot, std::string num, std::string len, std::string restrict)
{
    solve_F2L(scr, rot, num, len, restrict);
}

EMSCRIPTEN_BINDINGS(my_module)
{
    emscripten::function("solve", &controller);
}