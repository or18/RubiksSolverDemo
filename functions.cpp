#include <emscripten/bind.h>
#include <emscripten.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <string>
#include <sstream>

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
				alg.push_back(std::distance(move_names.begin(), it));
			}
		}
	}
	return alg;
}

std::string MirrorScramble(std::string scramble){
	std::vector<int> alg = StringToAlg(scramble);
	std::vector<int> ret;
	for(int i : alg){//14->12 face=4->4 rot=2->0
		int face = i/3;
		int rot = i%3;
		if(face==2){
		    face = 3;
		    rot = (3-rot%3-1)%3;
		}else if(face==3){
			face = 2;
			rot = (3-rot%3-1)%3;
		}else{
			rot = (3-rot%3-1)%3;
		}
		ret.push_back(3*face+rot);
	}
	return AlgToString(ret);
}

std::string ReverseScramble(std::string scramble){
	std::vector<int> alg = StringToAlg(scramble);
	std::reverse(alg.begin(), alg.end());
	std::vector<int> ret;
	for(int i : alg){
		int face = i/3;
		int rot = i%3;
		rot = (3-rot%3-1)%3;
		ret.push_back(3*face+rot);
	}
	return AlgToString(ret);
}


EMSCRIPTEN_BINDINGS(my_module)
{
	emscripten::function("scr_mirror", &MirrorScramble);
	emscripten::function("scr_reverse", &ReverseScramble);
}