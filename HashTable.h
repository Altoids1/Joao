#pragma once
#include "Directory.h"

//#define HASHTABLE_DEBUG

/*
A Hashtable implementation that allocates all of its memory in one contiguous block,
without no deleted buckets.
Access is guaranteed to never require iterating over more than the set of keys which collide with the index, including itself.

Further, no more than {$collision_block_percent}% of the elements will ever be collided elements, putting a hard cap on access/removal times,
even for very large HashTables.
*/
template <typename Key, typename Value>
class HashTable
{
    //The ratio between the main block and the collision block. "4" would mean a 4:1 in favour of main.
    static constexpr size_t collision_block_ratio = 4;

    struct Bucket
    {
        bool used = false; // Used AT ALL. There are no deleted buckets; the pointer this stores is moot when the bucket is unused.
        alignas(Key) std::byte key_bytes[sizeof(Key)];
        alignas(Value) std::byte value_bytes[sizeof(Value)];
        Bucket* next_collision_bucket = nullptr;
        Key* key() { return reinterpret_cast<Key*>(key_bytes); }
        Value* value() { return reinterpret_cast<Value*>(value_bytes); }

        Bucket() = default;
        Bucket(const Bucket&) = delete;
        Bucket& operator=(Bucket& dead_buck) = delete;

#ifdef HASHTABLE_DEBUG
        ~Bucket()
        {
            if (used) UNLIKELY
            {
                std::cout << "Bucket was deleted with un-deleted contents inside!\n";
            }
        }
#endif
        inline void clear()
        {
            if(used)
            {
                //std::cout << "Deleting " << *key() << "\t" << value()->to_string() << "\n";
                key()->~Key();
                value()->~Value();
            }
            used = false;
            next_collision_bucket = nullptr;
            //Intentionally does not reset next_collision_bucket. Check ~Hashtable() for why.
        }

        //Move constructor! woo!
        //This constructor assumes that this is a bucket "worth saving",
        //and so must have a key and value.
        //Also, it doesn't bother to care about any linked collision bucket,
        //since it's probably from a dying, old memory block anyways.
        Bucket(Bucket&& dead_buck)
            :used(dead_buck.used)
        {
            if (used) LIKELY
            {
                if (*reinterpret_cast<char*>(&used) > 1)
                {
                    std::cout << "Corrupt bucket detected.\n";
                    return;
                }
                new (key()) Key(std::move(*dead_buck.key()));
                new (value()) Value(std::move(*dead_buck.value()));
                next_collision_bucket = dead_buck.next_collision_bucket;
                dead_buck.used = false;
                dead_buck.next_collision_bucket = nullptr;
            }
        }
        Bucket& operator=(Bucket&& dead_buck) noexcept
        {
            if(used)
            {
                key()->~Key();
                value()->~Value();
                next_collision_bucket = nullptr;
            }
            used = dead_buck.used;
            if(used) LIKELY
            {
                new (key()) Key(std::move(*dead_buck.key()));
                new (value()) Value(std::move(*dead_buck.value()));
                next_collision_bucket = dead_buck.next_collision_bucket;
                dead_buck.used = false;
                dead_buck.next_collision_bucket = nullptr;
            }
            return *this;
        }
    };

    struct Iterator
    {
        Bucket* const block;
        const size_t main_capacity;
        //Creates a begin() iterator.
        Iterator(Bucket* blk,size_t size)
        :block(blk)
        ,main_capacity(size)
        {
            if(block[main_index].used)
            {
                next_bucket = block;
            }
            else
            {
                this->operator++(0);
            }
        }
        //Creates an end() iterator.
        Iterator(Bucket* blk,size_t size, bool)
        :block(blk)
        ,main_capacity(size)
        ,main_index(size)
        {

        }
        size_t main_index = 0;
        Bucket* next_bucket = nullptr;
        bool operator==(const Iterator& other) const
        {
            return main_index == other.main_index && next_bucket == other.next_bucket;
        }
        bool operator!=(const Iterator& other) const
        {
            return !(this->operator==(other));
        }
        Iterator operator++(int)
        {
            return this->operator++();
        }
        Iterator& operator++()
        {
            if(next_bucket) // Try going deeper in the linked list
            {
                next_bucket = next_bucket->next_collision_bucket;
            }
            if(!next_bucket) // If that don't work, try the next slot in the main array
            {
                ++main_index;
                while(main_index < main_capacity) // Should be the case that incrementing an end() iterator is a no-op.
                {
                    if(block[main_index].used)
                    {
                        next_bucket = block + main_index;
                        break;
                    }
                    ++main_index;
                }
            }
            return *this;
        }
        const Key& key()
        {
            if(!next_bucket) UNLIKELY
            {
                throw std::out_of_range("Hashtable Iterator out of bounds!");
            }
            return *next_bucket->key();
        }
        Value& value()
        {
            if(!next_bucket) UNLIKELY
            {
                throw std::out_of_range("Hashtable Iterator out of bounds!");
            }
            return *next_bucket->value();
        }
#ifdef HASHTABLE_DEBUG
        size_t index() const { return main_index; }
#endif
        //Perhaps slow; prefer using key() and value().
        std::pair<Key&,Value&> operator*(void)
        {
            if(!next_bucket) UNLIKELY
            {
                throw std::out_of_range("Hashtable Iterator out of bounds!");
            }
            return {*next_bucket->key(),*next_bucket->value()};
        }
    };
    

    /*
        Main memory      Collision buckets
    [] [] [] [] [] [] | [] []
    */

    Bucket* bucket_block;
    size_t total_capacity; // the literal amount of memory reserved.
    size_t main_capacity; // the amount of un-collisioned buckets.
    size_t used_bucket_count; // The number of used buckets.
    struct CollisionData
    {
        size_t capacity; // Capacity of the collision memory.
        Bucket* begin; // Where the collision-reserved memory begins.
        size_t next; // The next slot in collision memory available for link use.
        std::queue<Bucket*> known_holes; // Known holes behind $next that should be filled before $next's hole.
        CollisionData(Bucket* b, size_t c, size_t total)
            :capacity(c)
            ,begin(b + (total - c))
            ,next(0)
        {

        }

        //Dummy construct. Use wisely.
        CollisionData(bool)
        {

        }
        Bucket* allocate_collision_bucket()
        {
            if(!known_holes.empty())
            {
                Bucket* ptr = known_holes.front();
#ifdef HASHTABLE_DEBUG
                if(ptr < begin || ptr > begin + capacity) UNLIKELY
                {
                    throw;
                }
#endif
                known_holes.pop();
                return ptr;
            }
            //FIXME: Maybe we should keep a pointer to the next empty bucket in collision memory?
            for(/*next*/; next < capacity; ++next)
            {
                Bucket* ptr = begin + next;
                if(!ptr->used) LIKELY
                {
                    return ptr;
                }
            }
            return nullptr;
        }
    } collision_data;
    [[no_unique_address]] std::hash<Key> hasher;

    inline size_t generate_index(const Key& key, size_t m_capacity) const
    {
        //if (math::popcount(m_capacity) != 1)
            //throw std::runtime_error("god damnit");
        return hasher(key) & (m_capacity - 1);
    }
    inline size_t generate_index(const Key& key) const { return generate_index(key, main_capacity); }

    //Takes in all args as out refs.
    //Calculates the ideal arena setup to ensure a given capacity, or greater.
    void compute_real_capacities(size_t& new_capacity, size_t& main_capacity, size_t& collision_capacity)
    {
        //FIXME: This is a slow way to calculate this.
        main_capacity = collision_block_ratio;
        collision_capacity = 1;
        while (main_capacity + collision_capacity < new_capacity)
        {
            main_capacity <<= 1;
            collision_capacity <<= 1;
        }
        new_capacity = main_capacity + collision_capacity;
    }

    //FIXME: It's somewhat problematic that we iterate over *all* old buckets during a rehash.
    void rehash(size_t new_capacity)
    {
        size_t new_collision_capacity, new_main_capacity;
        compute_real_capacities(new_capacity, new_collision_capacity, new_main_capacity);
        Bucket* new_block = new Bucket[new_capacity];
        CollisionData new_collision_data = CollisionData(new_block, new_collision_capacity, new_capacity);

        for(size_t i = 0; i < total_capacity; ++i)
        {
            Bucket& old_buck = bucket_block[i];
            if(!old_buck.used) continue;
            old_buck.next_collision_bucket = nullptr; // The collision chains are all broken by the rehash.
            //Ain't too much use to keep'em.

            size_t new_index = generate_index(*old_buck.key(),new_main_capacity);
            Bucket& new_buck = new_block[new_index];
            if(!new_buck.used)
            {
                new (&new_buck) Bucket(std::move(old_buck));
                continue;
            }
            Bucket* shits_dad = &new_buck;
            while (shits_dad->next_collision_bucket)
            {
                shits_dad = shits_dad->next_collision_bucket;
            }
            Bucket* shit = new_collision_data.allocate_collision_bucket();
            if (!shit) UNLIKELY // SHIT!!!
            {
#ifdef HASHTABLE_DEBUG
                std::cout << "Shit condition reached.\n";
                exit(1);
#endif
                delete[] new_block; // FIXME: This doesn't exactly work correctly.
                rehash(new_capacity * 2);
                return;
            }
            new (shit) Bucket(std::move(old_buck));
            shits_dad->next_collision_bucket = shit;
        }
        delete[] bucket_block;
        bucket_block = new_block;
        total_capacity = new_capacity;
        main_capacity = new_main_capacity;
        collision_data = new_collision_data;
    }
    //Takes in a key and returns its associated bucket.
    //Generic version of at() for internal use.
    //Returns nullptr if none found.
    Bucket* at_bucket(const Key& key) const
    {
        size_t index = generate_index(key);
        Bucket& buck = bucket_block[index];
        if(!buck.used)
            return nullptr;
        if(*buck.key() == key)
        {
            return &buck;
        }
        Bucket* collision_ptr = buck.next_collision_bucket;
        while(collision_ptr)
        {
            if(!collision_ptr->used) UNLIKELY
                return nullptr;
            if(*collision_ptr->key() == key)
            {
                return collision_ptr;
            }
            collision_ptr = collision_ptr->next_collision_bucket;
        }
        return nullptr;
    }

    //Already knowing that key is not in me,
    //find a bucket for it anyways.
    Bucket* make_bucket(const Key& key)
    {
        while(true)
        {
            //main array
            size_t index = generate_index(key);
            Bucket* buck = bucket_block + index;
            if(!buck->used)
                return buck;
            //collision array time
            while(buck->next_collision_bucket)
                buck = buck->next_collision_bucket;
            Bucket* new_buck = collision_data.allocate_collision_bucket();
            if(!new_buck) //if we fail, loop back to trying main again after the rehash
            {
                rehash(total_capacity*2);
                continue;
            }
            buck->next_collision_bucket = new_buck; // inform our hash neighbors that we exist
            return new_buck;
        }
    }

    //Used the handle the pointer-induced awkwardness of copying from another HashTable.
    //This class making use of Bucket* over size_t indexes probably does improve speed by reducing pointer arithmetic,
    //but it does mean that copies are complex and slowed down by the process seen below.
    inline void block_copy(const HashTable& other)
    {
        bool is_negative; // My glorious 65th bit, everyone
        size_t byte_offset;
        if (bucket_block > other.bucket_block) // if our pointer is bigger than theirs
        { // the offset is positive, we're moving higher up the address space
            is_negative = false;
            byte_offset = reinterpret_cast<size_t>(bucket_block) - reinterpret_cast<size_t>(other.bucket_block);
            //We have to do a reinterpret to size_t to ensure that there isn't a ghost floor-division caused by the nuances of pointer arithmetic.
        }
        else // Theirs is bigger than ours
        { // the offset is negative.
            is_negative = true;
            byte_offset = reinterpret_cast<size_t>(other.bucket_block) - reinterpret_cast<size_t>(bucket_block);
        }
        for (size_t i = 0; i < total_capacity; ++i)
        {
            Bucket& other_buck = other.bucket_block[i];
            if (other_buck.used)
            {
                Bucket& buck = bucket_block[i];
                buck.used = true;
                new (buck.key()) Key(*other_buck.key());
                new (buck.value()) Value(*other_buck.value());
                if (other_buck.next_collision_bucket)
                {
                    if (is_negative)
                        buck.next_collision_bucket = reinterpret_cast<Bucket*>(reinterpret_cast<size_t>(other_buck.next_collision_bucket) - byte_offset);
                    else
                        buck.next_collision_bucket = reinterpret_cast<Bucket*>(reinterpret_cast<size_t>(other_buck.next_collision_bucket) + byte_offset);
                }
            }
        }
        if (is_negative)
            collision_data.begin = reinterpret_cast<Bucket*>(reinterpret_cast<size_t>(collision_data.begin) - byte_offset);
        else
            collision_data.begin = reinterpret_cast<Bucket*>(reinterpret_cast<size_t>(collision_data.begin) + byte_offset);
        if(!collision_data.known_holes.empty())
            collision_data.known_holes = {}; //FIXME.
    }
public:
    //Basic helpers
    size_t capacity() const { return total_capacity;}
    size_t bucket_count() const { return total_capacity;}
    constexpr float max_load_factor() const { return 1.0;}
    size_t size() const { return used_bucket_count;}
    bool contains(const Key& key) const { return at_bucket(key) != nullptr;}
    size_t count(const Key& key) const { return contains(key);}

    //Capacity & memory API
    inline void clear() { *this = HashTable();}
    void ensure_capacity(size_t new_capacity)
    {
        if(new_capacity > total_capacity)
            rehash(new_capacity);
    }

    //Iterators
    Iterator begin() const
    {
        if(bucket_block) LIKELY
            return Iterator(bucket_block,main_capacity);
        else
            return end();
    }
    const Iterator end() const
    {
        return Iterator(bucket_block,main_capacity,true);
    }
#ifdef HASHTABLE_DEBUG
    void dump() const
    {
        if(!bucket_block)
        {
            std::cout << "This HashTable is empty.";
            return;
        }
        bool unprinted_collision_ptr = true;
        for(size_t i = 0; i < total_capacity; ++i)
        {
            Bucket& buck = bucket_block[i];
            if (&buck == collision_data.begin)
            {
                std::cout << "There's more.\n"; // No!!
                unprinted_collision_ptr = false;
            }
            std::cout << std::to_string(reinterpret_cast<size_t>(&buck));
            if(!buck.used)
            {
                if (buck.next_collision_bucket)
                {
                    std::cout << " Empty Bucket WITH COLLISION BUCKET: " << std::to_string(reinterpret_cast<size_t>(buck.next_collision_bucket)) << '\n';
                    continue;
                }
                std::cout << " Empty Bucket\n";
                continue;
            }
            
            std::cout << "This is a bucket: {";
            if constexpr (std::is_same<Key, std::string>::value || std::is_same<Key,::Value>::value) // FIXME: Really need a more general solution to this.
            {
                std::cout << *buck.key();
            }
            else if constexpr (std::is_enum<Key>::value)
            {
                std::cout << std::to_string(static_cast<size_t>(*buck.key()));
            }
            else if constexpr(std::is_pointer<Key>::value)
            {
                std::cout << std::to_string(reinterpret_cast<size_t>(*buck.key()));
            }
            else if constexpr(std::is_same<Key, ImmutableString>::value)
            {
                std::cout << buck.key()->to_string();
            }
            else if constexpr (std::is_arithmetic<Value>::value)
            {
                std::cout << std::to_string(*buck.value());
            }
            else
            {
                std::cout << "???";
            }
            std::cout << "\t";
            if constexpr (std::is_same<Value, std::string>::value || std::is_same<Value, ::Value>::value)
            {
                std::cout << *buck.value();
            }
            else if constexpr (std::is_enum<Value>::value)
            {
                std::cout << std::to_string(static_cast<size_t>(*buck.value()));
            }
            else if constexpr (std::is_pointer<Value>::value)
            {
                std::cout << std::to_string(reinterpret_cast<size_t>(*buck.value()));
            }
            else if constexpr (std::is_integral<Value>::value)
            {
                std::cout << std::to_string(*buck.value());
            }
            else
            {
                std::cout << "???";
            }
            if (buck.next_collision_bucket)
            {
                std::cout << '\t' << std::to_string(reinterpret_cast<size_t>(buck.next_collision_bucket));
            }
            std::cout << "}\n"; // Dear god...
        }
        if(unprinted_collision_ptr)
        {
            std::cout << "Dangling collision_block_begin pointer!\n";
            std::cout << std::to_string(reinterpret_cast<size_t>(collision_data.begin)) << std::endl;
            std::cout << std::to_string(collision_data.begin - bucket_block) << std::endl;
        }
    }
    size_t hash_collisions = 0;
#endif
    
    HashTable()
    :bucket_block(new Bucket[collision_block_ratio + 1])
    ,total_capacity(collision_block_ratio + 1)
    ,main_capacity(collision_block_ratio)
    ,used_bucket_count(0)
    ,collision_data(bucket_block,1,collision_block_ratio+1)
    {

    }
    HashTable(std::initializer_list<std::pair<Key,Value>> list)
    :bucket_block(nullptr)
    ,total_capacity(0)
    ,main_capacity(0)
    ,used_bucket_count(0)
    ,collision_data(false)
    {
        rehash(list.size() ? list.size() : (collision_block_ratio+1));
        for(auto& ptr : list)
        {
            insert(ptr);
        }
    }

    HashTable(const HashTable& other)
    :bucket_block(new Bucket[other.total_capacity])
    ,total_capacity(other.total_capacity)
    ,main_capacity(other.main_capacity)
    ,used_bucket_count(other.used_bucket_count)
    ,collision_data(other.collision_data)
    {
        //So, there's a lot of Bucket* data in the current implementation.
        //We're going to have to... awkwardly move all that around. :/
        block_copy(other);
    }
    HashTable(HashTable&& other)
    :bucket_block(other.bucket_block)
    ,total_capacity(other.total_capacity)
    ,main_capacity(other.main_capacity)
    ,used_bucket_count(other.used_bucket_count)
    ,collision_data(other.collision_data)
    {
        //FIXME: think over how to not even have to do this step on other to keep it in a valid-ish state
        other.bucket_block = nullptr;
        other.total_capacity = 0;
        other.main_capacity = 0;
        other.used_bucket_count = 0;
        other.collision_data = CollisionData(other.bucket_block,0,0);
    }

    HashTable& operator=(const HashTable& other)
    {
        //So, there's a lot of Bucket* data in the current implementation.
        //We're going to have to... awkwardly move all that around. :/
        clear_bucket_block(); // FIXME: Try to make a faster path for when we happen to have the same capacity as the new table, even though we're dirtied with elements.
        bucket_block = new Bucket[other.total_capacity];
        total_capacity = other.total_capacity;
        main_capacity = other.main_capacity;
        used_bucket_count = other.used_bucket_count;
        collision_data = other.collision_data;
        block_copy(other);
        return *this;
    }
    HashTable& operator=(HashTable&& other)
    {
        clear_bucket_block(); // FIXME: Try to make a faster path for when we happen to have the same capacity as the new table, even though we're dirtied with elements.
        bucket_block = other.bucket_block;
        total_capacity = other.total_capacity;
        main_capacity = other.main_capacity;
        used_bucket_count = other.used_bucket_count;
        collision_data = other.collision_data;

        //FIXME: think over how to not even have to do this step on other to keep it in a valid-ish state
        other.bucket_block = nullptr;
        other.total_capacity = 0;
        other.main_capacity = 0;
        other.used_bucket_count = 0;
        other.collision_data = CollisionData(other.bucket_block,0,0);
        return *this;
    }
    ~HashTable()
    {
        clear_bucket_block();
    }
    inline void clear_bucket_block()
    {
        if(!bucket_block)
            return;
        size_t buckets_left = used_bucket_count;
        if(buckets_left)
        {
            for (size_t i = 0; i < main_capacity; ++i)
            {
                Bucket& buck = bucket_block[i];
                if (!buck.used) continue;
                Bucket* buck_ptr = buck.next_collision_bucket;
                while(buck_ptr)
                {
                    Bucket* temp_ptr = buck_ptr->next_collision_bucket;
                    buck_ptr->clear();
                    --buckets_left;
                    buck_ptr = temp_ptr;
                }
                buck.clear();
                --buckets_left;
                if (!buckets_left) break;
            }
#ifdef HASHTABLE_DEBUG
            if (buckets_left)
            {
                std::cout << "Hashtable failed to delete " << std::to_string(used_bucket_count) << " entries! Or something.\n";
            }
#endif
        }
        delete[] bucket_block;
        bucket_block = nullptr; // Just to be safe :3
    }

    //Tries to find something for this key, returns nullptr if we don't have it. Non-throwing, non-allocating.
    Value* lazy_at(const Key& key) const
    {
        Bucket* buck = at_bucket(key);
        if (buck)
        {
            return buck->value();
        }
        return nullptr;
    }

    //throwing
    Value& at(const Key& key) const
    {
        Bucket* buck = at_bucket(key);
        if(!buck)
            throw std::out_of_range("Hashtable indexing failed!");
        return *buck->value();
    }

    //non-throwing; will default-construct a Value into a novel bucket if this key doesn't exist
    Value& operator[](const Key& key)
    {
        size_t index = generate_index(key);
        Bucket* buck = bucket_block + index;
        Bucket* fav_buck = nullptr; // the bucket we'll construct into if we confirm that nothing can be found
        if(buck->used)
        {
#ifdef HASHTABLE_DEBUG
            hash_collisions++;
#endif
            if(*buck->key() == key)
            {
                return *buck->value();
            }
            while (true)
            {
                Bucket* linked_buck = buck->next_collision_bucket;
                if(!linked_buck) // End of the line!
                {
                    fav_buck = collision_data.allocate_collision_bucket(); // Make a new bucket just for us
                    if(!fav_buck) // If we've run out of collision space
                    {
                        rehash(total_capacity * 2); // Just rehash
                        return this->operator[](key); // and try again.
                    }
                    buck->next_collision_bucket = fav_buck;
                    break;
                }
                if(linked_buck->used) LIKELY
                {
                    if(*linked_buck->key() == key)
                    {
                        return *linked_buck->value();
                    }
                }
                else // What? How?
                {
                    fav_buck = linked_buck;
                    break;
                }
                buck = linked_buck;
            }
            
        }
        else
        {
            fav_buck = buck;
            
        }
#ifdef HASHTABLE_DEBUG
        if(fav_buck < bucket_block || fav_buck >= bucket_block + total_capacity) UNLIKELY
        {
            throw std::out_of_range("Bucket found for assignment was out of scope!");
        }
        if (fav_buck->used) UNLIKELY
        {
            std::cout << "Warning, overriding previously-used bucket!";
            throw;
        }
        if (fav_buck->next_collision_bucket) UNLIKELY
        {
            std::cout << "fav_buck started out with fraudulent bucket pointer!";
        dump();
            throw;
        }
#endif
        if constexpr (std::is_default_constructible<Value>())
        {
            ++used_bucket_count;
            fav_buck->used = true;
            new (fav_buck->key()) Key(key);
            new (fav_buck->value()) Value();
            return *fav_buck->value();
        }
        else
        {
            throw std::out_of_range("Attempted to lazy-index into a HashTable with a Value type that is not default-constructable!");
        }
    }

    void remove(const Key& key)
    {
        size_t index = generate_index(key);
        Bucket* buck = bucket_block + index;
        if(!buck->used) UNLIKELY
            return;
        if(*buck->key() == key) // Found it in the main array.
        {
            if(buck->next_collision_bucket) // Aghast, we actually link into collision space!
            {
                Bucket* collision_buck = buck->next_collision_bucket;
                /*
                So, We could just do nothing and just declare this a deleted bucket.
                However, deleted buckets are:
                    -Kinda annoying to track
                    -Introduce a pretty scary upper-bound on how long any linked list can get
                    -Confusing to code around
                So we're just not having them. This bucket gets to be "promoted" into the main memory.
                */
                collision_data.known_holes.push(collision_buck);
                *buck = std::move(*collision_buck);
                --used_bucket_count;
                return;
            }
            else
            {
                buck->clear();
                --used_bucket_count;
                return;
            }
        }
        
#ifdef HASHTABLE_DEBUG
        size_t i = 0;
#endif
        for(Bucket* next_buck = buck->next_collision_bucket; next_buck; buck = next_buck, next_buck = next_buck->next_collision_bucket)
        {
            if(!next_buck->used)
                return;
            if(*next_buck->key() == key)
            {
                buck->next_collision_bucket = next_buck->next_collision_bucket; // Bridge the pointer gap we're making in the linked list
                next_buck->clear();
                collision_data.known_holes.push(next_buck);
                --used_bucket_count;
                return;
            }
#ifdef HASHTABLE_DEBUG
            if (++i > 10'000) UNLIKELY
            {
                dump();
                exit(69);
            }
#endif
        }
        return;
    }
    inline void erase(const Key& key) { return remove(key);}

    void insert(Iterator begin, const Iterator& end)
    {
        for(;begin != end; ++begin)
        {
            if (at_bucket(begin.key())) // If this already has a bucket
            {
                continue; // Don't overwrite. This behaviour is a layover from std::unordered_map.
            }
            this->operator[](begin.key()) = begin.value();
        }
    }
    void insert(const std::pair<Key,Value>& pair)
    {
        if (at_bucket(pair.first)) // If this already has a bucket
        {
            return; // Don't overwrite. This behaviour is a layover from std::unordered_map.
        }
        Bucket* fav_buck = make_bucket(pair.first);
        ++used_bucket_count;
        fav_buck->used = true;
        new (fav_buck->key()) Key(pair.first);
        new (fav_buck->value()) Value(pair.second);
    }
    void insert(const Key& key, const Value& value)
    {
        if (at_bucket(key)) // If this already has a bucket
        {
            return; // Don't overwrite. This behaviour is a layover from std::unordered_map.
        }
        Bucket* fav_buck = make_bucket(key);
        ++used_bucket_count;
        fav_buck->used = true;
        new (fav_buck->key()) Key(key);
        new (fav_buck->value()) Value(value);
    }

    void merge(const HashTable<Key,Value>& other)
    {
        return insert(other.begin(), other.end());
    }
};