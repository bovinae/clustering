#include "ei.h"
#include "error_code.h"
#include "segmentation.h"
#include "SpectralClustering.h"
#include "thread_pool.h"
extern "C" {
#include "gperftools-httpd.h"
}

#include <istream>
#include <streambuf>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <set>
#include <algorithm>
#include <thread>
#include <atomic>
#include <mutex>
#include <math.h>
#include <iomanip>

using namespace std;

size_t HISTOGRAM_BAR_NUM = 1000;
size_t TOPK = 10000;
int CLUSTER_NUM = 3;

vector<string> g_header;
vector<FieldType> g_field_types;

unordered_map<int, string> g_field_type_map {
    {FIELD_TYPE_NAME, "name"},
    {FIELD_TYPE_PHONE, "phone"},
    {FIELD_TYPE_MIX, "mix"},
    {FIELD_TYPE_CHINESE, "chinese"},
    {FIELD_TYPE_ENGLISH, "english"},
    {FIELD_TYPE_NUMBER, "number"},
    {FIELD_TYPE_EMPTY, "empty"},
};

int main(int argc, char const *argv[]) {
    ghttpd();

    for (int i = 1; i < argc; i++) {
        if (i == 1) HISTOGRAM_BAR_NUM = atoi(argv[i]);
        else if (i == 2) TOPK = atoi(argv[i]);
        else if (i == 3) CLUSTER_NUM = atoi(argv[i]);
    }

    vector<vector<string>> data;
    errorcode err_code = read_csv("./test.csv", g_header, data);
    if (err_code != E_OK) {
        return err_code;
    }

    vector<vector<int>> clusters = process(data);

    // output clustered items
    // items are ordered according to distance from cluster centre
    for (unsigned int i=0; i < clusters.size(); i++) {
        std::cout << "Cluster " << i << ": " << "Item ";
        for (auto it = clusters[i].begin(); it != clusters[i].end(); it++) {
            cout << g_header[*it] << ", ";
        }
        // std::copy(clusters[i].begin(), clusters[i].end(), std::ostream_iterator<int>(std::cout, ", "));
        std::cout << std::endl;
    }
    return 0;
}

errorcode read_csv(string file_name, vector<string>& header, vector<vector<string>>& data) {
    ifstream csv_data(file_name, ios::in);
    if (!csv_data.is_open()) {
        cout << "Error: opening file fail" << endl;
        return E_OPENFAIL;
    }

    // 读取标题行
    string line;
    string field;
    istringstream sin;
    getline(csv_data, line);
    sin.str(line);
    while (getline(sin, field, ',')) {
        header.push_back(field);
    }

    vector<string> row;
    // 读取数据
    while (getline(csv_data, line)) {
        sin.clear();
        sin.str(line);
        row.clear();
        while (getline(sin, field, ',')) {
            field = trim(field);
            if (field == "null" || field == "NULL" || field == "\"\"") {
                field.clear();
            }
            row.push_back(field);
        }
        data.push_back(row);
    }
    csv_data.close();
    return E_OK;
}

vector<vector<int>> process(const vector<vector<string>>& data) {
    unordered_map<int, vector<string>> labelVectorMap{}; // columnId -> label_vector
    unordered_map<int, unordered_map<string, vector<int>>> labelFreqMap{};

    ClassifyField cf;
    SegmentByDelimiter seg_mix;
    SegmentChinese seg_ch;
    SegmentByDelimiter seg_en;
    SpendTime st;
    mutex mtx_string;
    vector<future<void>> futures;
    ThreadPool tp(thread::hardware_concurrency());
    tp.init();
    cout << "parsing ..." << endl;
    size_t row = data.size();
    size_t col = data[0].size();
    g_field_types.resize(col);
    for (size_t j = 0; j < col; j++) {
        futures.push_back(tp.submit([&](int j){
            set<FieldType> ft_set;
            vector<vector<string>> tokens(row);
            unordered_map<string, int64_t> tokenFreq{};
            bool is_all_number = true;
            for (size_t i = 0; i < row; i++) {
                auto ft = cf.classify(data[i][j]);
                ft_set.insert(ft);
                if (ft == FIELD_TYPE_NUMBER) {
                    tokens[i] = vector<string>{data[i][j]};
                    tokenFreq[data[i][j]]++;
                    continue;
                }
                is_all_number = false;

                vector<string> words;
                switch (ft) {
                case FIELD_TYPE_MIX:
                    if (cf.is_mask_phone(data[i][j])) words.push_back(data[i][j]);
                    else seg_mix.segment(data[i][j], words);
                    break;
                case FIELD_TYPE_CHINESE:
                    if (cf.is_mask_name(data[i][j])) words.push_back(data[i][j]);
                    else seg_ch.segment(data[i][j], words);
                    break;
                case FIELD_TYPE_ENGLISH:
                    seg_en.segment(data[i][j], words);
                    break;
                default:
                    words.push_back(data[i][j]);
                }
                tokens[i] = words;
                for (auto &&word : words) tokenFreq[word]++;
            }
            g_field_types[j] = *ft_set.begin();

            if (is_all_number) {
                vector<string> label_vector{};
                unordered_map<string, vector<int>> label_invert_index{};
                genHistogram(data, j, label_vector, label_invert_index);
                mtx_string.lock();
                labelVectorMap[j] = label_vector;
                labelFreqMap[j] = label_invert_index;
                mtx_string.unlock();
                return ;
            }

            vector<TokenCount> tc{};    
            for (auto &&token : tokenFreq) {
                if (token.first.size() == 0) continue;
                tc.push_back(TokenCount(token.second, token.first));
            }
            stable_sort(tc.begin(), tc.end(), [](TokenCount t1, TokenCount t2) {return t1._count > t2._count;});
            unordered_map<string, int64_t> labels{};
            size_t tc_size = tc.size();
            for (size_t i = 0; i < TOPK && i < tc_size; i++) {
                labels[tc[i]._token] = i+1;
            }

            vector<string> label_vector{};
            unordered_map<string, vector<int>> label_invert_index{};
            convertField2Label(tokens, labels, label_vector, label_invert_index);

            mtx_string.lock();
            labelVectorMap[j] = label_vector;
            labelFreqMap[j] = label_invert_index;
            mtx_string.unlock();
        }, j));
    }
    for (auto &&future : futures) {
        future.get();
    }
    tp.shutdown();
    st.print("parse cost: ");

    for (size_t i = 0; i < g_field_types.size(); i++) {
        cout << g_header[i] << ": " << g_field_type_map[g_field_types[i]] << "\t";
    }
    cout << endl;

    st.start();
    cout << "generating correlation matrix ..." << endl;
    vector<vector<double>> correlation = genCorrelationMatrix(data, labelVectorMap, labelFreqMap);
    cout << setiosflags(ios::fixed) << setprecision(2);
    cout << "\t";
    for (size_t j = 0; j < correlation[0].size(); j++) {
        cout << j << "\t";
    }
    cout << endl;
    for (size_t i = 0; i < correlation.size(); i++) {
        cout << i << "\t";
        for (size_t j = 0; j < correlation[i].size(); j++) {
            // char tmp[10];
            // sprintf(tmp, "%02d_%02d:", i, j);
            cout << correlation[i][j] << "\t";
        }
        cout << endl;
    }
    st.print("generate correlation matrix cost: ");

    st.start();
    cout << "spectral clustering ..." << endl;
    vector<vector<int>> clusters = doSpectralClustering(correlation);
    st.print("spectral clustering cost: ");

    return clusters;
}

bool not_histogram(int j) {
    return g_header[j].find("id") != string::npos || g_header[j].find("no") != string::npos;
}

void origin_normalize_invert_index(const vector<vector<string>>& data, int j, vector<string>& label_vector, unordered_map<string, vector<int>>& label_invert_index) {
    size_t row = data.size();
    for (size_t i = 0; i < row; i++) {
        label_vector.push_back(data[i][j]);
        label_invert_index[data[i][j]].push_back(i);
    }
}

void genHistogram(const vector<vector<string>>& data, int j, vector<string>& label_vector, unordered_map<string, vector<int>>& label_invert_index) {
    size_t row = data.size();
    if (not_histogram(j)) {
        origin_normalize_invert_index(data, j, label_vector, label_invert_index);
        return ;
    }

    vector<double> number;
    vector<double> histogram{};
    for (size_t i = 0; i < row; i++) {
        number.push_back(atof(data[i][j].c_str()));
    }
    vector<double> sorted_number = number;
    stable_sort(sorted_number.begin(), sorted_number.end(), [](double t1, double t2) {return t1 < t2;});
    auto it = unique(sorted_number.begin(), sorted_number.end());
    size_t unique_len = it - sorted_number.begin();
    if (unique_len <= HISTOGRAM_BAR_NUM) {
        origin_normalize_invert_index(data, j, label_vector, label_invert_index);
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
    size_t number_size = number.size();
    for (size_t i = 0; i < number_size; i++) {
        auto it = upper_bound(histogram.begin(), histogram.end(), number[i]);
        it--;
        auto tmp = to_string(*it);
        label_vector.push_back(tmp);
        label_invert_index[tmp].push_back(i);
    }
}

void convertField2Label(const vector<vector<string>>& tokens, unordered_map<string, int64_t>& labels, vector<string>& label_vector, unordered_map<string, vector<int>>& label_invert_index) {
    size_t token_size = tokens.size();
    for (size_t i = 0; i < token_size; i++) {
        vector<string> tmp{};
        for (size_t j = 0; j < tokens[i].size(); j++) {
            // if (labels.count(tokens[i][j]) > 0) tmp.push_back(labels[tokens[i][j]]);
            if (labels.count(tokens[i][j]) > 0) tmp.push_back(tokens[i][j]);
        }
        if (tmp.size() == 0) {
            label_vector.push_back({});
            label_invert_index[{}].push_back(i);
            continue;
        }
        // stable_sort(tmp.begin(), tmp.end(), [](int64_t t1, int64_t t2) {return t1 < t2;});
        string label{tmp[0]};
        std::for_each (tmp.begin()+1, tmp.end(), [&](string i) {label += "_" + i;});
        label_vector.push_back(label);
        label_invert_index[label].push_back(i);
    }
}

ClassifyField::ClassifyField(/* args */) : 
    pattern_number("^[-]*[0-9]*[\\.]?[0-9]*$"), 
    pattern_chinese_english(L"^[a-zA-Z\\u4e00-\\u9fa5 ]+$"),
    pattern_has_chinese(L"^.*[\\u4e00-\\u9fa5]+.*$"), 
    pattern_english("^[a-zA-Z ]+$"),
    pattern_time("((([0-9]{3}[1-9]|[0-9]{2}[1-9][0-9]{1}|[0-9]{1}[1-9][0-9]{2}|[1-9][0-9]{3})/(((0[13578]|1[02])-(0[1-9]|[12][0-9]|3[01]))|((0[469]|11)/(0[1-9]|[12][0-9]|30))|(02-(0[1-9]|[1][0-9]|2[0-8]))))|((([0-9]{2})(0[48]|[2468][048]|[13579][26])|((0[48]|[2468][048]|[3579][26])00))-02-29))\\s+([0-1]?[0-9]|2[0-3]):([0-5][0-9]):([0-5][0-9])"),
    pattern_mask_name(L"^[\\u4e00-\\u9fa5]{1,2}[*]{1,2}"),
    pattern_mask_phone("[0-9]{3}[*]{4}[0-9]{4}") {}

FieldType ClassifyField::classify(const string& field) {
    if (field.size() == 0) return FIELD_TYPE_EMPTY;

    if (is_mask_name(field)) return FIELD_TYPE_NAME;
    if (is_mask_phone(field)) return FIELD_TYPE_PHONE;

    if (regex_match(field, pattern_number)) return FIELD_TYPE_NUMBER;

    if (regex_match(to_wide_string(field), pattern_has_chinese)) return FIELD_TYPE_CHINESE;

    if (regex_match(field, pattern_english)) return FIELD_TYPE_ENGLISH;

    return FIELD_TYPE_MIX;
}

bool ClassifyField::is_mask_name(const string& field) const {
    return regex_match(to_wide_string(field), pattern_mask_name);
}

bool ClassifyField::is_mask_phone(const string& field) const {
    return regex_match(field, pattern_mask_phone);
}

vector<vector<double>> genCorrelationMatrix(const vector<vector<string>>& data, unordered_map<int, vector<string>>& labelVectorMap, unordered_map<int, unordered_map<string, vector<int>>>& labelFreqMap) {
    size_t numDims = data[0].size();
    vector<vector<double>> correlation(numDims);
    for (size_t i = 0; i < numDims; i++) correlation[i].resize(numDims);

    vector<future<void>> futures;
    ThreadPool tp(thread::hardware_concurrency());
    tp.init();
    for (size_t i = 0; i < numDims; i++) {
        for (size_t j = i; j < numDims; j++) {
            if (i == j) {
                correlation[i][j] = 1;
                continue;
            }
            futures.push_back(tp.submit([&](int i, int j){
                double sum = 0;
                // int64_t total = 0;
                // if (field_types[i] == FIELD_TYPE_CHINESE && field_types[j] == FIELD_TYPE_CHINESE) {
                //     for (size_t k = 0; k < labelVectorMap[i].size(); k++) {
                //         // if (data[i][k].size() > data[j][k].size()) {
                //         //     labelVectorMap
                //         // }
                //         if (contains(labelVectorMap[i][k], labelVectorMap[j][k])) total++;
                //     }
                // }
                sum = calcCorrelationSum(i, j, labelVectorMap, labelFreqMap, numDims);
                // sum += total / double(data.size());
                // if (sum > 1) sum = 1;

                correlation[i][j] = sum;
                correlation[j][i] = sum;
            }, i, j));
        }
    }
    for (auto &&future : futures) {
        future.get();
    }
    tp.shutdown();

    return correlation;
}

double calcCorrelationSum(int i, int j, unordered_map<int, vector<string>>& labelVectorMap, unordered_map<int, unordered_map<string, vector<int>>>& labelFreqMap, int64_t sigma) {
    int ft1 = g_field_types[i];
    int ft2 = g_field_types[j];
    vector<string>& v1 = labelVectorMap[i];
    unordered_map<string, vector<int>>& f1 = labelFreqMap[i];
    vector<string>& v2 = labelVectorMap[j];
    unordered_map<string, vector<int>>& f2 = labelFreqMap[j];
    size_t v1_size = v1.size();
    size_t v2_size = v2.size();
    size_t not_empty_row = 0;
    for (size_t k = 0; k < v1_size; k++) {
        if (v1[k].size() == 0 && v2[k].size() == 0) continue;
        not_empty_row++;
    }
    double threshold = double(1) / not_empty_row;
    unordered_map<string, int> visited;
    double sum = 0;
    for (size_t k = 0; k < v1_size; k++) {
        if (v1[k].size() == 0 && v2[k].size() == 0) continue;
        string visited_key = getVisitedKey(v1[k], v2[k]);
        if (visited.count(visited_key) > 0) continue;

        // 行号都是有序的
        vector<int>& rows_i = f1[v1[k]];
        vector<int>& rows_j = f2[v2[k]];
        // vector<int> result;
        // set_intersection(rows_i.begin(), rows_i.end(), rows_j.begin(), rows_j.end(), back_inserter(result));
        size_t i = 0, j = 0;
        int64_t intersect_size = 0;
        size_t rows_i_size = rows_i.size();
        size_t rows_j_size = rows_j.size();
        while (i < rows_i_size && j < rows_j_size) {
            if (rows_i[i] == rows_j[j]) {
                intersect_size++;
                i++;
                j++;
            } else if (rows_i[i] < rows_j[j]) {
                i++;
            } else {
                j++;
            }
        }
        if (intersect_size > 0) {
            double p_xy = intersect_size / double(not_empty_row);
            double p_x = rows_i_size / double(not_empty_row);
            double p_y = rows_j_size / double(not_empty_row);
            sum += contribute(ft1, ft2, v1[k], v2[k], p_xy, p_x, p_y, threshold);
        }
        visited[visited_key]++;
    }
    // sum = exp(-sum/(2*sigma*sigma));
    // if (sum < 0.0001) sum = 0;
    return sum > 1 ? 1 : sum;
}

double contribute(int field_type1, int field_type2, const string& v1, const string& v2, double p_xy, double p_x, double p_y, double threshold) {
    bool name_phone1 = (field_type1 == FIELD_TYPE_NAME) || (field_type1 == FIELD_TYPE_PHONE);
    bool name_phone2 = (field_type2 == FIELD_TYPE_NAME) || (field_type2 == FIELD_TYPE_PHONE);
    if (name_phone1 && name_phone2) {
        return p_xy;
    }
    if (name_phone1 || name_phone2) {
        return 0;
    }

    if (contains(v1, v2)) return 2*p_xy;

    if (p_x > 0.5 && p_y > 0.5) return p_xy;

    if (p_x > 0.5 || p_y > 0.5) return 0;
    
    return (p_xy > threshold) ? p_xy : 0;
}

string getVisitedKey(const string& v1, const string& v2) {
    return v1 + "_" + v2;
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
    SpectralClustering* c = new SpectralClustering(m, CLUSTER_NUM);

    // whether to use auto-tuning spectral clustering or kmeans spectral clustering
    bool autotune = false;

    vector<vector<int> > clusters;
    if (autotune) {
        // auto-tuning clustering
        clusters = c->clusterRotate();
    } else {
        // how many clusters you want
        int numClusters = CLUSTER_NUM;
        clusters = c->clusterKmeans(numClusters);
    }

    return clusters;
}
