#pragma once
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
    //What percent of the allocated memory to reserve for collision buckets.
    static constexpr size_t collision_block_percent = 25;

    struct Bucket
    {
        bool used = false; // Used AT ALL. There are no deleted buckets; the pointer this stores is moot when the bucket is unused.
        alignas(Key) std::byte key_bytes[sizeof(Key)];
        alignas(Value) std::byte value_bytes[sizeof(Value)];
        Bucket* next_collision_bucket = nullptr;
        Key* key() { return reinterpret_cast<Key*>(key_bytes);}
        Value* value() { return reinterpret_cast<Value*>(value_bytes);}

        Bucket() = default;
        Bucket(const Bucket&) = default;
        Bucket& operator=(Bucket& dead_buck) = default;

        //FIXME: I would prefer it if Bucket's destruction was controlled by HashTable rather than implicitly happening on every bucket-destruct,
        //since sometimes the bucket has no real data to even destruct in the first place.
        ~Bucket()
        {
            clear();
        }
        inline void clear()
        {
            if(used)
            {
                //std::cout << "Deleting " << *key() << "\t" << value()->to_string() << "\n";
                key()->~Key();
                value()->~Value();
            }
            used = false;
        }

        //Move constructor! woo!
        //This constructor assumes that this is a bucket "worth saving",
        //and so must have a key and value.
        //Also, it doesn't bother to care about any linked collision bucket,
        //since it's probably from a dying, old memory block anyways.
        Bucket(Bucket&& dead_buck)
            :used(dead_buck.used)
        {
            if(used) [[likely]]
            {
                new (key()) Key(std::move(*dead_buck.key()));
                new (value()) Value(std::move(*dead_buck.value()));
                next_collision_bucket = dead_buck.next_collision_bucket;
                dead_buck.used = false;
            }
        }
        Bucket& operator=(Bucket&& dead_buck)
        {
            used = dead_buck.used;
            if(used) [[likely]]
            {
                new (key()) Key(std::move(*dead_buck.key()));
                new (value()) Value(std::move(*dead_buck.value()));
                next_collision_bucket = dead_buck.next_collision_bucket;
                dead_buck.used = false;
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
        Key& key()
        {
            if(!next_bucket) [[unlikely]]
            {
                throw std::out_of_range("Hashtable Iterator out of bounds!");
            }
            return *next_bucket->key();
        }
        Value& value()
        {
            if(!next_bucket) [[unlikely]]
            {
                throw std::out_of_range("Hashtable Iterator out of bounds!");
            }
            return *next_bucket->value();
        }
#ifdef DEBUG
        size_t index() const { return main_index; }
#endif
        //Perhaps slow; prefer using key() and value().
        std::pair<Key&,Value&> operator*(void)
        {
            if(!next_bucket) [[unlikely]]
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
        CollisionData(Bucket* b, size_t total)
#ifdef __GNUC__
            :capacity((total * collision_block_percent / 100) ?: 1) // ELVIS OPERATOR WARNING
#else
            :capacity((total* collision_block_percent / 100) ? (total * collision_block_percent / 100) : 1) // :(
#endif
            ,begin(b + (total - capacity))
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
#ifdef DEBUG
                if(ptr < begin || ptr > begin + capacity) [[unlikely]]
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
                if(!ptr->used) [[likely]]
                {
                    return ptr;
                }
            }
            return nullptr;
        }
    } collision_data;
    [[no_unique_address]] std::hash<Key> hasher;

    //FIXME: It's somewhat problematic that we iterate over *all* old buckets during a rehash.
    void rehash(size_t new_capacity)
    {
        Bucket* new_block = new Bucket[new_capacity];
        CollisionData new_collision_data = CollisionData(new_block,new_capacity);
        size_t new_main_capacity = new_capacity - new_collision_data.capacity;

        for(size_t i = 0; i < total_capacity; ++i)
        {
            Bucket& old_buck = bucket_block[i];
            if(!old_buck.used) continue;
            old_buck.next_collision_bucket = nullptr; // The collision chains are all broken by the rehash.
            //Ain't too much use to keep'em.

            size_t new_index = hasher(*old_buck.key()) % new_main_capacity;
            Bucket& new_buck = new_block[new_index];
            if(!new_buck.used)
            {
                new (&new_buck) Bucket(std::move(old_buck));
                continue;
            }
            Bucket* shit = new_collision_data.allocate_collision_bucket();
            if(!shit) [[unlikely]] // SHIT!!!
            {
                delete[] new_block;
                rehash(new_capacity * 2);
                return;
            }
            new (shit) Bucket(std::move(old_buck));
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
        size_t index = hasher(key) % main_capacity;
        Bucket* buck = bucket_block + index;
        if(!buck->used)
            return nullptr;
        if(*buck->key() == key)
        {
            return buck;
        }
        while((buck = buck->next_collision_bucket))
        {
            if(!buck->used)
                return nullptr;
            if(*buck->key() == key)
            {
                return buck;
            }
        }
        return nullptr;
    }
public:
    //Basic helpers
    [[nodiscard]] size_t capacity() const { return total_capacity;}
    [[nodiscard]] size_t bucket_count() const { return total_capacity;}
    [[nodiscard]] constexpr float max_load_factor() const { return 1.0;}
    [[nodiscard]] size_t size() const { return used_bucket_count;}
    [[nodiscard]] bool contains(const Key& key) const { return at_bucket(key) != nullptr;}
    [[nodiscard]] size_t count(const Key& key) const { return contains(key);}

    //Capacity & memory API
    inline void clear() { *this = HashTable();}
    void ensure_capacity(size_t new_capacity)
    {
        if(new_capacity > total_capacity)
            rehash(new_capacity);
    }

    //Iterators
    Iterator begin()
    {
        return Iterator(bucket_block,main_capacity);
    }
    const Iterator end() const
    {
        return Iterator(bucket_block,main_capacity,true);
    }
#ifdef DEBUG
    void dump() const
    {
        bool unprinted_collision_ptr = true;
        for(size_t i = 0; i < total_capacity; ++i)
        {
            Bucket& buck = bucket_block[i];
            if(&buck == collision_data.begin)
            {
                std::cout << "There's more.\n"; // No!!
                unprinted_collision_ptr = false;
            }
            if(!buck.used)
            {
                std::cout << "Empty Bucket\n";
                continue;
            }
            
            std::cout << "This is a bucket: {" << *buck.key() << "\t" << *buck.value() << "}\n"; // Dear god...
        }
        if(unprinted_collision_ptr)
        {
            std::cout << "Dangling collision_block_begin pointer!\n";
            std::cout << std::to_string(reinterpret_cast<size_t>(bucket_block)) << std::endl;
            std::cout << std::to_string(collision_data.begin - bucket_block) << std::endl;
        }
    }
    size_t hash_collisions = 0;
#endif
    
    HashTable()
    :bucket_block(new Bucket[4])
    ,total_capacity(4)
    ,main_capacity(3)
    ,used_bucket_count(0)
    ,collision_data(bucket_block,total_capacity)
    {

    }
    HashTable(std::initializer_list<std::pair<Key,Value>> list)
    :bucket_block(nullptr)
    ,total_capacity(0)
    ,main_capacity(0)
    ,used_bucket_count(0)
    ,collision_data(false)
    {
        rehash(list.size()); // FIXME: This is weird.
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
        ptrdiff_t offset = bucket_block - other.bucket_block;
        for(size_t i = 0; i < total_capacity; ++i)
        {
            Bucket& other_buck = other.bucket_block[i];
            if(other_buck.used)
            {
                Bucket& buck = bucket_block[i];
                buck.used = true;
                new (buck.key()) Key(*other_buck.key());
                new (buck.value()) Value(*other_buck.value()); 
                if(other_buck.next_collision_bucket)
                    buck.next_collision_bucket = other_buck.next_collision_bucket + offset;
            }
        }
        collision_data.begin += offset;
        collision_data.known_holes = {}; //FIXME.
    }
    HashTable& operator=(const HashTable& other)
    {
        //So, there's a lot of Bucket* data in the current implementation.
        //We're going to have to... awkwardly move all that around. :/
        ptrdiff_t offset = bucket_block - other.bucket_block;
        for(size_t i = 0; i < total_capacity; ++i)
        {
            Bucket& other_buck = other.bucket_block[i];
            if(other_buck.used)
            {
                Bucket& buck = bucket_block[i];
                buck.used = true;
                new (buck.key()) Key(*other_buck.key());
                new (buck.value()) Value(*other_buck.value()); 
                if(other_buck.next_collision_bucket)
                    buck.next_collision_bucket = other_buck.next_collision_bucket + offset;
            }
        }
        collision_data.begin += offset;
        collision_data.known_holes = {}; //FIXME.
        return *this;
    }
    ~HashTable()
    {
        delete[] bucket_block;
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
        size_t index = hasher(key) % main_capacity;
        Bucket* buck = bucket_block + index;
        Bucket* fav_buck = nullptr; // the bucket we'll construct into if we confirm that nothing can be found
        if(buck->used)
        {
#ifdef DEBUG
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
                if(linked_buck->used) [[likely]]
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
#ifdef DEBUG
        if(fav_buck < bucket_block || fav_buck >= bucket_block + total_capacity) [[unlikely]]
        {
            throw std::out_of_range("Bucket found for assignment was out of scope!");
        }
#endif
        ++used_bucket_count;
        fav_buck->used = true;
        new (fav_buck->key()) Key(key);
        new (fav_buck->value()) Value();
        return *fav_buck->value();
    }

    void remove(const Key& key)
    {
        size_t index = hasher(key) % main_capacity;
        Bucket* buck = bucket_block + index;
        if(!buck->used) [[unlikely]]
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
        Bucket* next_buck = buck->next_collision_bucket;
        while(next_buck)
        {
            if(next_buck->used && *next_buck->key() == key)
            {
                buck->next_collision_bucket = next_buck->next_collision_bucket;
                next_buck->clear();
                collision_data.known_holes.push(next_buck);
                --used_bucket_count;
                return;
            }
            next_buck = buck->next_collision_bucket;
        }
        return;
    }
    inline void erase(const Key& key) { return remove(key);}

    void insert(Iterator begin, const Iterator& end)
    {
        for(;begin != end; ++begin)
        {
            this->operator[](begin.key()) = begin.value();
        }
    }
    void insert(const std::pair<Key,Value>& pair)
    {
        this->operator[](pair.first) = pair.second; // FIXME: Implement an insert variant that skips over the unnecessary default-construct that operator[] does.
    }
};