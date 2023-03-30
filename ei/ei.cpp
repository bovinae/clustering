#include "ei.h"
#include "error_code.h"
#include "segmentation.h"

#include <iostream>
#include <istream>
#include <streambuf>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <set>
#include <algorithm>

using namespace std;

int main(int argc, char const *argv[]) {
    vector<vector<string>> data;
    errorcode err_code = read_csv("./test.csv", data);
    if (err_code != E_OK) {
        return err_code;
    }

    // for (auto &&row : data) {
    //     for (auto &&field : row) {
    //         cout << field << endl;
    //     }
    // }
    
    parse(data);
    
    return 0;
}

errorcode read_csv(string file_name, vector<vector<string>>& data) {
    ifstream csv_data(file_name, ios::in);
    if (!csv_data.is_open()) {
        cout << "Error: opening file fail" << endl;
        return E_OPENFAIL;
    }

    // 读取标题行
    string line;
    getline(csv_data, line);
    // cout << line << endl;

    istringstream sin;         //将整行字符串line读入到字符串istringstream中
    vector<string> row;
    string field;
    // 读取数据
    while (getline(csv_data, line)) {
        sin.clear();
        sin.str(line);
        row.clear();
        while (getline(sin, field, ',')) { //将字符串流sin中的字符读到field字符串中，以逗号为分隔符
            if (field == "null" || field == "NULL") {
                field.clear();
            }
            row.push_back(field); //将每一格中的数据逐个push
            // cout << atol(word.c_str());
        }
        data.push_back(row);
    }
    csv_data.close();
    return E_OK;
}

void parse(vector<vector<string>>& data) {
    vector<FieldType> field_types = get_field_type(data);

    for (auto &&field_type : field_types) {
        cout << field_type << endl;
    }

    unordered_map<int, vector<double>> histogram;
    unordered_map<int, vector<string>> labelVectorMap{}; // columnId -> labelVector
    unordered_map<int, unordered_map<string, int32_t>> labelFreqMap{};
    for (size_t j = 0; j < data[0].size(); j++) {
        if (field_types[j] == FIELD_TYPE_NUMBER) {
            histogram[j] = genHistogram(data, j);
            continue;
        }
        vector<vector<string>> tokens{};
        unordered_map<string, int64_t> tokenFreq{};
        unordered_map<string, int32_t> labels = genTokensAndLabels(data, j, field_types, tokens, tokenFreq);
        vector<string> labelVector{};
        unordered_map<string, int32_t> labelMap{};
        convertField2Label(tokens, labels, labelVector, labelMap);
        labelVectorMap[j] = labelVector;
        labelFreqMap[j] = labelMap;
    }
}

vector<double> genHistogram(vector<vector<string>>& data, int j) {
    vector<double> number{};
    for (size_t i = 0; i < data.size(); i++) {
        number.push_back(atof(data[i][j].c_str()));
    }
    sort(number.begin(), number.end(), [](double t1, double t2) {return t1 < t2;});
    if (number.size() <= HISTOGRAM_BAR_NUM) return number;

    vector<double> histogram(HISTOGRAM_BAR_NUM);
    int quotient = number.size() / HISTOGRAM_BAR_NUM;
    int remainder = number.size() % HISTOGRAM_BAR_NUM;
    for (size_t i = 0; i < number.size(); i += quotient) {
        histogram.push_back(number[i]);
        if (remainder > 0) {
            i++;
            remainder--;
        }
    }
    return histogram;
}

unordered_map<string, int32_t> genTokensAndLabels(const vector<vector<string>>& data, int j, const vector<FieldType>& field_types, vector<vector<string>>& tokens, unordered_map<string, int64_t>& tokenFreq) {
    if (field_types[j] == FIELD_TYPE_ENGLISH) {
        SegmentByDelimiter seg_en;
        for (size_t i = 0; i < data.size(); i++) {
            vector<string> words = seg_en(data[i][j]);
            tokens.push_back(words);
            for (auto &&word : words) tokenFreq[word]++;
        }
    } else if (field_types[j] == FIELD_TYPE_CHINESE) {
        SegmentChinese seg_ch;
        for (size_t i = 0; i < data.size(); i++) {
            vector<string> words = seg_ch(data[i][j]);
            tokens.push_back(words);
            for (auto &&word : words) tokenFreq[word]++;
        }
    } else if (field_types[j] == FIELD_TYPE_MIX) {
        SegmentByDelimiter seg_mix;
        seg_mix.SetDelimiter("|");
        for (size_t i = 0; i < data.size(); i++) {
            vector<string> words = seg_mix(data[i][j]);
            tokens.push_back(words);
            for (auto &&word : words) tokenFreq[word]++;
        }
    }

    vector<TokenCount> tc{};    
    for (auto &&token : tokenFreq) {
        tc.push_back(TokenCount(token.second, token.first));
    }
    sort(tc.begin(), tc.end(), [](TokenCount t1, TokenCount t2) {return t1._count > t2._count;});
    unordered_map<string, int32_t> labels{};
    for (size_t i = 0; i < TOPK && i < tc.size(); i++) {
        labels[tc[i]._token] = i+1;
    }
    return labels;
}

void convertField2Label(const vector<vector<string>>& tokens, unordered_map<string, int32_t>& labels, vector<string>& labelVector, unordered_map<string, int32_t>& labelMap) {
    for (size_t i = 0; i < tokens.size(); i++) {
        vector<int32_t> tmp{};
        for (size_t j = 0; j < tokens[i].size(); j++) {
            if (labels.count(tokens[i][j]) > 0) tmp.push_back(labels[tokens[i][j]]);
        }
        if (tmp.size() == 0) continue;

        sort(tmp.begin(), tmp.end(), [](int32_t t1, int32_t t2) {return t1 < t2;});
        string label{to_string(tmp[0])};
        for_each (tmp.begin()+1, tmp.end(), [&](int32_t i) {label += "_" + to_string(i);});
        labelVector.push_back(label);
        labelMap[label]++;
    }
}

vector<FieldType> get_field_type(vector<vector<string>>& data) {
    vector<FieldType> field_types{};
    if (data.size() == 0) return field_types;

    ClassifyField cf;
    for (size_t j = 0; j < data[0].size(); j++) {
        set<FieldType> ft_set;
        for (size_t i = 0; i < data.size(); i++) {
            ft_set.insert(cf.classify(data[i][j]));
        }
        field_types.push_back(*ft_set.begin());
    }
    return field_types;
}

ClassifyField::ClassifyField(/* args */) : 
    pattern_number("^[-]*[0-9]*[\\.]?[0-9]*$"), 
    pattern_chinese_english(L"^[a-zA-Z\\u4e00-\\u9fa5 ]+$"),
    pattern_has_chinese(L"^.*[\\u4e00-\\u9fa5]+.*$"), 
    pattern_english("^[a-zA-Z ]+$") {
}

FieldType ClassifyField::classify(string& field) {
    if (field.size() == 0) return FIELD_TYPE_EMPTY;

    if (regex_match(field, pattern_number)) return FIELD_TYPE_NUMBER;

    if (regex_match(to_wide_string(field), pattern_chinese_english)) {
        if (regex_match(field, pattern_english)) return FIELD_TYPE_ENGLISH;
        return FIELD_TYPE_CHINESE;
    }

    return FIELD_TYPE_MIX;
}
