#include <algorithm>
#include <cstdlib>
#include <future>
#include <map>
#include <numeric>
#include <random>
#include <string>
#include <vector>
#include <algorithm>
#include <set>



template <typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys");

    struct Access {
        std::lock_guard<std::mutex> m ;
        Value& ref_to_value;

    };

    explicit ConcurrentMap(size_t bucket_count):maps(bucket_count){
        size_ = bucket_count;
    }

    Access operator[](const Key& key){
        uint64_t  maps_index = uint64_t(key) % size_;
        return { std::lock_guard(maps[maps_index].m), maps[maps_index].map[key]};
    }

    std::map<Key, Value> BuildOrdinaryMap(){
        std::map<Key, Value> res;

        for(int i = 0; i < size_; i++){     
            std::lock_guard g(maps[i].m);                       
            res.insert(maps[i].map.begin(), maps[i].map.end());
        }

        return res;
    }

    void erase(Key key){
        uint64_t  maps_index = uint64_t(key) % size_;
        std::lock_guard(maps[maps_index].m);
        
        maps[maps_index].map.erase(key);
    }

    size_t size(){
        size_t ret = 0;
        for(const maps_mutex& map_ : maps){
            ret += map_.map.size();
        }

        return ret;
    }


private:
    int size_ = 0;

    struct maps_mutex{
        std::mutex m;
        std::map<Key, Value> map;
    };    
    std::vector<maps_mutex> maps;
};



template <typename Value>
class ConcurrentSet {
public:
    static_assert(std::is_integral_v<Value>, "ConcurrentMap supports only integer keys");

    struct Access {
        std::lock_guard<std::mutex> m ;
        Value& ref_to_value;

    };

    explicit ConcurrentSet(size_t bucket_count):maps(bucket_count){
        size_ = bucket_count;
    }

    Access operator[](const Value& key){
        uint64_t  maps_index = uint64_t(key) % size_;
        auto it = maps[maps_index].map.find(key);
        return { std::lock_guard(maps[maps_index].m), *it};
    }

    std::set<Value> BuildOrdinarySet(){
        std::set<Value> res;

        for(int i = 0; i < size_; i++){     
            std::lock_guard g(maps[i].m);                       
            res.insert(maps[i].map.begin(), maps[i].map.end());
        }

        return res;
    }

    void erase(Value value){
        uint64_t  maps_index = uint64_t(value) % size_;
        std::lock_guard(maps[maps_index].m);
        
        maps[maps_index].map.erase(value);
    }

    void insert(Value value){
        uint64_t  maps_index = uint64_t(size_current) % size_;
        std::lock_guard(maps[maps_index].m);
        maps[maps_index].map.insert(value);
        size_current++;
    }

    // size_t count(Key key){
    //     //size_t res = 0;
    //     uint64_t  maps_index = uint64_t(key) % size_;
    //     //std::lock_guard(maps[maps_index].m);
    //     return maps[maps_index].map.count(key);

    // }

    // std::pair<Key, Value> at(Key key){
    //     uint64_t  maps_index = uint64_t(key) % size_;
    //     return maps[maps_index].map.at(key);
    // } 

    size_t size(){

        return size_current;
    }


    // class Iterator{
    //     public:
           

    //         Access access = {};

    //     private:


    // };


private:
    int size_ = 0;
    int size_current = 0;

    struct maps_mutex{
        std::mutex m;
        std::set<Value> map;
    };    
    std::vector<maps_mutex> maps;
};