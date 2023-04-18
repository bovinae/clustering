#ifndef _EI_H
#define _EI_H

#include "error_code.h"

#include <string>
#include <vector>
#include <regex>
#include <locale>
#include <codecvt>
#include <unordered_map>
#include <type_traits>
#include <chrono>
#include <iostream>

using namespace std;

enum FieldType {
    FIELD_TYPE_NAME = 1,
    FIELD_TYPE_PHONE,
    FIELD_TYPE_CHINESE,
    FIELD_TYPE_MIX,
    FIELD_TYPE_ENGLISH,
    FIELD_TYPE_NUMBER,
    FIELD_TYPE_EMPTY,
};

class TokenCount {
public:
    int64_t _count;
    string _token;

    TokenCount(int64_t count, string token) : _count(count), _token(token) {}
};

class ClassifyField
{
private:
    regex pattern_number;
    wregex pattern_chinese_english;
    wregex pattern_has_chinese;
    regex pattern_english;

    regex pattern_time;
    wregex pattern_mask_name;
    regex pattern_mask_phone;
public:
    ClassifyField(/* args */);

    FieldType classify(const string& field);
    bool is_mask_name(const string& field) const;
    bool is_mask_phone(const string& field) const;
};

/*1、打印耗时，取变量构造函数与析构函数的时间差，单位ms*/
class SpendTime
{
public:
    SpendTime() : _curTimePoint(chrono::steady_clock::now()) {}
    void start() {
        _curTimePoint = chrono::steady_clock::now();
    }
    void print(const string& msg) {
        auto curTime = chrono::steady_clock::now();
        auto duration = chrono::duration_cast<chrono::milliseconds>(curTime - _curTimePoint);
        cout << msg << duration.count() / double(1000) << "s" << endl;
    }

private:
    chrono::steady_clock::time_point _curTimePoint;
};

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

template <typename T>
typename enable_if<is_same<T, string>::value, string>::type
tostring(T str) {
    return str;
}

template <typename T>
typename enable_if<is_same<T, double>::value, string>::type
tostring(T d) {
    return to_string(d);
}

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

bool contains(const string& v1, const string& v2) {
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

errorcode read_csv(string file_name, vector<string>& header, vector<vector<string>>& data);

vector<vector<int>> process(const vector<vector<string>>& data);

bool not_histogram(int j);

void origin_normalize_invert_index(const vector<vector<string>>& data, int j, vector<string>& label_vector, unordered_map<string, vector<int>>& label_invert_index);

void genHistogram(const vector<vector<string>>& data, int j, vector<string>& label_vector, unordered_map<string, vector<int>>& label_invert_index);

void convertField2Label(const vector<vector<string>>& tokens, unordered_map<string, int64_t>& labels, vector<string>& label_vector, unordered_map<string, vector<int>>& label_invert_index);

vector<vector<double>> genCorrelationMatrix(const vector<vector<string>>& data, unordered_map<int, vector<string>>& labelVectorMap, unordered_map<int, unordered_map<string, vector<int>>>& labelFreqMap);
                                            
vector<vector<int> > doSpectralClustering(vector<vector<double>>& correlation);

double calcCorrelationSum(int i, int j, unordered_map<int, vector<string>>& labelVectorMap, unordered_map<int, unordered_map<string, vector<int>>>& labelFreqMap, int64_t sigma);

double contribute(int field_type1, int field_type2, const string& v1, const string& v2, double p_xy, double p_x, double p_y, double threshold);

string getVisitedKey(const string& v1, const string& v2);

#endif
