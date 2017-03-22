#include "cachelab.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#define MAXFILELEN 100
#define MAXCMDLEN 20
struct Line{
    int valid;
    int tag;
    int lru;  //we use lru algo
};

struct Set{
    struct Line *lines;
};

struct Cache{
    int setNum;
    int lineNum;
    struct Set *sets;
};

int misses=0;
int hits=0;    
int evictions=0;


int Getopt(int argc,char** argv,int*s,int*E,int*b,char* trace,int*verbose);   // read cmdline
void initCache(int s,int E,int b,struct Cache *cache);    //malloc and set ptr
void updateCache(struct Cache* cache,int setNum,int tagNum,int verbose);//check miss or hit and set validbit, setbit
void updateLru(struct Cache* cache,int setNum,int index);   // index is the index of updated block
int minLru(struct Cache* cache,int setNo);  //the minLru block is the least recent used block and we can evict it




int main(int argc,char**argv)
{
    int s,E,b,verbose=0;
    char trace[MAXFILELEN];
    char opt[MAXCMDLEN];
    int addr;
    int size;
    struct Cache cache;
    if(Getopt(argc,argv,&s,&E,&b,trace,&verbose)==-1){
        printf("can't' get opt.\n");
        exit(0);
    }
    //printf("opt got.\n");
    //printf("trace:%s\n",trace);
    initCache(s,E,b,&cache);
    FILE *file=fopen(trace,"r");
    if(file==NULL)
        printf("failed to open file.\n");
    while((fscanf(file,"%s %x,%d",opt,&addr,&size))!=EOF){
        //printf("\nread a line.\n");
        if(strcmp(opt,"I")==0)    //if 'I' do nothing
            continue;
        if(verbose)
            printf("%s %x,%d ",opt,addr,size);
        int setNo=(addr>>b)&((1<<s)-1); 
        int tagNo=(addr>>(s+b));
        if(strcmp(opt,"S")==0){    
            updateCache(&cache,setNo,tagNo,verbose);
            if(verbose==1)
                printf("\n");
            continue;
        }
        if(strcmp(opt,"L")==0){   // "L" equals to "S" in this simulator
            updateCache(&cache,setNo,tagNo,verbose);
            if(verbose==1)
                printf("\n");
            continue;
        }
        if(strcmp(opt,"M")==0){   //"M" has the same consequence of two "S" or "L"
            updateCache(&cache,setNo,tagNo,verbose);                        
            updateCache(&cache,setNo,tagNo,verbose);
            if(verbose==1)
                printf("\n");
            continue;
        }
        

        
    }
    //printf("cache end.\n");
    printSummary(hits, misses, evictions);
    return 0;
}
int Getopt(int argc,char**argv,int*s,int*E,int*b,char*trace,int*verbose){
    int ch;
    while((ch=getopt(argc,argv,"hvs:E:b:t:"))!=-1){
        switch(ch)
        {
            case 'h':
            exit(0);
            case 'v':
            *verbose=1;
            break;
            case 's':
            *s=atoi(optarg);
            break;
            case 'E':
            *E=atoi(optarg);
            break;
            case 'b':
            *b=atoi(optarg);
            break;
            case 't':
            strcpy(trace,optarg);
            break;
        }
    }return 1;
}
void initCache(int s,int E,int b,struct Cache* cache){
    cache->setNum=1<<s;
    cache->lineNum=E;
    cache->sets=(struct Set*)malloc((1<<s)*sizeof(struct Set));
    int i,j;
    for(i=0;i<(1<<s);i++){
        cache->sets[i].lines=(struct Line*)malloc(E*sizeof(struct Line));
        for(j=0;j<E;j++){
            cache->sets[i].lines[j].valid=0;
            cache->sets[i].lines[j].lru=0;
        }
    }
}

void updateLru(struct Cache* cache,int setNo,int index){
    int k;
    cache->sets[setNo].lines[index].lru=9999;   //recent used block's lru number is the biggest
    for(k=0;k<cache->lineNum;k++){
            if(k!=index)
                cache->sets[setNo].lines[k].lru--; //update other line's lru
    }
}

int minLru(struct Cache* cache,int setNo){
    int i;
    int minIdx=0;
    int min=9999;
    for(i=0;i<cache->lineNum;i++){
        if(cache->sets[setNo].lines[i].lru<min){
            minIdx=i;
            min=cache->sets[setNo].lines[i].lru;
        }
    }
    return minIdx;
}

void updateCache(struct Cache* cache,int setNo,int tagNo, int verbose){
    int i;


    for(i=0;i<cache->lineNum;i++){
        if((cache->sets[setNo].lines[i].valid==1)
        &&(cache->sets[setNo].lines[i].tag==tagNo)){ //cache hits
             hits++;
            updateLru(cache,setNo,i);
            if(verbose==1)
                printf("hit ");
            return;
        } 
    }


    misses++; //cache misses
    if(verbose==1)
        printf("miss ");


    for(i=0;i<cache->lineNum;i++){
        if(cache->sets[setNo].lines[i].valid==0){ //find an empty block
            cache->sets[setNo].lines[i].valid=1;
            cache->sets[setNo].lines[i].tag=tagNo;
            updateLru(cache,setNo,i);
            return;
        }
    }
    evictions++; //cache evictions
    int evictionIdx=minLru(cache,setNo);
    cache->sets[setNo].lines[evictionIdx].valid=1;
    cache->sets[setNo].lines[evictionIdx].tag=tagNo;                
    updateLru(cache,setNo,evictionIdx);
    if(verbose==1)
        printf("eviction ");




    



}