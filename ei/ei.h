#ifndef _EI_H
#define _EI_H

#include "error_code.h"

#include <string>
#include <vector>
#include <regex>
#include <locale>
#include <codecvt>
#include <unordered_map>

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

errorcode read_csv(string file_name, vector<vector<string>>& data);
void parse(vector<vector<string>>& data);
vector<FieldType> get_field_type(vector<vector<string>>& data);
vector<double> genHistogram(vector<vector<string>>& data, int j);
unordered_map<string, int32_t> genTokensAndLabels(const vector<vector<string>>& data, int j, const vector<FieldType>& field_types, vector<vector<string>>& tokens, unordered_map<string, int64_t>& tokenFreq);
void convertField2Label(const vector<vector<string>>& tokens, unordered_map<string, int32_t>& labels, vector<string>& labelVector, unordered_map<string, int32_t>& labelMap);

#endif
