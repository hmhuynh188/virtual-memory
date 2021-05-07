#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <alloca.h>

#define FRAME_SIZE 256       
#define TOTAL_NUMBER_OF_FRAMES 256  
#define ADDRESS_MASK  0xFFFF 
#define OFFSET_MASK  0xFF 
#define TLB_SIZE 16      
#define PAGE_TABLE_SIZE 256 

int pageTableNumbers[PAGE_TABLE_SIZE]; 
int pageTableFrames[PAGE_TABLE_SIZE];  

int TLBPageNumber[TLB_SIZE];  
int TLBFrameNumber[TLB_SIZE]; 

int physicalMemory[TOTAL_NUMBER_OF_FRAMES][FRAME_SIZE]; 

int pageFaults = 0;  
int TLBHits = 0;     
int firstAvailableFrame = 0; 
int firstAvailablePageTableNumber = 0; 
int numberOfTLBEntries = 0;             

#define BUFFER_SIZE         10

#define CHUNK               256

FILE    *address_file;
FILE    *backing_store;

char    address[BUFFER_SIZE];
int     logical_address;
signed char     buffer[CHUNK];
signed char     value;


void getPage(int address);
void readFromStore(int pageNumber);
void insertIntoTLB(int pageNumber, int frameNumber);

void getPage(int logical_address){
    
    int pageNumber = ((logical_address & ADDRESS_MASK)>>8);
    int offset = (logical_address & OFFSET_MASK);
    
  
    int frameNumber = -1; 
    
    int i; 
    for(i = 0; i < TLB_SIZE; i++){
        if(TLBPageNumber[i] == pageNumber){   
            frameNumber = TLBFrameNumber[i];  
                TLBHits++;                
        }
    }
    
    if(frameNumber == -1){
        int i;  
        for(i = 0; i < firstAvailablePageTableNumber; i++){
            if(pageTableNumbers[i] == pageNumber){         
                frameNumber = pageTableFrames[i];         
            }
        }
        if(frameNumber == -1){                
            readFromStore(pageNumber);            
            pageFaults++;                         
            frameNumber = firstAvailableFrame - 1;  
        }
    }
    
    insertIntoTLB(pageNumber, frameNumber);  
    value = physicalMemory[frameNumber][offset]; 
   printf("frame number: %d\n", frameNumber);
printf("offset: %d\n", offset); 
    printf("Virtual address: %d Physical address: %d Value: %d\n", logical_address, (frameNumber << 8) | offset, value);
}

void insertIntoTLB(int pageNumber, int frameNumber){
    
    int i;  
    for(i = 0; i < numberOfTLBEntries; i++){
        if(TLBPageNumber[i] == pageNumber){
            break;
        }
    }
    
    if(i == numberOfTLBEntries){
        if(numberOfTLBEntries < TLB_SIZE){ 
            TLBPageNumber[numberOfTLBEntries] = pageNumber;    
            TLBFrameNumber[numberOfTLBEntries] = frameNumber;
        }
        else{                                            
            for(i = 0; i < TLB_SIZE - 1; i++){
                TLBPageNumber[i] = TLBPageNumber[i + 1];
                TLBFrameNumber[i] = TLBFrameNumber[i + 1];
            }
            TLBPageNumber[numberOfTLBEntries-1] = pageNumber;  
            TLBFrameNumber[numberOfTLBEntries-1] = frameNumber;
        }        
    }
    
    else{
        for(i = i; i < numberOfTLBEntries - 1; i++){     
            TLBPageNumber[i] = TLBPageNumber[i + 1];     
            TLBFrameNumber[i] = TLBFrameNumber[i + 1];
        }
        if(numberOfTLBEntries < TLB_SIZE){               
            TLBPageNumber[numberOfTLBEntries] = pageNumber;
            TLBFrameNumber[numberOfTLBEntries] = frameNumber;
        }
        else{                                             
            TLBPageNumber[numberOfTLBEntries-1] = pageNumber;
            TLBFrameNumber[numberOfTLBEntries-1] = frameNumber;
        }
    }
    if(numberOfTLBEntries < TLB_SIZE){
        numberOfTLBEntries++;
    }    
}

void readFromStore(int pageNumber){
    if (fseek(backing_store, pageNumber * CHUNK, SEEK_SET) != 0) {
        fprintf(stderr, "Error seeking in backing store\n");
    }
    
    if (fread(buffer, sizeof(signed char), CHUNK, backing_store) == 0) {
        fprintf(stderr, "Error reading from backing store\n");        
    }
    
    int i;
    for(i = 0; i < CHUNK; i++){
        physicalMemory[firstAvailableFrame][i] = buffer[i];
    }
    
    pageTableNumbers[firstAvailablePageTableNumber] = pageNumber;
    pageTableFrames[firstAvailablePageTableNumber] = firstAvailableFrame;
    
    firstAvailableFrame++;
    firstAvailablePageTableNumber++;
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr,"Usage: ./a.out [input file]\n");
        return -1;
    }
    
    backing_store = fopen("BACKING_STORE.bin", "rb");
    
    if (backing_store == NULL) {
        fprintf(stderr, "Error opening BACKING_STORE.bin %s\n","BACKING_STORE.bin");
        return -1;
    }
    
    address_file = fopen(argv[1], "r");
    
    if (address_file == NULL) {
        fprintf(stderr, "Error opening addresses.txt %s\n",argv[1]);
        return -1;
    }
    
    int numberOfTranslatedAddresses = 0;
    while ( fgets(address, BUFFER_SIZE, address_file) != NULL) {
        logical_address = atoi(address);
        getPage(logical_address);
        numberOfTranslatedAddresses++;     
    }
    
    printf("Number of translated addresses = %d\n", numberOfTranslatedAddresses);
    double pfRate = pageFaults / (double)numberOfTranslatedAddresses;
    double TLBRate = TLBHits / (double)numberOfTranslatedAddresses;
    
    printf("Page Faults = %d\n", pageFaults);
    printf("Page Fault Rate = %.3f\n",pfRate);
    printf("TLB Hits = %d\n", TLBHits);
    printf("TLB Hit Rate = %.3f\n", TLBRate);
    
    fclose(address_file);
    fclose(backing_store);
    
    return 0; 
}
