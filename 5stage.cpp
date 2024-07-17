#include <unordered_map>
#include <string>
#include <functional>
#include <vector>
#include <fstream>
#include <exception>
#include <iostream>
#include <boost/tokenizer.hpp>
#include <climits>
using namespace std;

struct MIPS_Architecture
{
	int registers[32] = {0}, PCcurr = 0, PCnext;
	unordered_map<string, function<int(MIPS_Architecture &, string, string, string)>> instructions;
	unordered_map<string, int> registerMap, address;
    pair<int,int> overwrite = {INT_MAX,INT_MAX};
    
    vector<vector<string>> stages;
    int locks[32] = {0};

	static const int MAX = (1 << 20);
	int data[MAX >> 2] = {0};
	vector<vector<string>> commands;

	// constructor to initialise the instruction set
	MIPS_Architecture(ifstream &file) //constructor
	{
        stages = {{"khaali"}, {"khaali"}, {"khaali"}, {"khaali"}, {"khaali"}};
		instructions = {{"add", &MIPS_Architecture::func}, 
						{"sub", &MIPS_Architecture::func}, 
						{"mul", &MIPS_Architecture::func}, 
						{"slt", &MIPS_Architecture::func}, 
						{"beq", &MIPS_Architecture::func}, 
						{"bne", &MIPS_Architecture::func}, 
						{"j", &MIPS_Architecture::func}, 
						{"lw", &MIPS_Architecture::func}, 
						{"sw", &MIPS_Architecture::func}, 
						{"addi", &MIPS_Architecture::func}};

		for (int i = 0; i < 32; ++i)
			registerMap["$" + to_string(i)] = i;
		registerMap["$zero"] = 0;
		registerMap["$at"] = 1;
		registerMap["$v0"] = 2;
		registerMap["$v1"] = 3;
		for (int i = 0; i < 4; ++i)
			registerMap["$a" + to_string(i)] = i + 4;
		for (int i = 0; i < 8; ++i)
			registerMap["$t" + to_string(i)] = i + 8, registerMap["$s" + to_string(i)] = i + 16;
		registerMap["$t8"] = 24;
		registerMap["$t9"] = 25;
		registerMap["$k0"] = 26;
		registerMap["$k1"] = 27;
		registerMap["$gp"] = 28;
		registerMap["$sp"] = 29;
		registerMap["$s8"] = 30;
		registerMap["$ra"] = 31;

		constructCommands(file);
	}

    int func(string r1, string r2, string r3)
	{
		return 0;
	}

	// checks if the register is a valid one
	inline bool checkRegister(string r)  
	{	
		return registerMap.find(r) != registerMap.end();
	}

	// checks if all of the registers are valid or not
	bool checkRegisters(vector<string> regs)
	{
		return all_of(regs.begin(), regs.end(), [&](string r)
						   { return checkRegister(r); });
	}

    // checks if label is valid
	inline bool checkLabel(string str)
	{
		return str.size() > 0 && isalpha(str[0]) && all_of(++str.begin(), str.end(), [](char c)
														   { return (bool)isalnum(c); }) &&
			   instructions.find(str) == instructions.end();
	}

	// parse the command assuming correctly formatted MIPS instruction (or label)
	void parseCommand(string line)
	{
		// strip until before the comment begins
		line = line.substr(0, line.find('#'));
		vector<string> command;
		boost::tokenizer<boost::char_separator<char>> tokens(line, boost::char_separator<char>(", \t"));
		for (auto &s : tokens)
			command.push_back(s);
		// empty line or a comment only line
		if (command.empty())
			return;
		else if (command.size() == 1)
		{
			string label = command[0].back() == ':' ? command[0].substr(0, command[0].size() - 1) : "?";
			if (address.find(label) == address.end())
				address[label] = commands.size();
			else
				address[label] = -1;
			command.clear();
		}
		else if (command[0].back() == ':')
		{
			string label = command[0].substr(0, command[0].size() - 1);
			if (address.find(label) == address.end())
				address[label] = commands.size();
			else
				address[label] = -1;
			command = vector<string>(command.begin() + 1, command.end());
		}
		else if (command[0].find(':') != string::npos)
		{
			int idx = command[0].find(':');
			string label = command[0].substr(0, idx);
			if (address.find(label) == address.end())
				address[label] = commands.size();
			else
				address[label] = -1;
			command[0] = command[0].substr(idx + 1);
		}
		else if (command[1][0] == ':')
		{
			if (address.find(command[0]) == address.end())
				address[command[0]] = commands.size();
			else
				address[command[0]] = -1;
			command[1] = command[1].substr(1);
			if (command[1] == "")
				command.erase(command.begin(), command.begin() + 2);
			else
				command.erase(command.begin(), command.begin() + 1);
		}
		if (command.empty())
			return;
		if (command.size() > 4)
			for (int i = 4; i < (int)command.size(); ++i)
				command[3] += " " + command[i];
		command.resize(4);
		commands.push_back(command);
	}
    
	// construct the commands vector from the input file
	void constructCommands(ifstream &file)
	{
		string line;
		while (getline(file, line))
			parseCommand(line);
		file.close();
	}

    int op(string name, int v1, int v2)
    {
        if(name == "add")  return v1 + v2;
        else if(name == "sub")  return v1 - v2;
        else if(name == "mul")  return v1 * v2;
        else if(name == "slt")  return v1 < v2;
        else return INT_MIN;
    }

    int str_to_int(string s)
    {
        if(s == "") return 0;
        else return stoi(s);
    }

    void print_vector(vector<string> input)
    {
        for(auto x: input) cout<<x<<" ";
        cout<<endl;
    }

    vector<string> IF(vector<string> command)
    {
        vector<string> res;
        res.push_back(command[0]);
        if(command.size() > 1) res.push_back(command[1]);
        else res.push_back("");
        if(command.size() > 2) res.push_back(command[2]);
        else res.push_back("");
        if(command.size() > 3) res.push_back(command[3]);
        else res.push_back("");
        return res;
    }

    vector<string> DR(vector<string> input)
    {
        string r1 = input[1];
        string r2 = input[2];
        string r3 = input[3];
        int r2val = registers[registerMap[r2]];
        int r3val = registers[registerMap[r3]];
        input[2] = to_string(r2val);
        input[3] = to_string(r3val);
        locks[registerMap[r1]] += 1;
        return input;
    }

    string getAddress(vector<string> input)
    {
        if (input[2].back() == ')')
        {
            int lparen = input[2].find('(');
            string reg = input[2].substr(lparen + 1);
            reg.pop_back();
            return reg;
        }
        else return input[2];
    }

    vector<string> DRi(vector<string> input)
    {
        if(input[0]=="addi")
        {
            string r1 = input[1];
            string r2 = input[2];
            int r2val = registers[registerMap[r2]];
            input[2] = to_string(r2val);
            locks[registerMap[r1]] += 1;
            return input;
        }
        else
        {
            if (input[2].back() == ')')
            {
                int lparen = input[2].find('(');
                string offset = lparen == 0 ? "0" : input[2].substr(0, lparen);
                string reg = input[2].substr(lparen + 1);
                reg.pop_back();
                string r1 = input[1];
                int r1val = registers[registerMap[r1]];
                if (input[0]=="lw") locks[registerMap[r1]] += 1;
                int regval = registers[registerMap[reg]];
                input[2] = to_string(regval);
                input[3] = offset;
                vector<string>ans {input[0],input[1],input[2],input[3],to_string(r1val)};
                return ans;
            }
            else
            {
                string r1 = input[1];
                int r1val = registers[registerMap[r1]];
                if (input[0]=="lw") locks[registerMap[r1]] += 1;
                int address = str_to_int(input[2]);
                input[2] = to_string(address);
                input[3] = "0";
                vector<string>ans {input[0],input[1],input[2],input[3],to_string(r1val)};
                return ans;
            }
        }
    }

    vector<string> DRb(vector<string> input)
    {
        string r1 = input[1];
        string r2 = input[2];
        int r1val = registers[registerMap[r1]];
        int r2val = registers[registerMap[r2]];
        input[1] = to_string(r1val);
        input[2] = to_string(r2val);
        return input;
    }

    vector<string> ALU(vector<string> input)
    {
        int r2val = str_to_int(input[2]);
        int r3val = str_to_int(input[3]);
        int v = op(input[0], r2val, r3val);
        vector<string> res = {to_string(v), input[1]};
        return res;
    }

    vector<string> ALUi(vector<string> input)
    {
        if(input[0]=="addi")
        {
            int r2val = str_to_int(input[2]);
            int ival = str_to_int(input[3]);
            int v = r2val + ival;
            vector<string> ans{to_string(v), input[1]};
            return ans;
        }
        else
        {
            input[2] = to_string(str_to_int(input[2]) + str_to_int(input[3]));
            return input;
        }
    }

    vector<string> DM(vector<string>input)
    {
        if(input[0]=="sw")
        {
            data[str_to_int(input[2])>>2] = str_to_int(input[4]);
            overwrite = {str_to_int(input[2])>>2,str_to_int(input[4])};
            vector<string> ans{"sw"};
            return ans;
        }
        else
        {
            int fetch = data[str_to_int(input[2])>>2];
            vector<string> ans{to_string(fetch),input[1]};
            return ans;
        }
    }

    void WB(vector<string> input)
    {
        if(input[0]=="sw" || input[0] == "beq" || input[0] == "bne" || input[0] == "j"){ return;}
        string r1 = input[1];
        registers[registerMap[r1]] = str_to_int(input[0]);
        locks[registerMap[r1]] -= 1;
    }

    void execute5stage()
    {
        PCnext = 1;
        if (commands.size() >= MAX / 4)
		{
            std::cerr << "Memory limit exceeded\n";
			return;
		}

        int clockCycles = 0;
        vector<vector<string>> next_commands(5);
        vector<vector<string>> comp = {{"khaali"}, {"khaali"},{"khaali"},{"khaali"},{"khaali"} };

        bool jump = false;
        bool branch = false;
        bool bctr = false;
        bool stall = false;
        printRegisters(clockCycles);
        while(true)
        {
            overwrite = {INT_MAX,INT_MAX};
            int temp = PCcurr + 1;
            vector<string> empty{"khaali"};
			vector<string> command = (PCcurr < commands.size()) ?  commands[PCcurr] : empty;
            // if the instruction in DR stage of the pipeline is a jump instruction
            if(!jump && !branch && PCcurr < commands.size() && instructions.find(command[0])==instructions.end())
 			{
				std::cerr << "Syntax error encountered\n";
				return;
			}  
            if(jump)
            {
                command = empty;
                jump = false;
            }
            // if the instruction in DR stage of the pipeline is a branch instruction which would imply 2 stalls
            if(branch)
            {
                if(bctr)
                {
                    // cout<<"Second entry\n";
                    if(!stall) bctr = branch = false;
                    command = empty;
                }
                else
                {
                    // cout<<"First entry\n";
                    if(!stall)bctr = true;
                    command = empty;
                }
            }
            stages[0] = command;
            if(stages == comp) break;
            clockCycles++; // include the current cycle if pipeline is not empty

            // first half of cycle
            if(stages[4][0] != "khaali")
            {
                vector<string> wb_input = stages[4];
                WB(wb_input);
            }

            // second half of cycle
            // DM
            if(stages[3][0]!="khaali")
            {
                vector<string> wb_input;
                if(stages[3][0] == "lw" || stages[3][0] == "sw") wb_input = DM(stages[3]);
                else {wb_input = stages[3];}
                next_commands[4] = wb_input;
            }
            else
            {
                next_commands[4] = {"khaali"};
            }

            // ALU
            if(stages[2][0]!="khaali")
            {
                vector<string> mem_input;
                if(stages[2][0] == "beq" || stages[2][0] == "bne")
                {
                    mem_input = stages[2];
                    int r1val = str_to_int(stages[2][1]);
                    int r2val = str_to_int(stages[2][2]);
                    if((stages[2][0] == "beq" && r1val==r2val) || (stages[2][0] == "bne" && r1val!=r2val))
                    {
                        temp = address[stages[2][3]];
                    }
                }
                else if(stages[2][0] == "j") mem_input = stages[2];
                else if(stages[2][0] == "lw" || stages[2][0] == "addi" || stages[2][0] == "sw")  mem_input = ALUi(stages[2]);
                else mem_input = ALU(stages[2]);
                next_commands[3] = mem_input;
            }
            else
            {
                next_commands[3] = {"khaali"};
            }

            // DR
            if(stages[1][0]!="khaali")
            {
                // j
                if(stages[1][0] == "j")
                {
                    temp = address[stages[1][1]];
                    next_commands[2] = stages[1];
                }
                // branch 
                else if(stages[1][0] == "beq" || stages[1][0] == "bne")
                {
                    if(locks[registerMap[stages[1][1]]] || locks[registerMap[stages[1][2]]])
                    {
                        next_commands[2] = {"khaali"};
                        next_commands[1] = stages[1];
                        next_commands[0] = stages[0];
                        stages = next_commands;   
                        printRegisters(clockCycles);
                        stall = true;
                        continue;
                    }
                    vector<string> alu_input = DRb(stages[1]);
                    next_commands[2] = alu_input;
                }
                // addi
                else if(stages[1][0] == "addi")
                {
                    if(locks[registerMap[stages[1][2]]])
                    { 
                        next_commands[2] = {"khaali"};
                        next_commands[1] = stages[1];
                        next_commands[0] = stages[0];
                        stages = next_commands;   
                        printRegisters(clockCycles);
                        continue;
                    }
                    vector<string> alu_input = DRi(stages[1]);
                    next_commands[2] = alu_input;
                }
                // lw
                else if(stages[1][0] == "lw")
                {
                    string addr = getAddress(stages[1]);
                    if(checkRegister(addr) && locks[registerMap[addr]])
                    { 
                        next_commands[2] = {"khaali"};
                        next_commands[1] = stages[1];
                        next_commands[0] = stages[0];
                        stages = next_commands;   
                        printRegisters(clockCycles);
                        continue;
                    }
                    vector<string> alu_input = DRi(stages[1]);
                    next_commands[2] = alu_input;
                }
                // sw
                else if(stages[1][0] == "sw")
                {
                    string addr = getAddress(stages[1]);
                    if(locks[registerMap[stages[1][1]]] || (checkRegister(addr) && locks[registerMap[addr]]))
                    { 
                        next_commands[2] = {"khaali"};
                        next_commands[1] = stages[1];
                        next_commands[0] = stages[0];
                        stages = next_commands;   
                        printRegisters(clockCycles);
                        continue;
                    }
                    vector<string> alu_input = DRi(stages[1]);
                    next_commands[2] = alu_input;
                }
                // r type instructions
                else
                {
                    if(locks[registerMap[stages[1][2]]] || locks[registerMap[stages[1][3]]])
                    { 
                        next_commands[2] = {"khaali"};
                        next_commands[1] = stages[1];
                        next_commands[0] = stages[0];
                        stages = next_commands;   
                        printRegisters(clockCycles);
                        continue;
                    }
                    vector<string> alu_input = DR(stages[1]);
                    next_commands[2] = alu_input;
                }
            } 
            else
            {
                next_commands[2] = {"khaali"};
            }

            // IF
            if(stages[0][0]!="khaali")
            {
                if(stages[0][0] == "j") jump = true;
                if(stages[0][0] == "beq" || stages[0][0] == "bne") branch = true;
                vector<string> dr_input = IF(stages[0]);
                next_commands[1] = dr_input;
            }
            else{
                next_commands[1] = {"khaali"};
            }

            stages = next_commands;
            printRegisters(clockCycles);
            stall = false; // stall in branch instruction
            if(!jump && !branch) PCcurr = temp;
        }
    }

	// print the register data in hexadecimal
	void printRegisters(int clockCycle)
	{
		for (int i = 0; i < 32; ++i)
			cout << registers[i] << ' ';
		cout << dec << '\n';
        if(overwrite.first == INT_MAX) cout<<0<<'\n';
        else cout<<1<<" "<<overwrite.first<<" "<<overwrite.second<<'\n';
	}
};

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		std::cerr << "Required argument: file_name\n./MIPS_interpreter <file name>\n";
		return 0;
	}
	std::ifstream file(argv[1]);
	MIPS_Architecture *mips;
	if (file.is_open())
		mips = new MIPS_Architecture(file);
	else
	{
		std::cerr << "File could not be opened. Terminating...\n";
		return 0;
	}

	mips->execute5stage();
	return 0;
}