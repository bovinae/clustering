#include "ei.h"
#include "error_code.h"
#include "segmentation.h"
#include "SpectralClustering.h"

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
    
    vector<vector<int>> clusters = process(data);
    // output clustered items
    // items are ordered according to distance from cluster centre
    for (unsigned int i=0; i < clusters.size(); i++) {
        std::cout << "Cluster " << i << ": " << "Item ";
        std::copy(clusters[i].begin(), clusters[i].end(), std::ostream_iterator<int>(std::cout, ", "));
        std::cout << std::endl;
    }
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

vector<vector<int>> process(vector<vector<string>>& data) {
    vector<FieldType> field_types = get_field_type(data);

    // for (auto &&field_type : field_types) {
    //     cout << field_type << endl;
    // }

    unordered_map<int, vector<double>> numberVectorMap{};
    unordered_map<int, unordered_map<double, vector<int>>> numberFreqMap{};
    unordered_map<int, vector<string>> labelVectorMap{}; // columnId -> labelVector
    unordered_map<int, unordered_map<string, vector<int>>> labelFreqMap{};
    for (size_t j = 0; j < data[0].size(); j++) {
        if (field_types[j] == FIELD_TYPE_NUMBER) {
            vector<double> number{};
            unordered_map<double, vector<int>> numberFreq{};
            genHistogram(data, j, number, numberFreq);
            numberVectorMap[j] = number;
            numberFreqMap[j] = numberFreq;
            continue;
        }
        vector<vector<string>> tokens{};
        unordered_map<string, int64_t> tokenFreq{};
        unordered_map<string, int32_t> labels = genTokensAndLabels(data, j, field_types, tokens, tokenFreq);
        vector<string> labelVector{};
        unordered_map<string, vector<int>> labelFreq{};
        convertField2Label(tokens, labels, labelVector, labelFreq);
        labelVectorMap[j] = labelVector;
        labelFreqMap[j] = labelFreq;
    }

    vector<vector<double>> correlation = genCorrelationMatrix(data[0].size(), numberVectorMap, numberFreqMap, labelVectorMap, labelFreqMap);

    vector<vector<int>> clusters = doSpectralClustering(correlation);
    return clusters;
}

void genHistogram(vector<vector<string>>& data, int j, vector<double>& number, unordered_map<double, vector<int>>& numberFreq) {
    vector<double> histogram{};
    for (size_t i = 0; i < data.size(); i++) {
        number.push_back(atof(data[i][j].c_str()));
    }
    vector<double> sorted_number = number;
    stable_sort(sorted_number.begin(), sorted_number.end(), [](double t1, double t2) {return t1 < t2;});
    auto it = unique(sorted_number.begin(), sorted_number.end());
    size_t unique_len = it - sorted_number.begin();
    if (unique_len <= HISTOGRAM_BAR_NUM) {
        histogram = sorted_number;
        return ;
    }

    int quotient = unique_len / HISTOGRAM_BAR_NUM;
    int remainder = unique_len % HISTOGRAM_BAR_NUM;
    for (size_t i = 0; i < unique_len; i += quotient) {
        histogram.push_back(sorted_number[i]);
        if (remainder > 0) {
            i++;
            remainder--;
        }
    }
    for (size_t i = 0; i < number.size(); i++) {
        auto it = upper_bound(histogram.begin(), histogram.end(), number[i]);
        it--;
        number[i] = *it;
        numberFreq[number[i]].push_back(i);
    }
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
    stable_sort(tc.begin(), tc.end(), [](TokenCount t1, TokenCount t2) {return t1._count > t2._count;});
    unordered_map<string, int32_t> labels{};
    for (size_t i = 0; i < TOPK && i < tc.size(); i++) {
        labels[tc[i]._token] = i+1;
    }
    return labels;
}

void convertField2Label(const vector<vector<string>>& tokens, unordered_map<string, int32_t>& labels, vector<string>& labelVector, unordered_map<string, vector<int>>& labelFreq) {
    for (size_t i = 0; i < tokens.size(); i++) {
        vector<int32_t> tmp{};
        for (size_t j = 0; j < tokens[i].size(); j++) {
            if (labels.count(tokens[i][j]) > 0) tmp.push_back(labels[tokens[i][j]]);
        }
        if (tmp.size() == 0) {
            labelVector.push_back({});
            labelFreq[{}].resize(0);
            continue;
        }

        stable_sort(tmp.begin(), tmp.end(), [](int32_t t1, int32_t t2) {return t1 < t2;});
        string label{to_string(tmp[0])};
        for_each (tmp.begin()+1, tmp.end(), [&](int32_t i) {label += "_" + to_string(i);});
        labelVector.push_back(label);
        labelFreq[label].push_back(i);
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

vector<vector<double>> genCorrelationMatrix(size_t numDims, unordered_map<int, vector<double>>& numberVectorMap, unordered_map<int, unordered_map<double, vector<int>>>& numberFreqMap, 
                                            unordered_map<int, vector<string>>& labelVectorMap, unordered_map<int, unordered_map<string, vector<int>>>& labelFreqMap) {
    vector<vector<double>> correlation(numDims);
    for (size_t i = 0; i < numDims; i++) correlation[i].resize(numDims);

    // sum(p(xy) * (p(x) + p(y)) / 2)
    for (size_t i = 0; i < numDims; i++) {
        for (size_t j = i; j < numDims; j++) {
            if (i == j) {
                correlation[i][j] = 1;
                continue;
            }
            double sum = 0;
            if (numberVectorMap.count(i) > 0) {
                if (numberVectorMap.count(j) > 0) {
                    sum = calcCorrelationSum(numberVectorMap[i], numberFreqMap[i], numberVectorMap[j], numberFreqMap[j]);
                } else {
                    sum = calcCorrelationSum(numberVectorMap[i], numberFreqMap[i], labelVectorMap[j], labelFreqMap[j]);
                }
            } else {
                if (numberVectorMap.count(j) > 0) {
                    sum = calcCorrelationSum(labelVectorMap[i], labelFreqMap[i], numberVectorMap[j], numberFreqMap[j]);
                } else {
                    sum = calcCorrelationSum(labelVectorMap[i], labelFreqMap[i], labelVectorMap[j], labelFreqMap[j]);
                }
            }
            correlation[i][j] = sum;
            correlation[j][i] = sum;
        }
    }

    return correlation;
}

template<typename T, typename U>
double calcCorrelationSum(vector<T>& v1, unordered_map<T, vector<int>>& f1, vector<U>& v2, unordered_map<U, vector<int>>& f2) {
    unordered_map<string, int> visited;
    double sum = 0;
    for (size_t k = 0; k < v1.size(); k++) {
        string visited_key = getVisitedKey(v1[k], v2[k]);
        if (visited.count(visited_key) > 0) continue;

        // 行号都是有序的
        vector<int>& rows_i = f1[v1[k]];
        vector<int>& rows_j = f2[v2[k]];
        vector<int> result;
        set_intersection(rows_i.begin(), rows_i.end(), rows_j.begin(), rows_j.end(), back_inserter(result));
        double p_xy = result.size() / double(v1.size());
        double p_x_and_y = (rows_i.size() + rows_j.size()) / double(v1.size());
        sum += p_xy * p_x_and_y / 2;
        visited[visited_key]++;
    }
    return sum;
}

template<typename T, typename U>
string getVisitedKey(T& v1, U& v2) {
    return tostring(v1) + "_" + tostring(v2);
}

vector<vector<int> > doSpectralClustering(vector<vector<double>>& correlation) {
    size_t numDims = correlation.size();

    Eigen::MatrixXd m = Eigen::MatrixXd::Zero(numDims, numDims);
    for (size_t i = 0; i < numDims; i++) {
        for (size_t j = i; j < numDims; j++) {
            m(i, j) = correlation[i][j];
            m(j, i) = correlation[i][j];
        }
    }

    // do eigenvalue decomposition
    SpectralClustering* c = new SpectralClustering(m, numDims);

    // whether to use auto-tuning spectral clustering or kmeans spectral clustering
    bool autotune = true;

    vector<vector<int> > clusters;
    if (autotune) {
        // auto-tuning clustering
        clusters = c->clusterRotate();
    } else {
        // how many clusters you want
        int numClusters = 5;
        clusters = c->clusterKmeans(numClusters);
    }

    return clusters;
}
