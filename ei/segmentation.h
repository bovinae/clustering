#ifndef _SEGMENTATION_H
#define _SEGMENTATION_H

#include "cppjieba/Jieba.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <set>

using namespace std;
using std::cout;
using std::endl;
using std::string;
using std::vector;

const char * const DICT_PATH = "/data/linwei/clustering/cppjieba/dict/jieba.dict.utf8";//最大概率法(MPSegment: Max Probability)分词所使用的词典路径
const char * const HMM_PATH = "/data/linwei/clustering/cppjieba/dict/hmm_model.utf8";//隐式马尔科夫模型(HMMSegment: Hidden Markov Model)分词所使用的词典路径
const char * const USER_DICT_PATH = "/data/linwei/clustering/cppjieba/dict/user.dict.utf8";//用户自定义词典路径
const char* const IDF_PATH = "/data/linwei/clustering/cppjieba/dict/idf.utf8";//IDF路径
const char* const STOP_WORD_PATH = "/data/linwei/clustering/cppjieba/dict/stop_words.utf8";//停用词路径


std::string& trim(std::string &s) {
    if (s.empty()) {
        return s;
    }
 
    s.erase(0,s.find_first_not_of(" "));
    s.erase(s.find_last_not_of(" ") + 1);
    return s;
}

class SegmentChinese // 使用结巴分词库进行分词
{
public:
    SegmentChinese()
        : _jieba(DICT_PATH, HMM_PATH, USER_DICT_PATH,IDF_PATH,STOP_WORD_PATH){}

    void segment(const string& str, vector<string>& words) {
        _jieba.Cut(str, words); // FullSegment
        for (auto it = words.begin(); it != words.end(); ) {
            // if (it->size() == 0 || trim(*it).size() == 0 || *it == "*" || *it == "\"") {
            if (it->length() <= 3 || trim(*it).length() <= 3) {
                it = words.erase(it);
            } else {
                it++;
            }
        }
    }
private:
    cppjieba::Jieba _jieba;
};

class SegmentByDelimiter // 使用空格进行分词
{
public:
    SegmentByDelimiter(){
        const string& special_chars = "!#$%&()+;<=>?@[]\\^`{}|~"; // "!#$%&()*+-/:;<=>?@[]\\^_`{}|~"
        for (auto &&c : special_chars) _delimiters.insert(c);
    }

    void segment(const string& str, vector<string>& words) {
        int32_t pre_pos = -1;
        size_t i = 0;
        for (; i < str.size(); i++) {
            if (_delimiters.count(str[i]) == 0) {
                if (pre_pos == -1) pre_pos = i;
                continue;
            }
            if (pre_pos >= 0) {
                words.push_back(str.substr(pre_pos, i - pre_pos));
                pre_pos = -1;
            }
        }
        if (pre_pos >= 0) words.push_back(str.substr(pre_pos, i - pre_pos));
    }

private:
    unordered_set<char> _delimiters;
};

#endif
