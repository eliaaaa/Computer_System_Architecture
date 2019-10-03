/*
Cache Simulator
Level one L1 and level two L2 cache parameters are read from file (block size, line per set and set per cache).
The 32 bit address is divided into tag bits (t), set index bits (s) and block offset bits (b)
s = log2(#sets)   b = log2(block size)  t=32-s-b
*/
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <iomanip>
#include <stdlib.h>
#include <cmath>
#include <bitset>

using namespace std;
//access state:
#define NA 0 // no action
#define RH 1 // read hit
#define RM 2 // read miss
#define WH 3 // Write hit
#define WM 4 // write miss




struct config{
    int L1blocksize;
    int L1setsize;
    int L1size;
    int L2blocksize;
    int L2setsize;
    int L2size;
};

/* you can define the cache class here, or design your own data structure for L1 and L2 cache
class cache {

      }
*/

class Cache{
    int L1_blocksize, L2_blocksize, L1_setsize, L2_setsize;
    unsigned long L1_offset, L1_setAmount, L1_indexLength, L1_tagLength;
    unsigned long L2_offset, L2_setAmount, L2_indexLength, L2_tagLength;
    // here in the vector unsigned long store tag array
    vector<vector<unsigned long>> Cache_L1;
    vector<vector<unsigned long>> Cache_L2;
    // store LRU
    vector<vector<unsigned long>> LRU_L1;
    vector<vector<unsigned long>> LRU_L2;

    public:
    explicit Cache(config config_param);;
    void resizeCache(int &L1_wayAmount, int &L2_wayAmount){


        this -> Cache_L1.resize(this -> L1_setAmount);
        for(int i=0; i < this -> L1_setAmount;i++) {
            this -> Cache_L1[i].resize(static_cast<unsigned long>(L1_wayAmount));

        }

        this -> Cache_L2.resize(this -> L2_setAmount);
        for(int i=0; i < this -> L2_setAmount;i++) {
            this -> Cache_L2[i].resize(static_cast<unsigned long>(L2_wayAmount));

        }

    }
    void resizeLRU(int &L1_wayAmount, int &L2_wayAmount){
        this -> LRU_L1.resize(this -> L1_setAmount);
        for(int i=0; i < this -> L1_setAmount;i++) {
            this -> LRU_L1[i].resize(static_cast<unsigned long>(L1_wayAmount)+1);
            // last item in each set is to denote the MRU count for that way

        }

        this -> LRU_L2.resize(this -> L2_setAmount);
        for(int i=0; i < this -> L2_setAmount;i++) {
            this -> LRU_L2[i].resize(static_cast<unsigned long>(L2_wayAmount)+1);
            // last item in each set is to denote the MRU count for that way
        }

    }

    // convert bitset accessaddr to string and get the store location for L1,L2 cache
    vector<long> getAddrCacheLocation(bitset<32> accessaddr) {

        string stringAddr = accessaddr.to_string();

        vector<string> CacheLocation(6);
        // L1
        CacheLocation[0] = stringAddr.substr(0, this->L1_tagLength); // tag array
        CacheLocation[1] = stringAddr.substr(this->L1_tagLength, this->L1_indexLength); // index
        CacheLocation[2] = stringAddr.substr(this->L1_tagLength + this->L1_indexLength, this->L1_offset); //offset
        // L2
        CacheLocation[3] = stringAddr.substr(0, this->L2_tagLength);
        CacheLocation[4] = stringAddr.substr(this->L2_tagLength, this->L2_indexLength);
        CacheLocation[5] = stringAddr.substr(this->L2_tagLength + this->L2_indexLength,this->L2_offset);

        vector<long> CacheLocation_digit(6); // binary format for tag, decimal for index and offset

        // L1
        CacheLocation_digit[0]= strtol(CacheLocation[0].c_str(), nullptr, 10);  // TAG
        if(!CacheLocation[1].empty()){
            CacheLocation_digit[1]= strtol(CacheLocation[1].c_str(), nullptr, 2);  // index of this addr in L1
        }
        CacheLocation_digit[2]= strtol(CacheLocation[2].c_str(), nullptr, 2);  //offset

        // L2
        CacheLocation_digit[3]= strtol(CacheLocation[3].c_str(), nullptr, 10);
        if(!CacheLocation[4].empty()){
            CacheLocation_digit[4]= strtol(CacheLocation[4].c_str(), nullptr, 2);  // index of this addr in L1
        }
        CacheLocation_digit[5]= strtol(CacheLocation[5].c_str(), nullptr, 2);


        return CacheLocation_digit;
    }

    bool is_inL1(long L1_index, long L1_tag){

        bool is_in_L1 = false;
        // check different ways in this set
        for(int i = 0; i < this -> L1_setsize; i++){
            // check tag in "index"th set and "i"th way
            if(this->Cache_L1[L1_index][i] == L1_tag ){
                // hit
                is_in_L1 = true;
                break;
            }
        }
        return is_in_L1;
    }

    bool is_inL2 (long L2_index, long L2_tag){

        bool is_in_L2 = false;
        for(int i = 0; i < this -> L2_setsize; i++){
            // check tag in "index"th set and "i"th way
            if(this->Cache_L2[L2_index][i] == L2_tag ){
                // hit
                is_in_L2 = true;
                break;
            }
        }
        return is_in_L2;
    }
    // return the way index of the tag in this set
    int find_inL1(long L1_index, long L1_tag){

        int way = 0;
        // check different ways in this set
        for(int i = 0; i < this -> L1_setsize; i++){
            // check tag in "index"th set and "i"th way
            if(this->Cache_L1[L1_index][i] == L1_tag ){
                // hit
                way = i;
                break;
            }
        }
        return way;
    }

    // return the way index of the tag in this set
    int find_inL2(long L2_index, long L2_tag){

        int way = 0;
        // check different ways in this set
        for(int i = 0; i < this -> L2_setsize; i++){
            // check tag in "index"th set and "i"th way
            if(this->Cache_L2[L2_index][i] == L2_tag ){
                // hit
                way = i;
                break;
            }
        }
        return way;
    }

    bool is_setFullL1(long L1_index){
        bool is_set_full = true;
        // set size is number of ways
        for(int i = 0; i < this -> L1_setsize; i++){
            // check if empty in "index"th set and "i"th way
            if(this -> Cache_L1[L1_index][i] == 0 ){
                // not full
                is_set_full = false;
                break;
            }
        }

        return is_set_full;
    }

    bool is_setFullL2(long L2_index){
        bool is_set_full = true;
        // set size is number of ways
        for(int i = 0; i < this -> L2_setsize; i++){
            // check if empty in "index"th set and "i"th way
            if(this -> Cache_L2[L2_index][i] == 0 ){
                // not full
                is_set_full = false;
                break;
            }
        }
        return is_set_full;
    }

    int getEmptyWayIndex_L1(long L1_index){

        int index = L1_setsize; // when cannot locate empty, return the set size, designed for debug
        for(int i = 0; i < this -> L1_setsize; i++){
            // check if empty in "L1_index"th set and "i"th way
            if(this -> Cache_L1[L1_index][i] == 0 ){
                index = i;
                break;
            }
        }
        return index;
    }

    int getEmptyWayIndex_L2(long L2_index){

        int index = L2_setsize; // when cannot locate empty, return the set size, designed for debug
        for(int i = 0; i < this -> L2_setsize; i++){
            // check if empty in "L2_index"th set and "i"th way
            if(this -> Cache_L2[L2_index][i] == 0 ){
                index = i;
                break;
            }
        }
        return index;
    }

    void put_L1(long L1_index, int way_index, long tag){
        this->Cache_L1[L1_index][way_index] = tag;

    }
    void put_L2(long L2_index, int way_index, long tag){
        this->Cache_L2[L2_index][way_index] = tag;

    }
    void update_LRU_L1(long L1_index, int way_index){
        // the higher the count, the more recent it's used
        this->LRU_L1[L1_index][way_index] = this->LRU_L1[L1_index][this->L1_setsize+1] + 1;
        // update new MRU count
        this->LRU_L1[L1_index][this->L1_setsize+1] = this->LRU_L1[L1_index][this->L1_setsize+1] + 1;
    }

    void update_LRU_L2(long L2_index, int way_index){
        // the higher the count, the more recent it's used
        this->LRU_L2[L2_index][way_index] = this->LRU_L2[L2_index][this->L2_setsize+1] + 1;
        // update new MRU count
        this->LRU_L2[L2_index][this->L2_setsize+1] = this->LRU_L2[L2_index][this->L2_setsize+1] + 1;
    }
    // return the way index of the evicted LRU
    int get_evict_LRU_L1(long L1_index){

        unsigned long LRU = this->LRU_L1[L1_index][0]; // initialize to the first one
        int LRU_index = 0;

        // then find the index of the smallest
        for(int i = 1; i < this -> L1_setsize; i++){
            // check if smaller in "index"th set and "i"th way
            if(this -> LRU_L1[L1_index][i] < LRU ){
                LRU = this -> LRU_L1[L1_index][i];
                LRU_index = i;
            }
        }
        return LRU_index;

    }
    // return the way index of the evicted LRU
    int get_evict_LRU_L2(long L2_index){

        unsigned long LRU = this->LRU_L2[L2_index][0]; // initialize to the first one
        int LRU_index = 0;

        // then find the index of the smallest
        for(int i = 1; i < this -> L2_setsize; i++){
            // check if smaller in "index"th set and "i"th way
            if(this -> LRU_L2[L2_index][i] < LRU ){
                LRU = this -> LRU_L2[L2_index][i];
                LRU_index = i;
            }
        }
        return LRU_index;
    }


};

Cache::Cache(config config_param) {

    // deal with setsize of fully associativity: number of ways
    if (config_param.L1setsize == 0) {
        this -> L1_setsize = 1024 * config_param.L1size /config_param.L1blocksize; // full associativity
    }else {
        this -> L1_setsize = config_param.L1setsize;
    }
    if (config_param.L2setsize == 0) {
        this -> L2_setsize = 1024 * config_param.L2size /config_param.L2blocksize; // full associativity
    }else {
        this -> L2_setsize = config_param.L2setsize;
    }


    // L1
    this -> L1_blocksize = config_param.L1blocksize;

    this -> L1_offset = static_cast<unsigned long>(log2(config_param.L1blocksize));
    this -> L1_setAmount = static_cast<unsigned long>(1024 * config_param.L1size / (this -> L1_blocksize * this -> L1_setsize));
    this -> L1_indexLength = static_cast<unsigned long>(log2(L1_setAmount));
    this -> L1_tagLength = 32 - L1_offset - L1_indexLength;

    // L2
    this -> L2_blocksize = config_param.L2blocksize;

    this -> L2_offset = static_cast<unsigned long>(log2(config_param.L2blocksize));
    this -> L2_setAmount = static_cast<unsigned long>(1024 * config_param.L2size / (this -> L2_blocksize * this -> L2_setsize));
    this -> L2_indexLength = static_cast<unsigned long>(log2(L2_setAmount));
    this -> L2_tagLength = 32 - L2_offset - L2_indexLength;

    // resize cache and LRU
    resizeCache(this->L1_setsize, this->L2_setsize);
    resizeLRU(this->L1_setsize, this->L2_setsize);

}


int main(int argc, char* argv[]){


    config cacheconfig;
    ifstream cache_params;
    string dummyLine;

    cache_params.open(argv[1]); // cacheconfig.txt
    while(!cache_params.eof())  // read config file
    {
        cache_params>>dummyLine;
        cache_params>>cacheconfig.L1blocksize;
        cache_params>>cacheconfig.L1setsize;
        cache_params>>cacheconfig.L1size;
        cache_params>>dummyLine;
        cache_params>>cacheconfig.L2blocksize;
        cache_params>>cacheconfig.L2setsize;
        cache_params>>cacheconfig.L2size;
    }


    // Implement by you:
    // initialize the hirearch cache system with those configs
    // probably you may define a Cache class for L1 and L2, or any data structure you like
    Cache* myCache = new Cache(cacheconfig);



    int L1AcceState =0; // L1 access state variable, can be one of NA, RH, RM, WH, WM;
    int L2AcceState =0; // L2 access state variable, can be one of NA, RH, RM, WH, WM;


    ifstream traces;
    ofstream tracesout;
    string outname;
    outname = string(argv[2]) + ".out";

    traces.open(argv[2]);
    tracesout.open(outname.c_str());

    string line;
    string accesstype;  // the Read/Write access type from the memory trace;
    string xaddr;       // the address from the memory trace store in hex;
    unsigned int addr;  // the address from the memory trace store in unsigned int;
    bitset<32> accessaddr; // the address from the memory trace store in the bitset;

    int L1_hit, L2_hit;
    int empty_i, evict_i, find_i;

    if (traces.is_open()&&tracesout.is_open()){
        while (getline (traces,line)){   // read mem access file and access Cache

            istringstream iss(line);
            if (!(iss >> accesstype >> xaddr)) {break;} // xaddr: string type (hex) address to deal with
            stringstream saddr(xaddr);
            saddr >> std::hex >> addr;  // address in unsigned int
            accessaddr = bitset<32> (addr); // 32 bit address in bitset

            //
            vector<long> Location = myCache -> getAddrCacheLocation(accessaddr);
            // tag, index, offset of this address in L1
            long L1_tag = Location[0]; // binary
            long L1_index = Location[1];  // decimal

            // tag, index, offset of this address in L2
            long L2_tag = Location[3];
            long L2_index = Location[4];

            L1_hit = (0 != myCache->is_inL1(L1_index, L1_tag));
            L2_hit = (0 != myCache->is_inL2(L2_index, L2_tag));


            // access the L1 and L2 Cache according to the trace; "Read"
            if (accesstype.compare("R")==0)

            {
                //Implement by you:
                // read access to the L1 Cache,
                //  and then L2 (if required),
                //  update the L1 and L2 access state variable;


                if (L1_hit == 0) {
                    // L2 miss
                    if (L2_hit == 0) {
                        L1AcceState = 2;
                        L2AcceState = 2;
                        // get data from main memory and update to L1, L2
                        // if L1 or L2 full, evict some data

                        // for the respective set in L2 , check if full
                        if ((myCache -> is_setFullL2(L2_index)) != true){ // not full
                            // get empty way index
                            empty_i = myCache->getEmptyWayIndex_L2(L2_index);
                            // update L2
                            myCache -> put_L2(L2_index, empty_i, L2_tag);
                            // update LRU
                            myCache -> update_LRU_L2(L2_index, empty_i);


                        }else{ // full: evict the LRU
                            // get the evict way index
                            evict_i = myCache -> get_evict_LRU_L2(L2_index);
                            //put new tag
                            myCache -> put_L2(L2_index, evict_i, L2_tag);
                            // UPDATE LRU
                            myCache -> update_LRU_L2(L2_index, evict_i);

                        }

                        // check if L1 full
                        if (myCache -> is_setFullL1(L1_index) != true){ // not full
                            // get empty way index
                            empty_i = myCache->getEmptyWayIndex_L1(L1_index);
                            // update L1
                            myCache->put_L1(L1_index, empty_i, L1_tag);
                            // update LRU
                            myCache -> update_LRU_L1(L1_index, empty_i);

                        }else{ // full: evict the LRU and
                            // get the evict way index
                            evict_i = myCache -> get_evict_LRU_L1(L1_index);
                            // put new tag
                            myCache -> put_L1(L1_index, evict_i, L1_tag);
                            // UPDATE LRU
                            myCache -> update_LRU_L1(L1_index, evict_i);

                        }

                        // L2  hit
                    } else  {
                        L1AcceState = 2;
                        L2AcceState = 1;

                        // update L2 LRU
                        find_i = myCache ->find_inL2(L2_index, L2_tag);
                        myCache -> update_LRU_L2(L2_index,find_i);


                        // if L1 full, make room in L1 for data returned from L2
                        // if not full, get empty index and put

                        // check if L1 full
                        if (myCache -> is_setFullL1(L1_index) != true){ // not full
                            // get empty way index
                            empty_i = myCache->getEmptyWayIndex_L1(L1_index);
                            // update L1
                            myCache->put_L1(L1_index, empty_i, L1_tag);
                            // update LRU
                            myCache -> update_LRU_L1(L1_index, empty_i);

                        }else{ // full: evict the LRU and
                            // get the evict way index
                            evict_i = myCache -> get_evict_LRU_L1(L1_index);
                            // put new tag
                            myCache -> put_L1(L1_index, evict_i, L1_tag);
                            // UPDATE LRU
                            myCache -> update_LRU_L1(L1_index, evict_i);

                        }
                    }
                }
                else {  //L1 hit
                    L1AcceState = 1;
                    L2AcceState = 0;

                    // update LRU in L1
                    // need to update in L2?

                    // find the way index in L1, update LRU
                    find_i = myCache ->find_inL1(L1_index, L1_tag);
                    myCache -> update_LRU_L1(L1_index,find_i);
                }
            }
            else // "Write"
            {
                //Implement by you:
                // write access to the L1 Cache,
                //and then L2 (if required),
                //update the L1 and L2 access state variable;

                // L1 miss
                if (L1_hit == 0){
                    // L2 miss
                    if (L2_hit == 0) {

                        L1AcceState = 4;
                        L2AcceState = 4;
                        // write miss does not update LRU


                    } else{ // L2  hit
                        L1AcceState = 4;
                        L2AcceState = 3;

                        // find the way index of the tag in L2
                        find_i = myCache ->find_inL2(L2_index, L2_tag);
                        // Write to L2
                        myCache -> put_L2(L2_index, find_i, L2_tag);
                        // UPDATE LRU in L2
                        myCache -> update_LRU_L2(L2_index,find_i);
                    }


                }else{ // L1 hit
                    L1AcceState = 3;
                    L2AcceState = 3;
                    // UPDATE tag and LRU in both L1 and L2

                    // find the way index of the tag in L2
                    find_i = myCache ->find_inL2(L2_index, L2_tag);
                    // Write to L2
                    myCache -> put_L2(L2_index, find_i, L2_tag);
                    // UPDATE LRU in L2
                    myCache -> update_LRU_L2(L2_index,find_i);

                    // find the way index of the tag in L1
                    find_i = myCache ->find_inL1(L1_index, L1_tag);
                    // Write to L1
                    myCache -> put_L1(L1_index, find_i, L1_tag);
                    // UPDATE LRU in L1
                    myCache -> update_LRU_L1(L1_index,find_i);

                }

            }


            tracesout<< L1AcceState << " " << L2AcceState << endl;
            // Output hit/miss results for L1 and L2 to the output file;


        }
        traces.close();
        tracesout.close();
    }
    else cout<< "Unable to open trace or traceout file ";



    return 0;
}

