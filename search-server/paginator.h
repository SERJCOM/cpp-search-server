#pragma once
#include <vector>
#include <algorithm>
#include <utility>

template <typename Iterator>
class IteratorRange {
private:
    Iterator _begin;
    Iterator _end;
    std::size_t _size;
    
public:
    IteratorRange(Iterator begin, Iterator end, size_t size):
    _begin(begin), _end(end), _size(size){}

    Iterator begin() const{
        return _begin;
    }

    Iterator end() const{
        return _end;
    }

    std::size_t size() const{
        return _size;
    }

};


template <typename Iterator>
class Paginator {
private:
    std::vector<IteratorRange<Iterator>> _pages;
public:

    Paginator() = default;
   
    Paginator(Iterator begin, Iterator end, std::size_t size){
        
        //cout << distance(end, begin) << endl;
    
        for(auto temp = begin; temp < end; std::advance(temp,size)){
            auto _begin = temp;
            Iterator _end = _begin;
            std::advance(_end, size);
            std::distance(temp, end) < size ? _end = end : _end = _end;
            IteratorRange temp_page(_begin, _end, size);
            _pages.push_back(temp_page);
        }
    } 
    
    auto begin() const {
        return _pages.begin();
    }
    
    auto end() const{
        return _pages.end();
    }
}; 


template <typename Iterator>
std::ostream& operator<<(std::ostream& stream, IteratorRange<Iterator> pages){
    for(auto i = pages.begin(); i != pages.end(); i++){
        stream << *i ;
    }
    return stream;
}


template <typename Container>
auto Paginate(const Container& c, std::size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}