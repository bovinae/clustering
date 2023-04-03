#ifndef _SEGMENTATION_H
#define _SEGMENTATION_H

#include "cppjieba/Jieba.hpp"

#include <iostream>
#include <string>
#include <vector>

using std::cout;
using std::endl;
using std::string;
using std::vector;

const char * const DICT_PATH = "/data/linwei/clustering/cppjieba/dict/jieba.dict.utf8";//最大概率法(MPSegment: Max Probability)分词所使用的词典路径
const char * const HMM_PATH = "/data/linwei/clustering/cppjieba/dict/hmm_model.utf8";//隐式马尔科夫模型(HMMSegment: Hidden Markov Model)分词所使用的词典路径
const char * const USER_DICT_PATH = "/data/linwei/clustering/cppjieba/dict/user.dict.utf8";//用户自定义词典路径
const char* const IDF_PATH = "/data/linwei/clustering/cppjieba/dict/idf.utf8";//IDF路径
const char* const STOP_WORD_PATH = "/data/linwei/clustering/cppjieba/dict/stop_words.utf8";//停用词路径

class SegmentChinese // 使用结巴分词库进行分词
{
public:
    SegmentChinese()
        : _jieba(DICT_PATH, HMM_PATH, USER_DICT_PATH,IDF_PATH,STOP_WORD_PATH){}

    vector<string> operator()(const string str) {
        vector<string> words{};
        _jieba.Cut(str, words); // FullSegment
        return words;
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

    vector<string> operator()(string str) {
        int32_t pre_pos = -1;
        vector<string> words{};
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
        return words;
    }

private:
    unordered_set<char> _delimiters;
};

#endif
