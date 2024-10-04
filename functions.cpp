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
	{"U'", "U'"}, 
	{"D", "D"}, 
	{"D2", "D2"}, 
	{"D'", "D'"}, 
	{"L", "L"}, 
	{"L2", "L2"}, 
	{"L'", "L'"},
	{"R", "R"}, 
	{"R2", "R2"}, 
	{"R'", "R'"},
	{"F", "F"}, 
	{"F2", "F2"}, 
	{"F'", "F'"}, 
	{"B", "B"}, 
	{"B2", "B2"}, 
	{"B'", "B'"}, 
	{"u", "y D"}, 
	{"u2", "y2 D2"}, 
	{"u'", "y' D'"},   
	{"d", "y' U"}, 
	{"d2", "y2 U2"}, 
	{"d'", "y U'"}, 
	{"l", "x' R"}, 
	{"l2", "x2 R2"}, 
	{"l'", "x R'"}, 
	{"r", "x L"}, 
	{"r2", "x2 L2"}, 
	{"r'", "x' L'"}, 
	{"f", "z B"}, 
	{"f2", "z2 B2"}, 
	{"f'", "z' B'"}, 
	{"b", "z' F"}, 
	{"b2", "z2 F2"}, 
	{"b'", "z F'"}, 
	{"M", "x' L' R"}, 
	{"M2", "x2 L2 R2"}, 
	{"M'", "x L R'"},
	{"E", "y' U D'"}, 
	{"E2", "y2 U2 D2"}, 
	{"E'", "y U' D"},
	{"S", "z F' B"}, 
	{"S2", "z2 F2 B2"}, 
	{"S'", "z' F B'"}  
};

std::unordered_map<std::string, std::string> move_mirror = {
	{"U", "U'"}, 
	{"U2", "U2"}, 
	{"U'", "U"}, 
	{"D", "D'"}, 
	{"D2", "D2"}, 
	{"D'", "D"}, 
	{"L", "R'"}, 
	{"L2", "R2"}, 
	{"L'", "R"},
	{"R", "L'"}, 
	{"R2", "L2"}, 
	{"R'", "L"},
	{"F", "F'"}, 
	{"F2", "F2"}, 
	{"F'", "F"}, 
	{"B", "B'"}, 
	{"B2", "B2"}, 
	{"B'", "B"}, 
	{"u", "u'"}, 
	{"u2", "u2"}, 
	{"u'", "u"},   
	{"d", "d'"}, 
	{"d2", "d2"}, 
	{"d'", "d"}, 
	{"l", "r'"}, 
	{"l2", "r2"}, 
	{"l'", "r"}, 
	{"r", "l'"}, 
	{"r2", "l2"}, 
	{"r'", "l"}, 
	{"f", "f'"}, 
	{"f2", "f2"}, 
	{"f'", "f"}, 
	{"b", "b'"}, 
	{"b2", "b2"}, 
	{"b'", "b"}, 
	{"M", "M"}, 
	{"M2", "M2"}, 
	{"M'", "M'"},
	{"E", "E'"}, 
	{"E2", "E2"}, 
	{"E'", "E"},
	{"S", "S'"}, 
	{"S2", "S2"}, 
	{"S'", "S"}  
};

std::unordered_map<std::string, std::string> move_reverse = {
	{"U", "U'"}, 
	{"U2", "U2"}, 
	{"U'", "U"}, 
	{"D", "D'"}, 
	{"D2", "D2"}, 
	{"D'", "D"}, 
	{"L", "L'"}, 
	{"L2", "L2"}, 
	{"L'", "L"},
	{"R", "R'"}, 
	{"R2", "R2"}, 
	{"R'", "R"},
	{"F", "F'"}, 
	{"F2", "F2"}, 
	{"F'", "F"}, 
	{"B", "B'"}, 
	{"B2", "B2"}, 
	{"B'", "B"}, 
	{"u", "u'"}, 
	{"u2", "u2"}, 
	{"u'", "u"},   
	{"d", "d'"}, 
	{"d2", "d2"}, 
	{"d'", "d"}, 
	{"l", "l'"}, 
	{"l2", "l2"}, 
	{"l'", "l"}, 
	{"r", "r'"}, 
	{"r2", "r2"}, 
	{"r'", "r"}, 
	{"f", "f'"}, 
	{"f2", "f2"}, 
	{"f'", "f"}, 
	{"b", "b'"}, 
	{"b2", "b2"}, 
	{"b'", "b"}, 
	{"M", "M'"}, 
	{"M2", "M2"}, 
	{"M'", "M"},
	{"E", "E'"}, 
	{"E2", "E2"}, 
	{"E'", "E"},
	{"S", "S'"}, 
	{"S2", "S2"}, 
	{"S'", "S"}  
};

std::unordered_map<std::string, std::string> rotation_mirror = {
	{"z", "z'"}, 
	{"z2", "z2"}, 
	{"z'", "z"}, 
	{"y", "y'"}, 
	{"y2", "y2"}, 
	{"y'", "y"}, 
	{"x", "x"}, 
	{"x2", "x2"}, 
	{"x'", "x'"}
};

std::unordered_map<std::string, std::string> rotation_reverse = {
	{"z", "z'"}, 
	{"z2", "z2"}, 
	{"z'", "z"}, 
	{"y", "y'"}, 
	{"y2", "y2"}, 
	{"y'", "y"}, 
	{"x", "x'"}, 
	{"x2", "x2"}, 
	{"x'", "x"}
};

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
		if (move_mirror.count(name)>0)
		{
			result += move_mirror[name]+" ";
		}else if (rotation_mirror.count(name)>0)
		{
			result += rotation_mirror[name]+" ";
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
		if (move_reverse.count(name)>0)
		{
			result += move_reverse[name]+" ";
		}else if (rotation_reverse.count(name)>0)
		{
			result += rotation_reverse[name]+" ";
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

std::string ConvertScramble(std::string str)
{
	std::string result1 = "";
	std::string result2 = "";
	std::string rotation = "";
	std::istringstream iss(str);
	std::string name;
	while (iss >> name)
	{
		if (move_convert.count(name)>0)
		{
			result1 += move_convert[name]+" ";
		}else if (rotation_reverse.count(name)>0)
		{
			result1 += name+" ";
		}
	}
	std::istringstream iss2(result1);
	while (iss2 >> name)
	{
		if (move_convert.count(name)>0)
		{
			result2 += name+" ";
		}else if (rotation_reverse.count(name)>0)
		{
			result2 = AlgToString(AlgRotation(StringToAlg(result2), name));
			rotation += name + " ";
		}
	}
	return result2;
}


EMSCRIPTEN_BINDINGS(my_module)
{
	emscripten::function("scr_mirror", &MirrorScramble);
	emscripten::function("scr_reverse", &ReverseScramble);
	emscripten::function("scr_converter", &ConvertScramble);
}