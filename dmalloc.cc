#define M61_DISABLE 1
#include "dmalloc.hh"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cinttypes>
#include <cassert>
#include <iostream> 
#include <unordered_map>
#include <string>
#include <vector>
#include <algorithm>
#include <iterator>
#include <set>
#include <iomanip>
using namespace std;

// You may write code here.
// (Helper functions, types, structs, macros, globals, etc.)
 unsigned long long nactive = 0;         // # active allocations
 unsigned long long active_size = 0;     // # bytes in active allocations
 unsigned long long ntotal = 0;          // # total allocations
 unsigned long long total_size = 0;      // # bytes in total allocations
 unsigned long long nfail = 0;           // # failed allocation attempts
 unsigned long long fail_size = 0;       // # bytes in failed alloc attempts
 uintptr_t heap_min = UINTPTR_MAX;                 // smallest allocated addr
 uintptr_t heap_max = 0; 

 typedef struct header
 {  
    void * ptr_alloc;
    unsigned long long size_allocated;
    int freed = 0;
    int canary = 1999;
    int counter = 0;
    header * next = NULL;
    header * prev = NULL;
    const char * file;
    long line;
 }header;

 typedef struct footer
 {  
     char canary[300];
     
 }footer;


bool search_node(header * head_lst, uintptr_t ptr, uintptr_t * offset)
{
    if (head_lst == NULL)
    {
        return false;
    }
    header * current = head_lst;
    while (current != NULL) 
    {   
        if (ptr == (uintptr_t) current->ptr_alloc) //found
        {
            return true;
        }
        else if (ptr > (uintptr_t) current->ptr_alloc && ptr < ((uintptr_t) current->ptr_alloc) + current->size_allocated)
        {
            *offset = ptr - (uintptr_t) current->ptr_alloc;
            return false;
        }
        current = current->next;
    }
    *offset = 0;
    return false; //not found
}

header* head_lst = (header*)malloc(sizeof(header));
unordered_map<string, unsigned long long> umap;

/// dmalloc_malloc(sz, file, line)
///    Return a pointer to `sz` bytes of newly-allocated dynamic memory.
///    The memory is not initialized. If `sz == 0`, then dmalloc_malloc must
///    return a unique, newly-allocated pointer value. The allocation
///    request was at location `file`:`line`.
int counter = 0;
size_t size_so_far = 0;

void* dmalloc_malloc(size_t sz, const char* file, long line) {
    (void) file, (void) line;   // avoid uninitialized variable warnings
    // Your code here.
    if ((sz + sizeof(header)) < sz)
    {
        //overflow
        nfail++;
        fail_size += sz;
        return NULL;
    }
    if (sz + sizeof(header) + sizeof(footer) < sz) 
    {
        nfail++;
        fail_size += sz;
        return NULL;
    }
    
    header * block = (header *)base_malloc(sz + sizeof(header) + sizeof(footer));
    if (block->freed == 1) block->freed = 0; //if a block is remalloc'ed
    
    if (block != NULL)
    {
        nactive++;
        active_size += sz;
        ntotal++;
        total_size += sz;
        block->size_allocated = sz;
        block->canary = 1999;
        
        block->file = file;
        block->counter = counter;
        counter++;
        header * temp = head_lst;
        block->next = temp;
        temp->prev = block;
        head_lst = block;
        block->prev = NULL;
        
        uintptr_t head = (uintptr_t)(void *) block;
        block->ptr_alloc = (void *) (head + sizeof(header));
        if (head + sizeof(header) < heap_min) heap_min = head + sizeof(header);
        if ((head + sizeof(header) + sz) > heap_max) heap_max = head + sizeof(header) + sz;
        head += sizeof(header) + sz;
        footer * block1 = (footer *) head;
        for (int i = 0; i < 300; i++)
        {
            block1->canary[i] = 'c' + i*1991;
        }
        uintptr_t go_back = head;
        long * blackstone = &line;
        block->line = *blackstone;
        
        go_back -= sz;
        void * ret = (void *)go_back; // go_back

        //adding to heavy hitters
        //set a random number using rand()
        std::string key = file;
        key += ":";
        key += std::to_string(line);
        key += ":";
        int done = 0;
         if ((sz/total_size) > 0.2)
        {
            if (umap.find(key) == umap.end()) 
            {
                umap.insert(make_pair(key, sz));
                size_so_far += sz;
                done++;
            }
            
            // If key found then iterator to that key is returned 
            else 
            {
                umap[key] += sz;
                size_so_far += sz;
                done++;
            }

        } 

         if (umap.find(key) == umap.end()) 
            {
                //umap[key] = sz;
            }
            
            // If key found then iterator to that key is returned 
        else if (((umap[key] + sz)/total_size) > 0.2 && done == 0)
        {   
            size_so_far += sz;
            umap[key] += sz;
            done++;
            
        }
        int rando = rand();
        if ((rando % 100) < 95 && done == 0)
        {
            if (umap.find(key) == umap.end()) 
            {
                umap.insert(make_pair(key, sz));
                size_so_far += sz;
            }
            
      
        // If key found then iterator to that key is returned 
            else
            {
                umap[key] += sz;
                size_so_far += sz;
            }
        }
        
        return ret;
        
    }
    else
    {
        nfail++;
        fail_size += sz;
        return NULL;

    }

   

}


/// dmalloc_free(ptr, file, line)
///    Free the memory space pointed to by `ptr`, which must have been
///    returned by a previous call to dmalloc_malloc. If `ptr == NULL`,
///    does nothing. The free was called at location `file`:`line`.

void dmalloc_free(void* ptr, const char* file, long line) {
    (void) file, (void) line;   // avoid uninitialized variable warnings
    // Your code here.
    
    uintptr_t offset = 1;
    uintptr_t * smth = &offset;
    if (ptr == NULL)
    {
        //do nothing
    }
    else if((uintptr_t) ptr < heap_min || (uintptr_t) ptr > heap_max)
    {
        fprintf(stderr, "MEMORY BUG %s:%ld: invalid free of pointer %lu, not in heap\n", file, line, (uintptr_t) ptr);
        exit(1);
    }

    else if (!(search_node(head_lst, (uintptr_t) ptr, smth)))
    {
        uintptr_t back = (uintptr_t) ptr;
        back -= (*smth + sizeof(header));
        header * head = (header *) back;
        
        if (head->freed == 1)
        {
            fprintf(stderr, "MEMORY BUG %s:%ld: invalid free of pointer %p, double free\n", file, line, ptr); //change output later
            exit(1);
        }
        else
        {
            fprintf(stderr, "MEMORY BUG: %s:%ld: invalid free of pointer %p, not allocated\n  %s:%ld: %p is %ld bytes inside a %llu byte region allocated here\n", file, line, ptr, file, head->line , ptr, *smth, head->size_allocated);
            exit(1);
        }
        
        
    }
   
    else
    {   
        
        nactive--;
        uintptr_t go_back = (uintptr_t) ptr;
        go_back -= sizeof(header);
        header * head = (header *) go_back;
        
        footer * check = (footer *) ((uintptr_t)(ptr) + head->size_allocated);
        for (int i = 0; i < 300; i++)
        {
            if (check->canary[i] != (char)('c' + i*1991))
            {
                fprintf(stderr, "MEMORY BUG: %s:%ld: detected wild write during free of pointer %p\n", file, line, ptr);
                exit(1);
                //i += 100;
            }
            
        }

       if (head->freed == 1)
        {

            fprintf(stderr, "MEMORY BUG %s:%ld: invalid free of pointer %p, double free\n", file, line, ptr); //change output later
            exit(1);
        }
        //header * block = (header *)((uintptr_t) ptr - sizeof(header));
        
        if (head->canary != 1999) 
        {
            fprintf(stderr, "MEMORY BUG: %s:%ld: detected wild write during free of pointer %p\n", file, line, ptr);
            exit(1);
        }
        
        

        
        active_size -= head->size_allocated;
        head->freed = 1;
        
        
        if (head->prev == NULL) 
        {       
            if (head->next == NULL)
            {
                head_lst = NULL;
            }
            else 
            {
                head->next->prev = NULL; 
                head_lst = head->next;
            }
            
        }
        else if (head->next == NULL)
        {
            head->prev->next = NULL;
            //head->next->prev = head->prev;
        }
        else
        {
            head->prev->next = head->next;
            head->next->prev = head->prev;
        }

        base_free(head);
    }
    


    
}


/// dmalloc_calloc(nmemb, sz, file, line)
///    Return a pointer to newly-allocated dynamic memory big enough to
///    hold an array of `nmemb` elements of `sz` bytes each. If `sz == 0`,
///    then must return a unique, newly-allocated pointer value. Returned
///    memory should be initialized to zero. The allocation request was at
///    location `file`:`line`.

void* dmalloc_calloc(size_t nmemb, size_t sz, const char* file, long line) {
    // Your code here (to fix test014).
    //overflow
    //size_t check = 0;
    unsigned long long check1 = (unsigned long long) nmemb;
    unsigned long long check2 = (unsigned long long) sz;

    if ((check1*check2)/sz != check1)
    {
        nfail++;
        fail_size += sz;
        return NULL;
    }
    if ((nmemb * sz) < nmemb || (nmemb * sz) < sz)
    {
        nfail++;
        fail_size += sz;
        return NULL;
    }
    
    void* ptr = dmalloc_malloc(nmemb * sz, file, line);
    if (ptr) {
        memset(ptr, 0, nmemb * sz);
    }
    //nactive++;
    return ptr;
    
    
}


/// dmalloc_get_statistics(stats)
///    Store the current memory statistics in `*stats`.

void dmalloc_get_statistics(dmalloc_statistics* stats) {
    // Stub: set all statistics to enormous numbers
    //memset(stats, 255, sizeof(dmalloc_statistics));
    // Your code here.
    stats->nactive = nactive;    
    stats->active_size = active_size;
    stats->ntotal = ntotal;    
    stats->total_size = total_size; 
    stats->nfail = nfail;      
    stats->fail_size = fail_size;  
    stats->heap_min = heap_min;            
    stats->heap_max = heap_max;
}


/// dmalloc_print_statistics()
///    Print the current memory statistics.

void dmalloc_print_statistics() {
    dmalloc_statistics stats;
    dmalloc_get_statistics(&stats);

    printf("alloc count: active %10llu   total %10llu   fail %10llu\n",
           stats.nactive, stats.ntotal, stats.nfail);
    printf("alloc size:  active %10llu   total %10llu   fail %10llu\n",
           stats.active_size, stats.total_size, stats.fail_size);
}


/// dmalloc_print_leak_report()
///    Print a report of all currently-active allocated blocks of dynamic
///    memory.

void dmalloc_print_leak_report() {
    // Your code here.
    header * current = head_lst;
    while (current != NULL)
    {   
        if(current->freed != 1 && current->ptr_alloc != NULL)
        {
            printf("LEAK CHECK: %s:%ld: allocated object %p with size %llu\n", current->file, current->line, current->ptr_alloc, current->size_allocated);
        }   

        current = current->next;
    }
}


/// dmalloc_print_heavy_hitter_report()
///    Print a report of heavily-used allocation locations.

void dmalloc_print_heavy_hitter_report() 
{
    // Your heavy-hitters code here
    //use unordered map
    float floatValue = 0;
    // Declaring the type of Predicate that accepts 2 pairs and return a bool
    typedef std::function<bool(std::pair<std::string, unsigned long long>, std::pair<std::string, unsigned long long>)> Comparator;
 
    // Defining a lambda function to compare two pairs. It will compare two pairs using second field
    Comparator compFunctor =
            [](std::pair<std::string, unsigned long long> elem1 ,std::pair<std::string, unsigned long long> elem2)
            {
                return elem1.second > elem2.second;
            };
 
    // Declaring a set that will store the pairs using above comparision logic
    std::set<std::pair<std::string, unsigned long long>, Comparator> s_map(
            umap.begin(), umap.end(), compFunctor);
    int counting = 0;
    // Iterate over a set using range base for loop
    // It will display the items in sorted order of values
    for (std::pair<std::string, unsigned long long> element : s_map)
    {
        if (counting == 5) break;
        counting++;
        floatValue = ((double)(element.second)/(double)size_so_far) * 100.0;
        floatValue = int(floatValue * 10 + 0.5);
        if (floatValue < 130) continue;
        std::cout << "HEAVY HITTER: " << element.first << "  "  << element.second << " bytes (~" << (float)(floatValue/10) << "%)" <<endl;   
    }
            
     
}
