#ifndef __BRANCH_PREDICTOR_HPP__
#define __BRANCH_PREDICTOR_HPP__

#include <vector>
#include <bitset>
#include <cassert>
using namespace std;
struct BranchPredictor {
    virtual bool predict(uint32_t pc) = 0;
    virtual void update(uint32_t pc, bool taken) = 0;
};

struct SaturatingBranchPredictor : public BranchPredictor {
    std::vector<std::bitset<2>> table;
    SaturatingBranchPredictor(int value) : table(1 << 14, value) {}

    bool predict(uint32_t pc) {
        // your code here
        int bits14 = (1<<14) - 1;
        int addr = pc & bits14;
        bitset<2> otherBits("10");
        return( (otherBits & table[addr]) == otherBits);
        // return false;
    }

    void update(uint32_t pc, bool taken) {
        // your code here
        int bits14 = (1<<14) - 1;
        int addr = pc & bits14;
        bitset<2> cur = table[addr];
        bitset<2> high("11");
        bitset<2> low("00");
        if(taken && cur!= high)
        {
            int val = cur.to_ulong();
            val++;
            table[addr] = std::bitset<2>(val);
        }
        if(!taken && cur!= low)
        {
            int val = cur.to_ulong();
            val--;
            table[addr] = std::bitset<2>(val);
        }
    }
};



struct BHRBranchPredictor : public BranchPredictor {
    std::vector<std::bitset<2>> bhrTable;
    std::bitset<2> bhr;
    BHRBranchPredictor(int value) : bhrTable(1 << 2, value), bhr(value) {}

    bool predict(uint32_t pc) {
        // your code here
        int index = bhr.to_ulong();

        int counter = bhrTable[index].to_ulong();
        return (counter >= 2); // predict taken if counter is 2 or 3
        // return false;
    }

    void update(uint32_t pc, bool taken) {
        // your code here
        int index = bhr.to_ulong(); // extract last 4 bits of pc
        int counter = bhrTable[index].to_ulong();
        if (taken) {
        if (counter < 3) counter++;
        } else {
        if (counter > 0) counter--;
        }
        bhrTable[index] = counter;
        bitset<2> cur("0" + to_string( (int)taken));
        bhr = (bhr << 1) | cur; // update the BHR
    }
};

struct SaturatingBHRBranchPredictor : public BranchPredictor {
    std::vector<std::bitset<2>> bhrTable;
    std::bitset<2> bhr;
    std::vector<std::bitset<2>> table;
    std::vector<std::bitset<2>> combination;
    SaturatingBHRBranchPredictor(int value, int size) : bhrTable(1 << 2, value), bhr(value), table(1 << 14, value), combination(size, value) {
        assert(size <= (1 << 16));
    }

    
    bool predict(uint32_t pc) {
        // your code here
        int index = pc & ((1 << 14 ) - 1);
        int bhrIndex = bhr.to_ulong();
        int combIndex = ((bhrIndex << 14) | index) & ((1 << combination.size()) - 1); //ensures max 2^16 concatenation
        int globalCounter = table[index].to_ulong();
        int bhrCounter = bhrTable[bhrIndex].to_ulong();
        int combCounter = combination[combIndex].to_ulong();

        int check = 0;
        if(combCounter >=2) check++;
        if(globalCounter >=2) check ++;
        if(bhrCounter >=2) check++;
        double w1 = 0.104;
        double w3 = 0.112;
        double w4 = 0.952;
        double w2 = 1 - w1 - w3;
        return (globalCounter*w2 + combCounter*w1 + bhrCounter*w3 >= w4  );
    }

    void update(uint32_t pc, bool taken) {
        // your code here
        int index = pc & ((1 << 14 ) - 1);
        int bhrIndex = bhr.to_ulong();
        int combIndex = ((bhrIndex << 14) | index) & ((1 << combination.size()) - 1); //ensures max 2^16
        int globalCounter = table[index].to_ulong();
        int bhrCounter = bhrTable[bhrIndex].to_ulong();
        int combCounter = combination[combIndex].to_ulong();

        if (taken) {
            if (globalCounter < 3) globalCounter++;
            if (bhrCounter < 3) bhrCounter++;
            if (combCounter < 3) combCounter++;
        } else {
            if (globalCounter > 0) globalCounter--;
            if (bhrCounter > 0) bhrCounter--;
            if (combCounter > 0) combCounter--;
        }

        table[index] = globalCounter;
        bhrTable[bhrIndex] = bhrCounter;
        combination[combIndex] = combCounter;

        bitset<2> cur("0" + to_string( (int)taken));
        bhr = (bhr << 1) | cur; // update the BHR

    }
};

#endif