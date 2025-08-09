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

std::vector<std::vector<int>> c_array = {{0}, {1, 1, 1, 1, 1, 1, 1, 1, 1}, {1, 2, 4, 8, 16, 32}, {1, 3, 9, 27, 81, 243}};

std::vector<std::vector<int>> base_array = {{0}, {0}, {1, 12, 12 * 11, 12 * 11 * 10, 12 * 11 * 10 * 9}, {1, 8, 8 * 7, 8 * 7 * 6, 8 * 7 * 6 * 5, 8 * 7 * 6 * 5 * 4, 8 * 7 * 6 * 5 * 4 * 3, 8 * 7 * 6 * 5 * 4 * 3 * 2, 8 * 7 * 6 * 5 * 4 * 3 * 2}};

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

std::vector<int> sorted(8);

std::vector<std::vector<int>> base_array2 = {{0}, {0}, {12, 11, 10, 9, 8}, {8, 7, 6, 5, 4, 3, 2, 1}};

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
			move_table[18 * i + j] = o_to_index(new_state.co, 3, 8);
		}
	}
	return move_table;
}

std::vector<int> create_multi_move_table(int n, int c, int pn, int size, const std::vector<int> &table)
{
	std::vector<int> move_table(size * 18 + 1, -1);
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

void create_prune_table(const std::vector<int> &table1, const std::vector<int> &table2, std::vector<unsigned char> &prune_table, std::vector<int> &move_restrict)
{
	int size = 88179840;
	int size2 = 2187;
	int next_i;
	int index1_tmp;
	int index2_tmp;
	int next_d;
	std::vector<std::string> af = {"", "L R' ", "L' R", "F B' ", "F2 B2 ", "F' B "};
	std::vector<std::string> ad = {"U D'", "U2 D2", "U' D"};
	for (int i = 0; i < 6; ++i)
	{
		index1_tmp = 0;
		index2_tmp = 0;
		std::string tmp_scr = af[i];
		std::vector<int> tmp_alg = StringToAlg(tmp_scr);
		for (int k : tmp_alg)
		{
			index1_tmp = table1[index1_tmp * 18 + k];
			index2_tmp = table2[index2_tmp * 18 + k];
		}
		prune_table[index1_tmp * size2 + index2_tmp] = 0;
		for (int j = 0; j < 3; ++j)
		{
			tmp_scr = ad[j];
			tmp_alg = StringToAlg(tmp_scr);
			int index1_tmp2 = index1_tmp;
			int index2_tmp2 = index2_tmp;
			for (int k : tmp_alg)
			{
				index1_tmp2 = table1[index1_tmp2 * 18 + k];
				index2_tmp2 = table2[index2_tmp2 * 18 + k];
			}
			prune_table[index1_tmp2 * size2 + index2_tmp2] = 0;
		}
	}
	int num = 24;
	int num_old = 24;
	for (int d = 0; d < 8; ++d)
	{
		next_d = d + 1;
		for (int i = 0; i < size; ++i)
		{
			if (prune_table[i] == d)
			{
				index1_tmp = (i / size2) * 18;
				index2_tmp = (i % size2) * 18;
				for (int j : move_restrict)
				{
					next_i = table1[index1_tmp + j] * size2 + table2[index2_tmp + j];
					if (prune_table[next_i] == 255)
					{
						prune_table[next_i] = next_d;
						num += 1;
					}
				}
			}
		}
		if (num == num_old)
		{
			break;
		}
		num_old = num;
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

struct search
{
	std::vector<int> sol;
	std::string scramble;
	std::string rotation;
	int max_length;
	int sol_num;
	int count;
	std::vector<int> single_cp_move_table;
	std::vector<int> cp_move_table;
	std::vector<int> co_move_table;
	std::vector<unsigned char> prune_table;
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
	int current_max_depth;

	search()
	{
		prune_table = std::vector<unsigned char>(88179840, 255);
		single_cp_move_table = create_cp_move_table();
		cp_move_table = create_multi_move_table(8, 1, 8, 40320, single_cp_move_table);
		ma = create_ma_table();
		co_move_table = create_co_move_table();
	}

	bool depth_limited_search(int arg_index1, int arg_index2, int depth, int prev)
	{
		for (int i : move_restrict)
		{
			if (ma[prev + i])
			{
				continue;
			}
			index1_tmp = cp_move_table[arg_index1 + i];
			index2_tmp = co_move_table[arg_index2 + i];
			prune_tmp = prune_table[index1_tmp * 2187 + index2_tmp];
			if (prune_tmp != 255 && prune_tmp >= depth)
			{
				continue;
			}
			sol[current_max_depth - depth] = i;
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
						if (index1_tmp2 == cp_move_table[index1_tmp2 + j] * 18 && index2_tmp2 == co_move_table[index2_tmp2 + j] * 18)
						{
							valid = false;
							break;
						}
						else
						{
							c += 1;
							index1_tmp2 = cp_move_table[index1_tmp2 + j];
							index2_tmp2 = co_move_table[index2_tmp2 + j];
							if (c < l && prune_table[index1_tmp2 * 2187 + index2_tmp2] == 0)
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
		}
		return false;
	}

	void start_search(std::string arg_scramble = "", std::string arg_rotation = "", int arg_sol_num = 100, int arg_max_length = 12, std::vector<std::string> arg_restrict = move_names)
	{
		scramble = arg_scramble;
		rotation = arg_rotation;
		max_length = arg_max_length;
		sol_num = arg_sol_num;
		restrict = arg_restrict;
		for (std::string name : restrict)
		{
			auto it = std::find(move_names.begin(), move_names.end(), name);
			move_restrict.emplace_back(std::distance(move_names.begin(), it));
		};
		create_prune_table(cp_move_table, co_move_table, prune_table, move_restrict);
		std::vector<int> alg = AlgRotation(StringToAlg(scramble), rotation);
		index1 = 0;
		index2 = 0;
		count = 0;
		for (int m : alg)
		{
			index1 = cp_move_table[index1 * 18 + m];
			index2 = co_move_table[index2 * 18 + m];
		}
		prune_tmp = prune_table[index1 * 2187 + index2];
		if (prune_tmp == 0)
		{
			update("Already solved.");
		}
		else
		{
			index1 *= 18;
			index2 *= 18;
			for (int d = 1; d <= max_length; d++)
			{
				current_max_depth = d;
				sol.resize(d);
				if (depth_limited_search(index1, index2, d, 324))
				{
					break;
				}
			}
			update("Search finished.");
		}
	}
};

void solve(std::string scramble, std::string rotation, std::string sol_num, std::string max_length, std::string restrict)
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
	search cs;
	cs.start_search(scramble, rotation, num, len, move_restrict);
}

EMSCRIPTEN_BINDINGS(my_module)
{
	emscripten::function("solve", &solve);
}
