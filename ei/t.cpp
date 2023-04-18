// // #include "ei.h"

// #include <iostream>
// #include <vector>
// #include <regex>

// using namespace std;

// vector<string> split(const string& in, const string& delim) {
//     vector<string> ret;
//     try {
//         regex re{delim};
//         return vector<string>{
//                 sregex_token_iterator(in.begin(), in.end(), re, -1),
//                 sregex_token_iterator()
//            };      
//     }
//     catch(const std::exception& e) {
//         cout<<"error:"<<e.what()<<std::endl;
//     }
//     return ret;
// }

// bool contains(string& v1, string& v2) {
//     if (v1.find(v2) != string::npos) 
//         return true;
//     if (v2.find(v1) != string::npos) return true;
//     if (v1.size() > v2.size()) {
//         vector<string> tokens = split(v2, "_");
//         for (auto &&token : tokens) {
//             if (v1.find(token) != string::npos) return true;
//         }
//     } else {
//         vector<string> tokens = split(v1, "_");
//         for (auto &&token : tokens) {
//             if (v2.find(token) != string::npos) return true;
//         }
//     }
//     return false;
// }

// int main() {
//     string s1 = "香蜜湖_街道_深圳_深圳市_福田_福田区_东海_花园_2_期_13_栋_5_F";
//     string s2 = "深圳_深圳市_东海";
//     cout << contains(s1, s2) << endl;
// }

// #include "segmentation.h"
extern "C" {
#include "gperftools-httpd.h"
}

#include <iostream>
#include <vector>
#include <regex>
#include <locale>
#include <codecvt>

using namespace std;

vector<string> split(const string& in, const string& delim) {
    vector<string> ret;
    try {
        regex re{delim};
        return vector<string>{
                sregex_token_iterator(in.begin(), in.end(), re, -1),
                sregex_token_iterator()
           };      
    }
    catch(const std::exception& e) {
        cout<<"error:"<<e.what()<<std::endl;
    }
    return ret;
}

bool contains(string&& v1, string&& v2) {
    if (v1.size() == 0 || v2.size() == 0) return false;

    if (v1.find(v2) != string::npos) return true;
    if (v2.find(v1) != string::npos) return true;
    if (v1.size() > v2.size()) {
        vector<string> tokens = split(v2, "_");
        for (auto &&token : tokens) {
            if (v1.find(token) != string::npos) return true;
        }
    } else {
        vector<string> tokens = split(v1, "_");
        for (auto &&token : tokens) {
            if (v2.find(token) != string::npos) return true;
        }
    }
    return false;
}

// convert string to wstring
inline wstring to_wide_string(const string& input) {
    wstring_convert<codecvt_utf8<wchar_t>> converter;
    return converter.from_bytes(input);
}
// convert wstring to string 
inline string to_byte_string(const wstring& input) {
    //wstring_convert<codecvt_utf8_utf16<wchar_t>> converter;
    wstring_convert<codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(input);
}

int main(int argc, char* argv[]) {
    if (argc != 2) return 0;

    regex pattern_mask_name{"[0-9]{3}[*]{4}[0-9]{4}"};
    if (regex_match(argv[1], pattern_mask_name)) {
        cout << "match" << endl;
    }

    // ghttpd();
    // while (1) {
    //     SegmentChinese seg;
    //     vector<string> tokens;
	// seg.segment(argv[1], tokens);
    //     for (size_t i = 0; i < tokens.size(); i++) {
    //         cout << tokens[i] << endl;
    //     }
    // }
}

// g++ -g -O0 t.cpp -I ../cppjieba/include/ -I/data/linwei/gperftools-httpd/trunk/ -L/data/linwei/gperftools-httpd/trunk/  -lghttpd -lprofiler -L/data/linwei/gperftools-master/src/ -lpthread
// g++ -g -O0 t.cpp -I ../cppjieba/include/ -lghttpd -lprofiler -lpthread
