#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>
#include <emscripten.h>
#endif
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <string>
#include <sstream>
#include <cstdlib>
#include <random>
#include <deque>
#include <tsl/robin_set.h>

#pragma GCC target("avx2")

// Helper function to get actual RSS (Linux only)
size_t get_rss_kb()
{
#ifdef __EMSCRIPTEN__
	// /proc/self/status is not available in WebAssembly
	return 0;
#else
	std::ifstream status("/proc/self/status");
	std::string line;
	while (std::getline(status, line))
	{
		if (line.substr(0, 6) == "VmRSS:")
		{
			size_t pos = line.find_last_of(" \t");
			if (pos != std::string::npos)
			{
				size_t kb = std::stoull(line.substr(6, pos - 6));
				return kb;
			}
		}
	}
	return 0;
#endif
}

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

struct SlidingDepthSets
{
	tsl::robin_set<uint64_t> prev, cur, next;
	size_t max_total_nodes;
	int current_depth;
	bool expansion_stopped;
	bool verbose;

	static const std::vector<size_t> expected_nodes_per_depth;
	static constexpr size_t BYTES_PER_ELEMENT = 32; // robin_set(24) + index_pairs(8)

	SlidingDepthSets(size_t max_nodes = SIZE_MAX, bool enable_verbose = true)
		: max_total_nodes(max_nodes), current_depth(0), expansion_stopped(false), verbose(enable_verbose)
	{
		// Set the load factor
		prev.max_load_factor(0.9f);
		cur.max_load_factor(0.9f);
		next.max_load_factor(0.9f);

		// Set initial capacity to a smaller value (memory efficiency)
		prev.reserve(1);
		cur.reserve(16);   // Initial nodes
		next.reserve(256); // Next depth
	}

	bool encounter_and_mark_next(uint64_t idx, bool &capacity_reached)
	{
		capacity_reached = false;

		if (expansion_stopped)
		{
			capacity_reached = true;
			return false;
		}

		// Using find() once is more efficient than calling count() three times
		if (cur.find(idx) != cur.end() ||
			prev.find(idx) != prev.end() ||
			next.find(idx) != next.end())
			return false;

		// Directly check the internal state of robin_hash to predict rehash
		if (next.will_rehash_on_next_insert())
		{
			if (verbose)
			{
				std::cout << "  [REHASH PREDICTED] size=" << next.size()
						  << ", buckets=" << next.bucket_count()
						  << ", threshold=" << next.load_threshold()
						  << ", available=" << next.available_capacity() << std::endl;
			}
			capacity_reached = true;
			expansion_stopped = true;
			return false;
		}

		next.insert(idx);
		return true;
	}

	void set_initial(uint64_t idx)
	{
		cur.insert(idx);
		current_depth = 0;
	}

	static size_t next_power_of_2(size_t n)
	{
		if (n == 0)
			return 1;
		n--;
		n |= n >> 1;
		n |= n >> 2;
		n |= n >> 4;
		n |= n >> 8;
		n |= n >> 16;
#if SIZE_MAX > 0xFFFFFFFF
		n |= n >> 32; // Only for 64-bit platforms
#endif
		return n + 1;
	}

	void advance_depth(std::vector<std::vector<uint64_t>> &index_pairs)
	{
		if (verbose)
		{
			std::cout << "  [advance_depth START] prev=" << prev.size()
					  << ", cur=" << cur.size()
					  << ", next=" << next.size()
					  << ", next.buckets=" << next.bucket_count() << std::endl;
		}

		// Detach element_vector (prev is already recorded)
		if (prev.get_element_vector() != nullptr)
		{
			prev.detach_element_vector();
		}

		// Detach element_vector of cur as well (required before move)
		if (cur.get_element_vector() != nullptr)
		{
			cur.detach_element_vector();
		}

		// Detach element_vector of next as well (required before move)
		if (next.get_element_vector() != nullptr)
		{
			next.detach_element_vector();
		}

		// Explicitly release memory
		prev.clear();
		prev = tsl::robin_set<uint64_t>(); // Complete release
		prev.max_load_factor(0.9f);

		// Move efficiently using move semantics (element_vector already detached)
		prev = std::move(cur);
		cur = std::move(next);

		// Create a new next and attach element_vector for the next depth
		next = tsl::robin_set<uint64_t>();
		next.max_load_factor(0.9f);
		expansion_stopped = false;
		current_depth++;

		// Clear and attach index_pairs for the next depth
		int next_depth_idx = current_depth + 1;
		if (static_cast<size_t>(next_depth_idx) < index_pairs.size())
		{
			index_pairs[next_depth_idx].clear();
			next.attach_element_vector(&index_pairs[next_depth_idx]);
			if (verbose)
			{
				std::cout << "  [Element vector] Attached to depth=" << next_depth_idx << std::endl;
			}
			size_t estimated_next_nodes = expected_nodes_per_depth[next_depth_idx];
			size_t current_node_count = prev.size() + cur.size();

			if (current_node_count < max_total_nodes)
			{
				size_t remaining_node_budget = max_total_nodes - current_node_count;

				size_t target_nodes;
				if (estimated_next_nodes == SIZE_MAX)
				{
					target_nodes = remaining_node_budget;
					if (verbose)
					{
						std::cout << "  [Unknown depth estimation] Using all remaining budget: "
								  << target_nodes << std::endl;
					}
				}
				else
				{
					target_nodes = std::min(estimated_next_nodes, remaining_node_budget);
				}

				// Calculate the maximum number of buckets that can be accommodated from memory
				const size_t total_memory_bytes = max_total_nodes * BYTES_PER_ELEMENT;
				// Existing node data + bucket array memory
				// Note: prev is excluded as it will be released immediately, only cur is counted
				const size_t current_memory_bytes = current_node_count * BYTES_PER_ELEMENT + cur.bucket_count() * 4;
				const size_t remaining_memory_bytes = total_memory_bytes - current_memory_bytes;

				if (verbose)
				{
					std::cout << "  [Memory calculation] total=" << (total_memory_bytes / 1024.0 / 1024.0) << " MB"
							  << ", current=" << (current_memory_bytes / 1024.0 / 1024.0) << " MB"
							  << ", remaining=" << (remaining_memory_bytes / 1024.0 / 1024.0) << " MB" << std::endl;
				}

				size_t max_affordable_buckets = 0;
				// Try in order: 128M → 64M → 32M → 16M → 8M → 4M → 2M
				// Memory = bucket array (4 bytes/bucket) + node data (32 bytes/node)
				// node capacity = bucket × 0.9
				for (size_t test_buckets = (1ULL << 27); test_buckets >= (1ULL << 21); test_buckets /= 2)
				{
					const size_t test_capacity = static_cast<size_t>(test_buckets * 0.9f);
					const size_t test_bucket_memory = test_buckets * 4; // Bucket array
					const size_t test_node_memory = test_capacity * 32; // Node data (robin_set 24 + index_pairs 8)
					const size_t test_total_memory = test_bucket_memory + test_node_memory;

					if (verbose)
					{
						std::cout << "    [Try] buckets=" << test_buckets
								  << " (" << (test_buckets >> 20) << "M)"
								  << ", capacity=" << test_capacity
								  << ", bucket_mem=" << (test_bucket_memory / 1024.0 / 1024.0) << " MB"
								  << ", node_mem=" << (test_node_memory / 1024.0 / 1024.0) << " MB"
								  << ", total=" << (test_total_memory / 1024.0 / 1024.0) << " MB"
								  << " <= " << (remaining_memory_bytes / 1024.0 / 1024.0) << " MB ? "
								  << (test_total_memory <= remaining_memory_bytes ? "YES" : "NO") << std::endl;
					}

					if (test_total_memory <= remaining_memory_bytes)
					{
						max_affordable_buckets = test_buckets;
						break;
					}
				}

				// If less than 2M buckets can be allocated, stop depth expansion
				if (max_affordable_buckets == 0 || max_affordable_buckets < (1ULL << 21))
				{
					std::cout << "  [Insufficient memory] Cannot allocate minimum 2M buckets for depth="
							  << next_depth_idx
							  << " (remaining=" << (remaining_memory_bytes / 1024.0 / 1024.0) << " MB)"
							  << ", stopping expansion" << std::endl;
					expansion_stopped = true;
					return;
				}

				// Ideal number of buckets required for target_nodes
				const size_t ideal_buckets_for_nodes = next_power_of_2(
					static_cast<size_t>(std::ceil(target_nodes / 0.9f)));

				if (verbose)
				{
					std::cout << "  [Bucket calculation] target_nodes=" << target_nodes
							  << ", ideal_buckets=" << ideal_buckets_for_nodes
							  << " (" << (ideal_buckets_for_nodes >> 20) << "M)"
							  << ", max_affordable=" << max_affordable_buckets
							  << " (" << (max_affordable_buckets >> 20) << "M)" << std::endl;
				}

				// Determine if there is a memory constraint
				const bool memory_constrained = (ideal_buckets_for_nodes > max_affordable_buckets);

				size_t reserve_value;

				if (memory_constrained)
				{
					// When memory constrained: reserve max_load_factor * 0.9
					reserve_value = static_cast<size_t>(max_affordable_buckets * 0.9 * 0.9);

					if (verbose)
					{
						std::cout << "  [Memory constrained] max_affordable=" << max_affordable_buckets
								  << ", reserve_value=" << reserve_value << std::endl;
					}
				}
				else
				{
					reserve_value = target_nodes;

					if (verbose)
					{
						std::cout << "  [Node constrained] target_nodes=" << target_nodes
								  << ", reserve_value=" << reserve_value << std::endl;
					}
				}

				try
				{
					next.reserve(reserve_value);

					const size_t final_buckets = next.bucket_count();
					const size_t final_threshold = next.load_threshold();
					const size_t final_available = next.available_capacity();

					if (verbose)
					{
						std::cout << "  [Reserve] estimated="
								  << (estimated_next_nodes == SIZE_MAX ? std::string("UNKNOWN") : std::to_string(estimated_next_nodes))
								  << ", final_buckets=" << final_buckets
								  << ", threshold=" << final_threshold
								  << ", available=" << final_available
								  << " (" << (final_available * 100.0 / final_buckets) << "%)"
								  << ", total=" << (current_node_count + final_available) << "/" << max_total_nodes
								  << std::endl;
					}
				}
				catch (const std::bad_alloc &)
				{
					std::cout << "  [Allocation failed] Bad alloc during reserve" << std::endl;
					expansion_stopped = true;
				}
			}
		}

		if (verbose)
		{
			std::cout << "  [advance_depth END]" << std::endl;
		}
	}

	void clear_all()
	{
		prev.clear();
		cur.clear();
		next.clear();
		expansion_stopped = false;
	}

	size_t total_size() const
	{
		return prev.size() + cur.size() + next.size();
	}

	size_t mem_bytes() const
	{
		return total_size() * BYTES_PER_ELEMENT;
	}
};

const std::vector<size_t> SlidingDepthSets::expected_nodes_per_depth = {
	1, 15, 182, 2286, 28611, 349811, 4169855, 47547352, SIZE_MAX}; // depth=8 represents "infinity" with SIZE_MAX

int create_prune_table_sparse(int index1, int index2, int index3, int size1, int size2, int size3, int max_depth, int max_memory_kb, const std::vector<int> &table1, const std::vector<int> &table2, const std::vector<int> &table3, std::vector<std::vector<uint64_t>> &index_pairs, std::vector<int> &num_list, bool verbose = true)
{
	const size_t fixed_overhead_kb = 20 * 1024;
	const size_t adjusted_memory_kb = (static_cast<size_t>(max_memory_kb) > fixed_overhead_kb) ? (static_cast<size_t>(max_memory_kb) - fixed_overhead_kb) : static_cast<size_t>(max_memory_kb);
	const size_t available_bytes = static_cast<size_t>(adjusted_memory_kb) * 1024;
	const size_t bytes_per_node = 32; // robin_set(24) + index_pairs(8)
	const uint64_t node_cap = available_bytes / bytes_per_node;

	SlidingDepthSets visited(node_cap, verbose);
	const uint64_t size23 = static_cast<size_t>(size2 * size3);
	num_list.resize(max_depth + 1, 0);
	index_pairs.resize(max_depth + 1);

	// Reserve initial capacity (capacity at depth 0 is small)
	index_pairs[0].reserve(1);

	std::cout << "Memory budget: " << (max_memory_kb / 1024.0) << " MB"
			  << " | Available: " << (available_bytes / 1024.0 / 1024.0) << " MB"
			  << " | Node capacity: " << node_cap << std::endl;

	const int index23 = index2 * size3 + index3;
	const uint64_t index123 = index1 * size23 + index23;
	visited.set_initial(index123);
	index_pairs[0].emplace_back(index123);
	num_list[0] = 1;

	// Attach element_vector for depth=1
	if (max_depth >= 1)
	{
		index_pairs[1].clear();
		visited.next.attach_element_vector(&index_pairs[1]);
		if (verbose)
		{
			std::cout << "[Element vector] Attached to depth=1" << std::endl;
		}
	}

	bool stop = false;
	int next_depth;
	int completed_depth = 0;

	// Define variables outside the loop (reduce memory allocation by reusing)
	int cur_index1, cur_index2, cur_index3;
	uint64_t cur_index23;
	int next_index1, next_index2, next_index3, next_index23;
	uint64_t next_index123;
	bool capacity_reached;

	for (int depth = 0; depth < max_depth; ++depth)
	{
		next_depth = depth + 1;
		if (visited.cur.empty())
			break;

		for (uint64_t cur_index123_val : visited.cur)
		{
			cur_index1 = cur_index123_val / size23;
			cur_index23 = cur_index123_val % size23;
			cur_index2 = cur_index23 / size3;
			cur_index3 = cur_index23 % size3;

			for (int i = 0; i < 18; ++i)
			{
				next_index1 = table1[cur_index1 * 18 + i];
				next_index2 = table2[cur_index2 * 18 + i];
				next_index3 = table3[cur_index3 * 18 + i];
				next_index23 = next_index2 * size3 + next_index3;
				next_index123 = next_index1 * size23 + next_index23;

				capacity_reached = false;
				if (visited.encounter_and_mark_next(next_index123, capacity_reached))
				{
					num_list[next_depth] += 1;
				}
				else if (capacity_reached)
				{
					if (verbose)
					{
						std::cout << "Depth: " << next_depth
								  << " | Nodes: " << num_list[next_depth]
								  << " | Total: " << visited.total_size()
								  << " | Capacity reached" << std::endl;
					}
					stop = true;
					break;
				}
			}
			if (stop)
				break;
		}

		if (stop)
			break;

		if (verbose)
		{
			std::cout << "Depth: " << next_depth
					  << " | Nodes: " << num_list[next_depth]
					  << " | Total: " << visited.total_size() << std::endl;
		}

		// Manage element_vector within advance_depth
		visited.advance_depth(index_pairs);

		completed_depth = depth;
		// Automatic recording by element_vector eliminates the need for manual copying
		// index_pairs[completed_depth] already contains data
	}

	// Process the remainder at the end of the loop
	int cur_depth = completed_depth + 1;
	int next_d = completed_depth + 2;

	// Detach element_vector from cur (cur may still be attached from the previous next)
	if (visited.cur.get_element_vector() != nullptr)
	{
		visited.cur.detach_element_vector();
		if (cur_depth < static_cast<int>(index_pairs.size()))
		{
			if (verbose)
			{
				std::cout << "[Element vector] Detached from cur at completion (depth=" << cur_depth
						  << ", recorded=" << index_pairs[cur_depth].size() << " nodes)" << std::endl;
			}
		}
	}
	else if (!visited.cur.empty())
	{
		// Manual copy only if element_vector is absent (does not normally occur)
		if (cur_depth < static_cast<int>(index_pairs.size()) && index_pairs[cur_depth].empty())
		{
			index_pairs[cur_depth].assign(visited.cur.begin(), visited.cur.end());
			if (verbose)
			{
				std::cout << "[Manual copy] Copied " << visited.cur.size() << " nodes to depth=" << cur_depth << std::endl;
			}
		}
	}

	// Detach element_vector from next
	if (visited.next.get_element_vector() != nullptr)
	{
		visited.next.detach_element_vector();
		if (next_d < static_cast<int>(index_pairs.size()))
		{
			if (verbose)
			{
				std::cout << "[Element vector] Detached from next at completion (depth=" << next_d
						  << ", recorded=" << index_pairs[next_d].size() << " nodes)" << std::endl;
			}
		}
		else
		{
			if (verbose)
			{
				std::cout << "[Element vector] Detached from next at completion (depth=" << next_d
						  << " is out of range, index_pairs.size()=" << index_pairs.size() << ")" << std::endl;
			}
		}
	}
	else if (!visited.next.empty())
	{
		// Manual copy only if element_vector is absent (does not normally occur)
		if (next_d < static_cast<int>(index_pairs.size()) && index_pairs[next_d].empty())
		{
			index_pairs[next_d].assign(visited.next.begin(), visited.next.end());
			if (verbose)
			{
				std::cout << "[Manual copy] Copied " << visited.next.size() << " nodes to depth=" << next_d << std::endl;
			}
		}
	}

	// Size verification
	if (!visited.cur.empty() && cur_depth < static_cast<int>(index_pairs.size()))
	{
		if (index_pairs[cur_depth].size() != visited.cur.size())
		{
			std::cout << "[Warning] cur size mismatch: vector=" << index_pairs[cur_depth].size()
					  << ", set=" << visited.cur.size() << std::endl;
		}
	}
	if (!visited.next.empty() && next_d < static_cast<int>(index_pairs.size()))
	{
		if (index_pairs[next_d].size() != visited.next.size())
		{
			std::cout << "[Warning] next size mismatch: vector=" << index_pairs[next_d].size()
					  << ", set=" << visited.next.size() << std::endl;
		}
	}

	visited.clear_all();
	return stop ? (next_depth - 1) : next_depth;
}

// ============================================================================
// Step 0: Local Expansion - Bucket size determination functions
// ============================================================================

const size_t MIN_BUCKET = (1 << 21); // 2M (2^21)
const size_t BYTES_PER_BUCKET = 4;	 // robin_hash bucket overhead
const size_t BYTES_PER_NODE = 32;	 // robin_set (24) + index_pairs (8)

#include "expansion_parameters.h" // Depth- and bucket-size-dependent parameter table

// ========================================
// Parameters are managed in expansion_parameters.h
// Optimized for combinations of depth (max_depth) and bucket size
// ========================================

// Calculate total memory usage from bucket size and node count
size_t calculate_memory(size_t bucket_size, size_t node_count)
{
	return bucket_size * BYTES_PER_BUCKET // bucket array
		   + node_count * BYTES_PER_NODE; // nodes + index_pairs
}

// Calculate effective node count from bucket size
size_t calculate_effective_nodes(int max_depth, size_t bucket_size)
{
	return static_cast<size_t>(bucket_size * get_effective_load_factor(max_depth, bucket_size));
}

struct LocalExpansionConfig
{
	bool enable_n1_expansion = false;
	bool enable_n2_expansion = false;
	size_t bucket_n1 = 0;
	size_t bucket_n2 = 0;
	size_t nodes_n1 = 0;
	size_t nodes_n2 = 0;
	size_t bucket_n = 0; // bucket size for depth=n (if retained)
};

// Step 0: Bucket size determination algorithm (1:1 or 1:2 balance, prefer n+1 <= n+2)
LocalExpansionConfig determine_bucket_sizes(
	int max_depth, // BFS maximum depth (used for parameter selection)
	size_t remaining_memory_bytes,
	size_t bucket_n_current,
	size_t cur_set_memory_bytes, // memory usage of depth=n (cur_set)
	bool verbose = true)
{
	LocalExpansionConfig config;
	config.bucket_n = bucket_n_current;

	if (verbose)
	{
		std::cout << "\n=== Step 0: Local Expansion Configuration ===" << std::endl;
		std::cout << "Remaining memory: " << (remaining_memory_bytes / 1024.0 / 1024.0) << " MB" << std::endl;
		std::cout << "Current depth=n bucket size: " << (bucket_n_current >> 20) << "M" << std::endl;
	}

	// Account for memory if keeping the depth=n bucket
	size_t memory_for_depth_n_bucket = bucket_n_current * BYTES_PER_BUCKET;
	size_t available_memory = remaining_memory_bytes - memory_for_depth_n_bucket;

	// Candidate bucket ratios for n+1 and n+2: only 1:1 or 1:2 (prefer n+1 <= n+2)
	struct BucketRatio
	{
		size_t bucket_n1;
		size_t bucket_n2;
		const char *ratio_name;
	};

	std::vector<BucketRatio> candidates;

	//  1:2 ratio (recommended when n+2 is large)
	candidates.push_back({(1 << 25), (1 << 26), "1:2 (32M:64M)"}); // 32M:64M
	candidates.push_back({(1 << 24), (1 << 25), "1:2 (16M:32M)"}); // 16M:32M
	candidates.push_back({(1 << 23), (1 << 24), "1:2 (8M:16M)"});  // 8M:16M
	candidates.push_back({(1 << 22), (1 << 23), "1:2 (4M:8M)"});   // 4M:8M
	candidates.push_back({(1 << 21), (1 << 22), "1:2 (2M:4M)"});   // 2M:4M

	// 1:1 ratio
	candidates.push_back({(1 << 26), (1 << 26), "1:1 (64M:64M)"}); // 64M:64M
	candidates.push_back({(1 << 25), (1 << 25), "1:1 (32M:32M)"}); // 32M:32M
	candidates.push_back({(1 << 24), (1 << 24), "1:1 (16M:16M)"}); // 16M:16M
	candidates.push_back({(1 << 23), (1 << 23), "1:1 (8M:8M)"});   // 8M:8M
	candidates.push_back({(1 << 22), (1 << 22), "1:1 (4M:4M)"});   // 4M:4M
	candidates.push_back({(1 << 21), (1 << 21), "1:1 (2M:2M)"});   // 2M:2M

	BucketRatio best_config = {0, 0, "none"};
	size_t best_total_nodes = 0;

	for (const auto &candidate : candidates)
	{
		// Calculate using effective load to avoid rehash
		// Retrieve parameters based on depth and bucket size
		auto params_n1 = g_expansion_params.get(max_depth, candidate.bucket_n1);
		auto params_n2 = g_expansion_params.get(max_depth, candidate.bucket_n2);

		double load_n1 = params_n1.effective_load_factor;
		double load_n2 = params_n2.backtrace_load_factor;

		size_t nodes_n1 = static_cast<size_t>(candidate.bucket_n1 * load_n1);
		size_t nodes_n2 = static_cast<size_t>(candidate.bucket_n2 * load_n2);

		// Compute required memory (assuming no rehash)
		size_t memory_n1 = nodes_n1 * 24 + candidate.bucket_n1 * 4;
		size_t memory_n2 = nodes_n2 * 24 + candidate.bucket_n2 * 4 + (nodes_n2 / 12) * 8; // includes sampled_nodes

		// Apply empirical safety factor (calculate against total memory including cur_set memory)
		// Use n+2's memory factor (the side that consumes more memory)
		double memory_factor = params_n2.measured_memory_factor;
		size_t total_memory = cur_set_memory_bytes + memory_n1 + memory_n2;
		size_t predicted_rss = static_cast<size_t>(total_memory * memory_factor);

		// Safety margin: within 98%
		if (predicted_rss <= static_cast<size_t>(available_memory * 0.98))
		{
			size_t total_nodes = nodes_n1 + nodes_n2;
			if (total_nodes > best_total_nodes)
			{
				best_total_nodes = total_nodes;
				best_config = candidate;
			}
		}
	}

	if (best_config.bucket_n1 > 0 && best_config.bucket_n2 > 0)
	{
		// n+2 is possible
		auto params_n1 = g_expansion_params.get(max_depth, best_config.bucket_n1);
		auto params_n2 = g_expansion_params.get(max_depth, best_config.bucket_n2);

		config.enable_n1_expansion = true;
		config.enable_n2_expansion = true;
		config.bucket_n1 = best_config.bucket_n1;
		config.bucket_n2 = best_config.bucket_n2;
		config.nodes_n1 = static_cast<size_t>(best_config.bucket_n1 * params_n1.effective_load_factor);
		config.nodes_n2 = static_cast<size_t>(best_config.bucket_n2 * params_n2.backtrace_load_factor);

		if (verbose)
		{
			std::cout << "\nSelected bucket ratio: " << best_config.ratio_name << std::endl;
		}
	}
	else
	{
		// n+2 not possible, try n+1 only (2M, 4M, 8M, 16M, 32M, 64M)
		for (size_t bucket : {(1 << 21), (1 << 22), (1 << 23), (1 << 24), (1 << 25), (1 << 26)})
		{ // 2M, 4M, 8M, 16M, 32M, 64M
			auto params = g_expansion_params.get(max_depth, bucket);
			double load = params.effective_load_factor;
			size_t nodes = static_cast<size_t>(bucket * load);
			size_t memory = nodes * 24 + bucket * 4;
			size_t total_memory = cur_set_memory_bytes + memory;
			size_t predicted_rss = static_cast<size_t>(total_memory * params.measured_memory_factor);

			if (predicted_rss <= static_cast<size_t>(available_memory * 0.98))
			{
				config.enable_n1_expansion = true;
				config.enable_n2_expansion = false;
				config.bucket_n1 = bucket;
				config.nodes_n1 = nodes;
				config.bucket_n2 = 0;
				config.nodes_n2 = 0;
				break;
			}
		}

		if (!config.enable_n1_expansion)
		{
			// n+1 also not possible
			config.enable_n1_expansion = false;
			config.enable_n2_expansion = false;
		}
	}

	// Display results
	if (verbose)
	{
		std::cout << "\nConfiguration:" << std::endl;
		if (config.enable_n1_expansion)
		{
			std::cout << "  depth=n+1: bucket=" << (config.bucket_n1 >> 20) << "M"
					  << ", nodes≈" << (config.nodes_n1 / 1e6) << "M (no rehash)" << std::endl;
		}
		else
		{
			if (verbose)
			{
				std::cout << "  depth=n+1: NOT POSSIBLE" << std::endl;
			}
		}

		if (config.enable_n2_expansion)
		{
			if (verbose)
			{
				std::cout << "  depth=n+2: bucket=" << (config.bucket_n2 >> 20) << "M"
						  << ", nodes≈" << (config.nodes_n2 / 1e6) << "M (no rehash)" << std::endl;
			}
		}
		else
		{
			if (verbose)
			{
				std::cout << "  depth=n+2: NOT POSSIBLE" << std::endl;
			}
		}

		if (verbose)
		{
			std::cout << "============================================\n"
					  << std::endl;
		}
	}

	return config;
}

// ============================================================================
// Step 1-6: Local Expansion implementation
// ============================================================================

// Step 1: Determine the number of nodes N that can be expanded at depth n+1
// Note: Already decided in Step 0 (config.nodes_n1)
// This function is provided for clarity
size_t get_n1_capacity(const LocalExpansionConfig &config)
{
	return config.nodes_n1;
}

// Step 2: Random expansion for depth n+1
void local_expand_step2_random_n1(
	const LocalExpansionConfig &config,
	const tsl::robin_set<uint64_t> &depth_n_nodes,
	tsl::robin_set<uint64_t> &depth_n1_nodes,
	const std::vector<int> &table1,
	const std::vector<int> &table2,
	const std::vector<int> &table3,
	int size2,
	int size3,
	std::vector<std::vector<uint64_t>> &index_pairs, // for element_vector
	int depth_n,									 // current depth
	bool verbose = true)
{
	if (!config.enable_n1_expansion)
	{
		if (verbose)
		{
			std::cout << "\n=== Step 2: Skipped (n+1 expansion not enabled) ===" << std::endl;
		}
		return;
	}

	if (verbose)
	{
		std::cout << "\n=== Step 2: Random Expansion for depth=n+1 ===" << std::endl;
		std::cout << "Target nodes: " << config.nodes_n1 << " (bucket=" << (config.bucket_n1 >> 20) << "M)" << std::endl;
		std::cout << "Depth=n nodes available: " << depth_n_nodes.size() << std::endl;
	}

	// Prepare robin_set for depth=n+1
	depth_n1_nodes.clear();
	depth_n1_nodes = tsl::robin_set<uint64_t>(); // fully release
	depth_n1_nodes.max_load_factor(0.9f);

	// Explicitly set bucket count (prevent rehash)
	depth_n1_nodes.rehash(config.bucket_n1);

	// Attach element_vector (automatically record depth=n+1 nodes)
	int depth_n1 = depth_n + 1;

	// Resize index_pairs if needed
	if (index_pairs.size() <= static_cast<size_t>(depth_n1))
	{
		index_pairs.resize(depth_n1 + 1);
		if (verbose)
		{
			std::cout << "  [Step 2] Resized index_pairs to " << index_pairs.size() << std::endl;
		}
	}

	index_pairs[depth_n1].clear();
	depth_n1_nodes.attach_element_vector(&index_pairs[depth_n1]);
	if (verbose)
	{
		std::cout << "  [Step 2] Element vector attached to depth=" << depth_n1 << std::endl;
	}
	// Prepare random selection (reuse static RNG)
	static std::random_device rd;
	static std::mt19937 gen(rd());

	// Convert depth=n nodes to vector (for random access)
	// Reserve capacity with reserve()
	std::vector<uint64_t> depth_n_vec;
	depth_n_vec.reserve(depth_n_nodes.size());
	depth_n_vec.assign(depth_n_nodes.begin(), depth_n_nodes.end());

	std::uniform_int_distribution<size_t> dist(0, depth_n_vec.size() - 1);

	// Initial estimate: N/10 (overestimate)
	size_t initial_estimate = std::max<size_t>(1, config.nodes_n1 / 10);
	size_t processed_parents = 0;
	uint64_t size23 = static_cast<uint64_t>(size2 * size3);

	// Set an upper limit for safety (up to all depth-n nodes)
	const size_t max_parent_nodes = depth_n_vec.size();

	if (verbose)
	{
		std::cout << "Strategy: Expand until rehash (initial estimate: ~" << initial_estimate << " parents)" << std::endl;
		std::cout << "Note: Duplicate selection is ignored (negligible probability)" << std::endl;
	}

	// Define variables outside the loop for reuse
	size_t random_idx;
	uint64_t parent_node;
	int cur_index1, cur_index2, cur_index3;
	uint64_t cur_index23;
	int next_index1, next_index2, next_index3, next_index23;
	uint64_t next_node;

	// Randomly select parent nodes and expand (until just before rehash)
	const size_t last_bucket_count = depth_n1_nodes.bucket_count();

	while (processed_parents < max_parent_nodes)
	{
		// Select a parent node at random
		random_idx = dist(gen);
		parent_node = depth_n_vec[random_idx];

		// Decompose parent node index
		cur_index1 = parent_node / size23;
		cur_index23 = parent_node % size23;
		cur_index2 = cur_index23 / size3;
		cur_index3 = cur_index23 % size3;

		// Try all 18 move transitions
		for (int move = 0; move < 18; ++move)
		{
			next_index1 = table1[cur_index1 * 18 + move];
			next_index2 = table2[cur_index2 * 18 + move];
			next_index3 = table3[cur_index3 * 18 + move];
			next_index23 = next_index2 * size3 + next_index3;
			next_node = next_index1 * size23 + next_index23;

			// Skip known nodes (find() is more efficient)
			if (depth_n_nodes.find(next_node) != depth_n_nodes.end() ||
				depth_n1_nodes.find(next_node) != depth_n1_nodes.end())
			{
				continue;
			}

			// Rehash check (before insert)
			if (depth_n1_nodes.will_rehash_on_next_insert())
			{
				if (verbose)
				{
					std::cout << "Rehash threshold reached (predicted), stopping expansion" << std::endl;
					std::cout << "Final n+1 nodes: " << depth_n1_nodes.size() << std::endl;
				}
				goto step2_end;
			}

			depth_n1_nodes.insert(next_node);

			// Rehash check (verify that the number of buckets has not changed after insertion)
			const size_t current_bucket_count = depth_n1_nodes.bucket_count();
			if (current_bucket_count != last_bucket_count)
			{
				if (verbose)
				{
					std::cout << "Rehash detected (bucket count changed: "
							  << last_bucket_count << " -> " << current_bucket_count
							  << "), stopping expansion" << std::endl;
					std::cout << "Final n+1 nodes: " << depth_n1_nodes.size() << std::endl;
				}
				goto step2_end;
			}
		}

		processed_parents++;

		// Progress display
		if (verbose && processed_parents % 100000 == 0)
		{
			if (verbose)
			{
				std::cout << "  Processed: " << processed_parents << " parents, "
						  << "Generated: " << depth_n1_nodes.size() << " n+1 nodes"
						  << " (load: " << (100.0 * depth_n1_nodes.size() / depth_n1_nodes.bucket_count()) << "%)" << std::endl;
			}
		}
	}

step2_end:
	// Detach element_vector
	if (depth_n1_nodes.get_element_vector() != nullptr)
	{
		depth_n1_nodes.detach_element_vector();
		if (verbose)
		{
			std::cout << "[Step 2] Element vector detached from depth=" << (depth_n + 1) << std::endl;
			std::cout << "  Recorded " << index_pairs[depth_n + 1].size() << " nodes in vector" << std::endl;
		}
		if (index_pairs[depth_n + 1].size() != depth_n1_nodes.size())
		{
			std::cout << "  ⚠ WARNING: Size mismatch! vector=" << index_pairs[depth_n + 1].size()
					  << ", set=" << depth_n1_nodes.size() << std::endl;
		}
		else
		{
			if (verbose)
			{
				std::cout << "  ✓ Perfect match: " << index_pairs[depth_n + 1].size() << " nodes" << std::endl;
			}
		}
	}

	if (verbose)
	{
		std::cout << "Step 2 complete: " << depth_n1_nodes.size() << " nodes at depth=n+1" << std::endl;
		std::cout << "  Bucket usage: " << depth_n1_nodes.size() << " / "
				  << depth_n1_nodes.bucket_count()
				  << " (" << (100.0 * depth_n1_nodes.size() / depth_n1_nodes.bucket_count()) << "%)" << std::endl;
	}
}

// Step 3: Data Reorganization
void local_expand_step3_reorganize(
	const LocalExpansionConfig &config,
	tsl::robin_set<uint64_t> &prev,
	tsl::robin_set<uint64_t> &cur,
	tsl::robin_set<uint64_t> &next,
	std::vector<std::vector<uint64_t>> &index_pairs,
	int depth_n,
	bool verbose = true)
{
	if (verbose)
	{
		std::cout << "\n=== Step 3: Data Reorganization ===" << std::endl;
		std::cout << "Current state:" << std::endl;
		std::cout << "  prev (depth=" << (depth_n - 1) << "): " << prev.size() << " nodes" << std::endl;
		std::cout << "  cur (depth=" << depth_n << "): " << cur.size() << " nodes" << std::endl;
		std::cout << "  next (depth=" << (depth_n + 1) << "): " << next.size() << " nodes" << std::endl;
	}

	// Move prev (depth=n-1) into index_pairs
	if (!prev.empty())
	{
		// Reserve capacity in advance (prevent reallocation)
		index_pairs[depth_n - 1].reserve(prev.size());
		index_pairs[depth_n - 1].assign(prev.begin(), prev.end());
		// Release unused capacity
		index_pairs[depth_n - 1].shrink_to_fit();

		prev.clear();
		prev = tsl::robin_set<uint64_t>();
		prev.max_load_factor(0.9f);

		if (verbose)
		{
			std::cout << "Moved depth=" << (depth_n - 1) << " to index_pairs ("
					  << index_pairs[depth_n - 1].size() << " nodes)" << std::endl;
		}
	}

	// Handle cur (depth=n)
	if (config.enable_n2_expansion)
	{
		// If expanding n+2: keep as a robin_set (for backtracing)
		// Move cur to prev (efficient via move semantics)
		prev = std::move(cur);
		cur = tsl::robin_set<uint64_t>();
		cur.max_load_factor(0.9f);

		if (verbose)
		{
			std::cout << "Kept depth=" << depth_n << " as robin_set for backtracing ("
					  << prev.size() << " nodes, "
					  << (prev.bucket_count() * 4 / 1024.0 / 1024.0) << " MB buckets)" << std::endl;
		}
	}
	else
	{
		// n+2 If not expanded: Move to index_pairs
		index_pairs[depth_n].reserve(cur.size());
		index_pairs[depth_n].assign(cur.begin(), cur.end());
		index_pairs[depth_n].shrink_to_fit();

		cur.clear();
		cur = tsl::robin_set<uint64_t>();
		cur.max_load_factor(0.9f);

		if (verbose)
		{
			std::cout << "Moved depth=" << depth_n << " to index_pairs ("
					  << index_pairs[depth_n].size() << " nodes)" << std::endl;
		}
	}

	// Move next (depth=n+1) to cur
	cur = std::move(next);
	next.clear();
	next = tsl::robin_set<uint64_t>();
	next.max_load_factor(0.9f);

	if (verbose)
	{
		std::cout << "Moved depth=" << (depth_n + 1) << " to cur ("
				  << cur.size() << " nodes)" << std::endl;

		std::cout << "\nAfter reorganization:" << std::endl;
		std::cout << "  prev (depth=" << depth_n << "): " << prev.size() << " nodes"
				  << (config.enable_n2_expansion ? " [robin_set]" : " [empty]") << std::endl;
		std::cout << "  cur (depth=" << (depth_n + 1) << "): " << cur.size() << " nodes" << std::endl;
		std::cout << "  next (depth=" << (depth_n + 2) << "): " << next.size() << " nodes [empty]" << std::endl;
	}
}

// Step 4: Calculate depth=n+2 Capacity
size_t local_expand_step4_calculate_n2_capacity(
	int max_depth, // BFS maximum depth (used for parameter retrieval)
	const LocalExpansionConfig &config,
	const tsl::robin_set<uint64_t> &prev,
	const tsl::robin_set<uint64_t> &cur,
	const std::vector<std::vector<uint64_t>> &index_pairs,
	size_t total_memory_limit,
	bool verbose = true)
{
	if (!config.enable_n2_expansion)
	{
		if (verbose)
		{
			std::cout << "\n=== Step 4: Skipped (n+2 expansion not enabled) ===" << std::endl;
		}
		return 0;
	}

	if (verbose)
	{
		std::cout << "\n=== Step 4: Calculate depth=n+2 Capacity ===" << std::endl;
	}

	// Measure current memory usage
	size_t current_memory = 0;

	// Memory of index_pairs
	for (size_t d = 0; d < index_pairs.size(); ++d)
	{
		current_memory += index_pairs[d].size() * 8;
	}

	// Memory of robin_set
	current_memory += prev.size() * 24 + prev.bucket_count() * 4;
	current_memory += cur.size() * 24 + cur.bucket_count() * 4;

	if (verbose)
	{
		std::cout << "Current memory usage: " << (current_memory / 1024.0 / 1024.0) << " MB" << std::endl;
		std::cout << "  index_pairs: " << ((index_pairs.size() > 0 ? index_pairs[0].size() : 0) * 8 / 1024.0 / 1024.0);
		for (size_t d = 1; d < index_pairs.size(); ++d)
		{
			std::cout << " + " << (index_pairs[d].size() * 8 / 1024.0 / 1024.0);
		}
		std::cout << " = " << ([&]()
							   {
            size_t total = 0;
            for (size_t d = 0; d < index_pairs.size(); ++d) total += index_pairs[d].size() * 8;
            return total / 1024.0 / 1024.0; })()
				  << " MB" << std::endl;
		if (verbose)
		{
			std::cout << "  prev (robin_set): " << (prev.size() * 24 / 1024.0 / 1024.0)
					  << " MB nodes + " << (prev.bucket_count() * 4 / 1024.0 / 1024.0)
					  << " MB buckets" << std::endl;
		}
		if (verbose)
		{
			std::cout << "  cur (robin_set): " << (cur.size() * 24 / 1024.0 / 1024.0)
					  << " MB nodes + " << (cur.bucket_count() * 4 / 1024.0 / 1024.0)
					  << " MB buckets" << std::endl;
		}
	}

	// Calculate remaining memory
	size_t remaining_memory = (total_memory_limit > current_memory)
								  ? (total_memory_limit - current_memory)
								  : 0;

	if (verbose)
	{
		std::cout << "Total memory limit: " << (total_memory_limit / 1024.0 / 1024.0) << " MB" << std::endl;
		std::cout << "Remaining memory: " << (remaining_memory / 1024.0 / 1024.0) << " MB" << std::endl;
	}

	// Apply safety factor (20% margin)
	size_t safe_remaining = static_cast<size_t>(remaining_memory * 0.8);

	if (verbose)
	{
		std::cout << "Safe remaining (80%): " << (safe_remaining / 1024.0 / 1024.0) << " MB" << std::endl;
	}

	// Determine bucket size (use Step 0 settings)
	size_t bucket_n2 = config.bucket_n2;

	// Retrieve depth- and bucket-size-dependent parameters from the parameter table
	auto params_n2 = g_expansion_params.get(max_depth, bucket_n2);

	// Use the effective load factor for Step 5 backtrace expansion (measured under random sampling)
	// To avoid rehashing, keep below this load factor
	double effective_load = params_n2.backtrace_load_factor;
	size_t M = static_cast<size_t>(bucket_n2 * effective_load);

	// Calculate required memory (calculated memory base)
	// Assume no rehash will occur (M is within effective_load)
	size_t calculated_memory = M * 24 + bucket_n2 * 4; // robin_set (no rehash)

	// Also account for sampled_nodes vector memory (M/12 nodes)
	size_t sampled_nodes_memory = (M / 12) * sizeof(uint64_t);
	calculated_memory += sampled_nodes_memory;

	// Apply empirical safety factor to predict actual RSS usage
	double memory_factor = params_n2.measured_memory_factor;
	size_t predicted_rss = static_cast<size_t>(calculated_memory * memory_factor);

	if (verbose)
	{
		std::cout << "\nCapacity calculation:" << std::endl;
		std::cout << "  Bucket size (n+2): " << (bucket_n2 >> 20) << "M" << std::endl;
		std::cout << "  Effective load factor: " << (effective_load * 100) << "% (Step 5 backtrace, no rehash)" << std::endl;
		std::cout << "  Capacity M: " << M << " nodes (" << (M / 1e6) << "M)" << std::endl;
		std::cout << "  Calculated memory (no rehash): " << (calculated_memory / 1024.0 / 1024.0) << " MB" << std::endl;
		std::cout << "  Predicted RSS (×" << memory_factor << "): "
				  << (predicted_rss / 1024.0 / 1024.0) << " MB" << std::endl;
		std::cout << "  Safety check: " << (predicted_rss <= safe_remaining ? "PASS" : "FAIL") << std::endl;
	}

	if (predicted_rss > safe_remaining)
	{
		// If memory is insufficient, reduce the bucket size.
		if (verbose)
		{
			std::cout << "Warning: Not enough memory for planned bucket size" << std::endl;
			std::cout << "Reducing bucket size..." << std::endl;
		}

		// Try smaller bucket sizes (consider down to 4M)
		size_t fallback_bucket = bucket_n2 / 2;
		while (fallback_bucket >= (1 << 22))
		{ // 4M or more
			auto fallback_params = g_expansion_params.get(max_depth, fallback_bucket);
			double fallback_load = fallback_params.backtrace_load_factor;
			size_t fallback_M = static_cast<size_t>(fallback_bucket * fallback_load);
			size_t fallback_calc = fallback_M * 24 + fallback_bucket * 4 + (fallback_M / 12) * 8; // no rehash
			size_t fallback_rss = static_cast<size_t>(fallback_calc * fallback_params.measured_memory_factor);

			if (fallback_rss <= safe_remaining)
			{
				bucket_n2 = fallback_bucket;
				M = fallback_M;
				predicted_rss = fallback_rss;
				params_n2 = fallback_params; // Update used parameters

				if (verbose)
				{
					std::cout << "  Adjusted to: " << (bucket_n2 >> 20) << "M bucket, M="
							  << M << " nodes (no rehash)" << std::endl;
					std::cout << "  Predicted RSS: " << (predicted_rss / 1024.0 / 1024.0) << " MB" << std::endl;
				}
				break;
			}

			fallback_bucket /= 2;
		}

		if (fallback_bucket < (1 << 22))
		{ // Less than 4M
			if (verbose)
			{
				std::cout << "  Error: Cannot allocate minimum bucket size (4M)" << std::endl;
			}
			return 0;
		}
	}

	if (verbose)
	{
		std::cout << "\nFinal capacity: M = " << M << " nodes" << std::endl;
		std::cout << "Expansion target: up to M nodes (will stop when reaching M or running out of parents)" << std::endl;
	}

	return M;
}

// ============================================================================
// Step 5: Sequential expansion at depth n+2 (with backtrace)
// ============================================================================

struct BacktraceExpansionResult
{
	size_t generated_nodes;	   // number of generated depth=n+2 nodes
	size_t processed_parents;  // number of depth=n+1 parents processed
	size_t rejected_n1_nodes;  // nodes rejected as depth=n+1 by backtrace
	size_t duplicate_n1_nodes; // nodes already present in depth_n1_nodes (sibling nodes)
	size_t duplicate_n2_nodes; // nodes already present in depth_n2_nodes
	size_t total_candidates;   // total candidate nodes generated
	bool stopped_by_rehash;	   // whether stopped due to rehash
	bool stopped_by_limit;	   // whether stopped due to M/12 limit
};

// Expand from depth n+1 to depth n+2 (filter using backtrace to depth n)
BacktraceExpansionResult expand_with_backtrace_filter(
	const LocalExpansionConfig &config,
	size_t capacity,						  // target capacity
	tsl::robin_set<uint64_t> &depth_n_nodes,  // depth=n (for backtrace)
	tsl::robin_set<uint64_t> &depth_n1_nodes, // depth=n+1 (input)
	tsl::robin_set<uint64_t> &depth_n2_nodes, // depth=n+2 (output, element_vector already attached)
	const std::vector<int> &multi_move_table_cross_edges,
	const std::vector<int> &multi_move_table_F2L_slots_edges,
	const std::vector<int> &multi_move_table_F2L_slots_corners,
	int size1, // size of cross_edges
	int size2, // size of F2L_slots_edges
	int size3, // size of F2L_slots_corners
	bool verbose = true)
{
	BacktraceExpansionResult result = {0, 0, 0, 0, 0, 0, false, false};

	if (verbose)
	{
		std::cout << "\n=== Backtrace Expansion (depth=n+2) ===" << std::endl;
		std::cout << "Target capacity: " << capacity << " nodes" << std::endl;
		std::cout << "Available depth=n+1 nodes: " << depth_n1_nodes.size() << std::endl;
	}

	// depth_n2_nodes is initialized by the caller (with element_vector attached)
	// Do not reinitialize!

	if (verbose)
	{
		std::cout << "Using pre-initialized depth=n+2 set:" << std::endl;
		std::cout << "  Bucket count: " << depth_n2_nodes.bucket_count() << std::endl;
		std::cout << "  Max load factor: " << depth_n2_nodes.max_load_factor() << std::endl;
		std::cout << "  Load threshold: " << depth_n2_nodes.load_threshold() << std::endl;

		// element_vector information
		if (depth_n2_nodes.get_element_vector() != nullptr)
		{
			std::cout << "  ✓ Element vector attached: " << depth_n2_nodes.get_element_vector() << std::endl;
			if (verbose)
			{
				std::cout << "    Initial vector capacity: " << depth_n2_nodes.get_element_vector()->capacity() << std::endl;
			}
		}
		else
		{
			if (verbose)
			{
				std::cout << "  ✗ Element vector NOT attached (recording disabled)" << std::endl;
			}
		}
	}

	size_t expansion_limit = capacity; // upper limit of generated nodes (not parent nodes)
	size_t report_interval = 10000;
	size_t next_report = report_interval;

	// Calculate size23: corners * edges
	int size23 = size3 * size2; // F2L_corners * F2L_edges

	// Random sampling: collect all nodes from depth_n1_nodes and shuffle
	std::vector<uint64_t> sampled_nodes;
	sampled_nodes.reserve(depth_n1_nodes.size());

	if (verbose)
	{
		std::cout << "Collecting all " << depth_n1_nodes.size() << " nodes from depth=n+1..." << std::endl;
	}

	for (auto it = depth_n1_nodes.begin(); it != depth_n1_nodes.end(); ++it)
	{
		sampled_nodes.push_back(*it);
	}

	// Shuffle to randomize order
	std::mt19937_64 rng(std::random_device{}());

	// Shuffle the sampled nodes (further randomization)
	std::shuffle(sampled_nodes.begin(), sampled_nodes.end(), rng);

	if (verbose)
	{
		std::cout << "Sampled and shuffled " << sampled_nodes.size() << " nodes" << std::endl;
		std::cout << "Processing with randomized order..." << std::endl;
		std::cout << "  size1=" << size1 << ", size2=" << size2 << ", size3=" << size3 << std::endl;
		std::cout << "  size23=" << size23 << std::endl;
	}

	// Process the sampled nodes
	size_t processed_count = 0;
	// size_t debug_limit = 1000;  // Debug release
	for (uint64_t node : sampled_nodes)
	{
		// if (processed_count >= debug_limit) {
		//     if (verbose) {
		//         std::cout << "\n*** DEBUG: Stopping at " << debug_limit << " nodes for testing ***" << std::endl;
		//     }
		//     break;
		// }

		// Early debug: boundary check for the first few nodes
		// if (verbose && processed_count < 5) {
		//     std::cout << "  Processing node " << processed_count << ": " << node << std::endl;
		// }

		// Decompose node index
		int edge_index = node / size23;
		uint64_t index23 = node % size23;
		int corner_index = index23 / size2;	  // F2L_corners
		int f2l_edge_index = index23 % size2; // F2L_edges

		// if (verbose && processed_count < 5) {
		//     std::cout << "    edge_index=" << edge_index << ", corner_index=" << corner_index
		//               << ", f2l_edge_index=" << f2l_edge_index << std::endl;
		// }

		// Boundary check (at decomposition)
		if (edge_index >= size1 || corner_index >= size3 || f2l_edge_index >= size2)
		{
			// if (verbose && processed_count < 10) {
			//     std::cout << "    SKIP: Out of bounds at decomposition" << std::endl;
			// }
			processed_count++;
			continue;
		}

		// Generate transitions for 18 moves
		for (int move = 0; move < 18; ++move)
		{
			// move_table boundary check
			size_t edge_table_index = static_cast<size_t>(edge_index) * 18 + move;
			size_t corner_table_index = static_cast<size_t>(corner_index) * 18 + move;
			size_t f2l_edge_table_index = static_cast<size_t>(f2l_edge_index) * 18 + move;

			if (edge_table_index >= multi_move_table_cross_edges.size() ||
				corner_table_index >= multi_move_table_F2L_slots_corners.size() ||
				f2l_edge_table_index >= multi_move_table_F2L_slots_edges.size())
			{

				// if (verbose && processed_count < 10) {
				//     std::cout << "    SKIP move " << move << ": Table index out of bounds" << std::endl;
				//     std::cout << "      edge_table_index=" << edge_table_index << " / " << multi_move_table_cross_edges.size() << std::endl;
				//     std::cout << "      corner_table_index=" << corner_table_index << " / " << multi_move_table_F2L_slots_corners.size() << std::endl;
				//     std::cout << "      f2l_edge_table_index=" << f2l_edge_table_index << " / " << multi_move_table_F2L_slots_edges.size() << std::endl;
				// }
				continue;
			}

			int next_edge = multi_move_table_cross_edges[edge_table_index];
			int next_corner = multi_move_table_F2L_slots_corners[corner_table_index];
			int next_f2l_edge = multi_move_table_F2L_slots_edges[f2l_edge_table_index];

			int next_index23 = next_corner * size2 + next_f2l_edge;
			uint64_t next_node = static_cast<uint64_t>(next_edge) * size23 + next_index23;

			result.total_candidates++;

			// Skip if already exists in depth_n1_nodes (sibling nodes)
			if (depth_n1_nodes.count(next_node))
			{
				result.duplicate_n1_nodes++;
				continue;
			}

			// Skip if already exists in depth_n2_nodes
			if (depth_n2_nodes.count(next_node))
			{
				result.duplicate_n2_nodes++;
				continue;
			}

			// Backtrace: check if reachable from depth=n
			bool is_depth_n_plus_1 = false;

			for (int back_move = 0; back_move < 18; ++back_move)
			{
				// Boundary check for backtrace table access
				size_t back_edge_table_index = static_cast<size_t>(next_edge) * 18 + back_move;
				size_t back_corner_table_index = static_cast<size_t>(next_corner) * 18 + back_move;
				size_t back_f2l_edge_table_index = static_cast<size_t>(next_f2l_edge) * 18 + back_move;

				if (back_edge_table_index >= multi_move_table_cross_edges.size() ||
					back_corner_table_index >= multi_move_table_F2L_slots_corners.size() ||
					back_f2l_edge_table_index >= multi_move_table_F2L_slots_edges.size())
				{
					break;
				}

				int parent_edge = multi_move_table_cross_edges[back_edge_table_index];
				int parent_corner = multi_move_table_F2L_slots_corners[back_corner_table_index];
				int parent_f2l_edge = multi_move_table_F2L_slots_edges[back_f2l_edge_table_index];

				int parent_index23 = parent_corner * size2 + parent_f2l_edge;
				uint64_t parent = static_cast<uint64_t>(parent_edge) * size23 + parent_index23;

				// Search depth_n_nodes → O(1)
				if (depth_n_nodes.count(parent))
				{
					// Reachable from depth=n → Confirm depth=n+1
					is_depth_n_plus_1 = true;
					result.rejected_n1_nodes++;
					break;
				}
			}

			if (is_depth_n_plus_1)
			{
				continue; // depth=n+1, so do not insert
			}

			// Confirmed depth=n+2 → Insert
			size_t buckets_before = depth_n2_nodes.bucket_count();

			if (depth_n2_nodes.will_rehash_on_next_insert())
			{
				// Probe distance limit reached - clear flag and continue
				// This allows filling the bucket despite hash collisions
				depth_n2_nodes.clear_grow_flag();

				if (verbose && result.generated_nodes % 100000 == 0)
				{
					if (verbose)
					{
						std::cout << "  Note: Probe distance limit reached at " << result.generated_nodes
								  << " nodes, continuing with degraded performance" << std::endl;
					}
				}
			}

			depth_n2_nodes.insert(next_node);
			result.generated_nodes++;

			// Monitor rehashing (check impact on element_vector)
			size_t buckets_after = depth_n2_nodes.bucket_count();
			if (buckets_after != buckets_before)
			{
				if (verbose)
				{
					std::cout << "  WARNING: Rehash occurred! Buckets: " << buckets_before
							  << " -> " << buckets_after << std::endl;
					if (depth_n2_nodes.get_element_vector() != nullptr)
					{
						std::cout << "    Element vector still attached: " << depth_n2_nodes.get_element_vector() << std::endl;
						std::cout << "    Vector size: " << depth_n2_nodes.get_element_vector()->size()
								  << " (should be " << result.generated_nodes << ")" << std::endl;
					}
				}
			}

			// Stop when capacity is reached (prevent rehash)
			if (result.generated_nodes >= expansion_limit)
			{
				if (verbose)
				{
					std::cout << "\nReached capacity limit (" << expansion_limit << " nodes), stopping expansion" << std::endl;
				}
				result.stopped_by_limit = true;
				return result;
			}
		}

		result.processed_parents++;
		processed_count++;

		// Progress display
		if (verbose && result.processed_parents >= next_report)
		{
			if (verbose)
			{
				std::cout << "  Processed: " << result.processed_parents << " parents"
						  << ", Generated: " << result.generated_nodes << " n+2 nodes"
						  << ", Rejected: " << result.rejected_n1_nodes << " n+1 nodes"
						  << " (load: " << (depth_n2_nodes.size() * 100.0 / depth_n2_nodes.bucket_count()) << "%)" << std::endl;
			}

			// element_vector information
			if (depth_n2_nodes.get_element_vector() != nullptr)
			{
				if (verbose)
				{
					std::cout << "    Element vector: size=" << depth_n2_nodes.get_element_vector()->size()
							  << ", capacity=" << depth_n2_nodes.get_element_vector()->capacity() << std::endl;
				}

				// Size consistency check
				if (depth_n2_nodes.get_element_vector()->size() != result.generated_nodes)
				{
					std::cout << "    ⚠ WARNING: Vector size mismatch! Expected " << result.generated_nodes
							  << " but got " << depth_n2_nodes.get_element_vector()->size() << std::endl;
				}
			}

			next_report += report_interval;
		}
	}

	// All sampled nodes processed
	result.stopped_by_limit = (sampled_nodes.size() >= expansion_limit);

backtrace_expansion_end:
	if (verbose)
	{
		std::cout << "Backtrace expansion complete:" << std::endl;
		std::cout << "  Generated depth=n+2 nodes: " << result.generated_nodes << std::endl;
		std::cout << "  Processed depth=n+1 nodes: " << result.processed_parents << std::endl;
		std::cout << "  Rejected depth=n+1 nodes (backtrace): " << result.rejected_n1_nodes << std::endl;
		std::cout << "  Duplicate in depth=n+1 (siblings): " << result.duplicate_n1_nodes << std::endl;
		if (verbose)
		{
			std::cout << "  Duplicate in depth=n+2: " << result.duplicate_n2_nodes << std::endl;
			std::cout << "  Total candidates generated: " << result.total_candidates << std::endl;
		}
		std::cout << "  Bucket usage: " << depth_n2_nodes.size() << " / " << depth_n2_nodes.bucket_count()
				  << " (" << (depth_n2_nodes.size() * 100.0 / depth_n2_nodes.bucket_count()) << "%)" << std::endl;
		std::cout << "  Stopped by rehash: " << (result.stopped_by_rehash ? "YES" : "NO") << std::endl;
		std::cout << "  Stopped by limit: " << (result.stopped_by_limit ? "YES" : "NO") << std::endl;

		// Final verification of element_vector
		if (depth_n2_nodes.get_element_vector() != nullptr)
		{
			if (verbose)
			{
				std::cout << "\nElement vector final state:" << std::endl;
				std::cout << "  Vector size: " << depth_n2_nodes.get_element_vector()->size() << std::endl;
			}
			std::cout << "  Vector capacity: " << depth_n2_nodes.get_element_vector()->capacity() << std::endl;
			std::cout << "  robin_set size: " << depth_n2_nodes.size() << std::endl;

			if (depth_n2_nodes.get_element_vector()->size() == depth_n2_nodes.size())
			{
				if (verbose)
				{
					std::cout << "  ✓ Perfect match: vector.size() == robin_set.size()" << std::endl;
				}
			}
			else
			{
				std::cout << "  ✗ MISMATCH: difference = "
						  << (int64_t)depth_n2_nodes.size() - (int64_t)depth_n2_nodes.get_element_vector()->size()
						  << std::endl;
			}

			// Estimate memory usage
			size_t vector_memory = depth_n2_nodes.get_element_vector()->capacity() * sizeof(uint64_t);
			size_t robin_memory = depth_n2_nodes.size() * 24 + depth_n2_nodes.bucket_count() * 4;
			std::cout << "  Vector memory: " << (vector_memory / 1024.0 / 1024.0) << " MB" << std::endl;
			std::cout << "  robin_set memory: " << (robin_memory / 1024.0 / 1024.0) << " MB" << std::endl;
			std::cout << "  Total: " << ((vector_memory + robin_memory) / 1024.0 / 1024.0) << " MB" << std::endl;
		}
	}

	return result;
}

// ============================================================================
// Wrapper Functions: Complete Search Database Construction
// ============================================================================

struct SearchDatabaseResult
{
	int reached_depth = 0;			   // Depth reached by BFS
	bool local_expansion_done = false; // Whether local expansion was performed
	size_t n1_nodes = 0;			   // Number of nodes at depth=n+1
	size_t n2_nodes = 0;			   // Number of nodes at depth=n+2
	bool n2_expansion_done = false;	   // Whether expansion at depth=n+2 was completed
};

// Complete Search Database Construction (BFS + Integrated Local Expansion)
// Updates index_pairs and num_list
void build_complete_search_database(
	int index1, int index2, int index3,
	int size1, int size2, int size3,
	int BFS_DEPTH,
	int MEMORY_LIMIT_MB,
	const std::vector<int> &multi_move_table_cross_edges,
	const std::vector<int> &multi_move_table_F2L_slots_edges,
	const std::vector<int> &multi_move_table_F2L_slots_corners,
	std::vector<std::vector<uint64_t>> &index_pairs,
	std::vector<int> &num_list,
	bool verbose = true)
{
	if (verbose)
	{
		std::cout << "\n=== Complete Search Database Construction ===" << std::endl;
		std::cout << "Phase 1: BFS (depth 0 to " << BFS_DEPTH << ")" << std::endl;
		std::cout << "Phase 2: Partial Expansion (depth " << BFS_DEPTH << "+1)" << std::endl;
		std::cout << "Phase 3: Local Expansion depth 7→8 (with depth 6 check)" << std::endl;
		std::cout << "Phase 4: Local Expansion depth 8→9 (with depth 6 distance check)" << std::endl;
		std::cout << "Memory limit: " << MEMORY_LIMIT_MB << " MB" << std::endl;
	}

	// ============================================================================
	// Phase 1: Execute BFS
	// ============================================================================
	int reached_depth = create_prune_table_sparse(
		index1, index2, index3,
		size1, size2, size3,
		BFS_DEPTH,
		1024 * MEMORY_LIMIT_MB,
		multi_move_table_cross_edges,
		multi_move_table_F2L_slots_edges,
		multi_move_table_F2L_slots_corners,
		index_pairs,
		num_list,
		verbose);

	if (verbose)
	{
		std::cout << "\nPhase 1 Complete: BFS reached depth " << reached_depth << std::endl;
		for (int d = 0; d <= reached_depth; ++d)
		{
			std::cout << "  depth=" << d << ": " << index_pairs[d].size() << " nodes" << std::endl;
		}
	}

	// ============================================================================
	// Memory optimization: Release excess reserved capacity (data retained)
	// ============================================================================
	if (reached_depth >= 6)
	{
		size_t freed_capacity = 0;
		for (int d = 0; d <= reached_depth - 2; ++d)
		{
			size_t before_capacity = index_pairs[d].capacity();
			index_pairs[d].shrink_to_fit();
			size_t after_capacity = index_pairs[d].capacity();
			freed_capacity += (before_capacity - after_capacity) * 8;
		}
		if (verbose && freed_capacity > 0)
		{
			std::cout << "\nMemory optimization: freed " << (freed_capacity / 1024.0 / 1024.0)
					  << " MB of unused capacity from depth 0-" << (reached_depth - 2) << std::endl;
		}
	}

	// ============================================================================
	// Phase 2-4: Gradual Local Expansion (depth 7, 8, 9)
	// ============================================================================
	if (reached_depth < 6)
	{
		if (verbose)
		{
			std::cout << "\nPhase 2-4 Skipped: BFS depth < 6" << std::endl;
		}
		return;
	}

	// New Implementation: Phase 2-4 Phased Local Expansion
	// ============================================================================
	if (verbose)
	{
		std::cout << "\n=== New Algorithm: Gradual Local Expansion ===" << std::endl;
		std::cout << "Phase 2: Partial expansion depth 7" << std::endl;
		std::cout << "Phase 3: Local expansion depth 7→8 (depth 6 check)" << std::endl;
		std::cout << "Phase 4: Local expansion depth 8→9 (depth 6 distance check)" << std::endl;
		std::cout << "\n[BFS Complete - RSS Check]" << std::endl;
		std::cout << "RSS after BFS (before depth_6_nodes): " << (get_rss_kb() / 1024.0) << " MB" << std::endl;
	}

	// ============================================================================
	// Preparation: Load depth 6 nodes into robin_set
	// ============================================================================
	tsl::robin_set<uint64_t> depth_6_nodes;
	depth_6_nodes.max_load_factor(0.9f);
	for (uint64_t node : index_pairs[6])
	{
		depth_6_nodes.insert(node);
	}

	if (verbose)
	{
		std::cout << "RSS after depth_6_nodes creation: " << (get_rss_kb() / 1024.0) << " MB" << std::endl;
		std::cout << "depth_6_nodes size: " << depth_6_nodes.size()
				  << ", buckets: " << depth_6_nodes.bucket_count() << std::endl;
	}

	// Memory calculation
	size_t current_memory_bytes = 0;
	for (size_t d = 0; d <= static_cast<size_t>(reached_depth); ++d)
	{
		current_memory_bytes += index_pairs[d].size() * 8;
	}
	current_memory_bytes += depth_6_nodes.size() * 24 + depth_6_nodes.bucket_count() * 4;

	size_t total_memory_limit = static_cast<size_t>(MEMORY_LIMIT_MB) * 1024 * 1024;
	size_t remaining_memory = total_memory_limit - current_memory_bytes;

	// Calculate theoretical memory (index_pairs only)
	size_t theoretical_phase1 = 0;
	for (size_t d = 0; d <= static_cast<size_t>(reached_depth); ++d)
	{
		theoretical_phase1 += index_pairs[d].size() * 8;
	}

	// Get actual RSS after Phase 1 for accurate bucket calculation
	size_t phase1_rss_kb = get_rss_kb();
	size_t phase1_rss_bytes = phase1_rss_kb * 1024;
	size_t actual_remaining_memory = total_memory_limit - phase1_rss_bytes;

	if (verbose)
	{
		std::cout << "\n[Phase 1 Complete - Memory Analysis]" << std::endl;
		std::cout << "Theoretical (index_pairs only): " << (theoretical_phase1 / 1024.0 / 1024.0) << " MB" << std::endl;
		std::cout << "Current memory estimate: " << (current_memory_bytes / 1024.0 / 1024.0) << " MB" << std::endl;
		std::cout << "Overhead ratio: " << (static_cast<double>(current_memory_bytes) / theoretical_phase1) << "x" << std::endl;
		std::cout << "Actual RSS: " << (phase1_rss_kb / 1024.0) << " MB" << std::endl;
		std::cout << "Actual remaining memory: " << (actual_remaining_memory / 1024.0 / 1024.0) << " MB" << std::endl;
	}

	const size_t MIN_BUCKET = (1 << 21); // 2M
	const uint64_t size23 = static_cast<uint64_t>(size2 * size3);

	// ============================================================================
	// Pre-calculate optimal bucket sizes for depth 7, 8, 9
	// ============================================================================
	// NEW STRATEGY: "Focus depth 7,8 (2:2:1)" - Aggressive allocation with Wasm-aware margins
	//
	// Rationale:
	// - Empirical measurements show actual overhead is ~12-14.5 bytes/slot (vs 38 conservative)
	// - Small buckets (2M-4M): 14.5 bytes/slot overhead
	// - Large buckets (8M-16M): 10.8 bytes/slot overhead
	// - Webassembly requires additional margin for:
	//   * Dynamic memory expansion (vector growth, robin_set rehashing)
	//   * Wasm-specific overhead (memory page alignment, etc.)
	//   * Maintaining computation speed on memory-constrained devices
	//
	// Focus on depth 7, 8 for accuracy:
	// - depth 7 large → better depth 8 parent diversity → higher depth 8 accuracy
	// - depth 8 large → better depth 9 parent diversity → good depth 9 accuracy
	// - depth 9 = depth 8 → maintains precision (depth 7 covers most depth 6 children)
	//
	// Allocation ratio: depth_7 : depth_8 : depth_9 = 2 : 2 : 2
	// Total memory: 2*d9 + 2*d9 + 2*d9 = 6*d9

	size_t bucket_7, bucket_8, bucket_9;

	// Use empirical overhead based on bucket size
	// Small buckets (≤4M): 14.5 bytes/slot, Large buckets (≥8M): 10.8 bytes/slot
	// Conservative: use 15 bytes/slot for calculation (vs 38 theoretical)
	const size_t EMPIRICAL_OVERHEAD = 33; // bytes per slot (measured from actual runs)

	// Webassembly-aware safety margin calculation (optimized to ~20%)
	// Accounts for: dynamic expansion, rehash, Wasm overhead
	size_t wasm_safety_margin;
	if (total_memory_limit < 1000 * 1024 * 1024)
	{
		// Small memory (<1GB): 25% of remaining (more conservative)
		wasm_safety_margin = actual_remaining_memory * 25 / 100;
	}
	else if (total_memory_limit < 2000 * 1024 * 1024)
	{
		// Medium memory (1-2GB): 20% of remaining
		wasm_safety_margin = actual_remaining_memory * 20 / 100;
	}
	else
	{
		// Large memory (>2GB): 20% of remaining
		wasm_safety_margin = actual_remaining_memory * 20 / 100;
	}

	// Usable memory = remaining - Wasm margin (minimum 75% of remaining)
	size_t usable_memory;
	if (actual_remaining_memory > wasm_safety_margin)
	{
		usable_memory = actual_remaining_memory - wasm_safety_margin;
	}
	else
	{
		// Fallback: use 75% of remaining when margin is too large
		usable_memory = actual_remaining_memory * 75 / 100;
	}

	// Allocation: d7=2*d9, d8=2*d9, d9=2*d9 (equal d8 and d9)
	// Total slots = 2*d9 + 2*d9 + 2*d9 = 6*d9
	// usable_memory = 6*d9 * EMPIRICAL_OVERHEAD
	// d9 = usable_memory / (6 * EMPIRICAL_OVERHEAD)

	size_t target_d9 = usable_memory / (6 * EMPIRICAL_OVERHEAD);
	size_t target_d7 = target_d9 * 2;
	size_t target_d8 = target_d9 * 2;
	size_t target_d9_actual = target_d9 * 2;  // 2:2:2 ratio

	// Round to power of 2, minimum 2M for reasonable coverage
	// Strategy: Round up if value >= 1.5x current (more aggressive utilization)
	auto round_to_power_of_2 = [](size_t value, size_t min_val, size_t max_val) -> size_t
	{
		size_t result = min_val;
		while (result < max_val)
		{
			size_t next = result * 2;
			// Upgrade to next power of 2 if value >= 1.5x current
			if (value >= result + result / 2 && next <= max_val)
			{
				result = next;
			}
			else
			{
				break;
			}
		}
		return result;
	};

	// Step 1: Calculate baseline bucket sizes (2:2:2 ratio)
	bucket_7 = round_to_power_of_2(target_d7, MIN_BUCKET, (1 << 25)); // 2M-32M
	bucket_8 = round_to_power_of_2(target_d8, MIN_BUCKET, (1 << 25)); // 2M-32M
	bucket_9 = round_to_power_of_2(target_d9_actual, MIN_BUCKET, (1 << 25)); // 2M-32M (2:2:2 ratio)
	
	// Step 2: Flexible allocation - upgrade buckets if memory budget allows
	size_t baseline_memory = (bucket_7 + bucket_8 + bucket_9) * EMPIRICAL_OVERHEAD;
	size_t remaining_budget = (baseline_memory < usable_memory) ? (usable_memory - baseline_memory) : 0;
	
	// Try to upgrade depth 8 first (typically larger than depth 9)
	if (remaining_budget >= bucket_8 * EMPIRICAL_OVERHEAD && bucket_8 < (1 << 25)) {
		size_t old_bucket_8 = bucket_8;
		size_t new_bucket_8 = bucket_8 * 2;
		if (new_bucket_8 <= (1 << 25)) {  // Max 32M
			bucket_8 = new_bucket_8;
			remaining_budget -= old_bucket_8 * EMPIRICAL_OVERHEAD;
		}
	}
	
	// Then try to upgrade depth 9
	if (remaining_budget >= bucket_9 * EMPIRICAL_OVERHEAD && bucket_9 < (1 << 25)) {
		size_t old_bucket_9 = bucket_9;
		size_t new_bucket_9 = bucket_9 * 2;
		if (new_bucket_9 <= (1 << 25)) {  // Max 32M
			bucket_9 = new_bucket_9;
			remaining_budget -= old_bucket_9 * EMPIRICAL_OVERHEAD;
		}
	}
	
	// Optionally upgrade depth 7 if still budget remains
	if (remaining_budget >= bucket_7 * EMPIRICAL_OVERHEAD && bucket_7 < (1 << 25)) {
		size_t new_bucket_7 = bucket_7 * 2;
		if (new_bucket_7 <= (1 << 25)) {  // Max 32M
			bucket_7 = new_bucket_7;
		}
	}

	if (verbose)
	{
		std::cout << "\n[Pre-calculated Bucket Allocation - Flexible Strategy]" << std::endl;
		std::cout << "Strategy: 2:2:2 baseline with flexible upgrades based on available memory" << std::endl;
		std::cout << "Empirical overhead: " << EMPIRICAL_OVERHEAD << " bytes/slot (vs 38 conservative)" << std::endl;
		std::cout << "Available memory: " << (actual_remaining_memory / 1024.0 / 1024.0) << " MB" << std::endl;
		std::cout << "Wasm safety margin: " << (wasm_safety_margin / 1024.0 / 1024.0) << " MB" << std::endl;
		std::cout << "Usable for buckets: " << (usable_memory / 1024.0 / 1024.0) << " MB" << std::endl;
		std::cout << "  depth 7 bucket: " << bucket_7 << " (" << (bucket_7 / (1 << 20)) << "M)" << std::endl;
		std::cout << "  depth 8 bucket: " << bucket_8 << " (" << (bucket_8 / (1 << 20)) << "M)" << std::endl;
		std::cout << "  depth 9 bucket: " << bucket_9 << " (" << (bucket_9 / (1 << 20)) << "M)" << std::endl;

		size_t estimated_total = (bucket_7 + bucket_8 + bucket_9) * EMPIRICAL_OVERHEAD;
		std::cout << "Estimated peak memory (empirical): " << (estimated_total / 1024.0 / 1024.0) << " MB" << std::endl;
		std::cout << "Phase 2 baseline + peak: " << ((120 * 1024 * 1024 + estimated_total) / 1024.0 / 1024.0) << " MB" << std::endl;
	}

	// ============================================================================
	// Phase 2: depth 7 Partial expansion
	// ============================================================================
	if (verbose)
	{
		std::cout << "\n--- Phase 2: Partial Expansion (depth 7) ---" << std::endl;
		std::cout << "Using pre-calculated bucket size for depth 7: " << bucket_7
				  << " (" << (bucket_7 / (1 << 20)) << "M)" << std::endl;
		std::cout << "RSS before depth_7_nodes creation: " << (get_rss_kb() / 1024.0) << " MB" << std::endl;
	}

	// Generate depth 7 nodes
	if (index_pairs.size() <= 7)
	{
		index_pairs.resize(8);
	}
	index_pairs[7].clear();

	tsl::robin_set<uint64_t> depth_7_nodes(bucket_7);
	depth_7_nodes.max_load_factor(0.9f);
	depth_7_nodes.attach_element_vector(&index_pairs[7]);

	if (verbose)
	{
		std::cout << "RSS after depth_7_nodes creation (empty): " << (get_rss_kb() / 1024.0) << " MB" << std::endl;
	}

	// Adaptive children per parent for depth 6→7
	std::vector<uint64_t> depth6_vec(depth_6_nodes.begin(), depth_6_nodes.end());
	std::mt19937_64 rng_d7(12345);
	std::shuffle(depth6_vec.begin(), depth6_vec.end(), rng_d7);

	// Estimate: depth 6→7 typically grows by ~12x (empirical observation)
	// To reach 95% load: need (bucket * 0.95) nodes for better coverage
	// Accounting for depth 6 rejection: effective growth ~10x
	size_t target_nodes_d7 = static_cast<size_t>(bucket_7 * 0.95);
	size_t available_parents = depth6_vec.size();

	int children_per_parent_d7 = 1;
	if (available_parents > 0)
	{
		// Calculate children needed: target_nodes / parents
		// Factor in ~40% rejection rate from depth 6 duplicates (empirically verified)
		// At large bucket sizes (32M), rejection rate is higher than small buckets
		double effective_multiplier = 1.4;  // Account for rejections (was 1.15, too optimistic)
		children_per_parent_d7 = static_cast<int>(std::ceil(
			(target_nodes_d7 * effective_multiplier) / available_parents));
		// Clamp to [1, 12] - allow more diversity for large buckets (32M needs 10-12)
		children_per_parent_d7 = std::min(children_per_parent_d7, 12);
		children_per_parent_d7 = std::max(children_per_parent_d7, 1);
	}

	size_t estimated_nodes_d7 = std::min(
		available_parents * children_per_parent_d7,
		static_cast<size_t>(bucket_7 * 0.9));
	double estimated_load_d7 = static_cast<double>(estimated_nodes_d7) / bucket_7;

	if (verbose)
	{
		std::cout << "Adaptive expansion from depth 6" << std::endl;
		std::cout << "Target load: 95%, Estimated load: " << (estimated_load_d7 * 100) << "%" << std::endl;
		std::cout << "Children per parent: " << children_per_parent_d7 << " (adaptive, max=12 for large buckets)" << std::endl;
		std::cout << "Estimated nodes: " << estimated_nodes_d7 << " / " << bucket_7 << std::endl;
	}

	// Expand depth 6 → 7
	for (size_t i = 0; i < depth6_vec.size(); ++i)
	{
		uint64_t node123 = depth6_vec[i];
		const uint64_t index1_cur = node123 / size23;
		const uint64_t index23 = node123 % size23;
		const uint64_t index2_cur = index23 / size3;
		const uint64_t index3_cur = index23 % size3;

		const int index1_tmp = static_cast<int>(index1_cur) * 18;
		const int index2_tmp = static_cast<int>(index2_cur) * 18;
		const int index3_tmp = static_cast<int>(index3_cur) * 18;

		// Generate selected moves
		std::vector<int> selected_moves;
		if (children_per_parent_d7 >= 18)
		{
			for (int m = 0; m < 18; ++m)
				selected_moves.push_back(m);
		}
		else if (children_per_parent_d7 == 1)
		{
			selected_moves.push_back(rng_d7() % 18);
		}
		else
		{
			std::vector<int> all_moves(18);
			for (int m = 0; m < 18; ++m)
				all_moves[m] = m;
			std::shuffle(all_moves.begin(), all_moves.end(), rng_d7);
			for (int j = 0; j < children_per_parent_d7; ++j)
			{
				selected_moves.push_back(all_moves[j]);
			}
		}

		for (int move : selected_moves)
		{
			const uint64_t next_index1 = multi_move_table_cross_edges[index1_tmp + move];
			const uint64_t next_index2 = multi_move_table_F2L_slots_edges[index2_tmp + move];
			const uint64_t next_index3 = multi_move_table_F2L_slots_corners[index3_tmp + move];
			const uint64_t next_node123 = next_index1 * size23 + next_index2 * size3 + next_index3;

			// Add only if not present in depth 6
			if (depth_6_nodes.find(next_node123) == depth_6_nodes.end())
			{
				// Check for rehash before insert
				if (depth_7_nodes.will_rehash_on_next_insert())
				{
					if (verbose)
					{
						std::cout << "Rehash predicted at depth=7, stopping expansion" << std::endl;
					}
					goto phase2_done;
				}

				// Actual rehash detection: check bucket_count before/after insert
				size_t buckets_before = depth_7_nodes.bucket_count();
				depth_7_nodes.insert(next_node123);
				if (depth_7_nodes.bucket_count() != buckets_before)
				{
					// Unexpected rehash occurred! Exit immediately
					if (verbose)
					{
						std::cout << "UNEXPECTED REHASH at depth=7! Exiting immediately" << std::endl;
						std::cout << "  Buckets: " << buckets_before << " -> " << depth_7_nodes.bucket_count() << std::endl;
					}
					goto phase2_done;
				}
			}
		}
	}

phase2_done:
	// Save size before detach
	size_t depth_7_final_size = depth_7_nodes.size();
	depth_7_nodes.detach_element_vector();

	// depth_7_nodes no longer needed - explicitly free it
	{
		tsl::robin_set<uint64_t> temp;
		depth_7_nodes.swap(temp);
	}

	if (verbose)
	{
		std::cout << "RSS after depth_7_nodes freed: " << (get_rss_kb() / 1024.0) << " MB" << std::endl;
	}

	if (num_list.size() <= 7)
	{
		num_list.resize(8, 0);
	}
	num_list[7] = static_cast<int>(depth_7_final_size);

	if (verbose)
	{
		std::cout << "Generated " << depth_7_final_size << " nodes at depth=7" << std::endl;
		std::cout << "  Load factor: " << (depth_7_final_size * 100.0 / bucket_7) << "%" << std::endl;
	}

	// Free depth_6_nodes to save memory (~100-150MB)
	// No longer needed after depth 7 generation
	// Use swap trick to actually deallocate memory (clear() keeps bucket array)
	{
		tsl::robin_set<uint64_t> temp;
		depth_6_nodes.swap(temp);
	}

	if (verbose)
	{
		std::cout << "RSS after depth_6_nodes freed: " << (get_rss_kb() / 1024.0) << " MB" << std::endl;
	}
	if (verbose)
	{
		std::cout << "Freed depth 6 nodes (~" << (4169855 * 32 / 1024 / 1024) << " MB estimate)" << std::endl;
	}

	// Calculate current memory usage after depth 7
	// Note: depth_6_nodes already freed above
	size_t current_memory_d7 = 0;
	size_t theoretical_phase2 = 0;
	for (size_t d = 0; d <= 7; ++d)
	{
		current_memory_d7 += index_pairs[d].size() * 8; // 8 bytes per node
		theoretical_phase2 += index_pairs[d].size() * 8;
	}
	// depth_6_nodes is already freed, so no need to count it

	size_t total_memory_bytes = static_cast<size_t>(MEMORY_LIMIT_MB) * 1024 * 1024;
	size_t remaining_memory_d7 = total_memory_bytes - current_memory_d7;

	if (verbose)
	{
		std::cout << "\n[Phase 2 Complete - Memory Analysis]" << std::endl;
		std::cout << "Theoretical depth 0-7 (index_pairs): " << (theoretical_phase2 / 1024.0 / 1024.0) << " MB" << std::endl;
		std::cout << "Current memory estimate: " << (current_memory_d7 / 1024.0 / 1024.0) << " MB" << std::endl;
		std::cout << "Overhead ratio: " << (static_cast<double>(current_memory_d7) / theoretical_phase2) << "x" << std::endl;
		std::cout << "Actual RSS: " << (get_rss_kb() / 1024.0) << " MB" << std::endl;
		if (verbose)
		{
			std::cout << "Remaining memory: " << (remaining_memory_d7 / 1024.0 / 1024.0) << " MB" << std::endl;
		}
	}

	// ============================================================================
	// Phase 3: depth 8 Local Expansion (with depth 6 check, random sampling)
	// ============================================================================
	if (verbose)
	{
		std::cout << "\n--- Phase 3: Local Expansion depth 7→8 (with depth 6 check, random sampling) ---" << std::endl;
		std::cout << "RSS at Phase 3 start (depth_7_nodes should be destroyed): " << (get_rss_kb() / 1024.0) << " MB" << std::endl;
		std::cout << "Using pre-calculated bucket size for depth 8: " << bucket_8
				  << " (" << (bucket_8 / (1 << 20)) << "M)" << std::endl;
		std::cout << "RSS before depth_8_nodes creation: " << (get_rss_kb() / 1024.0) << " MB" << std::endl;
	}

	// Generate depth 8 nodes
	if (index_pairs.size() <= 8)
	{
		index_pairs.resize(9);
	}
	index_pairs[8].clear();

	tsl::robin_set<uint64_t> depth_8_nodes(bucket_8);
	depth_8_nodes.max_load_factor(0.9f);
	depth_8_nodes.attach_element_vector(&index_pairs[8]);

	if (verbose)
	{
		std::cout << "RSS after depth_8_nodes creation (empty): " << (get_rss_kb() / 1024.0) << " MB" << std::endl;
	}

	size_t rejected_as_depth7 = 0;
	size_t inserted_count = 0; // Track successful inserts for diagnostics

	// Advance one move from all depth 7 nodes
	tsl::robin_set<uint64_t> depth7_set;
	for (uint64_t node : index_pairs[7])
	{
		depth7_set.insert(node);
	}

	// depth 7→8 is always random sampling (adaptive children per parent)
	std::vector<uint64_t> depth7_vec(depth7_set.begin(), depth7_set.end());
	std::mt19937_64 rng_phase3(23456);
	std::shuffle(depth7_vec.begin(), depth7_vec.end(), rng_phase3);

	// Adaptive children per parent strategy for depth 7→8
	// Check diversity: if parent nodes < bucket size, diversity is insufficient
	double diversity_ratio_d8 = static_cast<double>(depth7_vec.size()) / bucket_8;
	size_t max_nodes_with_one_child_d8 = std::min(depth7_vec.size(), static_cast<size_t>(bucket_8 * 0.9));
	double expected_load_factor_d8 = static_cast<double>(max_nodes_with_one_child_d8) / bucket_8;

	int children_per_parent_d8 = 1;
	// Diversity insufficient: parents < bucket, need more children per parent
	if (diversity_ratio_d8 < 1.0 && depth7_vec.size() > 0)
	{
		// Target: ~90% load factor
		double target_load = 0.90;
		// Account for rejections (depth 6 check, etc.) with 1.5x multiplier (increased from 1.2)
		children_per_parent_d8 = static_cast<int>(std::ceil((bucket_8 * target_load * 1.5) / depth7_vec.size()));
		children_per_parent_d8 = std::min(children_per_parent_d8, 6);  // Max 6 (increased from 3)
		children_per_parent_d8 = std::max(children_per_parent_d8, 1); // Min 1
	}
	else if (expected_load_factor_d8 < 0.85 && depth7_vec.size() > 0)
	{
		// Fallback: if load would be too low even with sufficient diversity
		double target_load = 0.90;
		children_per_parent_d8 = static_cast<int>(std::ceil((bucket_8 * target_load) / depth7_vec.size()));
		children_per_parent_d8 = std::min(children_per_parent_d8, 6);
		children_per_parent_d8 = std::max(children_per_parent_d8, 1);
	}

	if (verbose)
	{
		std::cout << "Random sampling: up to " << depth7_vec.size() << " parent nodes" << std::endl;
		std::cout << "Diversity ratio (parents/bucket): " << (diversity_ratio_d8 * 100) << "%" << std::endl;
		std::cout << "Expected load factor (1 child/parent): " << (expected_load_factor_d8 * 100) << "%" << std::endl;
		std::cout << "Children per parent: " << children_per_parent_d8 << " (adaptive)" << std::endl;
	}

	for (size_t idx = 0; idx < depth7_vec.size(); ++idx)
	{
		uint64_t node123 = depth7_vec[idx];
		const uint64_t index1_cur = node123 / size23;
		const uint64_t index23 = node123 % size23;
		const uint64_t index2_cur = index23 / size3;
		const uint64_t index3_cur = index23 % size3;

		const int index1_tmp = static_cast<int>(index1_cur) * 18;
		const int index2_tmp = static_cast<int>(index2_cur) * 18;
		const int index3_tmp = static_cast<int>(index3_cur) * 18;

		// Generate selected_moves based on children_per_parent_d8
		std::vector<int> selected_moves;
		if (children_per_parent_d8 >= 18)
		{
			for (int m = 0; m < 18; ++m)
				selected_moves.push_back(m);
		}
		else if (children_per_parent_d8 == 1)
		{
			selected_moves.push_back(rng_phase3() % 18);
		}
		else
		{
			std::vector<int> all_moves(18);
			for (int m = 0; m < 18; ++m)
				all_moves[m] = m;
			std::shuffle(all_moves.begin(), all_moves.end(), rng_phase3);
			for (int i = 0; i < children_per_parent_d8; ++i)
			{
				selected_moves.push_back(all_moves[i]);
			}
		}

		for (int move : selected_moves)
		{

			const uint64_t next_index1 = multi_move_table_cross_edges[index1_tmp + move];
			const uint64_t next_index2 = multi_move_table_F2L_slots_edges[index2_tmp + move];
			const uint64_t next_index3 = multi_move_table_F2L_slots_corners[index3_tmp + move];
			const uint64_t next_node123 = next_index1 * size23 + next_index2 * size3 + next_index3;

			// Skip if contained in depth 6
			if (depth_6_nodes.find(next_node123) != depth_6_nodes.end())
			{
				continue;
			}

			// Skip if already present in depth 7 (avoid duplicates)
			if (depth7_set.find(next_node123) != depth7_set.end())
			{
				rejected_as_depth7++; // Count as depth 7 reject
				continue;
			}

			// Check if connected to depth 6 by one move
			bool connected_to_depth6 = false;
			const uint64_t check_index1 = next_node123 / size23;
			const uint64_t check_index23 = next_node123 % size23;
			const uint64_t check_index2 = check_index23 / size3;
			const uint64_t check_index3 = check_index23 % size3;

			const int check_index1_tmp = static_cast<int>(check_index1) * 18;
			const int check_index2_tmp = static_cast<int>(check_index2) * 18;
			const int check_index3_tmp = static_cast<int>(check_index3) * 18;

			for (int m = 0; m < 18; ++m)
			{
				const uint64_t back_index1 = multi_move_table_cross_edges[check_index1_tmp + m];
				const uint64_t back_index2 = multi_move_table_F2L_slots_edges[check_index2_tmp + m];
				const uint64_t back_index3 = multi_move_table_F2L_slots_corners[check_index3_tmp + m];
				const uint64_t back_node = back_index1 * size23 + back_index2 * size3 + back_index3;

				if (depth_6_nodes.find(back_node) != depth_6_nodes.end())
				{
					connected_to_depth6 = true;
					break;
				}
			}

			if (connected_to_depth6)
			{
				rejected_as_depth7++;
				continue; // Confirmed as depth 7
			}

			// Preemptive rehash detection: Check will_rehash_on_next_insert() before insertion
			if (depth_8_nodes.will_rehash_on_next_insert())
			{
				if (verbose)
				{
					std::cout << "Rehash predicted at depth=8, stopping expansion" << std::endl;
					std::cout << "Final depth 8: size=" << depth_8_nodes.size()
							  << ", buckets=" << depth_8_nodes.bucket_count()
							  << ", threshold=" << depth_8_nodes.load_threshold()
							  << ", available=" << depth_8_nodes.available_capacity() << std::endl;
				}
				goto phase3_done;
			}
			// Add as candidate for depth 8 (with actual rehash detection)
			size_t buckets_before_d8 = depth_8_nodes.bucket_count();
			size_t size_before = depth_8_nodes.size();
			depth_8_nodes.insert(next_node123);
			if (depth_8_nodes.bucket_count() != buckets_before_d8)
			{
				// Unexpected rehash occurred! Exit immediately
				if (verbose)
				{
					std::cout << "UNEXPECTED REHASH at depth=8! Exiting immediately" << std::endl;
					std::cout << "  Buckets: " << buckets_before_d8 << " -> " << depth_8_nodes.bucket_count() << std::endl;
				}
				goto phase3_done;
			}
			if (depth_8_nodes.size() > size_before)
			{
				inserted_count++;
			}
		}
	}

phase3_done:
	// Save size before detach
	size_t depth_8_final_size = depth_8_nodes.size();

	depth_8_nodes.detach_element_vector();

	// depth_8_nodes no longer needed - explicitly free it
	{
		tsl::robin_set<uint64_t> temp;
		depth_8_nodes.swap(temp);
	}

	// Free temporary data structures - use swap trick to actually free memory
	{
		tsl::robin_set<uint64_t> temp;
		depth7_set.swap(temp);
	}
	depth7_vec.clear();
	depth7_vec.shrink_to_fit();

	if (num_list.size() <= 8)
	{
		num_list.resize(9, 0);
	}
	num_list[8] = static_cast<int>(depth_8_final_size); // Use saved size before detach/clear

	if (verbose)
	{
		size_t theoretical_phase3 = 0;
		for (size_t d = 0; d <= 8; ++d)
		{
			theoretical_phase3 += index_pairs[d].size() * 8;
		}
		size_t current_memory_d8 = current_memory_d7 + index_pairs[8].size() * 8;

		std::cout << "Generated " << depth_8_nodes.size() << " nodes at depth=8" << std::endl;
		std::cout << "  Rejected as depth 7: " << rejected_as_depth7 << std::endl;
		std::cout << "  Load factor: " << (depth_8_nodes.size() * 100.0 / bucket_8) << "%" << std::endl;
		std::cout << "\n[Phase 3 Complete - Memory Analysis]" << std::endl;
		std::cout << "Theoretical depth 0-8 (index_pairs): " << (theoretical_phase3 / 1024.0 / 1024.0) << " MB" << std::endl;
		std::cout << "Current memory estimate: " << (current_memory_d8 / 1024.0 / 1024.0) << " MB" << std::endl;
		std::cout << "Overhead ratio: " << (static_cast<double>(current_memory_d8) / theoretical_phase3) << "x" << std::endl;
		std::cout << "Actual RSS: " << (get_rss_kb() / 1024.0) << " MB" << std::endl;
	}

	// ============================================================================
	// Phase 4: depth 9 Local Expansion (Random sampling with single move)
	// ============================================================================
	if (verbose)
	{
		std::cout << "\n--- Phase 4: Local Expansion depth 8→9 (Random sampling with single move) ---" << std::endl;
		std::cout << "Note: Adaptive children per parent based on load factor" << std::endl;
		std::cout << "Using pre-calculated bucket size for depth 9: " << bucket_9
				  << " (" << (bucket_9 / (1 << 20)) << "M)" << std::endl;
	}

	// Generate depth 9 nodes
	if (index_pairs.size() <= 9)
	{
		index_pairs.resize(10);
	}
	index_pairs[9].clear();

	tsl::robin_set<uint64_t> depth_9_nodes(bucket_9);
	depth_9_nodes.max_load_factor(0.9f);
	depth_9_nodes.attach_element_vector(&index_pairs[9]);

	// Convert depth 8 nodes to vector and shuffle
	tsl::robin_set<uint64_t> depth8_set;
	for (uint64_t node : index_pairs[8])
	{
		depth8_set.insert(node);
	}

	std::vector<uint64_t> depth8_vec(depth8_set.begin(), depth8_set.end());
	std::mt19937_64 rng_phase4(54321); // Re-seed for reproducibility
	std::shuffle(depth8_vec.begin(), depth8_vec.end(), rng_phase4);

	// Adaptive children per parent strategy for depth 8→9
	// Check diversity: if parent nodes < bucket size, diversity is insufficient
	double diversity_ratio_d9 = static_cast<double>(depth8_vec.size()) / bucket_9;
	size_t max_nodes_with_one_child = std::min(depth8_vec.size(), static_cast<size_t>(bucket_9 * 0.9));
	double expected_load_factor = static_cast<double>(max_nodes_with_one_child) / bucket_9;

	// If expected load factor is too low, increase children per parent
	int children_per_parent = 1;
	// Diversity insufficient: parents < bucket, need more children per parent
	if (diversity_ratio_d9 < 1.0 && depth8_vec.size() > 0)
	{
		// Target: ~90% load factor
		double target_load = 0.90;
		// Account for rejections (backward check, etc.) with 1.5x multiplier (increased from 1.25)
		children_per_parent = static_cast<int>(std::ceil((bucket_9 * target_load * 1.5) / depth8_vec.size()));
		children_per_parent = std::min(children_per_parent, 6);  // Max 6 (increased from 3)
		children_per_parent = std::max(children_per_parent, 1); // Min 1
	}
	else if (expected_load_factor < 0.85 && depth8_vec.size() > 0)
	{
		// Fallback: if load would be too low even with sufficient diversity
		double target_load = 0.90;
		children_per_parent = static_cast<int>(std::ceil((bucket_9 * target_load) / depth8_vec.size()));
		children_per_parent = std::min(children_per_parent, 6);
		children_per_parent = std::max(children_per_parent, 1);
	}

	if (verbose)
	{
		std::cout << "Random sampling: up to " << depth8_vec.size() << " parent nodes" << std::endl;
		std::cout << "Diversity ratio (parents/bucket): " << (diversity_ratio_d9 * 100) << "%" << std::endl;
		std::cout << "Expected load factor (1 child/parent): " << (expected_load_factor * 100) << "%" << std::endl;
		std::cout << "Children per parent: " << children_per_parent << " (adaptive)" << std::endl;
	}

	for (size_t i = 0; i < depth8_vec.size(); ++i)
	{
		uint64_t node123 = depth8_vec[i];

		const uint64_t index1_cur = node123 / size23;
		const uint64_t index23 = node123 % size23;
		const uint64_t index2_cur = index23 / size3;
		const uint64_t index3_cur = index23 % size3;

		const int index1_tmp = static_cast<int>(index1_cur) * 18;
		const int index2_tmp = static_cast<int>(index2_cur) * 18;
		const int index3_tmp = static_cast<int>(index3_cur) * 18;

		// Adaptive: Select random children_per_parent moves
		std::vector<int> selected_moves;
		if (children_per_parent >= 18)
		{
			// All moves
			for (int m = 0; m < 18; ++m)
			{
				selected_moves.push_back(m);
			}
		}
		else if (children_per_parent == 1)
		{
			// Single random move
			selected_moves.push_back(rng_phase4() % 18);
		}
		else
		{
			// Random subset of moves
			std::vector<int> all_moves(18);
			for (int m = 0; m < 18; ++m)
				all_moves[m] = m;
			std::shuffle(all_moves.begin(), all_moves.end(), rng_phase4);
			for (int m = 0; m < children_per_parent; ++m)
			{
				selected_moves.push_back(all_moves[m]);
			}
		}

		for (int move : selected_moves)
		{
			const uint64_t next_index1 = multi_move_table_cross_edges[index1_tmp + move];
			const uint64_t next_index2 = multi_move_table_F2L_slots_edges[index2_tmp + move];
			const uint64_t next_index3 = multi_move_table_F2L_slots_corners[index3_tmp + move];
			const uint64_t next_node123 = next_index1 * size23 + next_index2 * size3 + next_index3;

			// Skip if contained in depth 6 or depth 8
			// Note: depth 7 is already freed, but we check depth 8 which includes depth 7 duplicates
			if (depth_6_nodes.find(next_node123) != depth_6_nodes.end() ||
				depth8_set.find(next_node123) != depth8_set.end())
			{
				continue;
			}

			// Preemptive rehash detection: Check will_rehash_on_next_insert() before insertion
			if (depth_9_nodes.will_rehash_on_next_insert())
			{
				if (verbose)
				{
					std::cout << "Rehash predicted at depth=9, stopping expansion" << std::endl;
					std::cout << "Final depth 9: size=" << depth_9_nodes.size()
							  << ", buckets=" << depth_9_nodes.bucket_count()
							  << ", threshold=" << depth_9_nodes.load_threshold()
							  << ", available=" << depth_9_nodes.available_capacity() << std::endl;
				}
				goto phase4_done;
			}

			// Actual rehash detection: check bucket_count before/after insert
			size_t buckets_before_d9 = depth_9_nodes.bucket_count();
			depth_9_nodes.insert(next_node123);
			if (depth_9_nodes.bucket_count() != buckets_before_d9)
			{
				// Unexpected rehash occurred! Exit immediately
				if (verbose)
				{
					std::cout << "UNEXPECTED REHASH at depth=9! Exiting immediately" << std::endl;
					std::cout << "  Buckets: " << buckets_before_d9 << " -> " << depth_9_nodes.bucket_count() << std::endl;
				}
				goto phase4_done;
			}
		}
	}

phase4_done:
	// Save size before detach
	size_t depth_9_final_size = depth_9_nodes.size();

	depth_9_nodes.detach_element_vector();

	// Free temporary data structures - use swap trick to actually free memory
	{
		tsl::robin_set<uint64_t> temp;
		depth8_set.swap(temp);
	}
	depth8_vec.clear();
	depth8_vec.shrink_to_fit();

	if (num_list.size() <= 9)
	{
		num_list.resize(10, 0);
	}
	num_list[9] = static_cast<int>(depth_9_nodes.size());

	// depth_9_nodes no longer needed - explicitly free it
	{
		tsl::robin_set<uint64_t> temp;
		depth_9_nodes.swap(temp);
	}

	if (verbose)
	{
		size_t theoretical_phase4 = 0;
		for (size_t d = 0; d <= 9; ++d)
		{
			theoretical_phase4 += index_pairs[d].size() * 8;
		}
		size_t current_memory_d8_val = current_memory_d7 + index_pairs[8].size() * 8;
		size_t current_memory_d9 = current_memory_d8_val + index_pairs[9].size() * 8;

		std::cout << "Generated " << depth_9_nodes.size() << " nodes at depth=9" << std::endl;
		std::cout << "  Load factor: " << (depth_9_nodes.size() * 100.0 / bucket_9) << "%" << std::endl;
		std::cout << "  (Note: Random sampling improves coverage and randomness)" << std::endl;
		std::cout << "\n[Phase 4 Complete - Memory Analysis]" << std::endl;
		std::cout << "Theoretical depth 0-9 (index_pairs): " << (theoretical_phase4 / 1024.0 / 1024.0) << " MB" << std::endl;
		std::cout << "Current memory estimate: " << (current_memory_d9 / 1024.0 / 1024.0) << " MB" << std::endl;
		std::cout << "Overhead ratio: " << (static_cast<double>(current_memory_d9) / theoretical_phase4) << "x" << std::endl;
		std::cout << "Actual RSS: " << (get_rss_kb() / 1024.0) << " MB" << std::endl;
	}

	// Step 6: Final Cleanup
	if (verbose)
	{
		std::cout << "\nStep 6: Final Cleanup" << std::endl;
	}

	// prev_set, cur_set, next_set are only used within existing code (currently commented out)

	// Memory optimization: shrink capacity of each vector in index_pairs to actual size
	for (auto &vec : index_pairs)
	{
		vec.shrink_to_fit();
	}

	if (verbose)
	{
		std::cout << "\n=== Database Construction Complete ===" << std::endl;
		std::cout << "Summary:" << std::endl;
		std::cout << "  Total depth range: 0-" << (index_pairs.size() - 1) << std::endl;
		std::cout << "  Total nodes: " << num_list.size() << std::endl;

		// Display actual memory usage
		size_t total_nodes = 0;
		for (const auto &vec : index_pairs)
		{
			total_nodes += vec.size();
		}
		std::cout << "  index_pairs memory (actual): " << (total_nodes * sizeof(uint64_t) / 1024.0 / 1024.0) << " MB" << std::endl;
	}
} // End of build_complete_search_database

// ============================================================================
// Legacy Functions (Kept for individual testing and backward compatibility)
// ============================================================================

// BFS Execution and Memory Usage Display
int run_bfs_phase(
	int index1, int index2, int index3,
	int size1, int size2, int size3,
	int BFS_DEPTH,
	int MEMORY_LIMIT_MB,
	const std::vector<int> &multi_move_table_cross_edges,
	const std::vector<int> &multi_move_table_F2L_slots_edges,
	const std::vector<int> &multi_move_table_F2L_slots_corners,
	std::vector<std::vector<uint64_t>> &index_pairs,
	std::vector<int> &num_list,
	bool verbose = true)
{
	if (verbose)
	{
		std::cout << "\n=== BFS Phase ===" << std::endl;
		std::cout << "Target depth: " << BFS_DEPTH << std::endl;
		std::cout << "Memory limit: " << MEMORY_LIMIT_MB << " MB" << std::endl;
	}

	// run BFS
	int reached_depth = create_prune_table_sparse(
		index1, index2, index3,
		size1, size2, size3,
		BFS_DEPTH,
		1024 * MEMORY_LIMIT_MB,
		multi_move_table_cross_edges,
		multi_move_table_F2L_slots_edges,
		multi_move_table_F2L_slots_corners,
		index_pairs,
		num_list,
		verbose);

	if (verbose)
	{
		std::cout << "\n=== BFS Results ===" << std::endl;
		std::cout << "Reached depth: " << reached_depth << std::endl;
		std::cout << "\nindex_pairs status:" << std::endl;
		for (int d = 0; d <= reached_depth; ++d)
		{
			std::cout << "  depth=" << d << ": " << index_pairs[d].size() << " nodes" << std::endl;
		}
	}

	return reached_depth;
}

// Execute Local Expansion Steps 1-4
struct LocalExpansionResults
{
	bool n1_expansion_done = false;
	bool n2_expansion_done = false;
	size_t n1_nodes = 0;
	size_t n2_nodes = 0;
	LocalExpansionConfig config;
};

LocalExpansionResults run_local_expansion_steps_1_to_4(
	int reached_depth,
	int size1, int size2, int size3,
	int MEMORY_LIMIT_MB,
	const std::vector<int> &multi_move_table_cross_edges,
	const std::vector<int> &multi_move_table_F2L_slots_edges,
	const std::vector<int> &multi_move_table_F2L_slots_corners,
	std::vector<std::vector<uint64_t>> &index_pairs,
	bool verbose = true)
{
	LocalExpansionResults results;

	if (reached_depth < 4)
	{
		if (verbose)
		{
			std::cout << "\n=== Local Expansion Skipped (depth < 4) ===" << std::endl;
		}
		return results;
	}

	if (verbose)
	{
		std::cout << "\n=== Local Expansion Phase ===" << std::endl;
		std::cout << "Current BFS depth: " << reached_depth << std::endl;
	}

	// Restore nodes at depth=n into robin_set
	tsl::robin_set<uint64_t> depth_n_nodes;
	depth_n_nodes.max_load_factor(0.9f);

	if (verbose)
	{
		std::cout << "Loading depth=" << reached_depth << " nodes into robin_set..." << std::endl;
	}

	for (uint64_t node : index_pairs[reached_depth])
	{
		depth_n_nodes.insert(node);
	}

	if (verbose)
	{
		std::cout << "Loaded " << depth_n_nodes.size() << " nodes at depth=" << reached_depth << std::endl;
	}

	// Estimate current memory usage
	size_t current_memory_bytes = 0;
	for (size_t d = 0; d <= static_cast<size_t>(reached_depth); ++d)
	{
		current_memory_bytes += index_pairs[d].size() * 8;
	}
	current_memory_bytes += depth_n_nodes.size() * 24;
	current_memory_bytes += depth_n_nodes.bucket_count() * 4;

	if (verbose)
	{
		std::cout << "Current memory usage: " << (current_memory_bytes / 1024.0 / 1024.0) << " MB" << std::endl;
	}

	// Calculate remaining memory
	size_t total_memory_limit = static_cast<size_t>(MEMORY_LIMIT_MB) * 1024 * 1024;
	size_t remaining_memory = total_memory_limit - current_memory_bytes;

	if (verbose)
	{
		std::cout << "Total memory limit: " << MEMORY_LIMIT_MB << " MB" << std::endl;
		std::cout << "Remaining memory: " << (remaining_memory / 1024.0 / 1024.0) << " MB" << std::endl;
	}

	// Step 0: Determine bucket sizes
	size_t bucket_depth_n = depth_n_nodes.bucket_count();
	size_t cur_set_memory = depth_n_nodes.size() * 24 + bucket_depth_n * 4;

	LocalExpansionConfig config = determine_bucket_sizes(
		reached_depth,
		remaining_memory,
		bucket_depth_n,
		cur_set_memory,
		verbose);

	results.config = config;

	// Step 1: Get N
	size_t N = get_n1_capacity(config);

	if (verbose)
	{
		std::cout << "\n=== Step 1: Capacity ===" << std::endl;
		std::cout << "N (depth=" << (reached_depth + 1) << " capacity) = " << N << " nodes" << std::endl;
	}

	// Step 2: Random Expansion
	if (config.enable_n1_expansion)
	{
		if (verbose)
		{
			std::cout << "\n=== Step 2: Random Expansion ===" << std::endl;
		}

		tsl::robin_set<uint64_t> depth_n1_nodes;
		depth_n1_nodes.max_load_factor(0.9f);

		local_expand_step2_random_n1(
			config,
			depth_n_nodes,
			depth_n1_nodes,
			multi_move_table_cross_edges,
			multi_move_table_F2L_slots_edges,
			multi_move_table_F2L_slots_corners,
			size2,
			size3,
			index_pairs,
			reached_depth,
			verbose);

		results.n1_expansion_done = true;
		results.n1_nodes = depth_n1_nodes.size();

		if (verbose)
		{
			std::cout << "\nStep 2 Results:" << std::endl;
			std::cout << "  Generated depth=" << (reached_depth + 1) << " nodes: " << depth_n1_nodes.size() << std::endl;
			std::cout << "  Expected: ~" << N << " nodes" << std::endl;
			std::cout << "  Bucket usage: " << (100.0 * depth_n1_nodes.size() / depth_n1_nodes.bucket_count()) << "%" << std::endl;
		}

		// Step 3: Data Reorganization
		if (verbose)
		{
			std::cout << "\n=== Step 3: Data Reorganization ===" << std::endl;
		}

		tsl::robin_set<uint64_t> prev_set, cur_set, next_set;
		prev_set.max_load_factor(0.9f);
		cur_set.max_load_factor(0.9f);
		next_set.max_load_factor(0.9f);

		// Load nodes at depth=n-1 into prev
		if (reached_depth > 0 && index_pairs.size() > static_cast<size_t>(reached_depth - 1))
		{
			if (verbose)
			{
				std::cout << "Loading depth=" << (reached_depth - 1) << " nodes into prev..." << std::endl;
			}
			for (uint64_t node : index_pairs[reached_depth - 1])
			{
				prev_set.insert(node);
			}
		}

		cur_set = depth_n_nodes;
		next_set = depth_n1_nodes;

		if (verbose)
		{
			std::cout << "Before Step 3:" << std::endl;
			std::cout << "  prev (depth=" << (reached_depth - 1) << "): " << prev_set.size() << " nodes" << std::endl;
			std::cout << "  cur (depth=" << reached_depth << "): " << cur_set.size() << " nodes" << std::endl;
			std::cout << "  next (depth=" << (reached_depth + 1) << "): " << next_set.size() << " nodes" << std::endl;
		}

		local_expand_step3_reorganize(
			config,
			prev_set,
			cur_set,
			next_set,
			index_pairs,
			reached_depth,
			verbose);

		if (verbose)
		{
			std::cout << "\nStep 3 Results:" << std::endl;
			std::cout << "  prev (depth=" << reached_depth << "): " << prev_set.size() << " nodes" << std::endl;
			std::cout << "  cur (depth=" << (reached_depth + 1) << "): " << cur_set.size() << " nodes" << std::endl;
			std::cout << "  next (depth=" << (reached_depth + 2) << "): " << next_set.size() << " nodes" << std::endl;
		}

		// Step 4: Calculate depth=n+2 Capacity
		if (config.enable_n2_expansion)
		{
			if (verbose)
			{
				std::cout << "\n=== Step 4: Calculate depth=n+2 Capacity ===" << std::endl;
			}

			size_t M = local_expand_step4_calculate_n2_capacity(
				reached_depth,
				config,
				prev_set,
				cur_set,
				index_pairs,
				total_memory_limit,
				verbose);

			if (verbose)
			{
				std::cout << "\nStep 4 Results:" << std::endl;
				if (M > 0)
				{
					std::cout << "  Calculated capacity M: " << M << " nodes (" << (M / 1e6) << "M)" << std::endl;
				}
				else
				{
					std::cout << "  Insufficient memory for depth=n+2 expansion" << std::endl;
				}
			}

			results.n2_expansion_done = (M > 0);
		}
	}
	else
	{
		if (verbose)
		{
			std::cout << "\nStep 2-4: Skipped (n+1 expansion not enabled)" << std::endl;
		}
	}

	return results;
}

void create_prune_table(int index, int size, int depth, const std::vector<int> table, std::vector<unsigned char> &prune_table)
{
	prune_table[index] = 0;
	int next_i;
	int index_tmp;
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
				index_tmp = i * 18;
				for (int j = 0; j < 18; ++j)
				{
					next_i = table[index_tmp + j];
					if (next_i != i && prune_table[next_i] == 255)
					{
						prune_table[next_i] = next_d;
						num_filled += 1;
					}
				}
			}
		}
		std::cout << "Depth " << next_d << ": " << num_filled << " / " << size << " filled." << std::endl;
		if (num_filled == num_old)
		{
			break;
		}
		else
		{
			num_old = num_filled;
		}
	}
}

void create_prune_table2(int index1, int index2, int size1, int size2, int depth, const std::vector<int> table1, const std::vector<int> table2, std::vector<unsigned char> &prune_table)
{
	int size = size1 * size2;
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
					if (next_i != i && prune_table[next_i] == 255)
					{
						prune_table[next_i] = next_d;
						num_filled += 1;
					}
				}
			}
		}
		std::cout << "Depth " << next_d << ": " << num_filled << " / " << size << " filled." << std::endl;
		if (num_filled == num_old)
		{
			break;
		}
		else
		{
			num_old = num_filled;
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

struct xxcross_search
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
	std::vector<int> multi_move_table_cross_edges;
	std::vector<int> multi_move_table_F2L_slots_edges;
	std::vector<int> multi_move_table_F2L_slots_corners;
	std::vector<unsigned char> prune_table1;
	std::vector<unsigned char> prune_table23_couple;
	std::vector<std::vector<uint64_t>> index_pairs;
	std::vector<int> num_list;
	std::vector<int> alg;
	std::vector<std::string> restrict;
	std::vector<int> move_restrict;
	std::vector<bool> ma;
	int index1;
	int index2;
	int index3;
	int index1_tmp;
	int index2_tmp;
	int index3_tmp;
	int prune1_tmp;
	int prune23_tmp;
	std::string tmp;
	std::mt19937 generator;
	int reached_depth;
	int size1, size2, size3;
	std::vector<int> cross_edges_goal;
	std::vector<int> F2L_slots_edges_goal;
	std::vector<int> F2L_slots_corners_goal;
	std::vector<int> cross_edges_goal_tmp;
	std::vector<int> F2L_slots_edges_goal_tmp;
	std::vector<int> F2L_slots_corners_goal_tmp;

	xxcross_search(bool adj = true, int BFS_DEPTH = 6, int MEMORY_LIMIT_MB = 1600, bool verbose = true)
	{
#ifdef __EMSCRIPTEN__
		// In WebAssembly environment, disable verbose output by default
		verbose = false;
		// Reduce memory limit for WASM (800MB initial memory, need headroom for overhead)
		// User can specify lower values, but values >700 will be capped
		int requested_memory = MEMORY_LIMIT_MB;
		if (MEMORY_LIMIT_MB > 700)
		{
			std::cout << "[WASM] Requested memory " << MEMORY_LIMIT_MB << "MB exceeds safe limit, capping at 700MB" << std::endl;
			MEMORY_LIMIT_MB = 700;
		}
		else
		{
			std::cout << "[WASM] Using memory limit: " << MEMORY_LIMIT_MB << "MB" << std::endl;
		}
#endif
		// Create move tables
		edge_move_table = create_edge_move_table();
		corner_move_table = create_corner_move_table();
		create_multi_move_table(4, 2, 12, 24 * 22 * 20 * 18, edge_move_table, multi_move_table_cross_edges);
		create_multi_move_table(2, 2, 12, 24 * 22, edge_move_table, multi_move_table_F2L_slots_edges);
		create_multi_move_table(2, 3, 8, 24 * 21, corner_move_table, multi_move_table_F2L_slots_corners);
		prune_table1 = std::vector<unsigned char>(24 * 22 * 20 * 18, 255);
		prune_table23_couple = std::vector<unsigned char>(24 * 22 * 24 * 21, 255);

		// Goal ;
		cross_edges_goal = {16, 18, 20, 22};
		if (adj)
		{
			F2L_slots_edges_goal = {0, 2};
			F2L_slots_corners_goal = {12, 15};
		}
		else
		{
			F2L_slots_edges_goal = {0, 4};
			F2L_slots_corners_goal = {12, 18};
		}
		cross_edges_goal_tmp = cross_edges_goal;
		F2L_slots_edges_goal_tmp = F2L_slots_edges_goal;
		F2L_slots_corners_goal_tmp = F2L_slots_corners_goal;
		index1 = array_to_index(cross_edges_goal_tmp, 4, 2, 12);
		index2 = array_to_index(F2L_slots_edges_goal_tmp, 2, 2, 12);
		index3 = array_to_index(F2L_slots_corners_goal_tmp, 2, 3, 8);
		size1 = 24 * 22 * 20 * 18;
		size2 = 24 * 22;
		size3 = 24 * 21;

		create_prune_table(index1, size1, 8, multi_move_table_cross_edges, prune_table1);
		create_prune_table2(index2, index3, size2, size3, 9, multi_move_table_F2L_slots_edges, multi_move_table_F2L_slots_corners, prune_table23_couple);

		size_t move_table_memory =
			multi_move_table_cross_edges.size() * sizeof(int) +
			multi_move_table_F2L_slots_edges.size() * sizeof(int) +
			multi_move_table_F2L_slots_corners.size() * sizeof(int);

		std::cout << "Move table memory: " << (move_table_memory / 1024.0 / 1024.0) << " MB" << std::endl;

		// Move table boundary value check
		// std::cout << "\nMove Table Validation:" << std::endl;
		// std::cout << "Cross Edges: size=" << multi_move_table_cross_edges.size()
		// 		  << ", min=" << *std::min_element(multi_move_table_cross_edges.begin(), multi_move_table_cross_edges.end())
		// 		  << ", max=" << *std::max_element(multi_move_table_cross_edges.begin(), multi_move_table_cross_edges.end()) << std::endl;
		// std::cout << "F2L Slots Edges: size=" << multi_move_table_F2L_slots_edges.size()
		// 		  << ", min=" << *std::min_element(multi_move_table_F2L_slots_edges.begin(), multi_move_table_F2L_slots_edges.end())
		// 		  << ", max=" << *std::max_element(multi_move_table_F2L_slots_edges.begin(), multi_move_table_F2L_slots_edges.end()) << std::endl;
		// std::cout << "F2L Slots Corners: size=" << multi_move_table_F2L_slots_corners.size()
		// 		  << ", min=" << *std::min_element(multi_move_table_F2L_slots_corners.begin(), multi_move_table_F2L_slots_corners.end())
		// 		  << ", max=" << *std::max_element(multi_move_table_F2L_slots_corners.begin(), multi_move_table_F2L_slots_corners.end()) << std::endl;

		// Execute BFS + Local Expansion (build complete search database in one line)
		build_complete_search_database(
			index1, index2, index3,
			size1, size2, size3,
			BFS_DEPTH,
			MEMORY_LIMIT_MB,
			multi_move_table_cross_edges,
			multi_move_table_F2L_slots_edges,
			multi_move_table_F2L_slots_corners,
			index_pairs,
			num_list,
			verbose);

		// reached_depth is calculated from the size of index_pairs
		reached_depth = static_cast<int>(index_pairs.size()) - 1;

		std::cout << "\nDatabase construction completed: reached_depth=" << reached_depth << std::endl;

		// Debug: Verify sizes of num_list and index_pairs
		std::cout << "\n=== Database Validation ===" << std::endl;
		std::cout << "num_list.size() = " << num_list.size() << std::endl;
		std::cout << "index_pairs.size() = " << index_pairs.size() << std::endl;

		for (size_t d = 0; d < index_pairs.size(); ++d)
		{
			size_t num_list_val = (d < num_list.size()) ? num_list[d] : 0;
			size_t index_pairs_size = index_pairs[d].size();
			bool match = (num_list_val == index_pairs_size);

			std::cout << "  depth=" << d
					  << ": num_list[" << d << "]=" << num_list_val
					  << ", index_pairs[" << d << "].size()=" << index_pairs_size
					  << (match ? " ✓" : " ✗ MISMATCH!") << std::endl;
		}
		if (verbose)
		{
			std::cout << "==================================" << std::endl;
		}
		ma = create_ma_table();
		std::random_device rd;
		generator.seed(rd());
		move_restrict = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17};
	}

	bool depth_limited_search(int arg_index1, int arg_index2, int arg_index3, int depth, int prev)
	{
		for (int i : move_restrict)
		{
			if (ma[prev + i])
			{
				continue;
			}
			index2_tmp = multi_move_table_F2L_slots_edges[arg_index2 + i];
			index3_tmp = multi_move_table_F2L_slots_corners[arg_index3 + i];
			prune23_tmp = prune_table23_couple[index2_tmp * size3 + index3_tmp];
			if (prune23_tmp >= depth)
			{
				continue;
			}
			index1_tmp = multi_move_table_cross_edges[arg_index1 + i];
			prune1_tmp = prune_table1[index1_tmp];
			if (prune1_tmp >= depth)
			{
				continue;
			}
			sol.emplace_back(i);
			if (depth == 1)
			{
				if (prune1_tmp == 0 && prune23_tmp == 0)
				{
					tmp = AlgToString(sol);
					return true;
				}
			}
			else if (depth_limited_search(index1_tmp * 18, index2_tmp * 18, index3_tmp * 18, depth - 1, i * 18))
			{
				return true;
			}
			sol.pop_back();
		}
		return false;
	}

	std::string get_xxcross_scramble(std::string arg_length = "7")
	{
		sol.clear();
		int len = std::stoi(arg_length);
		std::uniform_int_distribution<> distribution(0, num_list[len - 1] - 1);
		uint64_t xxcross_index = index_pairs[len - 1][distribution(generator)];
		index1 = static_cast<int>(xxcross_index / (size2 * size3));
		int index23packed = static_cast<int>(xxcross_index % (size2 * size3));
		index2 = index23packed / size3;
		index3 = index23packed % size3;
		std::cout << xxcross_index << ", " << index1 << ", " << index2 << ", " << index3 << std::endl;
		index1 *= 18;
		index2 *= 18;
		index3 *= 18;
		for (int d = len; d <= reached_depth + 1; ++d)
		{
			if (depth_limited_search(index1, index2, index3, d, 324))
			{
				break;
			}
		}
		std::cout << "XXCross Solution from get_xxcross_scramble: " << AlgToString(sol) << std::endl;
		return tmp;
	}

	std::string start_search(std::string arg_scramble = "")
	{
		sol.clear();
		scramble = arg_scramble;
		max_length = 14;
		std::vector<int> alg = StringToAlg(scramble);
		cross_edges_goal_tmp = cross_edges_goal;
		F2L_slots_edges_goal_tmp = F2L_slots_edges_goal;
		F2L_slots_corners_goal_tmp = F2L_slots_corners_goal;
		index1 = array_to_index(cross_edges_goal_tmp, 4, 2, 12);
		index2 = array_to_index(F2L_slots_edges_goal_tmp, 2, 2, 12);
		index3 = array_to_index(F2L_slots_corners_goal_tmp, 2, 3, 8);
		for (int move : alg)
		{
			index1 = multi_move_table_cross_edges[index1 * 18 + move];
			index2 = multi_move_table_F2L_slots_edges[index2 * 18 + move];
			index3 = multi_move_table_F2L_slots_corners[index3 * 18 + move];
		}
		prune1_tmp = prune_table1[index1];
		prune23_tmp = prune_table23_couple[index2 * size3 + index3];
		if (prune1_tmp == 0 && prune23_tmp == 0)
		{
			std::cout << "Already solved state." << std::endl;
			return "";
		}
		index1 *= 18;
		index2 *= 18;
		index3 *= 18;
		for (int d = std::max(prune1_tmp, prune23_tmp); d <= max_length; ++d)
		{
			if (depth_limited_search(index1, index2, index3, d, 324))
			{
				break;
			}
		}
		std::cout << "XXCross Solution from start_search: " << AlgToString(sol) << std::endl;
		return tmp;
	}

	std::string func(std::string arg_scramble = "", std::string arg_length = "7")
	{
		std::string ret = arg_scramble + start_search(arg_scramble) + "," + get_xxcross_scramble(arg_length);
		return ret;
	}
};

int main()
{
	// Standard memory mode (default): 1600MB
	// Low memory test: Change to 1280MB or lower to test adaptive depth 7 expansion
	// Read MEMORY_LIMIT_MB from environment variable if set
	int memory_limit = 1600; // Default
	const char *env_limit = std::getenv("MEMORY_LIMIT_MB");
	if (env_limit != nullptr)
	{
		memory_limit = std::atoi(env_limit);
		std::cout << "Using MEMORY_LIMIT_MB from environment: " << memory_limit << " MB" << std::endl;
	}
	// Read VERBOSE from environment variable if set (default: true)
	bool verbose = true; // Default
	const char *env_verbose = std::getenv("VERBOSE");
	if (env_verbose != nullptr)
	{
		verbose = (std::string(env_verbose) != "0" && std::string(env_verbose) != "false" && std::string(env_verbose) != "False" && std::string(env_verbose) != "FALSE");
		std::cout << "Using VERBOSE from environment: " << (verbose ? "true" : "false") << std::endl;
	}
	xxcross_search xxcross_solver(true, 6, memory_limit, verbose); // BFS depth=6, local expansion to depths 7, 8 and 9
	std::string result;
	
	// Measure RSS after database construction (before search)
	std::cout << "\n=== RSS After Database Construction ===" << std::endl;
	std::cout << "RSS (before any search): " << (get_rss_kb() / 1024.0) << " MB" << std::endl;
	std::cout << "========================================" << std::endl;
	
	// exit(0);  // Immediately exit after database construction (for debugging)
	
	// Perform searches
	result = xxcross_solver.func("U R2 F B R B2 R U2 L B2 R U' D' R2 F R' L B2 U2 F2", "8");
	std::cout << "Result 8: " << result << std::endl;
	
	// Measure RSS after first search
	std::cout << "\n=== RSS After First Search ===" << std::endl;
	std::cout << "RSS (after search #1): " << (get_rss_kb() / 1024.0) << " MB" << std::endl;
	std::cout << "===============================" << std::endl;
	
	result = xxcross_solver.func("U R2 F B R B2 R U2 L B2 R U' D' R2 F R' L B2 U2 F2", "9");
	std::cout << "Result 9: " << result << std::endl;
	
	// Measure RSS after second search
	std::cout << "\n=== RSS After Second Search ===" << std::endl;
	std::cout << "RSS (after search #2): " << (get_rss_kb() / 1024.0) << " MB" << std::endl;
	std::cout << "================================" << std::endl;
	// result = xxcross_solver.func("U R2 F B R B2 R U2 L B2 R U' D' R2 F R' L B2 U2 F2", "3");
	// std::cout << "Result 3: " << result << std::endl;
	// result = xxcross_solver.func("U R2 F B R B2 R U2 L B2 R U' D' R2 F R' L B2 U2 F2", "5");
	// std::cout << "Result 5: " << result << std::endl;
	// result = xxcross_solver.func("U R2 F B R B2 R U2 L B2 R U' D' R2 F R' L B2 U2 F2", "6");
	// std::cout << "Result 6: " << result << std::endl;
	// result = xxcross_solver.func("U R2 F B R B2 R U2 L B2 R U' D' R2 F R' L B2 U2 F2", "6");
	// std::cout << "Result 6: " << result << std::endl;
	// result = xxcross_solver.func("U R2 F B R B2 R U2 L B2 R U' D' R2 F R' L B2 U2 F2", "7");
	// std::cout << "Result 7: " << result << std::endl;
	// result = xxcross_solver.func("U R2 F B R B2 R U2 L B2 R U' D' R2 F R' L B2 U2 F2", "8");
	// std::cout << "Result 8: " << result << std::endl;
	// result = xxcross_solver.func("U R2 F B R B2 R U2 L B2 R U' D' R2 F R' L B2 U2 F2", "7");
	// std::cout << "Result 7: " << result << std::endl;
	// result = xxcross_solver.func("U R2 F B R B2 R U2 L B2 R U' D' R2 F R' L B2 U2 F2", "8");
	// std::cout << "Result 8: " << result << std::endl;
	// result = xxcross_solver.func("U R2 F B R B2 R U2 L B2 R U' D' R2 F R' L B2 U2 F2", "7");
	// std::cout << "Result 7: " << result << std::endl;
	// result = xxcross_solver.func("U R2 F B R B2 R U2 L B2 R U' D' R2 F R' L B2 U2 F2", "8");
	// std::cout << "Result 8: " << result << std::endl;
	return 0;
}

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_BINDINGS(my_module)
{
	emscripten::class_<xxcross_search>("xxcross_search")
		.constructor<>()					 // Default: adj=true, BFS_DEPTH=6, MEMORY_LIMIT_MB=1600, verbose=true (auto-adjusted to 700 in WASM)
		.constructor<bool>()				 // adj
		.constructor<bool, int>()			 // adj, BFS_DEPTH
		.constructor<bool, int, int>()		 // adj, BFS_DEPTH, MEMORY_LIMIT_MB
		.constructor<bool, int, int, bool>() // adj, BFS_DEPTH, MEMORY_LIMIT_MB, verbose
		.function("func", &xxcross_search::func);
}
#endif