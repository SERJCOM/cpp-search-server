#pragma once

#include <vector>
#include <string>
#include <string_view>
#include <set>



std::vector<std::string> SplitIntoWords(const std::string& text) ;

std::vector<std::string_view> SplitIntoWordsView(std::string_view str) ;


template <typename StringContainer>
std::set<std::string_view> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    std::set<std::string_view> non_empty_strings;
    for (std::string_view str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}


// std::vector<std::string_view> SplitIntoWordsView(std::string_view str) {
//     std::vector<std::string_view> result;    
//     const int64_t pos_end = str.npos;
//     int64_t space = 0;
          
//     while ((str.size() != 0 ) ) {
//         while(str[0] == ' '){
//             str.remove_prefix(1);
//         }
//         if(str.size() == 0){
//             break;
//         }
//         space = str.find(' ');
        
//         result.push_back(space == pos_end ? str.substr(0) : str.substr(0, space));

//         str.remove_prefix(space == pos_end ? 0 : space);

//         if(space == pos_end){
//             break;
//         }
//     }

//     return result;
// }
