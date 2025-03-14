#include <emscripten/bind.h>
#include <emscripten.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <string>
#include <sstream>

std::vector<std::string> move_names = {"U", "U2", "U'", "D", "D2", "D'", "L", "L2", "L'", "R", "R2", "R'", "F", "F2", "F'", "B", "B2", "B'"};
std::unordered_map<std::string, std::string> move_convert =
	{
		{"U", "U"},
		{"U2", "U2"},
		{"U2'", "U2"},
		{"U'", "U'"},
		{"D", "D"},
		{"D2", "D2"},
		{"D2'", "D2"},
		{"D'", "D'"},
		{"L", "L"},
		{"L2", "L2"},
		{"L2'", "L2"},
		{"L'", "L'"},
		{"R", "R"},
		{"R2", "R2"},
		{"R2'", "R2"},
		{"R'", "R'"},
		{"F", "F"},
		{"F2", "F2"},
		{"F2'", "F2"},
		{"F'", "F'"},
		{"B", "B"},
		{"B2", "B2"},
		{"B2'", "B2"},
		{"B'", "B'"},
		{"u", "y D"},
		{"u2", "y2 D2"},
		{"u2'", "y2 D2"},
		{"u'", "y' D'"},
		{"Uw", "y D"},
		{"Uw2", "y2 D2"},
		{"Uw2'", "y2 D2"},
		{"Uw'", "y' D'"},
		{"d", "y' U"},
		{"d2", "y2 U2"},
		{"d2'", "y2 U2"},
		{"d'", "y U'"},
		{"Dw", "y' U"},
		{"Dw2", "y2 U2"},
		{"Dw2'", "y2 U2"},
		{"Dw'", "y U'"},
		{"l", "x' R"},
		{"l2", "x2 R2"},
		{"l2'", "x2 R2"},
		{"l'", "x R'"},
		{"Lw", "x' R"},
		{"Lw2", "x2 R2"},
		{"Lw2'", "x2 R2"},
		{"Lw'", "x R'"},
		{"r", "x L"},
		{"r2", "x2 L2"},
		{"r2'", "x2 L2"},
		{"r'", "x' L'"},
		{"Rw", "x L"},
		{"Rw2", "x2 L2"},
		{"Rw2'", "x2 L2"},
		{"Rw'", "x' L'"},
		{"f", "z B"},
		{"f2", "z2 B2"},
		{"f2'", "z2 B2"},
		{"f'", "z' B'"},
		{"Fw", "z B"},
		{"Fw2", "z2 B2"},
		{"Fw2'", "z2 B2"},
		{"Fw'", "z' B'"},
		{"b", "z' F"},
		{"b2", "z2 F2"},
		{"b2'", "z2 F2"},
		{"b'", "z F'"},
		{"Bw", "z' F"},
		{"Bw2", "z2 F2"},
		{"Bw2'", "z2 F2"},
		{"Bw'", "z F'"},
		{"M", "x' L' R"},
		{"M2", "x2 L2 R2"},
		{"M2'", "x2 L2 R2"},
		{"M'", "x L R'"},
		{"E", "y' U D'"},
		{"E2", "y2 U2 D2"},
		{"E2'", "y2 U2 D2"},
		{"E'", "y U' D"},
		{"S", "z F' B"},
		{"S2", "z2 F2 B2"},
		{"S2'", "z2 F2 B2"},
		{"S'", "z' F B'"}};

std::unordered_map<std::string, std::string> move_mirror = {
	{"U", "U'"},
	{"U2", "U2'"},
	{"U2'", "U2"},
	{"U'", "U"},
	{"D", "D'"},
	{"D2", "D2'"},
	{"D2'", "D2"},
	{"D'", "D"},
	{"L", "R'"},
	{"L2", "R2'"},
	{"L2'", "R2"},
	{"L'", "R"},
	{"R", "L'"},
	{"R2", "L2'"},
	{"R2'", "L2"},
	{"R'", "L"},
	{"F", "F'"},
	{"F2", "F2'"},
	{"F2'", "F2"},
	{"F'", "F"},
	{"B", "B'"},
	{"B2", "B2'"},
	{"B2'", "B2"},
	{"B'", "B"},
	{"u", "u'"},
	{"u2", "u2'"},
	{"u2'", "u2"},
	{"u'", "u"},
	{"Uw", "Uw'"},
	{"Uw2", "Uw2'"},
	{"Uw2'", "Uw2"},
	{"Uw'", "Uw"},
	{"d", "d'"},
	{"d2", "d2'"},
	{"d2'", "d2"},
	{"d'", "d"},
	{"Dw", "Dw'"},
	{"Dw2", "Dw2'"},
	{"Dw2'", "Dw2"},
	{"Dw'", "Dw"},
	{"l", "r'"},
	{"l2", "r2'"},
	{"l2'", "r2"},
	{"l'", "r"},
	{"Lw", "Rw'"},
	{"Lw2", "Rw2'"},
	{"Lw2'", "Rw2"},
	{"Lw'", "Rw"},
	{"r", "l'"},
	{"r2", "l2'"},
	{"r2'", "l2"},
	{"r'", "l"},
	{"Rw", "Lw'"},
	{"Rw2", "Lw2'"},
	{"Rw2'", "Lw2"},
	{"Rw'", "Lw"},
	{"f", "f'"},
	{"f2", "f2'"},
	{"f2'", "f2"},
	{"f'", "f"},
	{"Fw", "Fw'"},
	{"Fw2", "Fw2'"},
	{"Fw2'", "Fw2"},
	{"Fw'", "Fw"},
	{"b", "b'"},
	{"b2", "b2'"},
	{"b2'", "b2"},
	{"b'", "b"},
	{"Bw", "Bw'"},
	{"Bw2", "Bw2'"},
	{"Bw2'", "Bw2"},
	{"Bw'", "Bw"},
	{"M", "M"},
	{"M2", "M2"},
	{"M2'", "M2'"},
	{"M'", "M'"},
	{"E", "E'"},
	{"E2", "E2'"},
	{"E2'", "E2"},
	{"E'", "E"},
	{"S", "S'"},
	{"S2", "S2'"},
	{"S2'", "S2"},
	{"S'", "S"}};

std::unordered_map<std::string, std::string> move_reverse = {
	{"U", "U'"},
	{"U2", "U2'"},
	{"U2'", "U2"},
	{"U'", "U"},
	{"D", "D'"},
	{"D2", "D2'"},
	{"D2'", "D2"},
	{"D'", "D"},
	{"L", "L'"},
	{"L2", "L2'"},
	{"L2'", "L2"},
	{"L'", "L"},
	{"R", "R'"},
	{"R2", "R2'"},
	{"R2'", "R2"},
	{"R'", "R"},
	{"F", "F'"},
	{"F2", "F2'"},
	{"F2'", "F2"},
	{"F'", "F"},
	{"B", "B'"},
	{"B2", "B2'"},
	{"B2'", "B2"},
	{"B'", "B"},
	{"u", "u'"},
	{"u2", "u2'"},
	{"u2'", "u2"},
	{"u'", "u"},
	{"Uw", "Uw'"},
	{"Uw2", "Uw2'"},
	{"Uw2'", "Uw2"},
	{"Uw'", "Uw"},
	{"d", "d'"},
	{"d2", "d2'"},
	{"d2'", "d2"},
	{"d'", "d"},
	{"Dw", "Dw'"},
	{"Dw2", "Dw2'"},
	{"Dw2'", "Dw2"},
	{"Dw'", "Dw"},
	{"l", "l'"},
	{"l2", "l2'"},
	{"l2'", "l2"},
	{"l'", "l"},
	{"Lw", "Lw'"},
	{"Lw2", "Lw2'"},
	{"Lw2'", "Lw2"},
	{"Lw'", "Lw"},
	{"r", "r'"},
	{"r2", "r2'"},
	{"r2'", "r2"},
	{"r'", "r"},
	{"Rw", "Rw'"},
	{"Rw2", "Rw2'"},
	{"Rw2'", "Rw2"},
	{"Rw'", "Rw"},
	{"f", "f'"},
	{"f2", "f2'"},
	{"f2'", "f2"},
	{"f'", "f"},
	{"Fw", "Fw'"},
	{"Fw2", "Fw2'"},
	{"Fw2'", "Fw2"},
	{"Fw'", "Fw"},
	{"b", "b'"},
	{"b2", "b2'"},
	{"b2'", "b2"},
	{"b'", "b"},
	{"Bw", "Bw'"},
	{"Bw2", "Bw2'"},
	{"Bw2'", "Bw2"},
	{"Bw'", "Bw"},
	{"M", "M'"},
	{"M2", "M2'"},
	{"M2'", "M2"},
	{"M'", "M"},
	{"E", "E'"},
	{"E2", "E2'"},
	{"E2'", "E2"},
	{"E'", "E"},
	{"S", "S'"},
	{"S2", "S2'"},
	{"S2'", "S2"},
	{"S'", "S"}};

std::unordered_map<std::string, std::string> rotation_mirror = {
	{"z", "z'"},
	{"z2", "z2'"},
	{"z2'", "z2"},
	{"z'", "z"},
	{"y", "y'"},
	{"y2", "y2'"},
	{"y2'", "y2"},
	{"y'", "y"},
	{"x", "x"},
	{"x2", "x2"},
	{"x2'", "x2'"},
	{"x'", "x'"}};

std::unordered_map<std::string, std::string> rotation_reverse = {
	{"z", "z'"},
	{"z2", "z2'"},
	{"z2'", "z2"},
	{"z'", "z"},
	{"y", "y'"},
	{"y2", "y2'"},
	{"y2'", "y2"},
	{"y'", "y"},
	{"x", "x'"},
	{"x2", "x2'"},
	{"x2'", "x2"},
	{"x'", "x"}};

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
				alg.push_back(std::distance(move_names.begin(), it));
			}
		}
	}
	return alg;
}

std::string MirrorScramble(std::string str)
{
	std::string result = "";
	std::istringstream iss(str);
	std::string name;
	while (iss >> name)
	{
		if (move_mirror.count(name) > 0)
		{
			result += move_mirror[name] + " ";
		}
		else if (rotation_mirror.count(name) > 0)
		{
			result += rotation_mirror[name] + " ";
		}
	}
	return result;
}

std::string ReverseScramble(std::string str)
{
	std::reverse(str.begin(), str.end());
	std::string result = "";
	std::istringstream iss(str);
	std::string name;
	while (iss >> name)
	{
		std::reverse(name.begin(), name.end());
		if (move_reverse.count(name) > 0)
		{
			result += move_reverse[name] + " ";
		}
		else if (rotation_reverse.count(name) > 0)
		{
			result += rotation_reverse[name] + " ";
		}
	}
	return result;
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
	else if (rotation == "x2" || rotation == "x2'")
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
	else if (rotation == "y2" || rotation == "y2'")
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
	else if (rotation == "z2" || rotation == "z2'")
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

std::string ConvertScramble(std::string str)
{
	std::string result1 = "";
	std::string result2 = "";
	std::string rotation = "";
	std::istringstream iss(str);
	std::string name;
	while (iss >> name)
	{
		if (move_convert.count(name) > 0)
		{
			result1 += move_convert[name] + " ";
		}
		else if (rotation_reverse.count(name) > 0)
		{
			result1 += name + " ";
		}
	}
	std::istringstream iss2(result1);
	while (iss2 >> name)
	{
		if (move_convert.count(name) > 0)
		{
			result2 += name + " ";
		}
		else if (rotation_reverse.count(name) > 0)
		{
			result2 = AlgToString(AlgRotation(StringToAlg(result2), name));
			rotation += name + " ";
		}
	}
	return result2;
}

struct State
{
	std::vector<int> sc;

	State(std::vector<int> arg_sc = {0, 1, 2, 3, 4, 5, 6, 7, 8,
									 9, 10, 11, 12, 13, 14, 15, 16, 17,
									 18, 19, 20, 21, 22, 23, 24, 25, 26,
									 27, 28, 29, 30, 31, 32, 33, 34, 35,
									 36, 37, 38, 39, 40, 41, 42, 43, 44,
									 45, 46, 47, 48, 49, 50, 51, 52, 53}) : sc(arg_sc) {}

	State apply_move(State move)
	{
		std::vector<int> new_sc;
		for (int i = 0; i < 54; ++i)
		{
			int p = move.sc[i];
			new_sc.emplace_back(sc[p]);
		}
		return State(new_sc);
	}
};

std::string StateToInput(State state)
{
	std::vector<int> sc = state.sc;
	std::string input = "";
	for (int i : sc)
	{
		if (i < 9)
		{
			input += "U";
		}
		else if (i < 18)
		{
			input += "R";
		}
		else if (i < 27)
		{
			input += "F";
		}
		else if (i < 36)
		{
			input += "D";
		}
		else if (i < 45)
		{
			input += "L";
		}
		else
		{
			input += "B";
		}
	}
	return input;
}

std::unordered_map<std::string, State> moves = {
	{"U", State({6, 3, 0, 7, 4, 1, 8, 5, 2,
				 45, 46, 47, 12, 13, 14, 15, 16, 17,
				 9, 10, 11, 21, 22, 23, 24, 25, 26,
				 27, 28, 29, 30, 31, 32, 33, 34, 35,
				 18, 19, 20, 39, 40, 41, 42, 43, 44,
				 36, 37, 38, 48, 49, 50, 51, 52, 53})},
	{"R", State({0, 1, 20, 3, 4, 23, 6, 7, 26,
				 15, 12, 9, 16, 13, 10, 17, 14, 11,
				 18, 19, 29, 21, 22, 32, 24, 25, 35,
				 27, 28, 51, 30, 31, 48, 33, 34, 45,
				 36, 37, 38, 39, 40, 41, 42, 43, 44,
				 8, 46, 47, 5, 49, 50, 2, 52, 53})},
	{"F", State({0, 1, 2, 3, 4, 5, 44, 41, 38,
				 6, 10, 11, 7, 13, 14, 8, 16, 17,
				 24, 21, 18, 25, 22, 19, 26, 23, 20,
				 15, 12, 9, 30, 31, 32, 33, 34, 35,
				 36, 37, 27, 39, 40, 28, 42, 43, 29,
				 45, 46, 47, 48, 49, 50, 51, 52, 53})},
	{"D", State({0, 1, 2, 3, 4, 5, 6, 7, 8,
				 9, 10, 11, 12, 13, 14, 24, 25, 26,
				 18, 19, 20, 21, 22, 23, 42, 43, 44,
				 33, 30, 27, 34, 31, 28, 35, 32, 29,
				 36, 37, 38, 39, 40, 41, 51, 52, 53,
				 45, 46, 47, 48, 49, 50, 15, 16, 17})},
	{"L", State({53, 1, 2, 50, 4, 5, 47, 7, 8,
				 9, 10, 11, 12, 13, 14, 15, 16, 17,
				 0, 19, 20, 3, 22, 23, 6, 25, 26,
				 18, 28, 29, 21, 31, 32, 24, 34, 35,
				 42, 39, 36, 43, 40, 37, 44, 41, 38,
				 45, 46, 33, 48, 49, 30, 51, 52, 27})},
	{"B", State({11, 14, 17, 3, 4, 5, 6, 7, 8,
				 9, 10, 35, 12, 13, 34, 15, 16, 33,
				 18, 19, 20, 21, 22, 23, 24, 25, 26,
				 27, 28, 29, 30, 31, 32, 36, 39, 42,
				 2, 37, 38, 1, 40, 41, 0, 43, 44,
				 51, 48, 45, 52, 49, 46, 53, 50, 47})}};

std::string ScrambleToState(std::string scramble){
		std::vector<std::string> move_names2;
		std::vector<std::string> faces;
		for (const auto& pair : moves) {
			faces.push_back(pair.first);
		}
		for (const auto& face_name : faces) {
			move_names2.push_back(face_name);
			move_names2.push_back(face_name + "2");
			move_names2.push_back(face_name + "'");
			moves[face_name + "2"] = moves[face_name].apply_move(moves[face_name]);
			moves[face_name + "'"] = moves[face_name + "2"].apply_move(moves[face_name]);
		}

		State state = State();
		std::string name;
		std::istringstream iss(scramble);
		while (iss >> name)
		{
			if (!name.empty())
			{
				state = state.apply_move(moves[name]);
			}
		}
		std::string output = StateToInput(state);
		return output;
}

EMSCRIPTEN_BINDINGS(my_module)
{
	emscripten::function("scr_mirror", &MirrorScramble);
	emscripten::function("scr_reverse", &ReverseScramble);
	emscripten::function("scr_converter", &ConvertScramble);
	emscripten::function("ScrambleToState", &ScrambleToState);

}