#pragma once
#include <vector>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <sstream>
#include <iomanip>
namespace precision {
static_assert(sizeof(float)==4&&std::numeric_limits<float>::is_iec559,"IEEE binary32 required");
inline std::uint32_t bits(float x){std::uint32_t u;std::memcpy(&u,&x,4);return u;}
inline float from_bits(std::uint32_t u){float x;std::memcpy(&x,&u,4);return x;}
inline std::uint32_t round_even(std::uint32_t n,unsigned shift){if(shift==0)return n;if(shift>31)return 0;auto truncated=n>>shift;auto rem=n&((1U<<shift)-1);auto mid=1U<<(shift-1);return truncated+(rem>mid||(rem==mid&&(truncated&1)));}
inline std::uint16_t half_bits(float x){auto u=bits(x);auto sign=(u>>16)&0x8000U,exp=(u>>23)&255U,mant=u&0x7fffffU;if(exp==255)return static_cast<std::uint16_t>(sign|(mant?0x7e00:0x7c00));int e=static_cast<int>(exp)-127+15;if(e>=31)return static_cast<std::uint16_t>(sign|0x7c00);if(e<=0){if(e < -10)return static_cast<std::uint16_t>(sign);auto rounded=round_even(mant|0x800000U,static_cast<unsigned>(14-e));return static_cast<std::uint16_t>(sign|rounded);}auto rounded=round_even(mant,13);if(rounded==1024){rounded=0;++e;}return static_cast<std::uint16_t>(sign|(static_cast<unsigned>(e)<<10)|rounded);}
inline float half_value(std::uint16_t h){auto sign=static_cast<std::uint32_t>(h&0x8000U)<<16;unsigned e=(h>>10)&31,m=h&1023;if(e==31)return from_bits(sign|0x7f800000U|(m?0x400000U:0));if(e==0){if(!m)return from_bits(sign);float x=std::ldexp(static_cast<float>(m),-24);return sign?-x:x;}return from_bits(sign|((e+112)<<23)|(m<<13));}
inline float half(float x){return half_value(half_bits(x));}
inline void bounded(const std::vector<float>&x){if(x.empty()||x.size()>4096)throw std::runtime_error("input needs 1..4096 binary32 values");}
inline double reference_sum(const std::vector<float>&x){bounded(x);long double s=0;for(float v:x)s+=static_cast<long double>(v);return static_cast<double>(s);}
inline double sum(const std::vector<float>&x,bool binary16){bounded(x);float s=0;for(float v:x){if(binary16)s=half(half(s)+half(v));else {volatile float next=s+v;s=next;}}return s;}
struct Comparison{bool match;std::string classification;double abs_error,allowed;};
inline Comparison compare(double expected,double actual,double abs_tol,double rel_tol){if(!std::isfinite(abs_tol)||!std::isfinite(rel_tol)||abs_tol<0||rel_tol<0)throw std::runtime_error("invalid tolerance");if(std::isnan(expected)||std::isnan(actual))return {std::isnan(expected)&&std::isnan(actual),"nan_classification",0,0};if(std::isinf(expected)||std::isinf(actual))return {expected==actual,"infinity_classification",0,0};double error=std::abs(expected-actual),allowed=abs_tol+rel_tol*std::abs(expected);return {error<=allowed,"finite",error,allowed};}
inline bool mismatch(const std::vector<float>&x,double a,double r){return !compare(reference_sum(x),sum(x,true),a,r).match;}
inline std::vector<float> without(const std::vector<float>&x,std::size_t skip){std::vector<float> y;for(std::size_t j=0;j<x.size();++j)if(j!=skip)y.push_back(x[j]);return y;}
inline std::vector<float> minimize(std::vector<float>x,double a,double r){bounded(x);if(!mismatch(x,a,r))throw std::runtime_error("input does not reproduce mismatch");bool changed=true;while(changed&&x.size()>1){changed=false;for(std::size_t i=0;i<x.size();++i){auto y=without(x,i);if(mismatch(y,a,r)){x=std::move(y);changed=true;break;}}}return x;}
inline std::string number(double x){if(std::isnan(x))return "\"NaN\"";if(std::isinf(x))return x<0?"\"-Infinity\"":"\"Infinity\"";std::ostringstream s;s<<std::setprecision(17)<<x;return s.str();}
inline std::string passport(const std::vector<float>&original,double a,double r){auto reduced=minimize(original,a,r);auto reference=reference_sum(reduced),low=sum(reduced,true),wide=sum(reduced,false);auto c=compare(reference,low,a,r);std::ostringstream s;s<<"{\"schema_version\":1,\"kind\":\"cpu_numeric_differential_lab\",\"graph\":\"sequential_sum\",\"reference\":\"long_double_accumulation\",\"candidate\":\"software_binary16_round_each_add\",\"rounding\":\"nearest_ties_even\",\"metal_tested\":false,\"coreml_tested\":false,\"device\":\"host_cpu\",\"compiler\":\""<<__VERSION__<<"\",\"cpp_standard\":"<<__cplusplus<<",\"long_double_mantissa_bits\":"<<std::numeric_limits<long double>::digits<<",\"absolute_tolerance\":"<<number(a)<<",\"relative_tolerance\":"<<number(r)<<",\"original_elements\":"<<original.size()<<",\"reduced_elements\":"<<reduced.size()<<",\"minimality\":\"one_element_deletion\",\"reference_result\":"<<number(reference)<<",\"candidate_result\":"<<number(low)<<",\"float32_result\":"<<number(wide)<<",\"classification\":\""<<c.classification<<"\",\"absolute_error\":"<<number(c.abs_error)<<",\"float32_control_matches\":"<<(compare(reference,wide,a,r).match?"true":"false")<<",\"input_binary32_hex\":[";for(std::size_t i=0;i<reduced.size();++i){if(i)s<<',';s<<'"'<<std::hex<<std::setw(8)<<std::setfill('0')<<bits(reduced[i])<<'"';}s<<"]}";return s.str();}
}
