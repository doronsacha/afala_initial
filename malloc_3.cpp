#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <cstring>


#define CURRENT_HEAP sbrk((intptr_t)0)

typedef struct MallocMetadata
{
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
}MallocMetadata;


size_t _num_free_blocks();
size_t _num_free_bytes();
size_t _num_allocated_blocks();
size_t _num_allocated_bytes();
size_t _size_meta_data();
size_t _num_meta_data_bytes();


void remove_block_from_list(MallocMetadata *block);
void insert_node_to_list(MallocMetadata *new_block, size_t size);
size_t get_size_of_last_chunk();
bool the_last_chunk_is_free();
MallocMetadata *get_the_topmost();
MallocMetadata* get_the_right_metadata(MallocMetadata* block);
MallocMetadata* get_the_next_block(MallocMetadata *block);
MallocMetadata *join_next_block(MallocMetadata* block);
void sfree(void* p);
void* smalloc(size_t size);

///***********************************************************************************************************************************///
///****************************************    Global   *************************************************************************///
///*********************************************************************************************************************************///

MallocMetadata *head = nullptr;
MallocMetadata *tail = nullptr;

size_t mmap_blocks = 0;
size_t mmap_bytes = 0;

void *heap_start = nullptr;

///***********************************************************************************************************************************///
///****************************************    segel helpers functions     **********************************************************///
///*********************************************************************************************************************************///

size_t _num_free_blocks()
{
    if (head == nullptr)
    {
        return 0;
    }
    MallocMetadata *block = head;
    int counter = 0;
    while (block != nullptr)
    {
        if (block->is_free)
        {
            counter++;
        }
        block = block->next;
    }
    return counter;
}

size_t _num_free_bytes()
{
    if (head == nullptr)
    {
        return 0;
    }
    MallocMetadata *block = head;
    int counter = 0;
    while (block != nullptr)
    {
        if (block->is_free)
        {
            counter += block->size;
        }
        block = block->next;
    }
    return counter;
}

size_t _num_allocated_blocks()
{
    if (head == nullptr)
    {
        return 0;
    }
    MallocMetadata *block = head;
    int counter = 0;
    while (block != nullptr)
    {
        counter++;
        block = block->next;
    }
    return counter;
}

size_t _num_allocated_bytes()
{
    if (head == nullptr)
    {
        return 0;
    }
    MallocMetadata *block = head;
    int counter = 0;
    while (block != nullptr)
    {
        counter += block->size;
        block = block->next;
    }
    return counter;
}

size_t _size_meta_data()
{
    return 2*sizeof(MallocMetadata);
}

size_t _num_meta_data_bytes()
{
    return _num_allocated_blocks() * _size_meta_data();
}

///***********************************************************************************************************************************///
///****************************************    mmap functions     ****************************************************************///
///*********************************************************************************************************************************///

/*
void* mmap_malloc(size_t size){
    void * ptr_to_mem = mmap(NULL, size + _size_meta_data(), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (ptr_to_mem == (void*)(-1)){
        return nullptr;
    }
    MallocMetadata* mmap_meta_data = (MallocMetadata*)ptr_to_mem;
    mmap_meta_data->size = size; //only size matters here, it isn't connected to other mmap memory areas nor can it ever be free

    mmap_bytes += size;
    mmap_blocks++;

    return mmap_meta_data + _size_meta_data();
}

void mmap_free(void* p){
    MallocMetadata* meta = (MallocMetadata*) p;
    meta-=_size_meta_data();
    mmap_blocks--;
    mmap_bytes -= meta->size;
    munmap((void *)p, meta->size + _size_meta_data());
}

size_t min(size_t a, size_t b){
    if (a<b){
        return a;
    }
    return b;
}

void* mmap_realloc(void* p, size_t size){
    MallocMetadata* old_meta = (MallocMetadata*) p;
    old_meta-=_size_meta_data();
    if (size == old_meta->size){
        return p;
    }
    MallocMetadata* new_allocation = (MallocMetadata*)mmap_malloc(size);
    size_t minimum = min(size,old_meta->size);
    memmove(new_allocation,p,minimum);
    mmap_free(p);
    return new_allocation;
}
*/
///***********************************************************************************************************************************///
///****************************************    helpers functions     ****************************************************************///
///*********************************************************************************************************************************///



bool the_block_is_free_and_enough(MallocMetadata *block, size_t size)
{
    if (block->size >= size && block->is_free)
    {
        return true;
    }
    return false;
}

//get size
//retur a ptr
void *allocate_new_ptr(size_t size)
{
    MallocMetadata *new_ptr = (MallocMetadata *) sbrk((intptr_t) (size + (_size_meta_data())));
    new_ptr++;
    return (void *) (new_ptr);
}

//get a metadata
void initialize_metadata(MallocMetadata *block, size_t size, bool is_free)
{
    block->size = size;
    block->is_free = is_free;
    block->next = nullptr;
    block->prev = nullptr;
}

//get a pointer
//doesn't change the initial pointer
//return a pointer to the memory space after k bits
void *advance_k_bit(void *p, size_t k)
{
    char *temp = (char *) p;
    temp += k;
    return (void *) temp;
}

//get a pointer
//return a metadata
MallocMetadata *get_the_metadata_of_ptr(void *ptr)
{
    MallocMetadata *temp = (MallocMetadata *) ptr;
    temp--;
    return temp;
}

//get a pointer
//return a metadata
MallocMetadata *get_the_la_metadata_of_ptr(void *ptr, size_t size)
{
    void *temp = (MallocMetadata *) ptr;
    temp = advance_k_bit(temp, size);
    return (MallocMetadata *) temp;
}

//get a pointer
void initialize_ptr(void *ptr, size_t size, bool is_free)
{
    MallocMetadata *new_block = get_the_metadata_of_ptr(ptr);
    initialize_metadata(new_block, size, is_free);
    MallocMetadata *right_metadata_of_new_block = get_the_right_metadata(new_block);
    initialize_metadata(right_metadata_of_new_block, size, is_free);
}

//get a metadata
void initialize_block(MallocMetadata *block, size_t size, bool is_free)
{
    initialize_metadata(block, size, is_free);
    initialize_metadata(get_the_la_metadata_of_ptr(block+1,size), size, is_free);
}

//get a metadata
bool can_break(MallocMetadata *block, size_t size)
{
    size_t size_of_block = block->size;
    int res = (int)(size_of_block+_size_meta_data()-size-2*_size_meta_data());
    if (res >= 128)
    {
        return true;
    }
    return false;
}

//get a metadata
size_t calculate_size_of_new_block(MallocMetadata *initial_block, size_t size)
{
    size_t size_of_initial_block = initial_block->size-_size_meta_data();
    return size_of_initial_block - size;
}

//get a metadtata
//return a metadata
MallocMetadata *get_right_block(MallocMetadata *initial_block)
{
    size_t size = initial_block->size;
    initial_block++;//pass the ya metadata
    initial_block = (MallocMetadata *) advance_k_bit((void *) initial_block, size);
    initial_block++;
    return initial_block;
}

//get a metadata
//return a metadata
MallocMetadata *create_and_return_new_block(MallocMetadata *initial_block, size_t size)
{
    size_t new_size = calculate_size_of_new_block(initial_block, size);
    MallocMetadata *new_block = get_right_block(initial_block);
    initialize_block(new_block, new_size, true);
    return new_block;
}

//get a metadata
void cut_and_turn_to_allocate_initial_block(MallocMetadata *initial_block, size_t size)
{
    initialize_block(initial_block, size, false);
}

//get a metadata
void insert_block_to_list(MallocMetadata *new_block)
{
    insert_node_to_list(new_block, new_block->size);
}

//get a metadata
//return a metadata
MallocMetadata* break_the_block_and_return_the_allocated(MallocMetadata *initial_block, size_t size)
{
    remove_block_from_list(initial_block);
    MallocMetadata *new_block = create_and_return_new_block(initial_block, size);
    cut_and_turn_to_allocate_initial_block(initial_block, size);
    insert_block_to_list(new_block);
    insert_block_to_list(initial_block);
    if(get_the_next_block(new_block)->is_free)
    {
        join_next_block(new_block);
    }
    return initial_block;
}


//get a size
//return metadata
MallocMetadata* enlarge_top_most(size_t size)
{
    size_t size_of_last = get_size_of_last_chunk();
    MallocMetadata* last_top_most = get_the_topmost();
    MallocMetadata* new_block =(MallocMetadata*) sbrk((intptr_t)(size-size_of_last));
    if(new_block==(void*)-1)
    {
        return nullptr;
    }
    else
    {
        remove_block_from_list(last_top_most);
        initialize_block(last_top_most,size, false);
        insert_block_to_list(last_top_most);
        return last_top_most;
    }
}


//return a metadata
MallocMetadata* find_block_at_least(size_t size)
{
    MallocMetadata *block = head;
    while (block != nullptr)
    {
        if (the_block_is_free_and_enough(block, size))
        {
            return block;
        }
        block = block->next;
    }
    return nullptr;
}

//get a metadata
//return a metadata
MallocMetadata *get_la_metadata_of_the_previous_block(MallocMetadata *block)
{
    block--;
    return block;
}

//get a metadata
//return a metadata
MallocMetadata *get_ya_metadata_of_next_block(MallocMetadata *block)
{
    size_t block_size = block->size;
    block++;
    block = (MallocMetadata *) advance_k_bit((void *) block, block_size);
    block++;
    return block;
}

//check that the previous is not the beginig of the heap
//ptr to release is just after the head meta data
bool previous_block_free(void *ptr_to_release)
{
    void *block = (void *) get_the_metadata_of_ptr(ptr_to_release);
    if (block <= heap_start)
    {
        return false;
    }
    MallocMetadata *previous_block_la_metadata = get_la_metadata_of_the_previous_block((MallocMetadata *) block);
    if (previous_block_la_metadata->is_free)
    {
        return true;
    }
    return false;
}

//get a pointer
bool next_block_free(MallocMetadata* block)
{
    MallocMetadata *next_block = get_ya_metadata_of_next_block(block);
    if (((void *) next_block) >= CURRENT_HEAP)
    {
        return false;
    }
    if (next_block->is_free)
    {
        return true;
    }
    return false;
}

void *go_back_k_bit(void *p, size_t k)
{
    char *temp = (char *) p;
    temp -= k;
    return (void *) temp;
}

//get a metadata
//return a metadata
MallocMetadata *get_the_ya_meta_from_la(MallocMetadata *la_metadata)
{
    size_t size = la_metadata->size;
    MallocMetadata *ptr = (MallocMetadata *) go_back_k_bit((void *) la_metadata, size);
    return get_the_metadata_of_ptr(ptr);
}

//get a metadata
//return a metadata
MallocMetadata *get_the_previous_block(MallocMetadata *block)
{
    MallocMetadata *la_previous_block = get_la_metadata_of_the_previous_block(block);
    return get_the_ya_meta_from_la(la_previous_block);
}

//get two metadata
void remove_the_blocks_from_list(MallocMetadata *first_block, MallocMetadata *second_block)
{
    remove_block_from_list(first_block);
    remove_block_from_list(second_block);
}


/**
 * @param metadata
 * @return metadata
 */
MallocMetadata *join_prev_block(MallocMetadata* block)
{
    MallocMetadata *previous_block = get_the_previous_block(block);
    size_t new_size = block->size + previous_block->size+_size_meta_data();
    remove_the_blocks_from_list(block, previous_block);
    initialize_block(previous_block, new_size, true);
    insert_block_to_list(previous_block);
    return previous_block;
}

/**
 *
 * @param metadata
 * @return metadata
 */
MallocMetadata* get_the_next_block(MallocMetadata *block)
{
    size_t size_of_block = block->size;
    block++;
    MallocMetadata *next_block = (MallocMetadata *) advance_k_bit((void *) block, size_of_block);
    next_block++;
    return next_block;
}

//get a metadata
//return a metadata
MallocMetadata *join_next_block(MallocMetadata* block)
{
    MallocMetadata *next_block = get_the_next_block(block);
    return join_prev_block(next_block);
}

bool sizeValid(size_t size)
{
    if (size == 0 || size > 100000000)
    {
        return false;
    }
    return true;
}

//return a metadata
MallocMetadata *get_the_topmost()
{
    MallocMetadata *curr_heap = (MallocMetadata *) CURRENT_HEAP;
    curr_heap--;
    curr_heap=get_the_ya_meta_from_la(curr_heap);
    return curr_heap;
}

bool the_last_chunk_is_free()
{
    MallocMetadata *curr_heap = get_the_topmost();
    if (curr_heap->is_free)
    {
        return true;
    }
    return false;
}

size_t get_size_of_last_chunk()
{
    MallocMetadata *curr_heap = get_the_topmost();
    return curr_heap->size;
}

MallocMetadata* get_the_right_metadata(MallocMetadata* block)
{
    size_t size = block->size;
    block ++;
    return (MallocMetadata *)advance_k_bit(block,size);
}

void change_free_bit(MallocMetadata* block, bool is_free)
{
    block->is_free = is_free;
    MallocMetadata* right_meta = get_the_right_metadata(block);
    right_meta->is_free = is_free;
}

MallocMetadata* try_to_break_and_return_allocated_block(MallocMetadata* block_enough,size_t size)
{
    if (can_break(block_enough, size))
    {
        block_enough = break_the_block_and_return_the_allocated(block_enough, size);
        return block_enough;
    }
    else
    {
        change_free_bit(block_enough, false);
        return block_enough;
    }
}

//get a metadata
bool can_reuse_current_block(MallocMetadata* old_metadata,size_t size)
{
    if (old_metadata->size >= size)
    {
        return true;
    }
    return false;
}

//get a metadata
bool is_wilderness(MallocMetadata* block)
{
    if(block == get_the_topmost())
    {
        return true;
    }
    return false;
}

//get a metadata
bool sufficient(MallocMetadata* block, size_t size)
{
    if (block->size >= size)
    {
        return true;
    }
    return false;
}

//get a pointer
bool the_next_is_wilderness(void* oldp)
{
    MallocMetadata* curr_block = get_the_metadata_of_ptr(oldp);
    MallocMetadata* next_block =get_the_next_block(curr_block);
    if(is_wilderness(next_block))
    {
        return true;
    }
    return false;
}

bool will_be_sufficient_two_blocks(MallocMetadata* block1, MallocMetadata* block2,size_t size)
{
    if(is_wilderness(block1) || is_wilderness(block2))
    {
        return true;
    }
    if(block1->size + block2->size + _size_meta_data() >= size)
    {
        return true;
    }
    return false;
}

bool will_be_sufficient_tree_blocks(MallocMetadata* block1, MallocMetadata* block2,MallocMetadata* block3,size_t size)
{
    if(block1->size + block2->size + block3->size+ _size_meta_data() + _size_meta_data() >= size)
    {
        return true;
    }
    return false;
}

void reuse_current_block(MallocMetadata* block, size_t size)
{
    if (can_break(block, size))
    {
        block = break_the_block_and_return_the_allocated(block,size);
    }
    change_free_bit(block,false);
}

void move_the_oldp(MallocMetadata* block,void* oldp, size_t num_of_bit)
{
    block++;
    std::memmove(block, oldp, num_of_bit);
}

void* merge_with_lower_address(MallocMetadata* old_meta, size_t size, void* oldp)
{
    MallocMetadata *new_meta = join_prev_block(old_meta);
    if(sufficient(new_meta,size))
    {
        move_the_oldp(new_meta,oldp,old_meta->size);
        if(can_break(new_meta,size))
        {
            new_meta = break_the_block_and_return_the_allocated(new_meta,size);
        }
        change_free_bit(new_meta,false);
        return new_meta+1;
    }
    //b2
    if (is_wilderness(old_meta))
    {
        MallocMetadata *new_block = enlarge_top_most(size);
        if(new_block == nullptr)
        {
            return nullptr;
        }
        move_the_oldp(new_block,oldp,old_meta->size);
        change_free_bit(new_meta,false);
        return new_block+1;
    }
    else
    {
        return nullptr;
    }
}


void* merge_with_higher_address(MallocMetadata* old_meta, size_t size, void* oldp)
{
    MallocMetadata *new_meta = join_next_block(old_meta);
    if(sufficient(new_meta,size))
    {
        move_the_oldp(new_meta,oldp,old_meta->size);
        if(can_break(new_meta,size))
        {
            new_meta = break_the_block_and_return_the_allocated(new_meta,size);
        }
        change_free_bit(new_meta,false);
        return new_meta+1;
    }
    else
    {
        return nullptr;
    }
}

void* merge_all_three_adjacent_blocks(MallocMetadata* old_meta,void* oldp)
{
    MallocMetadata *new_meta = join_next_block(old_meta);
    new_meta = join_prev_block(new_meta);
    change_free_bit(new_meta,false);
    move_the_oldp(new_meta,oldp,old_meta->size);
    return new_meta+1;
}

void* try_to_merge_with_lower_and_higher_wild(MallocMetadata* old_meta, size_t size, void* oldp)
{
    MallocMetadata *new_meta = join_next_block(old_meta);
    new_meta = join_prev_block(new_meta);
    enlarge_top_most(size);
    change_free_bit(new_meta,false);
    move_the_oldp(new_meta,oldp,old_meta->size);
    return new_meta+1;
}

void* try_to_merge_only_with_higher_wild(MallocMetadata* old_meta, size_t size, void* oldp)
{
    MallocMetadata *new_meta = join_next_block(old_meta);
    enlarge_top_most(size);
    change_free_bit(new_meta,false);
    move_the_oldp(new_meta,oldp,old_meta->size);
    return new_meta+1;
}

void* find_a_different_block(MallocMetadata* block,MallocMetadata* old_meta, size_t size, void* oldp)
{
    sfree(oldp);
    MallocMetadata* new_block =try_to_break_and_return_allocated_block(block,size);
    move_the_oldp(block,oldp,old_meta->size);
    return new_block+1;
}


void* realoc_with_malloc(MallocMetadata* old_meta, size_t size, void* oldp)
{
    void *new_ptr = smalloc(size);
    if (new_ptr == nullptr)
    {
        return nullptr;
    }
    std::memmove(new_ptr, oldp, old_meta->size);
    sfree(oldp);
}

///****************************************    list functions     ****************************************************************///

MallocMetadata* last_with_same_size(MallocMetadata* it)
{
    MallocMetadata* it_next = it->next;
    if(it_next == nullptr){
        return it;
    }
    while (it->size == it_next->size){
        if(it_next->next == nullptr){
            return it_next;
        }
        it_next = it_next->next;
        it = it->next;
    }
    return it;
}


// if there is no prev(smallest size), returning nullptr
//doron should check case of empty list
//doron should also check if value returned is the tail, because insertion is different
MallocMetadata *find_prev_block_by_size(size_t size)
{
    MallocMetadata *it = head;
    MallocMetadata *it_next = head->next;

    if (it->size > size)
    {
        //case of our node being first
        return nullptr;
    }
    while (it_next != nullptr)
    {
        if (size >= it->size && size <= it_next->size)
        {
            if (it_next->size == size){
                return last_with_same_size(it_next);
            }
            else{
                return it;
            }
        }
        it_next = it_next->next;
        it = it->next;
    }
    return it;
}

bool the_list_is_empty()
{
    if (tail == nullptr)
    {
        return true;
    }
    return false;
}

void insert_block_in_middle(MallocMetadata *prev_block, MallocMetadata *block_to_insert)
{
    MallocMetadata *next_node = prev_block->next;
    prev_block->next = block_to_insert;
    block_to_insert->next = next_node;
    next_node->prev = block_to_insert;
    block_to_insert->prev = prev_block;
}

void insert_block_to_tail(MallocMetadata *block_to_insert)
{
    block_to_insert->prev = tail;
    tail->next = block_to_insert;
    tail = block_to_insert;
}

MallocMetadata *find_prev_block_by_ptr(MallocMetadata *ptr_block)
{
    MallocMetadata *it = head;
    if (it == ptr_block)
    {
        return nullptr;
    }
    while (it != nullptr)
    {
        if (it->next != nullptr && it->next == ptr_block)
        {
            return it;
        }
        it = it->next;
    }
    //we don't need to return at this point
    return nullptr;
}

void remove_the_head()
{
    if (head == nullptr)
    {
        return;
    }
    if (head->next == nullptr)
    {
        head = nullptr;
        tail = nullptr;
    } else {
        head->next->prev = nullptr;
        head = head->next;
    }
}

void remove_the_tail()
{
    if (tail == nullptr)
    {
        return;
    }
    if (tail->prev == nullptr)
    {
        head = nullptr;
        tail = nullptr;
    } else
    {
        tail->prev->next = nullptr;
        tail = tail->prev;
    }
}

void remove_midl_block_from_list(MallocMetadata *prev_block)
{
    MallocMetadata *next_node = prev_block->next->next;
    MallocMetadata *node_to_del = prev_block->next;
    prev_block->next = next_node;
    next_node->prev = prev_block;
    node_to_del->next = nullptr;
    node_to_del->prev = nullptr;
}

void remove_block_from_list(MallocMetadata *block)
{
    MallocMetadata *prev_block = find_prev_block_by_ptr(block);
    if (prev_block == nullptr)
    {
        remove_the_head();
    }
    else if (block == tail)
    {
        remove_the_tail();
    }
    else
    {
        remove_midl_block_from_list(prev_block);
    }
}

void insert_block_to_list_empty(MallocMetadata *new_block)
{
    head = new_block;
    tail = new_block;
}

void insert_block_to_head(MallocMetadata *new_block)
{
    head->prev = new_block;
    new_block->next = head;
    head = new_block;
}

void insert_block_to_non_empty_list(MallocMetadata *new_block, size_t size)
{
    MallocMetadata *prev_block = find_prev_block_by_size(size);
    if (prev_block == nullptr)
    {
        insert_block_to_head(new_block);
    }
    else if (prev_block == tail)
    {
        insert_block_to_tail(new_block);
    }
    else
    {
        insert_block_in_middle(prev_block, new_block);
    }
}

void insert_node_to_list(MallocMetadata *new_block, size_t size)
{
    if (the_list_is_empty())
    {
        insert_block_to_list_empty(new_block);
    } else
    {
        insert_block_to_non_empty_list(new_block, size);
    }
}


void init_heap_start()
{
    if(heap_start == nullptr)
    {
        heap_start= sbrk(0);
    }
}
///***********************************************************************************************************************************///
///****************************************    mallocs,realoc,calloc,...     ********************************************************///
///*********************************************************************************************************************************///

void *smalloc(size_t size)
{
    init_heap_start();
    if (!sizeValid(size))
    {
        return nullptr;
    }
    if (the_last_chunk_is_free())
    {
        MallocMetadata * topmost = get_the_topmost();
        if (can_break(topmost,size))
        {
            MallocMetadata* allocated_block = break_the_block_and_return_the_allocated(topmost, size);
            return allocated_block+1;
        }
    }
    MallocMetadata* block_enough= find_block_at_least(size);
    if (block_enough != nullptr)
    {
        MallocMetadata* new_block =try_to_break_and_return_allocated_block(block_enough,size);
        return new_block+1;
    }
    MallocMetadata *new_ptr = (MallocMetadata *) allocate_new_ptr(size);
    if (new_ptr == ((void *) -1))
    {
        return nullptr;
    }
    initialize_ptr(new_ptr, size, false);
    MallocMetadata* new_block = get_the_metadata_of_ptr(new_ptr);
    insert_node_to_list(new_block, size);
    return (void *) (new_ptr);
}

void *scalloc(size_t num, size_t size)
{
    init_heap_start();
    void *new_ptr = smalloc(num * size);
    if (new_ptr == nullptr)
    {
        return nullptr;
    }
    std::memset(new_ptr, 0, num * size);
    return ((void *) new_ptr);
}

void sfree(void *p)
{
    init_heap_start();
    if (p == nullptr)
    {
        return;
    }
    MallocMetadata* block_to_release = get_the_metadata_of_ptr(p);
    if(get_the_next_block(block_to_release)->is_free)
    {
        block_to_release=join_next_block(block_to_release);
    }
    if(get_the_previous_block(block_to_release)->is_free)
    {
        block_to_release=join_prev_block(block_to_release);
    }
    change_free_bit(block_to_release,true);
}

void *srealloc(void *oldp, size_t size)
{
    init_heap_start();
    if (oldp == nullptr)
    {
        return smalloc(size);
    }
    if (!sizeValid(size))
    {
        return nullptr;
    }
    MallocMetadata *old_meta = ((MallocMetadata *) oldp) - 1;
    //a
    if (can_reuse_current_block(old_meta, size))
    {
        reuse_current_block(old_meta,size);
        return old_meta+1;
    }
    //b
    if (get_the_previous_block(old_meta)->is_free && (will_be_sufficient_two_blocks(get_the_previous_block(old_meta), old_meta, size)) )
    {
        return merge_with_lower_address(old_meta,size,oldp);
    }
    //c
    if (is_wilderness(old_meta))
    {
        old_meta =enlarge_top_most(size);
        return old_meta+1;
    }
    //d
    if (get_the_next_block(old_meta)->is_free && (will_be_sufficient_two_blocks(get_the_previous_block(old_meta), old_meta, size)))
    {
        return merge_with_higher_address(old_meta,size,oldp);
    }
    //e
    if (get_the_next_block(old_meta)->is_free && get_the_previous_block(old_meta)->is_free && will_be_sufficient_tree_blocks(
            get_the_next_block(old_meta), get_the_previous_block(old_meta), old_meta,size))
    {
        return merge_all_three_adjacent_blocks(old_meta,oldp);
    }
    //f
    if (the_next_is_wilderness(oldp) && get_the_next_block(old_meta)->is_free)
    {
        //case i
        if (get_the_previous_block(old_meta)->is_free)
        {
            return try_to_merge_with_lower_and_higher_wild(old_meta,size,oldp);
        }
        //case ii
        else
        {
            return try_to_merge_only_with_higher_wild(old_meta,size,oldp);
        }
    }
    //g
    MallocMetadata *block = (MallocMetadata *) find_block_at_least(size);
    if (block != nullptr)
    {
        return find_a_different_block(block,old_meta,size,oldp);
    }
    //h
    return realoc_with_malloc(old_meta,size,oldp);
}
