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
    int tempregs[32] = {0};
	unordered_map<string, function<int(MIPS_Architecture &, string, string, string)>> instructions;
	unordered_map<string, int> registerMap, address;
    pair<int,int> overwrite = {INT_MAX,INT_MAX};
    
    vector<vector<string>> init,rjb,ls;
    int locks[32] = {0};    

	static const int MAX = (1 << 20);
	int data[MAX >> 2] = {0};
	vector<vector<string>> commands;

	// constructor to initialise the instruction set
	MIPS_Architecture(ifstream &file) //constructor
	{
        init = {{"khaali"}, {"khaali"}, {"khaali"}, {"khaali"}, {"khaali"}};
        rjb = {{"khaali"}, {"khaali"}};
        ls = {{"khaali"}, {"khaali"}, {"khaali"}, {"khaali"}};
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
        if(name == "add" || name == "addi")  return v1 + v2;
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

    vector<string> IF(vector<string> command,int &instr_taken)
    {
        vector<string> res;
        res.push_back(command[0]);
        if(command.size() > 1) res.push_back(command[1]);
        else res.push_back("");
        if(command.size() > 2) res.push_back(command[2]);
        else res.push_back("");
        if(command.size() > 3) res.push_back(command[3]);
        else res.push_back("");
        res.push_back(to_string(instr_taken));
        instr_taken++;
        return res;
    }

    vector<string> DR(vector<string> input)
    {
        if (input[2].back() == ')')
        {
            int lparen = input[2].find('(');
            string offset = lparen == 0 ? "0" : input[2].substr(0, lparen);
            string reg = input[2].substr(lparen + 1);
            reg.pop_back();
            input[2] = reg;
            input[3] = offset;
            return input;
        }
        else
        {
            input[3] = "0";
            return input;
        }
    }

    vector<string> RRi(vector<string> input)
    {
        if(input[0] == "lw")
        { 
            locks[registerMap[input[1]]] += 1;
        }
        return input;
    }

    vector<string> ALU(vector<string> input)
    {
        if(input[0]=="addi")
        {
            int r2val = tempregs[registerMap[input[2]]];
            int ival = str_to_int(input[3]);
            int v = op(input[0], r2val, ival);
            tempregs[registerMap[input[1]]] = v;
            vector<string> res = {to_string(v), input[1],input[4]};
            return res;
        }
        else
        {
            int r2val = tempregs[registerMap[input[2]]];
            int r3val = tempregs[registerMap[input[3]]];
            int v = op(input[0], r2val, r3val);
            tempregs[registerMap[input[1]]] = v;
            vector<string> res = {to_string(v), input[1],input[4]};
            return res;
        }
    }

    vector<string> ALUi(vector<string> input)
    {
        if(input[0]=="lw")
        {
            int r2val = tempregs[registerMap[input[2]]];
            int offset = str_to_int(input[3]);
            input[2] = to_string(r2val+offset);
            return input;
        }
        else
        {
            int r1val = tempregs[registerMap[input[1]]];
            int r2val = tempregs[registerMap[input[2]]];
            int offset = str_to_int(input[3]);
            input[1] = to_string(r1val);
            input[2] = to_string(r2val+offset);
            return input;
        }
    }

    vector<string> DM(vector<string>input)
    {
        if(input[0]=="sw")
        {
            data[str_to_int(input[2])>>2] = str_to_int(input[1]);
            overwrite = {str_to_int(input[2])>>2,str_to_int(input[1])};
            vector<string> ans{"sw",input[4]};
            return ans;
        }
        else
        {
            int fetch = data[str_to_int(input[2])>>2];
            tempregs[registerMap[input[1]]] = fetch;
            vector<string> ans{to_string(fetch),input[1],input[4]};
            return ans;
        }
    }

    void RW(vector<string> input)
    {
        string r1 = input[1];
        registers[registerMap[r1]] = str_to_int(input[0]);
    }

    void RWl(vector<string> input)
    {
        string r1 = input[1];
        registers[registerMap[r1]] = str_to_int(input[0]);
        locks[registerMap[r1]] -= 1;
    }

    void execute79stage()
    {
        PCnext = 1;
        if (commands.size() >= MAX / 4)
		{
            std::cerr << "Memory limit exceeded\n";
			return;
		}

        int clockCycles = 0;
        int instr_taken = 0;
        int instr_done = 0;
        vector<vector<string>> newinit(5),newrjb(2),newls(4);
        vector<vector<string>> comp1 = {{"khaali"}, {"khaali"},{"khaali"},{"khaali"},{"khaali"} };
        vector<vector<string>> comp2 = {{"khaali"}, {"khaali"} };
        vector<vector<string>> comp3 = {{"khaali"}, {"khaali"},{"khaali"},{"khaali"} };
        vector<string> empty{"khaali"};

        int jctr = 0;
        int bctr = 0;
        bool branch = false;
        bool jump = false;
        bool stall57 = false;
        bool stall59 = false;
        bool stall7 = false;
        bool stall9 = false;
        bool written = false;
        printRegisters(clockCycles);
        while(true)
        {
            overwrite = {INT_MAX,INT_MAX};
            int temp = PCcurr + 1;
            written = false;
			vector<string> command = (PCcurr < commands.size()) ?  commands[PCcurr] : empty;

            if(!stall57 && !stall59)
            {
                if(jump)
                {
                    if(jctr!=3)
                    { 
                        jctr++;
                        command = empty;
                    }
                    else
                    {
                        jctr = 0;
                        jump = false;
                    }
                }
                if(branch)
                {
                    if(bctr!=5) 
                    {
                        bctr++;
                        command = empty;
                    }
                    else 
                    {
                        {
                            bctr = 0;
                            branch = false;
                        }
                    }
                }
                init[0] = command;            
            }

            if(init == comp1 && rjb == comp2 && ls == comp3) break;
            clockCycles++; // include the current cycle if pipeline is not empty

            //RWl
            if(ls[3][0]!="khaali")
            {
                if(ls[3][0]!="sw")
                {
                    if(str_to_int(ls[3].back()) <= instr_done && !written)
                    {
                        vector<string> rwl = ls[3];
                        RWl(rwl);
                        instr_done++;
                        stall9 = false;
                        written = true;
                    }
                    else
                    {
                        stall9 = true;
                        newls = ls;
                    }
                }
            }
            else stall9 = false;

            if(!stall9)
            {
                //DM2
                if(ls[2][0]!="khaali")
                {
                    vector<string> rwlinput;
                    rwlinput = DM(ls[2]);
                    if(ls[2][0] == "sw") instr_done++;
                    newls[3] = rwlinput;
                }
                else newls[3] = {"khaali"};

                //DM1
                if(ls[1][0]!="khaali")
                {
                    vector<string> dm2input;
                    dm2input = ls[1];
                    newls[2] = dm2input;
                }
                else newls[2] = {"khaali"};

                //ALUi
                if(ls[0][0]!="khaali")
                {
                    if(ls[0][0]=="lw")
                    {
                        if(checkRegister(ls[0][2]))
                        {
                            if(!locks[registerMap[ls[0][2]]])
                            {
                                vector<string> dm1input;
                                dm1input = ALUi(ls[0]);
                                newls[1] = dm1input;
                            }
                            else
                            {
                                stall9 = true;
                                newls[1] = {"khaali"};
                                newls[0] = ls[0];
                                ls = newls;
                            }
                        }
                        else
                        {
                            vector<string> dm1input;
                            dm1input = ls[0];
                            newls[1] = dm1input;
                        }
                    }
                    else
                    {
                        if(checkRegister(ls[0][2]))
                        {
                            if(!locks[registerMap[ls[0][1]]] && !locks[registerMap[ls[0][2]]])
                            {
                                vector<string> dm1input;
                                dm1input = ALUi(ls[0]);
                                newls[1] = dm1input;
                            }
                            else
                            {
                                stall9 = true;
                                newls[1] = {"khaali"};
                                newls[0] = ls[0];
                                ls = newls;
                            }
                        }
                        else
                        {
                            if(!locks[registerMap[ls[0][1]]])
                            {
                                vector<string> dm1input;
                                dm1input = ls[0];
                                dm1input[1] = to_string(tempregs[registerMap[ls[0][1]]]);
                                newls[1] = dm1input;
                            }
                            else
                            {
                                stall9 = true;
                                newls[1] = {"khaali"};
                                newls[0] = ls[0];
                                ls = newls;
                            }
                        }
                    }
                }
                else newls[1] = {"khaali"};
            }

            // RW
            if(rjb[1][0]!="khaali")
            {
                if(rjb[1][0]!="j" && rjb[1][0]!="bne" && rjb[1][0]!="beq")
                {
                    if(str_to_int(rjb[1].back()) <= instr_done && !written)
                    {
                        vector<string> rw = rjb[1];
                        RW(rw);
                        stall7 = false;
                        written = true;
                        instr_done++;
                    }
                    else
                    {
                        newrjb = rjb;
                        stall7 = true;
                    }
                }
            }
            else stall7 = false;

            if(!stall7)
            {
                // ALU
                if(rjb[0][0]!="khaali")
                {
                    if(rjb[0][0]=="j")
                    {
                        newrjb[1] = rjb[0]; 
                        instr_done++;
                    }
                    else if(rjb[0][0]=="beq" || rjb[0][0]=="bne")
                    {
                        if(!locks[registerMap[rjb[0][1]]] && !locks[registerMap[rjb[0][2]]])
                        {
                            int r1val = tempregs[registerMap[rjb[0][1]]];
                            int r2val = tempregs[registerMap[rjb[0][2]]];
                            if((rjb[0][0] == "beq" && r1val == r2val) || (rjb[0][0] == "bne" && r1val != r2val))
                            {
                                temp = address[rjb[0][3]];
                                PCcurr = temp;
                            }
                            else
                            {
                                PCcurr++;
                            }
                            newrjb[1] = rjb[0];
                            instr_done++;
                        }
                        else
                        {
                            stall7 = true;
                            newrjb[1] = {"khaali"};
                            newrjb[0] = rjb[0];
                            rjb = newrjb;
                        }
                    }
                    else if(rjb[0][0]=="addi")
                    {
                        if(!locks[registerMap[rjb[0][2]]])
                        {
                            vector<string> rwinput;
                            rwinput = ALU(rjb[0]);
                            newrjb[1] = rwinput;
                        }
                        else
                        {
                            stall7 = true;
                            newrjb[1] = {"khaali"};
                            newrjb[0] = rjb[0];
                            rjb = newrjb;
                        }
                    }
                    else
                    {
                        if(!locks[registerMap[rjb[0][2]]] && !locks[registerMap[rjb[0][3]]])
                        {
                            // cout<<rjb[0][2]<<" "<<<<" "<<rjb[0][3]<<'\n';
                            vector<string> rwinput;
                            rwinput = ALU(rjb[0]);
                            newrjb[1] = rwinput;
                        }
                        else
                        {
                            stall7 = true;
                            newrjb[1] = {"khaali"};
                            newrjb[0] = rjb[0];
                            rjb = newrjb;
                        }
                    }
                }
                else newrjb[1] = {"khaali"};
            }

            // RR
            if(init[4][0]!="khaali")
            {
                if(init[4][0] == "lw" || init[4][0] == "sw")
                {
                    if(!stall7)
                    {
                        newrjb[0] = {"khaali"};
                        rjb = newrjb;
                    }
                    if(!stall9)
                    {
                        stall59 = false;
                        vector<string> aluinput;
                        aluinput = RRi(init[4]);
                        newls[0] = aluinput;
                    }
                    else
                    {
                        stall59 = true;
                        newinit = init;
                    }
                }
                else
                {
                    if(!stall9)
                    {
                        newls[0] = {"khaali"};
                        ls = newls;
                    }
                    if(!stall7)
                    {
                        stall57 = false;
                        vector<string> aluinput;
                        aluinput = init[4];
                        newrjb[0] = aluinput;
                    }
                    else
                    {
                        stall57 = true;
                        newinit = init;
                    }
                }
            }
            else
            {
                if(!stall9) newls[0] = {"khaali"};
                if(!stall7) newrjb[0] = {"khaali"};
            }

            if(!stall57 && !stall59)
            {
                // DE2
                if(init[3][0]!="khaali")
                {
                    if(init[3][0]=="j")
                    {
                        temp = address[init[3][1]];
                        PCcurr = temp;
                        newinit[4] = init[3];
                    }
                    else if(init[3][0] == "lw" || init[3][0] == "sw")
                    {
                        vector<string> rrinput;
                        rrinput = DR(init[3]);
                        newinit[4] = rrinput;
                    }
                    else
                    {
                        vector<string> rrinput;
                        rrinput = init[3];
                        newinit[4] = rrinput;
                    }
                }
                else newinit[4] = {"khaali"};

                // DE1
                if(init[2][0]!="khaali")
                {
                    vector<string> de2input;
                    de2input = init[2];
                    newinit[3] = de2input;
                }
                else newinit[3] = {"khaali"};

                // IF2
                if(init[1][0]!="khaali")
                {
                    vector<string> de1input;
                    de1input = init[1];
                    newinit[2] = de1input;
                }
                else newinit[2] = {"khaali"};

                // IF1
                if(init[0][0]!="khaali")
                {
                    if(init[0][0] == "j") jump = true;
                    if(init[0][0] == "beq" || init[0][0] == "bne") branch = true;
                    vector<string> if2input;
                    if2input = IF(init[0],instr_taken);
                    newinit[1] = if2input;
                }
                else newinit[1] = {"khaali"};
            }

            init = newinit;
            rjb = newrjb;
            ls = newls;
            printRegisters(clockCycles);
            if(!stall57 && !stall59 && !jump && !branch) PCcurr = temp;
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

	mips->execute79stage();
	return 0;
}