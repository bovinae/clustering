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
    SegmentByDelimiter(){}
    void SetDelimiter(const string& delimiter) {
        _delimiter = delimiter;
    }

    vector<string> operator()(string str) {
        size_t pos = 0;
        vector<string> words{};
        while ((pos = str.find(_delimiter)) != string::npos) {
            words.push_back(str.substr(0, pos));
            str.erase(0, pos + _delimiter.size());
        }
        return words;
    }

private:
    string _delimiter = " ";
};

#endif
