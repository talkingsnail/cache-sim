#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

const int word_size = 16;
const int mem_size = 65536;
const int block_size_max = 64;

struct block{
    bool valid;
    bool dirty;
    int tag_index;
    unsigned char c_bytes [64];
    int LRE;
};

void print_hexadecimal(unsigned char* str, int size){
    for(int i=0; i<size; i++){
        printf("%02x", str[i]);
    }
    printf("\n");
}

void hex_to_char(char* hex, int access, char* new_chars){
    
    int len = access*2;
 //   int ac_size = len/2;
    for(int i=0, j=0; j<access; i+=2, j++){
        *(new_chars+j) = (hex[i]%32 + 9)%25 * 16 + (hex[i+1]%32 +9)%25;
        //printf("%c\n", *(new_chars +j));
        }
    //printf("old chars are %s\n", hex);
    //printf("new chars are %s \n", new_chars);
    //return new_chars;
}

int Log2(int a){
    int k=0;
    while(a/2!=1){
        k++;
        a = a/2;
    }
    return k;
}

void load(int address, int access, int ways, int sets, struct block** cache, char* mem,  int block_size){

// hit scenario
    bool hit =0 ;
    int address_block = address / block_size;
    int check = address % block_size;
    unsigned char print_val [access];

    for (int i=0; i<ways; i++){
        for(int j=0; j<sets; j++){
            if(cache[i][j].tag_index== address_block && cache[i][j].valid==1){
                // obtain the correct block that needs to be ouputted
                for(int k = check; k<check+access; k++)
                    print_val[k-check] = cache[i][j].c_bytes[k];
//                printf(cache[i][j].c_bytes);
//                printf("\n %s\n", print_val);
                printf("load %.4x hit ", address);
                hit=1;
                print_hexadecimal(print_val, access);
                // update LRE chunk for the column
                if(cache[i][j].LRE !=0){
                    int prev_LRE = cache[i][j].LRE;
                    cache[i][j].LRE = 0;
                    // a column is a set, thus update the column values
                    for (int k = 0; k<ways; k++){
                        if(k!=i && cache[k][j].LRE<prev_LRE)
                            cache[k][j].LRE++;
                    }
                }
        }
    }
}

    //miss scenario
// ADD A DIRTY BIT WAY OF DOING THINGS - PUT THE CURRENT THING TO MEMORY IF DIRTY BIT IS 1

    if(hit==0){

        //print part
        int page = address_block *block_size;

        for(int k = check; k<check+access; k++){
            print_val[k-check] = mem[page + k];
            }
//        printf(cache[i][j].c_bytes);
//        printf("\n %s\n", print_val);
        printf("load %.4x miss ", address);
        print_hexadecimal(print_val, access);

        // push to the cache from memory part

        int cur_set = address_block % sets;
       // printf("current set is %d when address page is %d and number of sets is %d \n",cur_set, page, sets );
        for(int i=0; i<ways; i++){
            //update LRE, set valid bit to 1, set
            if(cache[i][cur_set].LRE == ways-1){
                cache[i][cur_set].valid = 1;
                cache[i][cur_set].LRE = 0;

                // check for dirty bit and push to memory current contents
                if(cache[i][cur_set].dirty==1){
                    for(int j = cache[i][cur_set].tag_index*block_size; j<block_size; j++)
                        mem[j] = cache[i][cur_set].c_bytes[j-cache[i][cur_set].tag_index*block_size];
                    cache[i][cur_set].dirty = 0;
                }

                cache[i][cur_set].tag_index = address_block;

                for(int j=page; j<page + block_size; j++ ){
                    cache[i][cur_set].c_bytes[j - page] = mem[j];
                }
                //printf("new page vals is ");
                //print_hexadecimal(cache[i][cur_set].c_bytes, block_size);
                //printf("\n");

            // update the LRE values of the rest of the numebrs in the set
                for (int k = 0; k<ways; k++){
                    if(k!=i && cache[k][cur_set].LRE<ways-1)
                        cache[k][cur_set].LRE++;
                }
                break;
            }
        }
    }

}

void store(int address, int access, int ways, int sets, struct block** cache, char* mem,  int block_size, bool write_back, unsigned char* info){
    // write through and non-allocate
    //update the memory when updating the cache + if write miss write directly to memory, without bringing it up to the cache
    bool hit = 0;
    int address_block = address / block_size;
    int check = address % block_size;

    if(write_back == 0){
        //hit
       // printf("ne wlize :(\n");
        for (int i=0; i<ways; i++){
            for(int j=0; j<sets; j++){
                if(cache[i][j].tag_index == address_block && cache[i][j].valid==1){
                    //printf("error2\n");
                    // obtain the correct block that needs to be changed
                    // put the new information both in memory and in the cache
                    for(int k = check; k<check+access; k++){
                        cache[i][j].c_bytes[k] = info[k-check];
                       // printf("%d, i is %d, j is %d \n",cache[i][j].c_bytes[k], i, j);
                        mem[address + k -check] = info[k-check];
                        //printf("%d ", address + k - check);
                    }
                    printf("store %.4x hit\n", address);
                    hit=1;
                    // update LRE chunk for the column
                    if(cache[i][j].LRE !=0){
                        int prev_LRE = cache[i][j].LRE;
                        cache[i][j].LRE = 0;
                        // a column is a set, thus update the column values
                        for (int k = 0; k<ways; k++){
                            if(k!=i && cache[k][j].LRE<prev_LRE)
                                cache[k][j].LRE++;
                        }
                    }
                }
            }
        }

        if(hit==0){
            printf("store %.4x miss\n", address);

            for(int i = address; i<address + access; i++){
                mem[i] = info[i-address];
                //printf("char in memory is %c, char in info is %c\n", mem[i], info[i-address]);
                }
            //printf("store miss new page val is ");
            //    print_hexadecimal(mem+page, block_size);
            //    printf("\n");

        }
    }

    if(write_back==1){
        //printf("wliza w write_back\n");
    //update lower level of memory upon removing the current value
    // update the cache only on a miss

    //search for a hit
    for (int i=0; i<ways; i++){
            for(int j=0; j<sets; j++){
                if(cache[i][j].tag_index == address_block && cache[i][j].valid==1){
                    //printf("error2\n");

                    // obtain the correct block that needs to be changed
                    // if it is a dirty block, push the current value to memory

                    if(cache[i][j].dirty == 1){
                        for(int k = cache[i][j].tag_index*block_size; k<block_size; k++)
                            mem[k] = cache[i][j].c_bytes[k-cache[i][j].tag_index*block_size];

                    }

                    // put the new value in the cache, dirty remains 1
                    for(int k = check; k<check+access; k++){
                        cache[i][j].c_bytes[k] = info[k-check];
                        }

                    printf("store %.4x hit\n", address);
                    hit=1;
                    // update LRE chunk for the column
                    if(cache[i][j].LRE !=0){
                        int prev_LRE = cache[i][j].LRE;
                        cache[i][j].LRE = 0;
                        // a column is a set, thus update the column values
                        for (int k = 0; k<ways; k++){
                            if(k!=i && cache[k][j].LRE<prev_LRE)
                                cache[k][j].LRE++;
                        }
                    }
                }
            }
        }

        if(hit==0){
            printf("store %.4x miss\n", address);
             int page = address_block *block_size;
            int cur_set = address_block % sets;

            for(int i=0; i<ways; i++){
                //update LRE, set valid bit to 1, set
                if(cache[i][cur_set].LRE == ways-1){
                    cache[i][cur_set].valid = 1;
                    cache[i][cur_set].LRE = 0;

                    // check for dirty bit and push to memory current contents
                    if(cache[i][cur_set].dirty==1){
                        for(int j = cache[i][cur_set].tag_index*block_size; j<block_size; j++)
                            mem[j] = cache[i][cur_set].c_bytes[j-cache[i][cur_set].tag_index*block_size];
                        cache[i][cur_set].dirty = 0;
                    }

                    cache[i][cur_set].tag_index = address_block;
                    

                    for(int j=check; j<check + access; j++ ){
                        cache[i][cur_set].c_bytes[j] = info[j-check];
                    }

                    // update the LRE values of the rest of the numebrs in the set
                    for (int k = 0; k<ways; k++){
                        if(k!=i && cache[k][cur_set].LRE<ways-1)
                            cache[k][cur_set].LRE++;
                    }
                    
                    //printf("the updated cache val %s \n", cache[i][cur_set].c_bytes);
                    break;
                }
            }
        }

    }
}


int main(int argc, char* argv[]) {

    //for(int i=1; i<argc; i++)
    //    printf("%s\n", argv[i]);


    char* file_name;
    char* p;
    
    long size_KB, ways, block_size;
    bool write_back= 0;
    bool write_through = 1;

    file_name = argv[1];

    size_KB = strtol(argv[2], &p, 10);
    ways = strtol(argv[3], &p, 10);
    
    if(argv[4][1]=='b'){
        write_back = 1;
        write_through = 0;
    }
    else {
        write_through = 1;
        write_back = 0;
    }
    block_size = strtol(argv[5], &p, 10);

    int frames, sets, offset, index, tag;

    frames = size_KB*1024/block_size;
    sets = frames/ways;
    offset = Log2(block_size);
    index = Log2(sets);
    tag = word_size - offset-index;

   // printf(" size_KB = %ld\n", size_KB);
   // printf("%ld\n ways", ways);
    //printf("%ld\n block_size", block_size);
    //printf("%d\n fra,mes", frames);
    //    printf("%d\n sets", sets);
   // printf("%d\n offset", offset);
   // printf("%d\n index", index); 
    //    printf("%d\n tag", tag);   

 // initialize the memory and the cache
 struct block * cache [ways];
    for(int i=0; i<ways; i++)
        cache[i] = (struct block *)malloc(sets*sizeof(struct block));

    char memory[mem_size];

    for (int i=0; i<ways; i++)
        for(int j=0; j<sets; j++){
            cache[i][j].dirty = 0;
            cache[i][j].valid = 0;
            cache[i][j].LRE = ways-1-i;
        }

    for(int i=0; i<mem_size; i++)
        memory[i] = 0;

// start IO
    FILE *fptr;
    fptr = fopen(file_name, "r");
    if(fptr == NULL)
        return 0;

    int line_length = 1000;
    char line[line_length];
    while(fgets(line, line_length, fptr)!=NULL){
        //printf("%s\n", line);
        int address, access;
        if(line[0]=='l'){
            char* addr = line + 5;
            *(addr + 4) = '\0';
            char* acc = line + 10;
            //printf("%s access\n", acc);
            
            address = (int)strtol(addr, NULL, 16);
            access = (int)strtol(acc, NULL, 16);
            //printf("%d address\n", address);
            //printf("%d access\n", access);

            load(address, access, ways, sets, cache, memory, block_size);
        }

        else{
            char* addr = line + 6;
            *(addr + 4) = '\0';

            char* acc;
            char* splitter = line+10;
            int i=0;

            while(true){
                splitter++;
                if(*(splitter)==' '){
                    splitter++;
                    break;
                    }
                i++;
            }
            //printf("the size of access is %d\n", i);
            acc = (char*)malloc(i*sizeof(char));
            i = 11;
            while(true){
                *(acc+i-11) = *(line+i);
                i++;
                //acc++;
                if(*(line+i)==' '){
                    //splitter++;
                    break;
                    }
            }


            address = (int)strtol(addr, NULL, 16);
            access = (int)strtol(acc, NULL, 16);
            //printf("access %d \n",access);
            
            char* new_chars = (char*)malloc((access +1)*sizeof(*new_chars));
            hex_to_char(splitter, access, new_chars);

            store(address, access, ways, sets, cache, memory, block_size, write_back, new_chars);
            free(new_chars);
        }
            
    }

    for(int i=0; i<ways; i++){
        free(cache[i]);

    }
    //free(cache);*/
    return 0;
}
