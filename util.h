#ifndef util_H
#define util_H 1
#include <iostream>
#define info_out(X) std::cout<<"==> "<<__LINE__<<" "<<#X<<" |"<<(X)<<"|\n"
#define debug_out(X) std::cerr<<"==> "<<__LINE__<<" "<<#X<<" |"<<(X)<<"|\n"
#include <string>
#include <tuple>
#include <string>
#include <array>
#include <utility>
#include <sstream>
#include <stdexcept>
#include <concepts>
#include <climits>
#include <mutex>
#include <chrono>
#include <iomanip>

#include <QColor>
#include <QTextEdit>
#include <QLayout>
#include <QSerialPortInfo>

#include "modbus.h"
namespace util{
void trim_space(std::string&);

template <class _tp>
std::vector<_tp> str_to_values(std::string const& x){
  if(x.empty()) return {};
  std::vector<_tp> store;
  std::string xc = x;
  trim_space(xc);
  std::istringstream isstr(xc.c_str());
  _tp begin; isstr>>begin;
  if (isstr.fail()) return store;
  store.emplace_back(begin);
  while(!isstr.fail()) {
      isstr>>begin;
      if (!isstr.fail())
          store.emplace_back(begin);
  }
  return store;
}

template <class _tp>
std::string float_str(_tp v, int fl){
  std::ostringstream osstr; osstr<<std::setprecision(fl);
  osstr<<std::to_string(fl).c_str();
  return osstr.str(); }

static std::mutex g_mutex;

struct modbus_parameters_t{
  int baud = 19200;
  char parity = 'N';
  int data_bit = 8;
  int stop_bit = 2; };

static std::string const item_names[] = {
    "time"
    ,"run_time"
    ,"temperature"
    ,"humidity"
    ,"set_v0"
    ,"set_v1"
    ,"set_v2"
    ,"set_v3"
    ,"v0"
    ,"v1"
    ,"v2"
    ,"v3"
    ,"i0"
    ,"i1"
    ,"i2"
    ,"i3"
};
template <class _tp>
std::string str_voltage(_tp value, std::string const& unit="V"){
    std::stringstream sstr;
    sstr<<std::fixed<<std::setprecision(2)<<value;
    if (unit.empty()){
      return sstr.str();
    }
    else if(unit=="V"){
      sstr<<" V";
      return sstr.str();
    }else if(unit=="kV"){

    }
}
template <class _tp>
std::string str_current(_tp value, std::string const& unit="nA"){
    std::stringstream sstr;
    sstr<<std::fixed<<std::setprecision(2)<<value;
    if (unit.empty()){
      return sstr.str();
    }
    else if(unit=="nA"){
      sstr<<" nA";
      return sstr.str();
    }else if(unit=="uA"){

    }else if(unit=="mA"){

    }else if(unit=="A"){

    }
}
template <class _tp>
std::string str_temperature(_tp value, std::string const& unit="°C"){
    std::stringstream sstr;
    sstr<<std::fixed<<std::setprecision(2)<<value;
    if (unit.empty()){
      return sstr.str();
    }
    else if(unit=="°C"){
      sstr<<" °C";
      return sstr.str();
    }else if(unit=="°F"){
    }else if(unit=="K"){
    }
    return "";
}
template <class _tp>
std::string str_humlity(_tp value, std::string const& unit="%"){
  std::stringstream sstr;
  sstr<<std::fixed<<std::setprecision(2)<<value;
  if (unit.empty()){
    return sstr.str();
  }
  else if(unit=="%"){
      sstr<<" %";
        return sstr.str();
    }
    return "";
}

std::string uint2timestr(uint32_t);

static const char blank_str[] =
    QT_TRANSLATE_NOOP("mainwindow", "N/A");

static std::array<QColor,3> color_palette = { 
  QColor{"#07B2FC"}
  ,QColor{"#534F02"}
  ,QColor{"#F80F05"}
};

enum class log_mode : size_t{ k_info = 0 ,k_warning ,k_error };

std::string qlayout_sizeconstraint_name(QLayout::SizeConstraint);

std::string format_time(std::chrono::system_clock::duration,double);

std::string rename(std::string const&);
std::string pwd();
std::string get_str(uint16_t const* addr);
float get_bmp280(uint16_t *, size_t=6);

float get_bmp280_1(uint16_t*);

float to_float(uint16_t* view);

void t_msleep(size_t);

uint16_t hexstr2u16(std::string const&);
std::string u162hexstr(uint16_t);

std::string timestamp();

int64_t highrestime_ns();
int64_t highrestime_ms();

std::string time_to_str();
std::string time_to_str_s();

std::array<std::string,4> uint2_dhms(uint32_t);

std::pair<modbus_t*,bool> connect_modbus(
    char const* device
    ,int baud
    ,int parity
    ,int data_bit
    ,int stop_bit
    ,int const slave_addr);

template <class _tp, class... args>
void to_stream(_tp& os,args&&... params){
  auto const& to_stream_impl = []<class _sp, class _tup>(_sp& os, _tup const& tup){
    constexpr std::size_t NN = std::tuple_size_v<_tup>;
    [&]<size_t... I>(std::index_sequence<I...>){
      auto* addr = std::addressof(os);
      (...,(*addr<<" "<<std::get<I>(tup)));
    }(std::make_index_sequence<NN>()); };
  to_stream_impl(os,std::make_tuple(std::forward<args>(params)...)); }

template <class _tp=QTextEdit, log_mode _ev=log_mode::k_info,class... args>
_tp* info_to(_tp* qtext, args&&... params){
  if (qtext->document()->lineCount() >= 501) qtext->clear();
  qtext->setTextColor(color_palette[(size_t)_ev]);
  std::stringstream sstr;
  to_stream(sstr
      ,_ev==log_mode::k_info ? "[INFO] " : _ev==log_mode::k_warning ? "[WARN] " : "[ERROR]"
      ,'[',util::time_to_str(),']'
      ,std::forward<args>(params)...);
  {std::lock_guard<std::mutex> lock(g_mutex); qtext->append(QString{sstr.str().c_str()});}
  return qtext; }

template <std::integral _tp>
inline bool bit(_tp const v,uint8_t addr=0){
  if (addr>=sizeof(_tp)*__CHAR_BIT__) throw std::out_of_range("");
  _tp mask = v>>addr;
  return mask&1; }

template <std::integral _tp>
inline void bit(_tp& v,uint8_t addr,bool s){
  if (addr>=sizeof(_tp)*__CHAR_BIT__) throw std::out_of_range("");
  _tp mask = _tp(1);
  if (s) v |= (mask<<addr);
  else v &= ~(mask<<addr);
}

template <std::integral _tp>
inline void bit_ot(_tp& v, uint8_t addr){
    //__atrribute__((unused(v)));
    //__atrribute__((unused(addr)));
}

template <std::integral _tp>
inline void bit_sw(_tp& a,_tp& b){ a^=b; b^=a; a^=b; }

std::string state_ss(bool v);

std::vector<std::string> get_ports_name();

template <class _iterx, class _itery>
std::pair<double,double> least_squart_line_fit(
    _iterx first, _iterx last, _itery yfirst, _itery ylast){
  auto cd = std::min(std::distance(first,last),std::distance(yfirst,ylast));
  if(cd==0) return std::make_pair(0.,0.);
  auto const& ave = []<class _tp>(_tp f, _tp l)->double{
    return std::accumulate(f,l,0.)/std::distance(f,l); };
  double ave_x = ave(first,last);
  double ave_y = ave(yfirst,ylast);
  double sum_xy=0., sum_xx=0.;
  for (size_t i=0; i<(size_t)cd; ++i) sum_xy += *first++**yfirst++;
  std::advance(first,-cd);
  for (size_t i=0; i<(size_t)cd; ++i) sum_xx += *first**first++;
  std::pair<double,double> rt;
  rt.first = (sum_xy-cd*ave_x*ave_y)/(sum_xx-cd*ave_x*ave_x);
  rt.second = ave_y-rt.first*ave_x;
  return rt;
}
std::string get_local_ip4_address();
}

namespace util{

static std::index_sequence<16,16,16,16> const b2i_indx;
static std::index_sequence<8,8> const b2u16_indx;
static std::index_sequence<16,16> const u162u32_indx;
namespace _meta{

template <class _tp
  ,template <class, class...> class _cont_tt=std::vector
  ,template <class...> class _tup_t=std::tuple
  ,class... _args>
_cont_tt<_tp> unpack(_tup_t<_args...> const& _p0){
  constexpr std::size_t NN = sizeof...(_args);
  _cont_tt<_tp> tmp(NN);
  [&]<std::size_t... I>(std::index_sequence<I...>){
    (...,(tmp.at(I) = static_cast<_tp>(std::get<I>(_p0))));
  }(std::make_index_sequence<NN>()); 
  return tmp; }

template <std::size_t N>
struct size_{
  constexpr std::size_t static value = N; };

template <std::size_t N
  ,std::size_t M
  ,std::size_t value_first
  , std::size_t... _values>
struct get_helper{
  constexpr std::size_t static value = get_helper<N,M+1,_values...>::value; };
template <std::size_t N, std::size_t value_first, std::size_t... _values>
struct get_helper<N,N,value_first,_values...>{
  constexpr std::size_t static value = value_first; };

template <std::size_t N, class...>
struct get;

template <std::size_t N, std::size_t... _values>
requires (N<=(sizeof...(_values)-1))
struct get<N,std::index_sequence<_values...>>{
  constexpr static std::size_t value = get_helper<N,0,_values...>::value;};

template <std::size_t N, std::size_t... _values>
requires (N<=(sizeof...(_values)-1))
struct get_c{
  constexpr static std::size_t value = get_helper<N,0,_values...>::value;};

template <std::size_t _index_l, std::size_t _index_u, std::size_t... _values>
requires (_index_l>=0 && _index_u<=sizeof...(_values) && _index_l<=_index_u)
struct range_sum_helper{
  static constexpr std::size_t value = get_c<_index_l,_values...>::value
    + range_sum_helper<_index_l+1,_index_u,_values...>::value; };
template <std::size_t _index_l, std::size_t... _values>
struct range_sum_helper<_index_l,_index_l,_values...>{
  static constexpr std::size_t value = 0UL; };

template <std::size_t,std::size_t,class... _args>
struct range_sum;

template <std::size_t _index_l, std::size_t _index_u, std::size_t... _values>
struct range_sum<_index_l,_index_u,std::index_sequence<_values...>>{
  static constexpr std::size_t value = 
    range_sum_helper<_index_l,_index_u,_values...>::value; };

template <std::size_t _index_l,std::size_t _index_u,std::size_t... _values>
struct range_sum_c{
  static constexpr std::size_t value = 
    range_sum_helper<_index_l,_index_u,_values...>::value; };

template <class _tp, class _up=_tp, class... _args, std::size_t... _values>
requires std::unsigned_integral<_tp> && (sizeof...(_args)>=sizeof...(_values))
void encode(_tp& _dest,std::index_sequence<_values...>
    ,_args&&... _codes){
  _dest = (std::numeric_limits<_tp>::min)();
  constexpr static std::size_t NN = sizeof...(_values);
  auto codes = unpack<_up>(std::make_tuple(std::forward<_args>(_codes)...));
  [&]<std::size_t... I>(std::index_sequence<I...>){
    (...,(_dest |= codes.at(I) <<(range_sum_c<I+1,NN,_values...>::value)));
  }(std::make_index_sequence<NN>{}); }

template <class _tp, std::size_t N>
requires (std::unsigned_integral<_tp> && N<=__CHAR_BIT__*sizeof(_tp))
_tp decode_helper(_tp const& _p0){
  _tp bs = (std::numeric_limits<_tp>::max)();
  bs >>= sizeof(_tp)*__CHAR_BIT__-N;
  return _p0 & bs; }
template<class _tp, class _up=_tp, std::size_t... _values>
std::array<_up,sizeof...(_values)> decode(_tp const& _from, std::index_sequence<_values...>){
  constexpr static std::size_t NN = sizeof...(_values);
  std::array<_up,NN> tmp{};
  [&]<std::size_t... I>(std::index_sequence<I...>){
    (...,(tmp.at(NN-I-1)=decode_helper<_up,get_c<I,_values...>::value>(static_cast<_up>(_from>>range_sum_c<I+1,NN,_values...>::value))));
  }(std::make_index_sequence<NN>{});
  return tmp; }
}
}
namespace meta = util::_meta;
#endif
