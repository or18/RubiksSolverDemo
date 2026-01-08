#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>
#include <emscripten/heap.h>
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
#include <iomanip>
#include <tsl/robin_set.h>
#include "bucket_config.h"

#ifndef __EMSCRIPTEN__
#include <malloc.h>  // For malloc_trim
#endif

#pragma GCC target("avx2")

// Helper function to log Emscripten heap usage
#ifdef __EMSCRIPTEN__
inline void log_emscripten_heap(const char* phase_name) {
	size_t total_heap_size = emscripten_get_heap_size();
	
	std::cout << "[Heap] " << phase_name << ": ";
	std::cout << "Total=" << (total_heap_size / 1024 / 1024) << " MB";
	std::cout << std::endl;
}
#else
inline void log_emscripten_heap(const char*) {}
#endif

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
		// Pre-reserve for fixed sizes (8 corners, 12 edges)
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
		cur.reserve(32);    // Initial nodes (measured: 17, reserving 2x)
		next.reserve(600);  // Next depth (measured: 255, reserving 2x)
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
			// Pre-reserve to reduce reallocation spikes during BFS expansion
			size_t estimated_next_nodes = expected_nodes_per_depth[next_depth_idx];
			if (estimated_next_nodes > 0)
			{
				index_pairs[next_depth_idx].reserve(estimated_next_nodes);
			}
			next.attach_element_vector(&index_pairs[next_depth_idx]);
			if (verbose)
			{
				std::cout << "  [Element vector] Attached to depth=" << next_depth_idx << " (reserved: " << estimated_next_nodes << ")" << std::endl;
			}
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
	17,        // depth 0: 17 goal states
	294,       // depth 1: measured
	3777,      // depth 2: measured
	46949,     // depth 3: measured
	561768,    // depth 4: measured
	6741216,   // depth 5: estimated (561768 × 12)
	66869540,  // depth 6: estimated (6741216 × 12)
	SIZE_MAX,  // depth 7+: infinity
	SIZE_MAX}; // depth 8 represents "infinity" with SIZE_MAX

int create_prune_table_sparse(int index1, int index2, int index3, int size1, int size2, int size3, int max_depth, int max_memory_kb, const std::vector<int> &table1, const std::vector<int> &table2, const std::vector<int> &table3, std::vector<std::vector<uint64_t>> &index_pairs, std::vector<int> &num_list, bool verbose = true, const ResearchConfig& research_config = ResearchConfig())
{
	// Memory limit handling
	const size_t fixed_overhead_kb = 20 * 1024;
	const size_t adjusted_memory_kb = (static_cast<size_t>(max_memory_kb) > fixed_overhead_kb) ? (static_cast<size_t>(max_memory_kb) - fixed_overhead_kb) : static_cast<size_t>(max_memory_kb);
	
	// If ignore_memory_limits is set, use very large budget
	const size_t available_bytes = research_config.ignore_memory_limits 
		? SIZE_MAX / 64  // Effectively unlimited
		: static_cast<size_t>(adjusted_memory_kb) * 1024;
	
	const size_t bytes_per_node = 32; // robin_set(24) + index_pairs(8)
	const uint64_t node_cap = available_bytes / bytes_per_node;

	SlidingDepthSets visited(node_cap, verbose);
	const uint64_t size23 = static_cast<size_t>(size2 * size3);
	num_list.resize(max_depth + 1, 0);
	index_pairs.resize(max_depth + 1);

	// Reserve initial capacity (capacity at depth 0 is small)
	index_pairs[0].reserve(17); // Initial nodes include 16 free pair state nodes

	if (research_config.ignore_memory_limits) {
		std::cout << "Memory budget: UNLIMITED (research mode)" << std::endl;
	} else {
		std::cout << "Memory budget: " << (max_memory_kb / 1024.0) << " MB"
				  << " | Available: " << (available_bytes / 1024.0 / 1024.0) << " MB"
				  << " | Node capacity: " << node_cap << std::endl;
	}

	const int index23 = index2 * size3 + index3;
	const uint64_t index123 = index1 * size23 + index23;
	visited.set_initial(index123);
	index_pairs[0].emplace_back(index123);

	// Insert 16 free pair state nodes at depth 0
	// NOTE: Cross edges (index1) remain solved; only F2L slots (index2, index3) are affected by free pair algorithms
	std::vector<std::string> auf = {"", "U", "U2", "U'"};
	std::vector<std::string> appl_moves = {"L U L'", "L U' L'", "B' U B", "B' U' B"};
	uint64_t index123_tmp_2;
	int index23_tmp_2, index1_tmp_2, index2_tmp_2, index3_tmp_2;
	for (int i  = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			// index1 (cross edges) stays solved - free pairs don't affect the cross
			index1_tmp_2 = index1;
			index2_tmp_2 = index2;
			index3_tmp_2 = index3;
			for (int m : StringToAlg(appl_moves[i] + " " + auf[j]))
			{
				// Only apply moves to F2L slots (index2, index3), not cross edges (index1)
				index2_tmp_2 = table2[index2_tmp_2 * 18 + m];
				index3_tmp_2 = table3[index3_tmp_2 * 18 + m];
			}
			index23_tmp_2 = index2_tmp_2 * size3 + index3_tmp_2;
			index123_tmp_2 = index1_tmp_2 * size23 + index23_tmp_2;
			visited.set_initial(index123_tmp_2);
			index_pairs[0].emplace_back(index123_tmp_2);
		}
	}

	num_list[0] = 17;

	// Attach element_vector for depth=1
	if (max_depth >= 1)
	{
		index_pairs[1].clear();
		// Pre-reserve for predictable depth=1 size (measured: 255 nodes, reserving 2x for safety)
		index_pairs[1].reserve(600);
		visited.next.attach_element_vector(&index_pairs[1]);
		if (verbose)
		{
			std::cout << "[Element vector] Attached to depth=1 (reserved: 20)" << std::endl;
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
		
		// Next-depth reserve optimization (full BFS mode only)
		if (research_config.enable_next_depth_reserve && !research_config.enable_local_expansion)
		{
			int future_depth = next_depth + 1;
			if (future_depth <= max_depth)
			{
				// Predict next depth size using current depth size
				size_t current_nodes = num_list[next_depth];
				size_t predicted_nodes = static_cast<size_t>(
					static_cast<float>(current_nodes) * research_config.next_depth_reserve_multiplier);
				
				// Apply upper limit to prevent memory explosion
				if (predicted_nodes > research_config.max_reserve_nodes)
				{
					predicted_nodes = research_config.max_reserve_nodes;
					if (verbose)
					{
						std::cout << "[Reserve] Capped prediction at max_reserve_nodes: "
								  << research_config.max_reserve_nodes << std::endl;
					}
				}
				
				// Reserve next depth
				if (predicted_nodes > 0)
				{
					index_pairs[future_depth].reserve(predicted_nodes);
					if (verbose)
					{
						std::cout << "[Reserve] depth=" << future_depth
								  << " reserved " << predicted_nodes << " nodes"
								  << " (multiplier=" << research_config.next_depth_reserve_multiplier
								  << " × " << current_nodes << " current nodes)" << std::endl;
					}
				}
			}
		}
	}

	// Process the remainder at the end of the loop
	int cur_depth = completed_depth + 1;
	int next_d = completed_depth + 2;

	if (verbose)
	{
		std::cout << "[BFS Cleanup] Processing remaining nodes..." << std::endl;
	}

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

	if (verbose)
	{
		std::cout << "[BFS Cleanup] Clearing visited sets..." << std::endl;
	}
	visited.clear_all();
	
	if (verbose)
	{
		std::cout << "[BFS Cleanup] Complete. Returning depth " << (stop ? (next_depth - 1) : next_depth) << std::endl;
	}
	return stop ? (next_depth - 1) : next_depth;
}

// ============================================================================
// Step 0: Local Expansion - Bucket size determination functions
// ============================================================================

const size_t MIN_BUCKET = (1 << 21); // 2M (2^21)
const size_t BYTES_PER_BUCKET = 4;	 // robin_hash bucket overhead
const size_t BYTES_PER_NODE = 32;	 // robin_set (24) + index_pairs (8)

#include "expansion_parameters.h" // Depth- and bucket-size-dependent parameter table --> not used currently?

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

		// Check against available memory (margin removed - handled by outer C++ cushion)
		if (predicted_rss <= available_memory)
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

			// Check against available memory (margin removed - handled by outer C++ cushion)
			if (predicted_rss <= available_memory)
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
	// Pre-reserve capacity to prevent reallocation spikes during attach
	size_t estimated_n1_capacity = std::min(config.nodes_n1, static_cast<size_t>(config.bucket_n1 * 0.9));
	index_pairs[depth_n1].reserve(estimated_n1_capacity);
	depth_n1_nodes.attach_element_vector(&index_pairs[depth_n1]);
	if (verbose)
	{
		std::cout << "  [Step 2] Element vector attached to depth=" << depth_n1 << " (reserved: " << estimated_n1_capacity << ")" << std::endl;
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
	// Pre-declare variables outside loop to avoid repeated allocations
	int edge_index, corner_index, f2l_edge_index;
	uint64_t index23_bt;
	size_t edge_table_index, corner_table_index, f2l_edge_table_index;
	int next_edge, next_corner, next_f2l_edge, next_index23;
	uint64_t next_node_bt;

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
		edge_index = node / size23;
		index23_bt = node % size23;
		corner_index = index23_bt / size2;	  // F2L_corners
		f2l_edge_index = index23_bt % size2; // F2L_edges

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
		// Pre-declare variables outside loop to avoid repeated allocations
		bool is_depth_n_plus_1 = false;
		size_t back_edge_table_index, back_corner_table_index, back_f2l_edge_table_index;
		int parent_edge, parent_corner, parent_f2l_edge, parent_index23;
		uint64_t parent;

		for (int back_move = 0; back_move < 18; ++back_move)
		{
			// Boundary check for backtrace table access
			back_edge_table_index = static_cast<size_t>(next_edge) * 18 + back_move;
			back_corner_table_index = static_cast<size_t>(next_corner) * 18 + back_move;
			back_f2l_edge_table_index = static_cast<size_t>(next_f2l_edge) * 18 + back_move;

			if (back_edge_table_index >= multi_move_table_cross_edges.size() ||
				back_corner_table_index >= multi_move_table_F2L_slots_corners.size() ||
				back_f2l_edge_table_index >= multi_move_table_F2L_slots_edges.size())
			{
				break;
			}

			parent_edge = multi_move_table_cross_edges[back_edge_table_index];
			parent_corner = multi_move_table_F2L_slots_corners[back_corner_table_index];
			parent_f2l_edge = multi_move_table_F2L_slots_edges[back_f2l_edge_table_index];

			parent_index23 = parent_corner * size2 + parent_f2l_edge;
			parent = static_cast<uint64_t>(parent_edge) * size23 + parent_index23;
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
	bool verbose = true,
	const BucketConfig& bucket_config = BucketConfig(),
	const ResearchConfig& research_config = ResearchConfig())
{
	if (verbose)
	{
		std::cout << "\n=== Complete Search Database Construction ===" << std::endl;
		std::cout << "Phase 1: BFS (depth 0 to " << BFS_DEPTH << ")" << std::endl;
		std::cout << "Phase 2: Local Expansion depth 5→6 (Partial Expansion)" << std::endl;
		std::cout << "Phase 3: Local Expansion depth 6→7 (with depth 5 check)" << std::endl;
		std::cout << "Phase 4: Local Expansion depth 7→8 (with depth 5 check)" << std::endl;
		std::cout << "Phase 5: Local Expansion depth 8→9 (with depth 5 check)" << std::endl;
		
		if (research_config.ignore_memory_limits) {
			std::cout << "Memory limit: UNLIMITED (research mode)" << std::endl;
		} else {
			std::cout << "Memory limit: " << MEMORY_LIMIT_MB << " MB" << std::endl;
		}
		
		if (!research_config.enable_local_expansion) {
			std::cout << "Local expansion: DISABLED (full BFS only)" << std::endl;
		}
		
		if (research_config.force_full_bfs_to_depth >= 0) {
			std::cout << "Forced full BFS to depth: " << research_config.force_full_bfs_to_depth << std::endl;
		}
	}

	// ============================================================================
	// Phase 1: Execute BFS
	// ============================================================================
	
	// Determine effective BFS depth
	int effective_bfs_depth = BFS_DEPTH;
	if (research_config.force_full_bfs_to_depth >= 0) {
		effective_bfs_depth = research_config.force_full_bfs_to_depth;
		if (verbose) {
			std::cout << "Forced BFS depth: " << effective_bfs_depth << " (overriding default " << BFS_DEPTH << ")" << std::endl;
		}
	}
	
	int reached_depth = create_prune_table_sparse(
		index1, index2, index3,
		size1, size2, size3,
		effective_bfs_depth,
		1024 * MEMORY_LIMIT_MB,
		multi_move_table_cross_edges,
		multi_move_table_F2L_slots_edges,
		multi_move_table_F2L_slots_corners,
		index_pairs,
		num_list,
		verbose,
		research_config);

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
	if (reached_depth >= 5)
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
	// Phase 2-4: Gradual Local Expansion (depth 6, 8, 9) --> (6, 7, 8)
	// ============================================================================
	if (reached_depth < 5)
	{
		if (verbose)
		{
			std::cout << "\nPhase 2-4 Skipped: BFS depth < 5" << std::endl;
		}
		return;
	}
	
	// Check if local expansion is disabled (research mode: full BFS only)
	if (!research_config.enable_local_expansion)
	{
		if (verbose)
		{
			std::cout << "\nPhase 2-4 Skipped: Local expansion disabled (full BFS mode)" << std::endl;
			std::cout << "Research mode: Only BFS depths 0-" << reached_depth << " were explored" << std::endl;
		}
		
		// Collect statistics if requested
		if (research_config.collect_detailed_statistics)
		{
			std::cout << "\n=== Full BFS Statistics (CSV Format) ===" << std::endl;
			std::cout << "depth,nodes,rss_mb" << std::endl;
			for (int d = 0; d <= reached_depth; ++d)
			{
				size_t rss_kb = get_rss_kb();
				std::cout << d << "," << index_pairs[d].size() << "," << (rss_kb / 1024.0) << std::endl;
			}
		}
		return;
	}

	// New Implementation: Phase 2-4 Phased Local Expansion
	// ============================================================================
	if (verbose)
	{
		std::cout << "\n=== New Algorithm: Gradual Local Expansion ===" << std::endl;
		std::cout << "Phase 2: Partial expansion depth 6" << std::endl;
		std::cout << "Phase 3: Local expansion depth 6→7 (depth 5 check)" << std::endl;
		std::cout << "Phase 4: Local expansion depth 7→8 (depth 5 distance check)" << std::endl;
		std::cout << "\n[BFS Complete - RSS Check]" << std::endl;
		std::cout << "RSS after BFS (before depth_5_nodes): " << (get_rss_kb() / 1024.0) << " MB" << std::endl;
	}

	// ============================================================================
	// Preparation: Load depth 5 nodes into robin_set
	// ============================================================================
	tsl::robin_set<uint64_t> depth_5_nodes;
	depth_5_nodes.max_load_factor(0.9f);
	for (uint64_t node : index_pairs[5])
	{
		depth_5_nodes.insert(node);
	}

	if (verbose)
	{
		std::cout << "RSS after depth_5_nodes creation: " << (get_rss_kb() / 1024.0) << " MB" << std::endl;
		std::cout << "depth_5_nodes size: " << depth_5_nodes.size()
				  << ", buckets: " << depth_5_nodes.bucket_count() << std::endl;
	}

	// Memory calculation
	size_t current_memory_bytes = 0;
	for (size_t d = 0; d <= static_cast<size_t>(reached_depth); ++d)
	{
		current_memory_bytes += index_pairs[d].size() * 8;
	}
	current_memory_bytes += depth_5_nodes.size() * 24 + depth_5_nodes.bucket_count() * 4;

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

	// ============================================================================
	// RSS Checkpoint: After Phase 1
	// ============================================================================
	if (verbose)
	{
		std::cout << "\n=== RSS Checkpoint: After Phase 1 ===" << std::endl;
		size_t rss_phase1 = get_rss_kb();
		std::cout << "RSS: " << (rss_phase1 / 1024.0) << " MB" << std::endl;
		log_emscripten_heap("Phase 1 Complete");
		std::cout << "========================================\n" << std::endl;
	}

	const size_t MIN_BUCKET = (1 << 21); // 2M
	const uint64_t size23 = static_cast<uint64_t>(size2 * size3);

	// ============================================================================
	// Bucket Size Configuration (CUSTOM Model Only)
	// ============================================================================
	size_t bucket_6, bucket_7, bucket_8;

	if (bucket_config.model == BucketModel::CUSTOM && research_config.enable_custom_buckets)
	{
		// Use custom bucket sizes directly (no auto-calculation)
		bucket_6 = bucket_config.custom_bucket_6; // Warning: bucket_config.h is not 
		bucket_7 = bucket_config.custom_bucket_7;
		bucket_8 = bucket_config.custom_bucket_8;

		if (verbose)
		{
			std::cout << "\n[Using CUSTOM Bucket Configuration]" << std::endl;
			std::cout << "  depth 6 bucket: " << bucket_6 << " (" << (bucket_6 / (1 << 20)) << "M)" << std::endl;
			std::cout << "  depth 7 bucket: " << bucket_7 << " (" << (bucket_7 / (1 << 20)) << "M)" << std::endl;
			std::cout << "  depth 8 bucket: " << bucket_8 << " (" << (bucket_8 / (1 << 20)) << "M)" << std::endl;
		}
	}
	else
	{
		throw std::runtime_error("Only CUSTOM bucket model is supported in development mode. Set bucket_config.model=BucketModel::CUSTOM and research_config.enable_custom_buckets=true");
	}

	// ============================================================================
	// Phase 2: depth 6 Partial expansion
	// ============================================================================
	if (verbose)
	{
		std::cout << "\n--- Phase 2: Partial Expansion (depth 6) ---" << std::endl;
		std::cout << "Using pre-calculated bucket size for depth 6: " << bucket_6
				  << " (" << (bucket_6 / (1 << 20)) << "M)" << std::endl;
		std::cout << "RSS before depth_6_nodes creation: " << (get_rss_kb() / 1024.0) << " MB" << std::endl;
	}

	// Generate depth 6 nodes
	if (index_pairs.size() <= 6)
	{
		index_pairs.resize(7);
	}
	index_pairs[6].clear();

	tsl::robin_set<uint64_t> depth_6_nodes;
	depth_6_nodes.clear();
	depth_6_nodes.max_load_factor(0.9f);
	depth_6_nodes.rehash(bucket_6);
	// Pre-reserve capacity to prevent reallocation spikes during attach
	size_t estimated_d6_capacity = static_cast<size_t>(bucket_6 * 0.95);
	index_pairs[6].reserve(estimated_d6_capacity);
	depth_6_nodes.attach_element_vector(&index_pairs[6]);

	if (verbose)
	{
		std::cout << "RSS after depth_6_nodes creation (empty): " << (get_rss_kb() / 1024.0) << " MB" << std::endl;
	}

	// Adaptive children per parent for depth 5→6
	std::vector<uint64_t> depth5_vec;
	depth5_vec.reserve(depth_5_nodes.size()); // Pre-reserve to avoid reallocation spike
	depth5_vec.assign(depth_5_nodes.begin(), depth_5_nodes.end());
	std::mt19937_64 rng_d6(std::random_device{}());

	// Calculate adaptive children per parent with face diversity
	// Target: 90% load factor with proper parent diversity
	size_t target_nodes_d6 = static_cast<size_t>(bucket_6 * 0.9);
	size_t available_parents = depth5_vec.size();
	int children_per_parent_d6 = 1;
	if (available_parents > 0) {
		// Account for ~10% rejection from duplicates
		double rejection_factor = 1.1;
		children_per_parent_d6 = static_cast<int>(std::ceil(
			(target_nodes_d6 * rejection_factor) / available_parents
		));
		// Clamp to reasonable range [1, 18]
		children_per_parent_d6 = std::min(children_per_parent_d6, 18);
		children_per_parent_d6 = std::max(children_per_parent_d6, 1);
	}

	// Random parent sampling strategy: Expand until rehash threshold
	// Use face-diverse move selection to avoid hash collisions
	size_t max_parent_nodes = depth5_vec.size() * 20; // Safety limit
	static std::mt19937_64 gen_d6(rng_d6());
	std::uniform_int_distribution<size_t> dist_d6(0, depth5_vec.size() - 1);
	std::uniform_int_distribution<int> face_dist_d6(0, 5);  // 6 faces
	std::uniform_int_distribution<int> rotation_dist_d6(0, 2);  // 3 rotations per face

	if (verbose)
	{
		std::cout << "Random parent sampling from depth 6" << std::endl;
		std::cout << "Strategy: Face-diverse expansion until rehash" << std::endl;
		std::cout << "Max load factor: 0.9 (90%)" << std::endl;
		std::cout << "Available parent nodes: " << available_parents << std::endl;
		std::cout << "Children per parent (adaptive): " << children_per_parent_d6 << std::endl;
		std::cout << "Expected nodes: ~" << std::min(available_parents * children_per_parent_d6, target_nodes_d6) << std::endl;
	}

	// Expand depth 5 → 6 via random parent sampling
	// Pre-declare loop variables outside to avoid repeated allocations
	size_t random_idx, processed_parents = 0;
	uint64_t parent_node;
	uint64_t index1_cur, index23, index2_cur, index3_cur;
	int index1_tmp, index2_tmp, index3_tmp;
	uint64_t next_index1, next_index2, next_index3, next_node123;
	const size_t last_bucket_count = depth_6_nodes.bucket_count();
	size_t duplicate_count_d6 = 0;
	size_t inserted_count_d6 = 0;

	while (processed_parents < max_parent_nodes)
	{
		// Randomly select parent node (duplicates possible, negligible probability)
		random_idx = dist_d6(gen_d6);
		parent_node = depth5_vec[random_idx];

		// Decompose parent node indices
		index1_cur = parent_node / size23;
		index23 = parent_node % size23;
		index2_cur = index23 / size3;
		index3_cur = index23 % size3;

		index1_tmp = static_cast<int>(index1_cur) * 18;
		index2_tmp = static_cast<int>(index2_cur) * 18;
		index3_tmp = static_cast<int>(index3_cur) * 18;

		// Generate moves with face diversity
		// Ensure we sample from different faces to avoid hash collisions
		std::vector<int> selected_moves;
		if (children_per_parent_d6 >= 18) {
			// Use all 18 moves
			for (int m = 0; m < 18; ++m) selected_moves.push_back(m);
		} else if (children_per_parent_d6 <= 6) {
			// Select one move per face for diversity (up to 6 faces)
			std::vector<int> faces = {0, 1, 2, 3, 4, 5};
			std::shuffle(faces.begin(), faces.end(), gen_d6);
			for (int i = 0; i < children_per_parent_d6; ++i) {
				int face = faces[i];
				int rotation = rotation_dist_d6(gen_d6);
				selected_moves.push_back(face * 3 + rotation);
			}
		} else {
			// For 7-17 children: ensure all 6 faces covered, then add more
			std::vector<int> all_moves(18);
			for (int m = 0; m < 18; ++m) all_moves[m] = m;
			std::shuffle(all_moves.begin(), all_moves.end(), gen_d6);
			// First 6: ensure face diversity
			std::vector<bool> face_covered(6, false);
			for (int m : all_moves) {
				int face = m / 3;
				if (!face_covered[face]) {
					selected_moves.push_back(m);
					face_covered[face] = true;
					if (selected_moves.size() >= 6) break;
				}
			}
			// Add remaining moves randomly
			for (int m : all_moves) {
				if (selected_moves.size() >= static_cast<size_t>(children_per_parent_d6)) break;
				if (std::find(selected_moves.begin(), selected_moves.end(), m) == selected_moves.end()) {
					selected_moves.push_back(m);
				}
			}
		}

		for (int move : selected_moves)
		{
			next_index1 = multi_move_table_cross_edges[index1_tmp + move];
			next_index2 = multi_move_table_F2L_slots_edges[index2_tmp + move];
			next_index3 = multi_move_table_F2L_slots_corners[index3_tmp + move];
			next_node123 = next_index1 * size23 + next_index2 * size3 + next_index3;

			// Skip if already in depth 5 or depth 6
			if (depth_5_nodes.find(next_node123) != depth_5_nodes.end() ||
				depth_6_nodes.find(next_node123) != depth_6_nodes.end())
			{
				duplicate_count_d6++;
				continue;
			}

			// Check for rehash before insert
			if (depth_6_nodes.will_rehash_on_next_insert())
			{
				if (verbose)
				{
					std::cout << "\nRehash threshold reached, stopping depth 6 expansion" << std::endl;
					std::cout << "Processed parents: " << processed_parents << std::endl;
					std::cout << "Inserted: " << inserted_count_d6 << ", Duplicates: " << duplicate_count_d6 << std::endl;
				}
				goto phase2_done;
			}

			depth_6_nodes.insert(next_node123);
			inserted_count_d6++;

			// Check for unexpected rehash (bucket count changed)
			const size_t current_bucket_count = depth_6_nodes.bucket_count();
			if (current_bucket_count != last_bucket_count)
			{
				if (verbose)
				{
					std::cout << "\nUnexpected rehash detected (buckets: "
							  << last_bucket_count << " -> " << current_bucket_count << ")" << std::endl;
					std::cout << "Stopping depth 6 expansion" << std::endl;
				}
				goto phase2_done;
			}
		}

		processed_parents++;
	}

phase2_done:
	// Save size before detach
	size_t depth_6_final_size = depth_6_nodes.size();
	depth_6_nodes.detach_element_vector();

	// depth_6_nodes no longer needed - explicitly free it
	size_t rss_before_d6_free = get_rss_kb();
	{
		tsl::robin_set<uint64_t> temp;
		depth_6_nodes.swap(temp);
	}
	size_t rss_after_d6_free = get_rss_kb();

	if (verbose)
	{
		size_t freed_kb = (rss_before_d6_free > rss_after_d6_free) ? (rss_before_d6_free - rss_after_d6_free) : 0;
		std::cout << "RSS before depth_6_nodes free: " << (rss_before_d6_free / 1024.0) << " MB" << std::endl;
		std::cout << "RSS after depth_6_nodes freed: " << (rss_after_d6_free / 1024.0) << " MB" << std::endl;
		std::cout << "Memory freed by depth_6_nodes: " << (freed_kb / 1024.0) << " MB" << std::endl;
	}

	if (num_list.size() <= 6)
	{
		num_list.resize(7, 0);
	}
	num_list[6] = static_cast<int>(depth_6_final_size);

	if (verbose)
	{
		std::cout << "\n[Phase 2 Complete]" << std::endl;
		std::cout << "Generated " << depth_6_final_size << " nodes at depth=6" << std::endl;
		std::cout << "Load factor: " << (depth_6_final_size * 100.0 / bucket_6) << "%" << std::endl;
		std::cout << "Inserted: " << inserted_count_d6 << ", Duplicates: " << duplicate_count_d6 << std::endl;
		double rejection_rate_d6 = (inserted_count_d6 + duplicate_count_d6) > 0 
			? (100.0 * duplicate_count_d6 / (inserted_count_d6 + duplicate_count_d6)) : 0.0;
		std::cout << "Rejection rate: " << rejection_rate_d6 << "%" << std::endl;
	}

	// NOTE: depth_5_nodes is still needed for Phase 3 (depth 7 expansion) and Phase 4 (depth 8 expansion)
	// DO NOT free it here - will be freed after Phase 4 completes

	if (verbose)
	{
		std::cout << "RSS after depth 6 generation: " << (get_rss_kb() / 1024.0) << " MB" << std::endl;
		std::cout << "Note: depth_5_nodes retained for Phase 3/4 validation (~" << (depth_5_nodes.size() * 32 / 1024 / 1024) << " MB)" << std::endl;
	}

	// Calculate current memory usage after depth 6
	// Note: depth_5_nodes already freed above
	size_t current_memory_d6 = 0;
	size_t theoretical_phase2 = 0;
	for (size_t d = 0; d <= 6; ++d)
	{
		current_memory_d6 += index_pairs[d].size() * 8; // 8 bytes per node
		theoretical_phase2 += index_pairs[d].size() * 8;
	}
	// depth_5_nodes is already freed, so no need to count it

	size_t total_memory_bytes = static_cast<size_t>(MEMORY_LIMIT_MB) * 1024 * 1024;
	size_t remaining_memory_d6 = total_memory_bytes - current_memory_d6;

	if (verbose)
	{
		std::cout << "\n[Phase 2 Complete - Memory Analysis]" << std::endl;
		std::cout << "Theoretical depth 0-6 (index_pairs): " << (theoretical_phase2 / 1024.0 / 1024.0) << " MB" << std::endl;
		std::cout << "Current memory estimate: " << (current_memory_d6 / 1024.0 / 1024.0) << " MB" << std::endl;
		std::cout << "Overhead ratio: " << (static_cast<double>(current_memory_d6) / theoretical_phase2) << "x" << std::endl;
		std::cout << "Actual RSS: " << (get_rss_kb() / 1024.0) << " MB" << std::endl;
		log_emscripten_heap("Phase 2 Complete");
		if (verbose)
		{
			std::cout << "Remaining memory: " << (remaining_memory_d6 / 1024.0 / 1024.0) << " MB" << std::endl;
		}
	}

	// ============================================================================
	// Phase 3: depth 7 Local Expansion (with depth 6 check, random sampling)
	// ============================================================================
	if (verbose)
	{
		std::cout << "\n--- Phase 3: Local Expansion depth 6→7 (with depth 5 check, random sampling) ---" << std::endl;
		std::cout << "RSS at Phase 3 start (depth_6_nodes should be destroyed): " << (get_rss_kb() / 1024.0) << " MB" << std::endl;
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

	tsl::robin_set<uint64_t> depth_7_nodes;
	depth_7_nodes.clear();
	depth_7_nodes.max_load_factor(0.9f);
	depth_7_nodes.rehash(bucket_7);
	// Pre-reserve capacity to prevent reallocation spikes during attach
	size_t estimated_d7_capacity = static_cast<size_t>(bucket_7 * 0.95);
	index_pairs[7].reserve(estimated_d7_capacity);
	depth_7_nodes.attach_element_vector(&index_pairs[7]);

	if (verbose)
	{
		std::cout << "RSS after depth_7_nodes creation (empty): " << (get_rss_kb() / 1024.0) << " MB" << std::endl;
	}

	size_t rejected_as_depth6 = 0;
	size_t inserted_count = 0; // Track successful inserts for diagnostics

	// Advance one move from all depth 6 nodes
	// Direct vector construction (no intermediate robin_set to avoid memory spike)
	std::vector<uint64_t> depth6_vec;
	depth6_vec.reserve(index_pairs[6].size());
	depth6_vec.assign(index_pairs[6].begin(), index_pairs[6].end());
	
	// Create robin_set for depth 6 duplicate checking (used in depth 7 expansion)
	tsl::robin_set<uint64_t> depth6_set;
	depth6_set.max_load_factor(0.9f);
	depth6_set.reserve(depth6_vec.size());
	for (uint64_t node : depth6_vec) {
		depth6_set.insert(node);
	}
	std::mt19937_64 rng_phase3(std::random_device{}());

	// Calculate adaptive children per parent with face diversity
	size_t target_nodes_d7 = static_cast<size_t>(bucket_7 * 0.9);
	size_t available_parents_d7 = depth6_vec.size();
	int children_per_parent_d7 = 1;
	if (available_parents_d7 > 0) {
		double rejection_factor = 1.1;
		children_per_parent_d7 = static_cast<int>(std::ceil(
			(target_nodes_d7 * rejection_factor) / available_parents_d7
		));
		children_per_parent_d7 = std::min(children_per_parent_d7, 18);
		children_per_parent_d7 = std::max(children_per_parent_d7, 1);
	}

	// Random parent sampling strategy: Expand until rehash threshold
	size_t max_parent_nodes_d7 = depth6_vec.size() * 20;
	static std::mt19937_64 gen_d7(rng_phase3());
	std::uniform_int_distribution<size_t> dist_d7(0, depth6_vec.size() - 1);
	std::uniform_int_distribution<int> face_dist_d7(0, 5);
	std::uniform_int_distribution<int> rotation_dist_d7(0, 2);

	if (verbose)
	{
		std::cout << "Random parent sampling from depth 6" << std::endl;
		std::cout << "Strategy: Face-diverse expansion until rehash" << std::endl;
		std::cout << "Max load factor: 0.9 (90%)" << std::endl;
		std::cout << "Available parent nodes: " << available_parents_d7 << std::endl;
		std::cout << "Children per parent (adaptive): " << children_per_parent_d7 << std::endl;
		std::cout << "Expected nodes: ~" << std::min(available_parents_d7 * children_per_parent_d7, target_nodes_d7) << std::endl;
	}

	// Pre-declare loop variables outside to avoid repeated allocations
	size_t random_idx_d7, processed_parents_d7 = 0;
	uint64_t parent_node_d7;
	uint64_t index1_cur_d7, index23_d7, index2_cur_d7, index3_cur_d7;
	int index1_tmp_d7, index2_tmp_d7, index3_tmp_d7;
	uint64_t next_index1_d7, next_index2_d7, next_index3_d7, next_node123_d7;
	const size_t last_bucket_count_d7 = depth_7_nodes.bucket_count();
	size_t duplicate_count_d7 = 0;
	size_t duplicates_from_depth5_d7 = 0;  // Counter for depth_5 duplicates
	size_t duplicates_from_depth6_d7 = 0;  // Counter for depth_6 duplicates
	size_t inserted_count_d7 = 0;

	while (processed_parents_d7 < max_parent_nodes_d7)
	{
		// Randomly select parent node
		random_idx_d7 = dist_d7(gen_d7);
		parent_node_d7 = depth6_vec[random_idx_d7];

		// Decompose parent node indices
		index1_cur_d7 = parent_node_d7 / size23;
		index23_d7 = parent_node_d7 % size23;
		index2_cur_d7 = index23_d7 / size3;
		index3_cur_d7 = index23_d7 % size3;

		index1_tmp_d7 = static_cast<int>(index1_cur_d7) * 18;
		index2_tmp_d7 = static_cast<int>(index2_cur_d7) * 18;
		index3_tmp_d7 = static_cast<int>(index3_cur_d7) * 18;

		// Generate moves with face diversity
		std::vector<int> selected_moves_d7;
		if (children_per_parent_d7 >= 18) {
			for (int m = 0; m < 18; ++m) selected_moves_d7.push_back(m);
		} else if (children_per_parent_d7 <= 6) {
			std::vector<int> faces = {0, 1, 2, 3, 4, 5};
			std::shuffle(faces.begin(), faces.end(), gen_d7);
			for (int i = 0; i < children_per_parent_d7; ++i) {
				int face = faces[i];
				int rotation = rotation_dist_d7(gen_d7);
				selected_moves_d7.push_back(face * 3 + rotation);
			}
		} else {
			std::vector<int> all_moves(18);
			for (int m = 0; m < 18; ++m) all_moves[m] = m;
			std::shuffle(all_moves.begin(), all_moves.end(), gen_d7);
			std::vector<bool> face_covered(6, false);
			for (int m : all_moves) {
				int face = m / 3;
				if (!face_covered[face]) {
					selected_moves_d7.push_back(m);
					face_covered[face] = true;
					if (selected_moves_d7.size() >= 6) break;
				}
			}
			for (int m : all_moves) {
				if (selected_moves_d7.size() >= static_cast<size_t>(children_per_parent_d7)) break;
				if (std::find(selected_moves_d7.begin(), selected_moves_d7.end(), m) == selected_moves_d7.end()) {
					selected_moves_d7.push_back(m);
				}
			}
		}

		for (int move : selected_moves_d7)
		{
			next_index1_d7 = multi_move_table_cross_edges[index1_tmp_d7 + move];
			next_index2_d7 = multi_move_table_F2L_slots_edges[index2_tmp_d7 + move];
			next_index3_d7 = multi_move_table_F2L_slots_corners[index3_tmp_d7 + move];
			next_node123_d7 = next_index1_d7 * size23 + next_index2_d7 * size3 + next_index3_d7;

			// Skip if already in depth 5, depth 6, or depth 7
			if (depth_5_nodes.find(next_node123_d7) != depth_5_nodes.end()) {
				duplicate_count_d7++;
				duplicates_from_depth5_d7++;
				continue;
			}
			if (depth6_set.find(next_node123_d7) != depth6_set.end()) {
				duplicate_count_d7++;
				duplicates_from_depth6_d7++;
				continue;
			}
			if (depth_7_nodes.find(next_node123_d7) != depth_7_nodes.end()) {
				duplicate_count_d7++;
				continue;
			}

			// Check for rehash before insert
			if (depth_7_nodes.will_rehash_on_next_insert())
			{
				if (verbose)
				{
					std::cout << "\nRehash threshold reached, stopping depth 7 expansion" << std::endl;
					std::cout << "Processed parents: " << processed_parents_d7 << std::endl;
					std::cout << "Inserted: " << inserted_count_d7 << ", Duplicates: " << duplicate_count_d7 << std::endl;
				}
				goto phase3_done;
			}

			depth_7_nodes.insert(next_node123_d7);
			inserted_count_d7++;

			// Check for unexpected rehash (bucket count changed)
			const size_t current_bucket_count_d7 = depth_7_nodes.bucket_count();
			if (current_bucket_count_d7 != last_bucket_count_d7)
			{
				if (verbose)
				{
					std::cout << "\nUnexpected rehash detected (buckets: "
							  << last_bucket_count_d7 << " -> " << current_bucket_count_d7 << ")" << std::endl;
					std::cout << "Stopping depth 7 expansion" << std::endl;
				}
				goto phase3_done;
			}
		}

		processed_parents_d7++;
	}

phase3_done:
	// Save size before detach
	size_t depth_7_final_size = depth_7_nodes.size();

	depth_7_nodes.detach_element_vector();

	// depth_7_nodes no longer needed - explicitly free it
	size_t rss_before_d7_free = get_rss_kb();
	{
		tsl::robin_set<uint64_t> temp;
		depth_7_nodes.swap(temp);
	}
	size_t rss_after_d7_free = get_rss_kb();

	// Free temporary data structures - use swap trick to actually free memory
	{
		tsl::robin_set<uint64_t> temp;
		depth6_set.swap(temp);
	}
	depth6_vec.clear();
	depth6_vec.shrink_to_fit();
	size_t rss_after_phase3_cleanup = get_rss_kb();

	if (verbose)
	{
		size_t freed_d7_kb = (rss_before_d7_free > rss_after_d7_free) ? (rss_before_d7_free - rss_after_d7_free) : 0;
		size_t freed_total_kb = (rss_before_d7_free > rss_after_phase3_cleanup) ? (rss_before_d7_free - rss_after_phase3_cleanup) : 0;
		std::cout << "RSS before depth_7_nodes free: " << (rss_before_d7_free / 1024.0) << " MB" << std::endl;
		std::cout << "RSS after depth_7_nodes freed: " << (rss_after_d7_free / 1024.0) << " MB (" << (freed_d7_kb / 1024.0) << " MB freed)" << std::endl;
		std::cout << "RSS after Phase 3 cleanup: " << (rss_after_phase3_cleanup / 1024.0) << " MB (total " << (freed_total_kb / 1024.0) << " MB freed)" << std::endl;
	}

	if (num_list.size() <= 7)
	{
		num_list.resize(8, 0);
	}
	num_list[7] = static_cast<int>(depth_7_final_size); // Use saved size before detach/clear

	if (verbose)
	{
		size_t theoretical_phase3 = 0;
		for (size_t d = 0; d <= 7; ++d)
		{
			theoretical_phase3 += index_pairs[d].size() * 8;
		}
		size_t current_memory_d7 = current_memory_d6 + index_pairs[7].size() * 8;

		std::cout << "\n[Phase 3 Complete]" << std::endl;
		std::cout << "Generated " << depth_7_final_size << " nodes at depth=7" << std::endl;
		std::cout << "Load factor: " << (depth_7_final_size * 100.0 / bucket_7) << "%" << std::endl;
		std::cout << "Inserted: " << inserted_count_d7 << ", Duplicates: " << duplicate_count_d7 << std::endl;
		std::cout << "  - Duplicates from depth 6: " << duplicates_from_depth5_d7 << " (" << (duplicate_count_d7 > 0 ? (100.0 * duplicates_from_depth5_d7 / duplicate_count_d7) : 0.0) << "%)" << std::endl;
		std::cout << "  - Duplicates from depth 6: " << duplicates_from_depth6_d7 << " (" << (duplicate_count_d7 > 0 ? (100.0 * duplicates_from_depth6_d7 / duplicate_count_d7) : 0.0) << "%)" << std::endl;
		double rejection_rate_d7 = (inserted_count_d7 + duplicate_count_d7) > 0 
			? (100.0 * duplicate_count_d7 / (inserted_count_d7 + duplicate_count_d7)) : 0.0;
		std::cout << "Rejection rate: " << rejection_rate_d7 << "%" << std::endl;
		std::cout << "Theoretical depth 0-8: " << (theoretical_phase3 / 1024.0 / 1024.0) << " MB" << std::endl;
		std::cout << "Actual RSS: " << (get_rss_kb() / 1024.0) << " MB" << std::endl;
		log_emscripten_heap("Phase 3 Complete");
	}

	// ============================================================================
	// Phase 4: depth 8 Local Expansion (Random sampling with single move)
	// ============================================================================
	if (verbose)
	{
		std::cout << "\n--- Phase 4: Local Expansion depth 7→8 (Random sampling with single move) ---" << std::endl;
		std::cout << "Note: Adaptive children per parent based on load factor" << std::endl;
		std::cout << "RSS at Phase 4 start (depth_7_nodes should be destroyed): " << (get_rss_kb() / 1024.0) << " MB" << std::endl;
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

	tsl::robin_set<uint64_t> depth_8_nodes;
	depth_8_nodes.clear();
	depth_8_nodes.max_load_factor(0.9f);
	depth_8_nodes.rehash(bucket_8);
	// Pre-reserve capacity to prevent reallocation spikes during attach
	size_t estimated_d8_capacity = static_cast<size_t>(bucket_8 * 0.95);
	index_pairs[8].reserve(estimated_d8_capacity);
	depth_8_nodes.attach_element_vector(&index_pairs[8]);

	if (verbose)
	{
		std::cout << "RSS after depth_8_nodes creation (empty): " << (get_rss_kb() / 1024.0) << " MB" << std::endl;
	}

	// Create robin_set for depth 7 duplicate checking (used in depth 8 expansion)
	// Use index_pairs[7] directly instead of creating temporary vector
	tsl::robin_set<uint64_t> depth7_set;
	depth7_set.max_load_factor(0.95f);  // Research mode: allow higher load
	
	// Calculate required buckets and round up to power of 2 to avoid rehashing
	size_t required_buckets_d7 = static_cast<size_t>(index_pairs[7].size() / 0.95);
	size_t power_of_2_d7 = 1;
	while (power_of_2_d7 < required_buckets_d7) {
		power_of_2_d7 <<= 1;
	}
	depth7_set.rehash(power_of_2_d7);  // Use rehash to guarantee bucket count
	
	// Bulk insert using insert(first, last) - more efficient than loop
	depth7_set.insert(index_pairs[7].begin(), index_pairs[7].end());
	
	if (verbose)
	{
		size_t rss_peak_phase4 = get_rss_kb();
		size_t depth7_set_buckets = depth7_set.bucket_count();
		size_t depth7_set_size = depth7_set.size();
		float depth7_set_load = depth7_set.load_factor();
		std::cout << "RSS at Phase 4 peak (all sets active): " << (rss_peak_phase4 / 1024.0) << " MB" << std::endl;
		log_emscripten_heap("Phase 4 Peak (depth7_set active)");
		std::cout << "  - depth_5_nodes: ~" << (depth_5_nodes.size() * 32 / 1024 / 1024) << " MB" << std::endl;
		std::cout << "  - depth_8_nodes (empty): ~" << (bucket_8 / 1024 / 1024) << " MB allocated" << std::endl;
		std::cout << "  - depth7_set: size=" << depth7_set_size << ", buckets=" << depth7_set_buckets 
				  << ", load=" << depth7_set_load << ", ~" << (depth7_set_buckets * 4 / 1024 / 1024) << " MB buckets + " 
				  << (depth7_set_size * 8 / 1024 / 1024) << " MB data" << std::endl;
		std::cout << "  - index_pairs[0-7]: ~" << ([&]() {
			size_t total = 0;
			for (size_t d = 0; d <= 7; ++d) total += index_pairs[d].size() * 8;
			return total / 1024 / 1024;
		}()) << " MB" << std::endl;
		std::cout << "  [Optimization: depth7_vec removed, using index_pairs[7] directly]" << std::endl;
	}
	
	std::mt19937_64 rng_phase4(std::random_device{}());

	// Calculate adaptive children per parent with face diversity
	size_t target_nodes_d8 = static_cast<size_t>(bucket_8 * 0.9);
	size_t available_parents_d8 = index_pairs[7].size();  // Use index_pairs[7] directly
	int children_per_parent_d8 = 1;
	if (available_parents_d8 > 0) {
		double rejection_factor = 1.1;
		children_per_parent_d8 = static_cast<int>(std::ceil(
			(target_nodes_d8 * rejection_factor) / available_parents_d8
		));
		children_per_parent_d8 = std::min(children_per_parent_d8, 18);
		children_per_parent_d8 = std::max(children_per_parent_d8, 1);
	}

	// Random parent sampling strategy: Expand until rehash threshold
	size_t max_parent_nodes_d8 = index_pairs[7].size() * 20;  // Use index_pairs[7] directly
	static std::mt19937_64 gen_d8(rng_phase4());
	std::uniform_int_distribution<size_t> dist_d8(0, index_pairs[7].size() - 1);  // Sample from index_pairs[7]
	std::uniform_int_distribution<int> face_dist_d8(0, 5);
	std::uniform_int_distribution<int> rotation_dist_d8(0, 2);

	if (verbose)
	{
		std::cout << "Random parent sampling from depth 7" << std::endl;
		std::cout << "Strategy: Face-diverse expansion until rehash" << std::endl;
		std::cout << "Max load factor: 0.9 (90%)" << std::endl;
		std::cout << "Available parent nodes: " << available_parents_d8 << std::endl;
		std::cout << "Children per parent (adaptive): " << children_per_parent_d8 << std::endl;
		std::cout << "Expected nodes: ~" << std::min(available_parents_d8 * children_per_parent_d8, target_nodes_d8) << std::endl;
	}

	// Pre-declare loop variables outside to avoid repeated allocations
	size_t random_idx_d8, processed_parents_d8 = 0;
	uint64_t parent_node_d8;
	uint64_t index1_cur_d8, index23_d8, index2_cur_d8, index3_cur_d8;
	int index1_tmp_d8, index2_tmp_d8, index3_tmp_d8;
	uint64_t next_index1_d8, next_index2_d8, next_index3_d8, next_node123_d8;
	const size_t last_bucket_count_d8 = depth_8_nodes.bucket_count();
	size_t duplicate_count_d8 = 0;
	size_t duplicates_from_depth7_d8 = 0;  // Counter for depth_7 duplicates
	size_t inserted_count_d8 = 0;

	// Pre-allocate selected_moves vector outside loop to avoid repeated allocations
	std::vector<int> selected_moves_d8;
	selected_moves_d8.reserve(18);  // Maximum possible size
	std::vector<int> faces = {0, 1, 2, 3, 4, 5};
	std::vector<int> all_moves(18);
	for (int m = 0; m < 18; ++m) all_moves[m] = m;
	std::vector<bool> face_covered(6, false);

	while (processed_parents_d8 < max_parent_nodes_d8)
	{
		// Randomly select parent node
		random_idx_d8 = dist_d8(gen_d8);
		parent_node_d8 = index_pairs[7][random_idx_d8];  // Direct access to index_pairs[7]

		// Decompose parent node indices
		index1_cur_d8 = parent_node_d8 / size23;
		index23_d8 = parent_node_d8 % size23;
		index2_cur_d8 = index23_d8 / size3;
		index3_cur_d8 = index23_d8 % size3;

		index1_tmp_d8 = static_cast<int>(index1_cur_d8) * 18;
		index2_tmp_d8 = static_cast<int>(index2_cur_d8) * 18;
		index3_tmp_d8 = static_cast<int>(index3_cur_d8) * 18;

		// Generate moves with face diversity (reuse vector)
		selected_moves_d8.clear();
		if (children_per_parent_d8 >= 18) {
			for (int m = 0; m < 18; ++m) selected_moves_d8.push_back(m);
		} else if (children_per_parent_d8 <= 6) {
			// Reuse faces vector
			std::shuffle(faces.begin(), faces.end(), gen_d8);
			for (int i = 0; i < children_per_parent_d8; ++i) {
				int face = faces[i];
				int rotation = rotation_dist_d8(gen_d8);
				selected_moves_d8.push_back(face * 3 + rotation);
			}
		} else {
			// Reuse all_moves and face_covered vectors
			std::shuffle(all_moves.begin(), all_moves.end(), gen_d8);
			std::fill(face_covered.begin(), face_covered.end(), false);
			for (int m : all_moves) {
				int face = m / 3;
				if (!face_covered[face]) {
					selected_moves_d8.push_back(m);
					face_covered[face] = true;
					if (selected_moves_d8.size() >= 6) break;
				}
			}
			for (int m : all_moves) {
				if (selected_moves_d8.size() >= static_cast<size_t>(children_per_parent_d8)) break;
				if (std::find(selected_moves_d8.begin(), selected_moves_d8.end(), m) == selected_moves_d8.end()) {
					selected_moves_d8.push_back(m);
				}
			}
		}

		for (int move : selected_moves_d8)
		{
			next_index1_d8 = multi_move_table_cross_edges[index1_tmp_d8 + move];
			next_index2_d8 = multi_move_table_F2L_slots_edges[index2_tmp_d8 + move];
			next_index3_d8 = multi_move_table_F2L_slots_corners[index3_tmp_d8 + move];
			next_node123_d8 = next_index1_d8 * size23 + next_index2_d8 * size3 + next_index3_d8;

			// Skip if already in depth 7 or depth 8
			// Note: depth 5/6 checks removed - cannot reach depth 5/6 from depth 7 in one move
			if (depth7_set.find(next_node123_d8) != depth7_set.end()) {
				duplicate_count_d8++;
				duplicates_from_depth7_d8++;
				continue;
			}
			if (depth_8_nodes.find(next_node123_d8) != depth_8_nodes.end()) {
				duplicate_count_d8++;
				continue;
			}

			// Check for rehash before insert
			if (depth_8_nodes.will_rehash_on_next_insert())
			{
				if (verbose)
				{
					std::cout << "\nRehash threshold reached, stopping depth 8 expansion" << std::endl;
					std::cout << "Processed parents: " << processed_parents_d8 << std::endl;
					std::cout << "Inserted: " << inserted_count_d8 << ", Duplicates: " << duplicate_count_d8 << std::endl;
				}
				goto phase4_done;
			}

			depth_8_nodes.insert(next_node123_d8);
			inserted_count_d8++;

			// Check for unexpected rehash (bucket count changed)
			const size_t current_bucket_count_d8 = depth_8_nodes.bucket_count();
			if (current_bucket_count_d8 != last_bucket_count_d8)
			{
				if (verbose)
				{
					std::cout << "\nUnexpected rehash detected (buckets: "
							  << last_bucket_count_d8 << " -> " << current_bucket_count_d8 << ")" << std::endl;
					std::cout << "Stopping depth 8 expansion" << std::endl;
				}
				goto phase4_done;
			}
		}

		processed_parents_d8++;
	}

phase4_done:
	// Save size before detach
	size_t depth_8_final_size = depth_8_nodes.size();

	depth_8_nodes.detach_element_vector();

	// Free temporary data structures - use swap trick to actually free memory
	size_t rss_before_phase4_cleanup = get_rss_kb();
	{
		tsl::robin_set<uint64_t> temp;
		depth7_set.swap(temp);
	}
	size_t rss_after_depth8_cleanup = get_rss_kb();
	
	// Free depth_5_nodes now that Phase 3 and 4 are complete
	if (verbose)
	{
		std::cout << "Freeing depth_5_nodes after Phase 4 completion" << std::endl;
	}
	{
		tsl::robin_set<uint64_t> temp;
		depth_5_nodes.swap(temp);
	}
	size_t rss_after_depth5_free = get_rss_kb();

	if (num_list.size() <= 8)
	{
		num_list.resize(9, 0);
	}
	num_list[8] = static_cast<int>(depth_8_nodes.size());

	// depth_8_nodes no longer needed - explicitly free it
	size_t rss_before_d8_free = get_rss_kb();
	{
		tsl::robin_set<uint64_t> temp;
		depth_8_nodes.swap(temp);
	}
	size_t rss_after_d8_free = get_rss_kb();

	if (verbose)
	{
		size_t freed_depth8_cleanup = (rss_before_phase4_cleanup > rss_after_depth8_cleanup) ? (rss_before_phase4_cleanup - rss_after_depth8_cleanup) : 0;
		size_t freed_depth6 = (rss_after_depth8_cleanup > rss_after_depth5_free) ? (rss_after_depth8_cleanup - rss_after_depth5_free) : 0;
		size_t freed_depth9 = (rss_before_d8_free > rss_after_d8_free) ? (rss_before_d8_free - rss_after_d8_free) : 0;
		std::cout << "\n[Phase 4 Memory Cleanup Details]" << std::endl;
		std::cout << "  After depth7_set cleanup: " << (rss_after_depth8_cleanup / 1024.0) << " MB (" << (freed_depth8_cleanup / 1024.0) << " MB freed)" << std::endl;
		std::cout << "  After depth_5_nodes free: " << (rss_after_depth5_free / 1024.0) << " MB (" << (freed_depth6 / 1024.0) << " MB freed)" << std::endl;
		std::cout << "  After depth_8_nodes free: " << (rss_after_d8_free / 1024.0) << " MB (" << (freed_depth9 / 1024.0) << " MB freed)" << std::endl;
	}

	if (verbose)
	{
		size_t theoretical_phase4 = 0;
		for (size_t d = 0; d <= 8; ++d)
		{
			theoretical_phase4 += index_pairs[d].size() * 8;
		}
		size_t current_memory_d7_val = current_memory_d6 + index_pairs[7].size() * 8;
		size_t current_memory_d8 = current_memory_d7_val + index_pairs[8].size() * 8;

		std::cout << "\n[Phase 4 Complete]" << std::endl;
		std::cout << "Generated " << depth_8_final_size << " nodes at depth=8" << std::endl;
		std::cout << "Load factor: " << (depth_8_final_size * 100.0 / bucket_8) << "%" << std::endl;
		std::cout << "Inserted: " << inserted_count_d8 << ", Duplicates: " << duplicate_count_d8 << std::endl;
		std::cout << "  - Duplicates from depth 7: " << duplicates_from_depth7_d8 << " (" << (duplicate_count_d8 > 0 ? (100.0 * duplicates_from_depth7_d8 / duplicate_count_d8) : 0.0) << "%)" << std::endl;
		double rejection_rate_d8 = (inserted_count_d8 + duplicate_count_d8) > 0 
			? (100.0 * duplicate_count_d8 / (inserted_count_d8 + duplicate_count_d8)) : 0.0;
		std::cout << "Rejection rate: " << rejection_rate_d8 << "%" << std::endl;
		std::cout << "Theoretical depth 0-8: " << (theoretical_phase4 / 1024.0 / 1024.0) << " MB" << std::endl;
		std::cout << "Actual RSS: " << (get_rss_kb() / 1024.0) << " MB" << std::endl;
		log_emscripten_heap("Phase 4 Complete");
		
		// Release allocator cache before Phase 5 to reduce memory baseline
#ifndef __EMSCRIPTEN__
		malloc_trim(0);
		std::cout << "RSS after malloc_trim (before Phase 5): " << (get_rss_kb() / 1024.0) << " MB" << std::endl;
#endif
	}

	// ============================================================================
	// Phase 5: Local Expansion depth 8→9 (Random sampling, single move)
	// ============================================================================
	if (bucket_config.custom_bucket_9 > 0)
	{
		if (verbose)
		{
			std::cout << "\n--- Phase 5: Local Expansion depth 8→9 (Random sampling with single move) ---" << std::endl;
			std::cout << "Note: Adaptive children per parent based on load factor" << std::endl;
			log_emscripten_heap("Phase 5 Start");
		}

		size_t bucket_d9 = bucket_config.custom_bucket_9;
		size_t rss_before_d9_nodes = get_rss_kb();

		if (verbose)
		{
			std::cout << "RSS at Phase 5 start: " << (rss_before_d9_nodes / 1024.0) << " MB" << std::endl;
			std::cout << "Using pre-calculated bucket size for depth 9: " << bucket_d9 << " (" << (bucket_d9 >> 20) << "M)" << std::endl;
			std::cout << "index_pairs[8] size: " << index_pairs[8].size() << std::endl;
			std::cout << "RSS before depth_9_nodes creation: " << (get_rss_kb() / 1024.0) << " MB" << std::endl;
		}

		// Skip if index_pairs[8] is empty
		if (index_pairs[8].empty())
		{
			if (verbose)
			{
				std::cout << "ERROR: index_pairs[8] is empty, skipping Phase 5" << std::endl;
			}
			goto phase5_skip;
		}

		// Initialize depth 9 bucket
		tsl::robin_set<uint64_t> depth_9_nodes;
		depth_9_nodes.max_load_factor(0.9f);
		depth_9_nodes.reserve(bucket_d9);

		size_t rss_after_d9_nodes_creation = get_rss_kb();
		
		// Track bucket count to detect rehash
		size_t last_bucket_count_d9 = depth_9_nodes.bucket_count();
		
		if (verbose)
		{
			std::cout << "RSS after depth_9_nodes creation (empty): " << (rss_after_d9_nodes_creation / 1024.0) << " MB" << std::endl;
			std::cout << "Initial bucket count: " << last_bucket_count_d9 << std::endl;
		}

		// Attach element vector BEFORE expansion (same as Phase 4 pattern)
		if (index_pairs.size() <= 9)
		{
			index_pairs.resize(10);
		}
		index_pairs[9].clear();
		
		// Pre-reserve capacity to prevent reallocation spikes during attach
		size_t estimated_d9_capacity = static_cast<size_t>(bucket_d9 * 0.95);
		index_pairs[9].reserve(estimated_d9_capacity);
		depth_9_nodes.attach_element_vector(&index_pairs[9]);
		
		if (verbose)
		{
			std::cout << "Attached element vector to depth_9_nodes (capacity: " << index_pairs[9].capacity() << ")" << std::endl;
		}

		// Build depth8_set from index_pairs[8] for duplicate detection
		// Use same pattern as Phase 4 to avoid memory spikes
		tsl::robin_set<uint64_t> depth8_set;
		depth8_set.max_load_factor(0.95f);
		size_t depth8_size = index_pairs[8].size();
		
		if (verbose)
		{
			std::cout << "Building depth8_set from " << depth8_size << " nodes..." << std::endl;
			std::cout << "RSS before depth8_set construction: " << (get_rss_kb() / 1024.0) << " MB" << std::endl;
		}
		
		// Calculate required buckets and round up to power of 2 to avoid rehashing
		size_t required_buckets_d8 = static_cast<size_t>(depth8_size / 0.95);
		size_t power_of_2_d8 = 1;
		while (power_of_2_d8 < required_buckets_d8) {
			power_of_2_d8 <<= 1;
		}
		depth8_set.rehash(power_of_2_d8);  // Use rehash to guarantee bucket count
		
		if (verbose)
		{
			std::cout << "RSS after rehash (before insert): " << (get_rss_kb() / 1024.0) << " MB" << std::endl;
		}
		
		// Bulk insert using insert(first, last) - more efficient than loop
		depth8_set.insert(index_pairs[8].begin(), index_pairs[8].end());
		
		if (verbose)
		{
			std::cout << "depth8_set built: " << depth8_set.size() << " nodes (buckets: " << depth8_set.bucket_count() << ")" << std::endl;
			std::cout << "RSS after depth8_set built: " << (get_rss_kb() / 1024.0) << " MB" << std::endl;
			log_emscripten_heap("Phase 5 - After depth8_set");
		}

		// Random sampling from depth 8
		if (verbose)
		{
			std::cout << "Random parent sampling from depth 8" << std::endl;
			std::cout << "Strategy: Face-diverse expansion until rehash" << std::endl;
			std::cout << "Max load factor: 0.9 (90%)" << std::endl;
		}

		size_t available_parents = depth8_size;
		if (verbose)
		{
			std::cout << "Available parent nodes: " << available_parents << std::endl;
		}

		// Adaptive children per parent
		int children_per_parent = 2;
		if (verbose)
		{
			std::cout << "Children per parent (adaptive): " << children_per_parent << std::endl;
			std::cout << "Expected nodes: ~" << (bucket_d9 * 0.9) << std::endl;
			std::cout << std::endl;
		}

		// Random sampling with face-diverse expansion
		std::mt19937_64 rng(std::random_device{}());
		std::uniform_int_distribution<size_t> parent_dist(0, available_parents - 1);
		std::uniform_int_distribution<int> move_dist(0, 17);
		
		size_t inserted_count_d9 = 0;
		size_t duplicate_count_d9 = 0;
		size_t processed_parents = 0;
		size_t target_nodes = static_cast<size_t>(bucket_d9 * 0.9);

		// Pre-declare variables for move application
		uint64_t parent_node123_d9, index1_d9, index2_d9, index3_d9;
		size_t index1_tmp_d9, index2_tmp_d9, index3_tmp_d9;
		uint64_t next_index1_d9, next_index2_d9, next_index3_d9, next_node123_d9;

		if (verbose)
		{
			std::cout << "RSS before expansion loop: " << (get_rss_kb() / 1024.0) << " MB" << std::endl;
		}

		while (depth_9_nodes.size() < target_nodes)
		{
			size_t parent_idx = parent_dist(rng);
			parent_node123_d9 = index_pairs[8][parent_idx];
			
			// Decompose composite index into index1, index2, index3
			index1_d9 = parent_node123_d9 / size23;
			index2_d9 = (parent_node123_d9 % size23) / size3;
			index3_d9 = parent_node123_d9 % size3;
			
			index1_tmp_d9 = index1_d9 * 18;
			index2_tmp_d9 = index2_d9 * 18;
			index3_tmp_d9 = index3_d9 * 18;
			
			for (int i = 0; i < children_per_parent; ++i)
			{
				int move = move_dist(rng);
				
				// Apply move to each component
				next_index1_d9 = multi_move_table_cross_edges[index1_tmp_d9 + move];
				next_index2_d9 = multi_move_table_F2L_slots_edges[index2_tmp_d9 + move];
				next_index3_d9 = multi_move_table_F2L_slots_corners[index3_tmp_d9 + move];
				next_node123_d9 = next_index1_d9 * size23 + next_index2_d9 * size3 + next_index3_d9;

				// Check if already in depth 8
				if (depth8_set.find(next_node123_d9) != depth8_set.end())
				{
					duplicate_count_d9++;
					continue;
				}

				// Insert into depth 9
				auto result = depth_9_nodes.insert(next_node123_d9);
				if (result.second)
				{
					inserted_count_d9++;
					
					// Periodic RSS check during expansion (every 500K nodes)
					if (verbose && inserted_count_d9 % 500000 == 0)
					{
						std::cout << "RSS at " << inserted_count_d9 << " nodes: " << (get_rss_kb() / 1024.0) << " MB" << std::endl;
					}
				}
				else
				{
					duplicate_count_d9++;
				}

				// Check if rehash occurred (bucket count changed)
				const size_t current_bucket_count_d9 = depth_9_nodes.bucket_count();
				if (current_bucket_count_d9 != last_bucket_count_d9)
				{
					if (verbose)
					{
						std::cout << "\nUnexpected rehash detected (buckets: "
								  << last_bucket_count_d9 << " -> " << current_bucket_count_d9 << ")" << std::endl;
						std::cout << "Stopping depth 9 expansion" << std::endl;
					}
					goto phase5_done;
				}
			}
			
			processed_parents++;
		}

	phase5_done:
		if (verbose)
		{
			std::cout << "Processed parents: " << processed_parents << std::endl;
			std::cout << "Inserted: " << inserted_count_d9 << ", Duplicates: " << duplicate_count_d9 << std::endl;
		}

		// Save size before detach (same as Phase 3 and 4 pattern)
		size_t depth_9_final_size = depth_9_nodes.size();
		
		// Detach element vector
		depth_9_nodes.detach_element_vector();
		
		size_t rss_before_depth9_free = get_rss_kb();
		
		// Free depth8_set
		{
			tsl::robin_set<uint64_t> temp;
			depth8_set.swap(temp);
		}
		
		size_t rss_after_depth8_free = get_rss_kb();
		if (verbose)
		{
			std::cout << "RSS before depth8_set free: " << (rss_before_depth9_free / 1024.0) << " MB" << std::endl;
			std::cout << "RSS after depth8_set freed: " << (rss_after_depth8_free / 1024.0) 
					  << " MB (" << ((rss_before_depth9_free - rss_after_depth8_free) / 1024.0) << " MB freed)" << std::endl;
		}

		// Free depth_9_nodes (already detached)
		size_t rss_before_d9_free = get_rss_kb();
		{
			tsl::robin_set<uint64_t> temp;
			depth_9_nodes.swap(temp);
		}
		
		size_t rss_after_d9_free = get_rss_kb();
		if (verbose)
		{
			std::cout << "RSS after depth_9_nodes cleanup: " << (rss_after_d9_free / 1024.0) 
					  << " MB (" << ((rss_before_d9_free - rss_after_d9_free) / 1024.0) << " MB freed)" << std::endl;
		}

		if (num_list.size() <= 9)
		{
			num_list.resize(10, 0);
		}
		num_list[9] = static_cast<int>(depth_9_final_size);
		if (verbose)
		{
			size_t theoretical_phase5 = 0;
			for (size_t d = 0; d <= 10; ++d)
			{
				theoretical_phase5 += index_pairs[d].size() * 8;
			}

			std::cout << "\n[Phase 5 Complete]" << std::endl;
			std::cout << "Generated " << depth_9_final_size << " nodes at depth=9" << std::endl;
			std::cout << "Load factor: " << (depth_9_final_size * 100.0 / bucket_d9) << "%" << std::endl;
			std::cout << "Inserted: " << inserted_count_d9 << ", Duplicates: " << duplicate_count_d9 << std::endl;
			double rejection_rate_d9 = (inserted_count_d9 + duplicate_count_d9) > 0 
				? (100.0 * duplicate_count_d9 / (inserted_count_d9 + duplicate_count_d9)) : 0.0;
			std::cout << "Rejection rate: " << rejection_rate_d9 << "%" << std::endl;
			std::cout << "Theoretical depth 0-9: " << (theoretical_phase5 / 1024.0 / 1024.0) << " MB" << std::endl;
			std::cout << "Actual RSS: " << (get_rss_kb() / 1024.0) << " MB" << std::endl;
			log_emscripten_heap("Phase 5 Complete");
		}
	}
	else if (verbose)
	{
		std::cout << "\n[Phase 5 Skipped - depth 9 expansion disabled (bucket_d9 = 0)]" << std::endl;
	}

phase5_skip:

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
		std::cout << "  Final RSS: " << (get_rss_kb() / 1024.0) << " MB" << std::endl;
		log_emscripten_heap("Final Cleanup Complete");
	}
	
	// Collect detailed statistics if requested
	if (research_config.collect_detailed_statistics)
	{
		std::cout << "\n=== Detailed Statistics (CSV Format) ===" << std::endl;
		std::cout << "depth,nodes,rss_mb,capacity_mb" << std::endl;
		
		size_t cumulative_capacity = 0;
		for (size_t d = 0; d < index_pairs.size(); ++d)
		{
			size_t nodes = index_pairs[d].size();
			size_t capacity = index_pairs[d].capacity();
			cumulative_capacity += capacity * sizeof(uint64_t);
			
			size_t rss_kb = get_rss_kb();
			double rss_mb = rss_kb / 1024.0;
			double capacity_mb = cumulative_capacity / 1024.0 / 1024.0;
			
			std::cout << d << "," << nodes << "," << rss_mb << "," << capacity_mb << std::endl;
		}
	}
	
	// Dry run mode: clear database (measurement only)
	if (research_config.dry_run)
	{
		if (verbose)
		{
			std::cout << "\n[DRY RUN MODE] Clearing database (measurement complete)" << std::endl;
		}
		
		// Clear all data
		for (auto &vec : index_pairs)
		{
			vec.clear();
			vec.shrink_to_fit();
		}
		index_pairs.clear();
		num_list.clear();
		
		if (verbose)
		{
			std::cout << "Database cleared. RSS after cleanup: " << (get_rss_kb() / 1024.0) << " MB" << std::endl;
		}
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
	// Cross edges are unaffected by algorithms that move slots like L U L' out as Free Pairs.
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
	int tmp_index;
	std::vector<std::string> auf = {"", "U", "U2", "U'"};
	std::vector<std::string> appl_moves = {"L U L'", "L U' L'", "B' U B", "B' U' B"};
	// Add 16 free pair goal states at depth 0
	// Note: This prune table is for F2L slots only (index2+index3), not cross edges
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			int index1_tmp_2 = index1;  // index1 here is actually index2 (F2L edges)
			int index2_tmp_2 = index2;  // index2 here is actually index3 (F2L corners)
			for (int m : StringToAlg(appl_moves[i] + " " + auf[j]))
			{
				index1_tmp_2 = table1[index1_tmp_2 * 18 + m];  // table1 is F2L edges
				index2_tmp_2 = table2[index2_tmp_2 * 18 + m];  // table2 is F2L corners
			}
			tmp_index = index1_tmp_2 * size2 + index2_tmp_2;  // size2 is actually size3 (corners size)
			if (prune_table[tmp_index] == 255)
			{
				prune_table[tmp_index] = 0;
				num_filled += 1;
			}
		}
	}
	num_old = num_filled;
	std::cout << "Depth 0: " << num_filled << " / " << size << " filled." << std::endl;
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

void create_prune_table3(int index1, int index2, int size1, int size2, int depth, const std::vector<int> table1, const std::vector<int> table2, std::vector<unsigned char> &prune_table)
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

// Production Helper: Parse bucket model name to BucketConfig
#ifdef __EMSCRIPTEN__
static BucketConfig parseBucketModel(const std::string& model_name) {
	BucketConfig config;
	config.model = BucketModel::CUSTOM;
	
	// New WASM tier configurations (2026-01-08)
	if (model_name == "1M/1M/2M/2M") {
		config.custom_bucket_6 = 1ULL * 1024 * 1024;  // 1M
		config.custom_bucket_7 = 1ULL * 1024 * 1024;  // 1M
		config.custom_bucket_8 = 2ULL * 1024 * 1024;  // 2M
		config.custom_bucket_9 = 2ULL * 1024 * 1024;  // 2M (~332 MB heap, ~664 MB dual)
	} else if (model_name == "1M/1M/1M/1M") {
		config.custom_bucket_6 = 1ULL * 1024 * 1024;  // 1M
		config.custom_bucket_7 = 1ULL * 1024 * 1024;  // 1M
		config.custom_bucket_8 = 1ULL * 1024 * 1024;  // 1M
		config.custom_bucket_9 = 1ULL * 1024 * 1024;  // 1M (~332 MB heap, ~664 MB dual)
	} else if (model_name == "1M/1M/2M/4M") {
		config.custom_bucket_6 = 1ULL * 1024 * 1024;  // 1M
		config.custom_bucket_7 = 1ULL * 1024 * 1024;  // 1M
		config.custom_bucket_8 = 2ULL * 1024 * 1024;  // 2M
		config.custom_bucket_9 = 4ULL * 1024 * 1024;  // 4M (~398 MB heap, ~796 MB dual)
	}
	// Legacy configurations (kept for compatibility)
	else if (model_name == "MOBILE_LOW") {
		config.custom_bucket_6 = 1ULL * 1024 * 1024;
		config.custom_bucket_7 = 1ULL * 1024 * 1024;
		config.custom_bucket_8 = 2ULL * 1024 * 1024;
		config.custom_bucket_9 = 4ULL * 1024 * 1024;
	} else if (model_name == "MOBILE_MIDDLE") {
		config.custom_bucket_6 = 2ULL * 1024 * 1024;
		config.custom_bucket_7 = 4ULL * 1024 * 1024;
		config.custom_bucket_8 = 4ULL * 1024 * 1024;
		config.custom_bucket_9 = 4ULL * 1024 * 1024;
	} else if (model_name == "MOBILE_HIGH") {
		config.custom_bucket_6 = 4ULL * 1024 * 1024;
		config.custom_bucket_7 = 4ULL * 1024 * 1024;
		config.custom_bucket_8 = 4ULL * 1024 * 1024;
		config.custom_bucket_9 = 4ULL * 1024 * 1024;
	} else if (model_name == "DESKTOP_STD") {
		config.custom_bucket_6 = 8ULL * 1024 * 1024;
		config.custom_bucket_7 = 8ULL * 1024 * 1024;
		config.custom_bucket_8 = 8ULL * 1024 * 1024;
		config.custom_bucket_9 = 8ULL * 1024 * 1024;
	} else if (model_name == "DESKTOP_HIGH") {
		config.custom_bucket_6 = 8ULL * 1024 * 1024;
		config.custom_bucket_7 = 16ULL * 1024 * 1024;
		config.custom_bucket_8 = 16ULL * 1024 * 1024;
		config.custom_bucket_9 = 16ULL * 1024 * 1024;
	} else if (model_name == "DESKTOP_ULTRA") {
		config.custom_bucket_6 = 16ULL * 1024 * 1024;
		config.custom_bucket_7 = 16ULL * 1024 * 1024;
		config.custom_bucket_8 = 16ULL * 1024 * 1024;
		config.custom_bucket_9 = 16ULL * 1024 * 1024;
	} else {
		// Default to 1M/1M/2M/2M (production standard, 2026-01-08)
		config.custom_bucket_6 = 1ULL * 1024 * 1024;
		config.custom_bucket_7 = 1ULL * 1024 * 1024;
		config.custom_bucket_8 = 2ULL * 1024 * 1024;
		config.custom_bucket_9 = 2ULL * 1024 * 1024;
	}
	
	return config;
}
#endif

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
	std::vector<unsigned char> prune_table23_couple_xxcross;
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
	
	// Configuration storage
	BucketConfig bucket_config_;
	ResearchConfig research_config_;
	
	// Helper functions for custom bucket configuration
	static BucketConfig create_custom_bucket_config(int b6_mb, int b7_mb, int b8_mb, int b9_mb) {
		BucketConfig config;
		config.model = BucketModel::CUSTOM;
		config.custom_bucket_6 = static_cast<size_t>(b6_mb) * 1024 * 1024;
		config.custom_bucket_7 = static_cast<size_t>(b7_mb) * 1024 * 1024;
		config.custom_bucket_8 = static_cast<size_t>(b8_mb) * 1024 * 1024;
		config.custom_bucket_9 = static_cast<size_t>(b9_mb) * 1024 * 1024;
		return config;
	}
	
	static ResearchConfig create_custom_research_config() {
		ResearchConfig config;
		config.enable_custom_buckets = true;
		config.skip_search = true;  // Only build database, no search
		// high_memory_wasm_mode defaults to true in WASM (set in bucket_config.h)
		
		// Production mode: Environment variables not needed
		// Local expansion is always enabled (enable_local_expansion = true by default)
		
		return config;
	}

	// Custom bucket constructor (for WASM tier selection)
	// Receives bucket sizes in MB, automatically sets up custom bucket config
	xxcross_search(bool adj, int bucket_6_mb, int bucket_7_mb, int bucket_8_mb, int bucket_9_mb)
		: xxcross_search(adj, 5, bucket_6_mb + bucket_7_mb + bucket_8_mb + bucket_9_mb + 300, true,
		                create_custom_bucket_config(bucket_6_mb, bucket_7_mb, bucket_8_mb, bucket_9_mb),
		                create_custom_research_config())
	{
		// Delegating constructor - all work done by main constructor
	}

#ifdef __EMSCRIPTEN__
	// Production constructor (WASM only) - User-friendly API with bucket model names
	// Forced verbose=false for minimal logging in production
	xxcross_search(bool adj, const std::string& bucket_model)
	{
		// Parse bucket model name to config
		BucketConfig config = parseBucketModel(bucket_model);
		
		// Create research config for custom buckets
		ResearchConfig research_config;
		research_config.enable_custom_buckets = true;
		research_config.skip_search = true;
		research_config.high_memory_wasm_mode = true;
		
		// Calculate total memory budget
		int total_mb = static_cast<int>(
			(config.custom_bucket_6 + config.custom_bucket_7 + 
			 config.custom_bucket_8 + config.custom_bucket_9) / (1024 * 1024)
		) + 300; // Base overhead
		
		bool verbose = false; // Force minimal logging in production
		
		// Delegate to main constructor
		new (this) xxcross_search(adj, 5, total_mb, verbose, config, research_config);
	}
#endif

	xxcross_search(bool adj = true, int BFS_DEPTH = 5, int MEMORY_LIMIT_MB = 1600, bool verbose = true,
	               const BucketConfig& bucket_config = BucketConfig(),
	               const ResearchConfig& research_config = ResearchConfig())
		: bucket_config_(bucket_config), research_config_(research_config)
	{
		// ============================================================================
		// Phase 1: Configuration Validation
		// ============================================================================
		
		if (verbose) {
			std::cout << "\n=== xxcross_search Constructor ===" << std::endl;
			std::cout << "Configuration:" << std::endl;
			std::cout << "  BFS_DEPTH: " << BFS_DEPTH << std::endl;
			std::cout << "  MEMORY_LIMIT_MB: " << MEMORY_LIMIT_MB << std::endl;
			std::cout << "  Adjacent slots: " << (adj ? "yes" : "no") << std::endl;
			std::cout << "Research mode flags:" << std::endl;
			std::cout << "  enable_local_expansion: " << research_config_.enable_local_expansion << std::endl;
			std::cout << "  force_full_bfs_to_depth: " << research_config_.force_full_bfs_to_depth << std::endl;
			std::cout << "  ignore_memory_limits: " << research_config_.ignore_memory_limits << std::endl;
			std::cout << "  collect_detailed_statistics: " << research_config_.collect_detailed_statistics << std::endl;
			std::cout << "  dry_run: " << research_config_.dry_run << std::endl;
			std::cout << "  enable_custom_buckets: " << research_config_.enable_custom_buckets << std::endl;
			std::cout << "  high_memory_wasm_mode: " << research_config_.high_memory_wasm_mode << std::endl;
			std::cout << "  developer_memory_limit_mb: " << research_config_.developer_memory_limit_mb << std::endl;
		}
		
		// Validate WASM environment constraints
		#ifdef __EMSCRIPTEN__
			if (!research_config_.high_memory_wasm_mode) {
				const size_t WASM_SAFE_LIMIT_MB = 1200;
				if (MEMORY_LIMIT_MB > WASM_SAFE_LIMIT_MB) {
					throw std::runtime_error(
						"[WASM] Budget exceeds safe limit: " + std::to_string(MEMORY_LIMIT_MB) + 
						" MB > " + std::to_string(WASM_SAFE_LIMIT_MB) + 
						" MB. Use ResearchConfig.high_memory_wasm_mode=true if intentional."
					);
				}
			}
			
			// In WASM, disable verbose by default (can be overridden)
			if (verbose) {
				std::cout << "[WASM] Environment detected, using WASM safety checks" << std::endl;
			}
		#endif
		
		// ============================================================================
		// Phase 2: Bucket Model Selection
		// ============================================================================
		
		BucketModel selected_model = bucket_config_.model;
		size_t bucket_d6 = 0, bucket_d7 = 0, bucket_d8 = 0, bucket_d9 = 0;
		
		if (selected_model == BucketModel::AUTO) {
			// Auto-select based on memory budget
			size_t budget_mb = (bucket_config_.memory_budget_mb > 0) 
			                 ? bucket_config_.memory_budget_mb 
			                 : MEMORY_LIMIT_MB;
			selected_model = select_model(budget_mb);
			
			if (verbose) {
				std::cout << "\nAuto-selected model based on budget " << budget_mb << " MB" << std::endl;
			}
		}
		
		// Validate model safety
		validate_model_safety(selected_model, MEMORY_LIMIT_MB, bucket_config_, research_config_);
		
		// Get bucket sizes from selected model
		if (selected_model == BucketModel::CUSTOM) {
			// Use custom buckets
			bucket_d6 = bucket_config_.custom_bucket_6;
			bucket_d7 = bucket_config_.custom_bucket_7;
			bucket_d8 = bucket_config_.custom_bucket_8;
			bucket_d9 = bucket_config_.custom_bucket_9;  // May be 0 (depth 9 disabled)
			
			if (verbose) {
				if (bucket_d9 > 0) {
					std::cout << "Using CUSTOM buckets: " 
					          << (bucket_d6 >> 20) << "M / "
					          << (bucket_d7 >> 20) << "M / "
					          << (bucket_d8 >> 20) << "M / "
					          << (bucket_d9 >> 20) << "M (depth 9 enabled)" << std::endl;
				} else {
					std::cout << "Using CUSTOM buckets: " 
					          << (bucket_d6 >> 20) << "M / "
					          << (bucket_d7 >> 20) << "M / "
					          << (bucket_d8 >> 20) << "M" << std::endl;
				}
				
				size_t estimated_rss = estimate_custom_rss(bucket_d6, bucket_d7, bucket_d8);
				std::cout << "Estimated RSS: " << estimated_rss << " MB (theoretical, depth 9 not included)" << std::endl;
			}
		} else if (selected_model == BucketModel::FULL_BFS) {
			// Full BFS mode: use minimal buckets (will be overridden by research mode)
			bucket_d6 = 1 << 20;  // 1M (placeholder)
			bucket_d7 = 1 << 20;
			bucket_d8 = 1 << 20;
			
			if (verbose) {
				std::cout << "Using FULL_BFS mode (local expansion disabled)" << std::endl;
			}
		} else {
			// Use preset model
			const auto& measured_data = get_measured_data();
			
			ModelData model_data;
			if (selected_model == BucketModel::ULTRA_HIGH) {
				model_data = get_ultra_high_config();
			} else if (measured_data.find(selected_model) != measured_data.end()) {
				model_data = measured_data.at(selected_model);
			} else {
				throw std::runtime_error("Unknown bucket model");
			}
			
			bucket_d6 = model_data.d6;
			bucket_d7 = model_data.d7;
			bucket_d8 = model_data.d8;
			
			if (verbose) {
				std::cout << "Using preset model with buckets: " 
				          << (bucket_d6 >> 20) << "M / "
				          << (bucket_d7 >> 20) << "M / "
				          << (bucket_d8 >> 20) << "M" << std::endl;
				
				if (model_data.measured_rss_mb > 0) {
					std::cout << "Measured RSS: " << model_data.measured_rss_mb << " MB" << std::endl;
				} else {
					std::cout << "WARNING: No measurement data available yet (spike fixes pending)" << std::endl;
				}
			}
		}
		
		// ============================================================================
		// Phase 3: Original Constructor Logic (Move Tables, Prune Tables, etc.)
		// ============================================================================
		
#ifdef __EMSCRIPTEN__
		// Legacy WASM check (now handled by validation above)
		// Keeping for backwards compatibility
		int requested_memory = MEMORY_LIMIT_MB;
#endif
		// Create move tables
		edge_move_table = create_edge_move_table();
		corner_move_table = create_corner_move_table();
		create_multi_move_table(4, 2, 12, 24 * 22 * 20 * 18, edge_move_table, multi_move_table_cross_edges);
		create_multi_move_table(2, 2, 12, 24 * 22, edge_move_table, multi_move_table_F2L_slots_edges);
		create_multi_move_table(2, 3, 8, 24 * 21, corner_move_table, multi_move_table_F2L_slots_corners);
		prune_table1 = std::vector<unsigned char>(24 * 22 * 20 * 18, 255);
		prune_table23_couple = std::vector<unsigned char>(24 * 22 * 24 * 21, 255);
		prune_table23_couple_xxcross = std::vector<unsigned char>(24 * 22 * 24 * 21, 255);

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

		std::cout << "DEBUG: index1 = " << index1 << ", index2 = " << index2 << ", index3 = " << index3 << std::endl;
		std::cout << "DEBUG: cross_edges_goal = {" << cross_edges_goal[0] << ", " << cross_edges_goal[1] << ", " << cross_edges_goal[2] << ", " << cross_edges_goal[3] << "}" << std::endl;

		create_prune_table(index1, size1, 8, multi_move_table_cross_edges, prune_table1);
		create_prune_table2(index2, index3, size2, size3, 9, multi_move_table_F2L_slots_edges, multi_move_table_F2L_slots_corners, prune_table23_couple);
		create_prune_table3(index2, index3, size2, size3, 9, multi_move_table_F2L_slots_edges, multi_move_table_F2L_slots_corners, prune_table23_couple_xxcross);

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
			verbose,
			bucket_config_,    // Pass bucket config
			research_config_); // Pass research config

		// reached_depth is calculated from the size of index_pairs
		reached_depth = static_cast<int>(index_pairs.size()) - 1;

		std::cout << "\nDatabase construction completed: reached_depth=" << reached_depth << std::endl;

		// =============================================================================
		// Detailed Memory Analysis (Post-Construction)
		// =============================================================================
		if (verbose)
		{
			std::cout << "\n=== POST-CONSTRUCTION MEMORY ANALYSIS ===" << std::endl;
			
			// First measurement: RSS before malloc_trim
			size_t rss_before_trim_kb = get_rss_kb();
			size_t rss_after_trim_kb;  // Declare outside the if/else blocks
			
			// Force allocator to return unused memory to OS
#ifndef __EMSCRIPTEN__
			// Check if malloc_trim should be disabled (for WASM-equivalent measurements)
			if (research_config_.disable_malloc_trim) {
				if (verbose) {
					std::cout << "\n[Allocator Cache Cleanup DISABLED]" << std::endl;
					std::cout << "  malloc_trim() skipped for WASM-equivalent measurement" << std::endl;
					std::cout << "  RSS (with allocator cache): " << (rss_before_trim_kb / 1024.0) << " MB" << std::endl;
				}
				rss_after_trim_kb = rss_before_trim_kb;  // No trim performed
			} else {
				// Normal operation: perform malloc_trim()
				if (verbose) {
					std::cout << "\n[Allocator Cache Cleanup]" << std::endl;
					std::cout << "  RSS before malloc_trim: " << (rss_before_trim_kb / 1024.0) << " MB" << std::endl;
				}
				malloc_trim(0);  // Return unused memory to OS
				rss_after_trim_kb = get_rss_kb();
				if (verbose) {
					size_t freed_by_trim = (rss_before_trim_kb > rss_after_trim_kb) ? (rss_before_trim_kb - rss_after_trim_kb) : 0;
					std::cout << "  RSS after malloc_trim: " << (rss_after_trim_kb / 1024.0) << " MB" << std::endl;
					std::cout << "  Allocator cache freed: " << (freed_by_trim / 1024.0) << " MB" << std::endl;
				}
			}
#else
			rss_after_trim_kb = rss_before_trim_kb;
#endif
			
			// Calculate theoretical memory usage
			size_t index_pairs_bytes = 0;
			for (size_t d = 0; d < index_pairs.size(); ++d) {
				index_pairs_bytes += index_pairs[d].size() * sizeof(uint64_t);
			}
			
			size_t move_tables_bytes = 
				multi_move_table_cross_edges.size() * sizeof(int) +
				multi_move_table_F2L_slots_edges.size() * sizeof(int) +
				multi_move_table_F2L_slots_corners.size() * sizeof(int);
			
			size_t prune_tables_bytes = 
				prune_table1.size() * sizeof(int) +
				prune_table23_couple.size() * sizeof(int);
			
			size_t theoretical_data_mb = 
				(index_pairs_bytes + move_tables_bytes + prune_tables_bytes) / 1024.0 / 1024.0;
			
			// Use post-trim RSS for overhead calculation
			size_t actual_rss_mb = rss_after_trim_kb / 1024;
			double overhead_ratio = static_cast<double>(actual_rss_mb) / theoretical_data_mb;
			int overhead_mb = actual_rss_mb - theoretical_data_mb;
			
			std::cout << std::fixed << std::setprecision(2);
			std::cout << "\n[Data Structures]" << std::endl;
			std::cout << "  index_pairs (depth 0-" << (index_pairs.size()-1) << "): " 
					  << (index_pairs_bytes / 1024.0 / 1024.0) << " MB" << std::endl;
			std::cout << "  move_tables (3 tables): " 
					  << (move_tables_bytes / 1024.0 / 1024.0) << " MB" << std::endl;
			std::cout << "  prune_tables (2 tables): " 
					  << (prune_tables_bytes / 1024.0 / 1024.0) << " MB" << std::endl;
			std::cout << "  Theoretical Total: " << theoretical_data_mb << " MB" << std::endl;
			
			std::cout << "\n[RSS Analysis]" << std::endl;
			std::cout << "  Actual RSS: " << actual_rss_mb << " MB" << std::endl;
			std::cout << "  Overhead: +" << overhead_mb << " MB (+" 
					  << ((overhead_ratio - 1.0) * 100) << "%)" << std::endl;
			
			if (overhead_ratio > 1.2) {
				std::cout << "\n  ⚠️ WARNING: Overhead exceeds 20% - potential memory leak!" << std::endl;
				std::cout << "  Expected: <10% for optimized implementation" << std::endl;
			} else if (overhead_ratio > 1.1) {
				std::cout << "\n  ℹ️ INFO: Overhead 10-20% - acceptable but could be optimized" << std::endl;
			} else {
				std::cout << "\n  ✅ EXCELLENT: Overhead <10% - memory efficient!" << std::endl;
			}
			
			std::cout << "\n[Per-Depth Breakdown]" << std::endl;
			size_t total_capacity_bytes = 0;
			for (size_t d = 0; d < index_pairs.size(); ++d) {
				size_t nodes = index_pairs[d].size();
				size_t capacity = index_pairs[d].capacity();
				size_t bytes = nodes * sizeof(uint64_t);
				size_t capacity_bytes = capacity * sizeof(uint64_t);
				size_t wasted = capacity_bytes - bytes;
				total_capacity_bytes += capacity_bytes;
				
				std::cout << "  depth " << d << ": " << nodes << " nodes, " 
						  << (bytes / 1024.0 / 1024.0) << " MB";
				if (capacity > nodes) {
					std::cout << " (capacity: " << capacity 
							  << ", wasted: " << (wasted / 1024.0 / 1024.0) << " MB)";
				}
				std::cout << std::endl;
			}
			
			size_t total_wasted = total_capacity_bytes - index_pairs_bytes;
			if (total_wasted > 1024 * 1024) {
				std::cout << "\n  ⚠️ Total capacity waste: " 
						  << (total_wasted / 1024.0 / 1024.0) << " MB" << std::endl;
			}
			
			std::cout << "==========================================" << std::endl;
		}

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

		// Output final statistics for WASM parser
#ifdef __EMSCRIPTEN__
		size_t final_heap = emscripten_get_heap_size();
		std::cout << "\n=== Final Statistics ===" << std::endl;
		std::cout << "Final heap: " << (final_heap / 1024.0 / 1024.0) << " MB" << std::endl;
		std::cout << "Peak heap: " << (final_heap / 1024.0 / 1024.0) << " MB" << std::endl;
		
		// Output bucket sizes for depths 7-10
		// Note: index_pairs is vector<uint64_t>, load factor ~0.88 is typical for robin_set during BFS
		for (int d = 7; d <= 10; ++d) {
			if (d < (int)index_pairs.size()) {
				size_t nodes = index_pairs[d].size();
				size_t bytes = nodes * sizeof(uint64_t);
				double estimated_load_factor = 0.88;  // Typical for robin_set after rehashing
				
				std::cout << "Load factor (d" << d << "): " << estimated_load_factor << std::endl;
				std::cout << "Bucket size (d" << d << "): " << (bytes / 1024.0 / 1024.0) << " MB" << std::endl;
			}
		}
		std::cout << "=========================" << std::endl;
#endif

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

	bool depth_limited_search_xxcross(int arg_index1, int arg_index2, int arg_index3, int depth, int prev)
	{
		for (int i : move_restrict)
		{
			if (ma[prev + i])
			{
				continue;
			}
			index2_tmp = multi_move_table_F2L_slots_edges[arg_index2 + i];
			index3_tmp = multi_move_table_F2L_slots_corners[arg_index3 + i];
			prune23_tmp = prune_table23_couple_xxcross[index2_tmp * size3 + index3_tmp];
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
			else if (depth_limited_search_xxcross(index1_tmp * 18, index2_tmp * 18, index3_tmp * 18, depth - 1, i * 18))
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
		
		// Validate depth range
		std::cout << "get_xxcross_scramble called with len=" << len << std::endl;
		std::cout << "num_list[" << len << "] = " << num_list[len] << std::endl;
		std::cout << "index_pairs[" << len << "].size() = " << index_pairs[len].size() << std::endl;
		
		if (num_list[len] == 0 || index_pairs[len].empty())
		{
			std::cout << "ERROR: No states available at depth " << len << std::endl;
			tmp = "";
			return tmp;
		}
		
		// Depth guarantee: Try multiple nodes until we find one with exact depth len
		// Reason: index_pairs[len] contains nodes discovered at depth len,
		// but some may have shorter optimal solutions (especially for depths 7+)
		const int max_attempts = 100;
		std::uniform_int_distribution<> distribution(0, num_list[len] - 1);
		
		for (int attempt = 0; attempt < max_attempts; ++attempt)
		{
			sol.clear();
			// Select random node from depth len
			uint64_t xxcross_index = index_pairs[len][distribution(generator)];
			index1 = static_cast<int>(xxcross_index / (size2 * size3));
			int index23packed = static_cast<int>(xxcross_index % (size2 * size3));
			index2 = index23packed / size3;
			index3 = index23packed % size3;
			
			if (attempt == 0) {
				std::cout << "Selected xxcross_index=" << xxcross_index 
				          << ", index1=" << index1 << ", index2=" << index2 
				          << ", index3=" << index3 << std::endl;
				prune1_tmp = prune_table1[index1];
				prune23_tmp = prune_table23_couple[index2 * size3 + index3];
				std::cout << "Prune values: prune1=" << prune1_tmp 
				          << ", prune23=" << prune23_tmp 
				          << ", max=" << std::max(prune1_tmp, prune23_tmp) << std::endl;
			}
			
			index1 *= 18;
			index2 *= 18;
			index3 *= 18;
			
			// Find actual optimal solution depth by trying depths 1 to len
			int actual_depth = -1;
			for (int d = 1; d <= len+3; ++d)
			{
				if (depth_limited_search(index1, index2, index3, d, 324))
				{
					actual_depth = d;
					break;
				}
			}
			
			// Check if actual depth matches requested depth
			if (actual_depth == len)
			{
				std::string tmp_sol = tmp;
				index1 /= 18;
				index2 /= 18;
				index3 /= 18;
				for (int move : sol)
				{
					index1 = multi_move_table_cross_edges[index1 * 18 + move];
					index2 = multi_move_table_F2L_slots_edges[index2 * 18 + move];
					index3 = multi_move_table_F2L_slots_corners[index3 * 18 + move];
				}
				std::cout << "✓ Found valid node at attempt " << (attempt + 1) 
				<< ": requested=" << len << ", actual=" << actual_depth << std::endl;
				std::cout << "XXCross Solution from get_xxcross_scramble: " << tmp_sol << std::endl;
				std::string scramble_adjust = start_search_index(index1, index2, index3);
				std::cout << "Adjustment scramble: " << scramble_adjust << std::endl;
				return  tmp_sol + scramble_adjust;
			}
			
			// Log mismatch and retry
			if (attempt < 5 || (attempt + 1) % 10 == 0) {
				std::cout << "✗ Attempt " << (attempt + 1) << ": requested=" << len 
				          << ", actual=" << actual_depth << " - retrying" << std::endl;
			}
		}
		
		// Failed to find exact depth after max_attempts
		std::cout << "ERROR: Could not find node with exact depth " << len 
		          << " after " << max_attempts << " attempts" << std::endl;
		std::cout << "WARNING: Returning last generated solution (may not be exact depth)" << std::endl;
		tmp = AlgToString(sol);
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
		prune23_tmp = prune_table23_couple_xxcross[index2 * size3 + index3];
		std::cout << "Prune values: prune1=" << prune1_tmp 
					<< ", prune23=" << prune23_tmp 
					<< ", max=" << std::max(prune1_tmp, prune23_tmp) << std::endl;
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
			if (depth_limited_search_xxcross(index1, index2, index3, d, 324))
			{
				break;
			}
		}
		std::cout << "XXCross Solution from start_search: " << AlgToString(sol) << std::endl;
		return tmp;
	}

	std::string start_search_index(int arg_index1, int arg_index2, int arg_index3)
	{
		sol.clear();
		max_length = 14;
		prune1_tmp = prune_table1[index1];
		prune23_tmp = prune_table23_couple_xxcross[index2 * size3 + index3];
		std::cout << "Prune values: prune1=" << prune1_tmp 
					<< ", prune23=" << prune23_tmp 
					<< ", max=" << std::max(prune1_tmp, prune23_tmp) << std::endl;
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
			if (depth_limited_search_xxcross(index1, index2, index3, d, 324))
			{
				break;
			}
		}
		std::cout << "XXCross Solution from start_search_index: " << AlgToString(sol) << std::endl;
		return tmp;
	}

	std::string func(std::string arg_scramble = "", std::string arg_length = "7")
	{
		std::string ret = arg_scramble + start_search(arg_scramble) + "," + get_xxcross_scramble(arg_length);
		return ret;
	}
	
	// Helper function: Calculate scramble length (number of moves)
	// Avoids passing vectors between JS and C++ (which can be problematic)
	int get_scramble_length(std::string scramble_str)
	{
		if (scramble_str.empty()) {
			return 0;
		}
		std::vector<int> moves = StringToAlg(scramble_str);
		return static_cast<int>(moves.size());
	}
};

#ifndef SKIP_SOLVER_MAIN
int main()
{
	// ============================================================================
	// Environment Variable Configuration
	// ============================================================================
	
	// Memory limit
	int memory_limit = 1600; // Default
	const char *env_limit = std::getenv("MEMORY_LIMIT_MB");
	if (env_limit != nullptr)
	{
		memory_limit = std::atoi(env_limit);
		std::cout << "MEMORY_LIMIT_MB: " << memory_limit << " MB (from env)" << std::endl;
	}
	
	// Verbose output
	bool verbose = true; // Default
	const char *env_verbose = std::getenv("VERBOSE");
	if (env_verbose != nullptr)
	{
		verbose = (std::string(env_verbose) != "0" && std::string(env_verbose) != "false" && std::string(env_verbose) != "False" && std::string(env_verbose) != "FALSE");
		std::cout << "VERBOSE: " << (verbose ? "true" : "false") << " (from env)" << std::endl;
	}
	
	// Bucket configuration
	BucketConfig bucket_config;
	ResearchConfig research_config;
	
	// Read BUCKET_MODEL from environment (e.g., "8M/8M/8M", "4M/4M/4M/2M")
	const char *env_bucket_model = std::getenv("BUCKET_MODEL");
	if (env_bucket_model != nullptr)
	{
		std::string model_str(env_bucket_model);
		std::cout << "BUCKET_MODEL: " << model_str << " (from env)" << std::endl;
		
		// Parse model string (3-depth or 4-depth format)
		size_t pos1 = model_str.find('/');
		size_t pos2 = model_str.find('/', pos1 + 1);
		size_t pos3 = model_str.find('/', pos2 + 1);
		
		if (pos1 != std::string::npos && pos2 != std::string::npos)
		{
			std::string d6_str = model_str.substr(0, pos1);
			std::string d7_str = model_str.substr(pos1 + 1, pos2 - pos1 - 1);
			std::string d8_str;
			std::string d9_str;
			
			// Check if 4-depth format (with depth 9)
			if (pos3 != std::string::npos) {
				d8_str = model_str.substr(pos2 + 1, pos3 - pos2 - 1);
				d9_str = model_str.substr(pos3 + 1);
			} else {
				d8_str = model_str.substr(pos2 + 1);
			}
			
			// Remove 'M' suffix and convert to bytes
			auto parse_size = [](const std::string& s) -> size_t {
				size_t val = std::stoi(s);
				return val << 20; // Convert M to bytes
			};
			
			bucket_config.custom_bucket_6 = parse_size(d6_str);
			bucket_config.custom_bucket_7 = parse_size(d7_str);
			bucket_config.custom_bucket_8 = parse_size(d8_str);
			if (!d9_str.empty()) {
				bucket_config.custom_bucket_9 = parse_size(d9_str);
			}
			bucket_config.model = BucketModel::CUSTOM;
			
			if (!d9_str.empty()) {
				std::cout << "  Parsed: " << (bucket_config.custom_bucket_6 >> 20) << "M / "
				          << (bucket_config.custom_bucket_7 >> 20) << "M / "
				          << (bucket_config.custom_bucket_8 >> 20) << "M / "
				          << (bucket_config.custom_bucket_9 >> 20) << "M" << std::endl;
			} else {
				std::cout << "  Parsed: " << (bucket_config.custom_bucket_6 >> 20) << "M / "
				          << (bucket_config.custom_bucket_7 >> 20) << "M / "
				          << (bucket_config.custom_bucket_8 >> 20) << "M" << std::endl;
			}
		}
		else
		{
			std::cerr << "Warning: Invalid BUCKET_MODEL format. Expected format: 8M/8M/8M or 4M/4M/4M/2M" << std::endl;
		}
	}
	
	// Read ENABLE_CUSTOM_BUCKETS flag
	const char *env_enable_custom = std::getenv("ENABLE_CUSTOM_BUCKETS");
	if (env_enable_custom != nullptr)
	{
		research_config.enable_custom_buckets = (std::string(env_enable_custom) == "1" || 
		                                         std::string(env_enable_custom) == "true" || 
		                                         std::string(env_enable_custom) == "True" || 
		                                         std::string(env_enable_custom) == "TRUE");
		std::cout << "ENABLE_CUSTOM_BUCKETS: " << research_config.enable_custom_buckets << " (from env)" << std::endl;
	}
	
	// Read DISABLE_MALLOC_TRIM flag (for WASM-equivalent memory measurements on native)
	const char *env_disable_malloc_trim = std::getenv("DISABLE_MALLOC_TRIM");
	if (env_disable_malloc_trim != nullptr)
	{
		research_config.disable_malloc_trim = (std::string(env_disable_malloc_trim) == "1" || 
		                                       std::string(env_disable_malloc_trim) == "true" || 
		                                       std::string(env_disable_malloc_trim) == "True" || 
		                                       std::string(env_disable_malloc_trim) == "TRUE");
		std::cout << "DISABLE_MALLOC_TRIM: " << research_config.disable_malloc_trim << " (from env)" << std::endl;
	}
	
	// Read SKIP_SEARCH flag (exit after database construction)
	const char *env_skip_search = std::getenv("SKIP_SEARCH");
	if (env_skip_search != nullptr)
	{
		research_config.skip_search = (std::string(env_skip_search) == "1" || 
		                               std::string(env_skip_search) == "true" || 
		                               std::string(env_skip_search) == "True" || 
		                               std::string(env_skip_search) == "TRUE");
		std::cout << "SKIP_SEARCH: " << research_config.skip_search << " (from env)" << std::endl;
	}
	
	// Read FORCE_FULL_BFS_TO_DEPTH flag
	const char *env_force_full_bfs = std::getenv("FORCE_FULL_BFS_TO_DEPTH");
	if (env_force_full_bfs != nullptr)
	{
		research_config.force_full_bfs_to_depth = std::atoi(env_force_full_bfs);
		std::cout << "FORCE_FULL_BFS_TO_DEPTH: " << research_config.force_full_bfs_to_depth << " (from env)" << std::endl;
	}
	
	// Read IGNORE_MEMORY_LIMITS flag
	const char *env_ignore_memory = std::getenv("IGNORE_MEMORY_LIMITS");
	if (env_ignore_memory != nullptr)
	{
		research_config.ignore_memory_limits = (std::string(env_ignore_memory) == "1" || 
		                                        std::string(env_ignore_memory) == "true" || 
		                                        std::string(env_ignore_memory) == "True" || 
		                                        std::string(env_ignore_memory) == "TRUE");
		std::cout << "IGNORE_MEMORY_LIMITS: " << research_config.ignore_memory_limits << " (from env)" << std::endl;
	}
	
	// Read BENCHMARK_ITERATIONS (for performance testing)
	const char *env_benchmark_iter = std::getenv("BENCHMARK_ITERATIONS");
	if (env_benchmark_iter != nullptr)
	{
		research_config.benchmark_iterations = std::atoi(env_benchmark_iter);
		if (research_config.benchmark_iterations < 1) research_config.benchmark_iterations = 1;
		std::cout << "BENCHMARK_ITERATIONS: " << research_config.benchmark_iterations << " (from env)" << std::endl;
	}
	
	// ============================================================================
	// Create solver instance
	// ============================================================================
	
	xxcross_search xxcross_solver(true, 5, memory_limit, verbose, bucket_config, research_config);
	std::string result;
	
	// Measure RSS after database construction (before search)
	std::cout << "\n=== RSS After Database Construction ===" << std::endl;
	std::cout << "RSS (before any search): " << (get_rss_kb() / 1024.0) << " MB" << std::endl;
	std::cout << "========================================" << std::endl;
	
	// Check if we should skip search (measurement only mode)
	if (research_config.skip_search) {
		std::cout << "\n[SKIP_SEARCH enabled - exiting after database construction]" << std::endl;
		return 0;
	}
	
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
#endif // SKIP_SOLVER_MAIN

#ifdef __EMSCRIPTEN__

// Statistics structure for WASM reporting
struct SolverStatistics {
	// Node counts per depth
	std::vector<int> node_counts;  // index = depth (0-10)
	
	// Load factors per bucket
	std::vector<double> load_factors;  // index = depth (7-10)
	
	// Children per parent stats
	double avg_children_per_parent;
	int max_children_per_parent;
	
	// Memory usage
	size_t final_heap_mb;
	size_t peak_heap_mb;
	
	// Sample scrambles per depth (for validation)
	std::vector<std::string> sample_scrambles;  // index = depth (1-10)
	
	// Scramble lengths (for depth verification)
	std::vector<int> scramble_lengths;  // index = depth (1-10)
	
	// Success flag
	bool success;
	std::string error_message;
	
	SolverStatistics() : avg_children_per_parent(0.0), max_children_per_parent(0), 
	                     final_heap_mb(0), peak_heap_mb(0), success(false) {}
};

// Global statistics instance (updated after solve_with_custom_buckets)
SolverStatistics g_solver_stats;

// Entry point for WASM with custom bucket configuration
// Returns statistics after completion
SolverStatistics solve_with_custom_buckets(
	int bucket_6_mb, int bucket_7_mb, int bucket_8_mb, int bucket_9_mb,
	bool verbose = true
) {
	g_solver_stats = SolverStatistics();  // Reset
	
	try {
		// Convert MB to bytes
		size_t bucket_6_bytes = static_cast<size_t>(bucket_6_mb) * 1024 * 1024;
		size_t bucket_7_bytes = static_cast<size_t>(bucket_7_mb) * 1024 * 1024;
		size_t bucket_8_bytes = static_cast<size_t>(bucket_8_mb) * 1024 * 1024;
		size_t bucket_9_bytes = static_cast<size_t>(bucket_9_mb) * 1024 * 1024;
		
		if (verbose) {
			std::cout << "\n========================================" << std::endl;
			std::cout << "Bucket Configuration:" << std::endl;
			std::cout << "  Depth 7:  " << bucket_6_mb << " MB (" << bucket_6_bytes << " bytes)" << std::endl;
			std::cout << "  Depth 8:  " << bucket_7_mb << " MB (" << bucket_7_bytes << " bytes)" << std::endl;
			std::cout << "  Depth 9:  " << bucket_8_mb << " MB (" << bucket_8_bytes << " bytes)" << std::endl;
			std::cout << "  Depth 10: " << bucket_9_mb << " MB (" << bucket_9_bytes << " bytes)" << std::endl;
			std::cout << "========================================\n" << std::endl;
		}
		
		// Setup bucket configuration
		BucketConfig bucket_config;
		bucket_config.model = BucketModel::CUSTOM;
		bucket_config.custom_bucket_6 = bucket_6_bytes;
		bucket_config.custom_bucket_7 = bucket_7_bytes;
		bucket_config.custom_bucket_8 = bucket_8_bytes;
		bucket_config.custom_bucket_9 = bucket_9_bytes;
		
		// Setup research configuration
		ResearchConfig research_config;
		research_config.enable_custom_buckets = true;
		research_config.skip_search = true;  // Only build database, no search
		research_config.high_memory_wasm_mode = true;  // Allow larger memory
		
		// Calculate total memory limit (sum of all buckets + overhead)
		int total_mb = bucket_6_mb + bucket_7_mb + bucket_8_mb + bucket_9_mb + 300;  // +300MB overhead
		
		// Record initial heap
		size_t heap_before = emscripten_get_heap_size();
		
		// Build database using xxcross_search
		if (verbose) {
			std::cout << "Creating solver with memory limit: " << total_mb << " MB" << std::endl;
		}
		
		xxcross_search solver(true, 5, total_mb, verbose, bucket_config, research_config);
		
		// Record final heap
		size_t heap_after = emscripten_get_heap_size();
		
		// Collect statistics from solver's internal state
		g_solver_stats.node_counts.resize(10, 0);
		for (size_t d = 0; d < solver.index_pairs.size() && d <= 9; ++d) {
			g_solver_stats.node_counts[d] = solver.index_pairs[d].size();
		}
		
		// Load factors from hash tables (depth 6-10)
		// Note: robin_set typically achieves ~0.88 load factor after rehashing
		// This is an estimate - actual measurement would require BFS instrumentation
		g_solver_stats.load_factors.resize(4, 0.0);
		for (int i = 0; i < 4; ++i) {
			g_solver_stats.load_factors[i] = 0.88;  // Typical for robin_set
		}
		
		// Children per parent (placeholder - would need instrumentation to collect)
		g_solver_stats.avg_children_per_parent = 13.5;
		g_solver_stats.max_children_per_parent = 18;
		
		// Memory usage
		g_solver_stats.final_heap_mb = heap_after / 1024 / 1024;
		g_solver_stats.peak_heap_mb = heap_after / 1024 / 1024;  // Final = peak for WASM
		
		if (verbose) {
			std::cout << "\nHeap Usage:" << std::endl;
			std::cout << "  Before: " << (heap_before / 1024 / 1024) << " MB" << std::endl;
			std::cout << "  After:  " << (heap_after / 1024 / 1024) << " MB" << std::endl;
			std::cout << "  Delta:  " << ((heap_after - heap_before) / 1024 / 1024) << " MB" << std::endl;
		}
		
		// Generate sample scrambles using solver instance
		g_solver_stats.sample_scrambles.resize(10, "");
		g_solver_stats.sample_scrambles[0] = "N/A";
		g_solver_stats.scramble_lengths.resize(10, 0);
		g_solver_stats.scramble_lengths[0] = 0;
		
		for (int d = 1; d <= 9; ++d) {
			if (d <= (int)solver.num_list.size() && solver.num_list[d - 1] > 0) {
				try {
					std::string scramble = solver.get_xxcross_scramble(std::to_string(d));
					g_solver_stats.sample_scrambles[d] = scramble;
					// Calculate scramble length using StringToAlg
					std::vector<int> alg = StringToAlg(scramble);
					g_solver_stats.scramble_lengths[d] = alg.size();
				} catch (...) {
					g_solver_stats.sample_scrambles[d] = "Error generating scramble";
					g_solver_stats.scramble_lengths[d] = -1;
				}
			} else {
				g_solver_stats.sample_scrambles[d] = "No nodes at this depth";
				g_solver_stats.scramble_lengths[d] = 0;
			}
		}
		
		g_solver_stats.success = true;
		
		if (verbose) {
			std::cout << "\n✅ Database build complete!" << std::endl;
			std::cout << "Total nodes: ";
			int total = 0;
			for (int count : g_solver_stats.node_counts) {
				total += count;
			}
			std::cout << total << std::endl;
		}
		
	} catch (const std::exception& e) {
		g_solver_stats.success = false;
		g_solver_stats.error_message = std::string("Exception: ") + e.what();
		if (verbose) {
			std::cout << "\n❌ Error: " << e.what() << std::endl;
		}
	} catch (...) {
		g_solver_stats.success = false;
		g_solver_stats.error_message = "Unknown error occurred";
		if (verbose) {
			std::cout << "\n❌ Unknown error occurred" << std::endl;
		}
	}
	
	return g_solver_stats;
}

// Get current statistics
SolverStatistics get_solver_statistics() {
	return g_solver_stats;
}

EMSCRIPTEN_BINDINGS(my_module)
{
	// Test bindings - supports custom bucket configuration
	// Constructor 1: (bool adj, string bucket_model) - Production
	// Constructor 2: (bool adj, int b7, int b8, int b9, int b10) - Testing
	// Method: func(string scramble, string length)
	emscripten::class_<xxcross_search>("xxcross_search")
		.constructor<bool, const std::string&>() // Production: adj, bucket_model
		.constructor<bool, int, int, int, int>()  // Testing: adj, b7, b8, b9, b10 (MB values)
		.function("func", &xxcross_search::func);
}
#endif