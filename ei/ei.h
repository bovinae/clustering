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

using namespace std;

const int HISTOGRAM_BAR_NUM = 100;
const int TOPK = 1000;

enum FieldType {
    FIELD_TYPE_MIX = 1,
    FIELD_TYPE_CHINESE,
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
public:
    ClassifyField(/* args */);

    FieldType classify(string& field);
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

errorcode read_csv(string file_name, vector<vector<string>>& data);

vector<vector<int>> process(vector<vector<string>>& data);

vector<FieldType> get_field_type(vector<vector<string>>& data);

void genHistogram(vector<vector<string>>& data, int j, vector<double>& number, unordered_map<double, vector<int>>& numberFreq);

unordered_map<string, int32_t> genTokensAndLabels(const vector<vector<string>>& data, int j, const vector<FieldType>& field_types, vector<vector<string>>& tokens, unordered_map<string, int64_t>& tokenFreq);

void convertField2Label(const vector<vector<string>>& tokens, unordered_map<string, int32_t>& labels, vector<string>& labelVector, unordered_map<string, vector<int>>& labelMap);

vector<vector<double>> genCorrelationMatrix(size_t col, unordered_map<int, vector<double>>& numberVectorMap, unordered_map<int, unordered_map<double, vector<int>>>& numberFreqMap, 
                                            unordered_map<int, vector<string>>& labelVectorMap, unordered_map<int, unordered_map<string, vector<int>>>& labelFreqMap);
                                            
vector<vector<int> > doSpectralClustering(vector<vector<double>>& correlation);

template<typename T, typename U>
double calcCorrelationSum(vector<T>& v1, unordered_map<T, vector<int>>& f1, vector<U>& v2, unordered_map<U, vector<int>>& f2);

template<typename T, typename U>
string getVisitedKey(T& v1, U& v2);

#endif
