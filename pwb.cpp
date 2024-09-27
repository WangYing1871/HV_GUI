#include <iostream>
#include <sstream>
#include <cfloat>
#include <cmath>
#include <fstream>
#include <map>
#include <unordered_map>

#include <QDebug>
#include <QTime>
#include <QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtCharts/QChartView>
#include <QtCharts/QValueAxis>
#include <QGraphicsLayout>
#include <QtMqtt/QMqttClient>
#include <QNetworkInterface>
#include <QDebug>
#include <Qt>

#include "pwb.h"
#include "util.h"
#include "style_sheet.h"
using namespace Qt::StringLiterals;
namespace {
std::unordered_map<int,std::string> vix_map={
    {0,"VI0X"} ,{1,"VI1X"} ,{2,"VI2X"} ,{3,"VI3X"} };
std::unordered_map<int,std::string> vi50gx_map={
    {0,"VI0X_50G"} ,{1,"VI1X_50G"} ,{2,"VI2X_50G"} ,{3,"VI3X_50G"} };
std::unordered_map<int,std::string> viy_map={
    {0,"VI0Y"} ,{1,"VI1Y"} ,{2,"VI2Y"} ,{3,"VI3Y"} };
std::unordered_map<int,std::string> vi50gy_map={
    {0,"VI0Y_50G"} ,{1,"VI1Y_50G"} ,{2,"VI2Y_50G"} ,{3,"VI3Y_50G"} };
}

int const pwb::s_tolerate_failed = 10;

pwb::pwb(QWidget* p):
  QWidget(p){
  ui.setupUi(this);
  read_config();
  read_config1();
  ui.lineEdit_20->setText(m_version.c_str());
  std::string ipv4 = util::get_local_ip4_address();
  ui.lineEdit_19->setText(ipv4.c_str());
  for(auto&& x : m_monitor_value) x=0.;
  this->setWindowTitle("hv_control");

  m_timers.insert(std::make_pair("monitor",new QTimer));
  m_timers.insert(std::make_pair("dispatch",new QTimer));
  m_timers["monitor"]->setInterval(1000);
  m_timers["dispatch"]->setInterval(1000);

  QObject::connect(m_timers["monitor"],&QTimer::timeout
    ,this,&pwb::Monitor);
  QObject::connect(m_timers["dispatch"],&QTimer::timeout
    ,this,&pwb::dispatch);
  QObject::connect(this,&pwb::setting_channel_s
    ,this,&pwb::setting_channel);
  QObject::connect(ui.pushButton_12,&QAbstractButton::clicked
    ,ui.textBrowser_3,&QTextBrowser::clear);

  ui.lineEdit->setEnabled(false);
  ui.lineEdit_2->setEnabled(false);
  ui.lineEdit_3->setEnabled(false);
  ui.lineEdit_4->setEnabled(false);
  ui.lineEdit_5->setEnabled(false);
  ui.lineEdit_6->setEnabled(false);
  ui.lineEdit_7->setEnabled(false);
  ui.lineEdit_8->setEnabled(false);
  ui.lineEdit_9->setEnabled(false);
  ui.lineEdit_10->setEnabled(false);
  m_button_map[0] = ui.pushButton;
  m_button_map[1] = ui.pushButton_2;
  m_button_map[2] = ui.pushButton_3;
  m_button_map[3] = ui.pushButton_4;
  init_future_tasks();
  set_style_sheet();

  for(auto&& [x,y] : m_set_v_tasks){
    QObject::connect(std::addressof(y.second),&QFutureWatcher<future_task_result_t>::started
      ,[this,&x]{this->set_v_handle_started(x);});
    QObject::connect(std::addressof(y.second),&QFutureWatcher<future_task_result_t>::finished
      ,[this,&x]{ this->set_v_handle_finished(x);});
  }
  init_switch();
  init_canvas();
  reload_serialports();
  QObject::connect(ui.pushButton_10,&QAbstractButton::clicked,this,&pwb::reload_serialports);


  for (auto&& x : m_axis){
    if (x.first.find('x')!=std::string::npos) x.second->setRange(0,100);
    if (x.first.find('y')!=std::string::npos && x.first.find('v')!=std::string::npos) x.second->setRange(0,2000);
    if (x.first.find('y')!=std::string::npos && x.first.find('i')!=std::string::npos) x.second->setRange(-20,100);
  }
  init_mqtt();

}

void pwb::init_mqtt(){
  m_mqtt_pub["PWB/temprature"];
  m_mqtt_pub["PWB/humidity"];
  m_mqtt_pub["PWB/voltage/0"];
  m_mqtt_pub["PWB/voltage/1"];
  m_mqtt_pub["PWB/voltage/2"];
  m_mqtt_pub["PWB/voltage/3"];
  m_mqtt_pub["PWB/current/0"];
  m_mqtt_pub["PWB/current/1"];
  m_mqtt_pub["PWB/current/2"];
  m_mqtt_pub["PWB/current/3"];

  m_mqtt_client = new QMqttClient(this);
  m_mqtt_client->setHostname(ui.lineEdit_19->text());
  m_mqtt_client->setPort(static_cast<quint16>(ui.spinBox_2->value()));

  QObject::connect(ui.lineEdit_19,&QLineEdit::textChanged,m_mqtt_client,&QMqttClient::setHostname);
  QObject::connect(ui.pushButton_11,&QAbstractButton::clicked,this,&pwb::connect_mqtt);
  connect(m_mqtt_client, &QMqttClient::messageReceived, this, [this](const QByteArray &message, const QMqttTopicName &topic) {
        const QString content = QDateTime::currentDateTime().toString()
                    + " Received Topic: "_L1
                    + topic.name()
                    + " Message: "_L1
                    + message
                    + u'\n';
      log(0,content.toStdString());
    });
}

void pwb::set_style_sheet(){
  QString default_css = ui.groupBox->styleSheet();
  ui.groupBox->setStyleSheet(css::group_box_font0);
  ui.groupBox_12->setStyleSheet(default_css);
  ui.groupBox_2->setStyleSheet(css::group_box_font0);
  ui.groupBox_3->setStyleSheet(css::group_box_font0);
  ui.groupBox_4->setStyleSheet(css::group_box_font0);
  ui.groupBox_9->setStyleSheet(css::group_box_font0);
  ui.groupBox_10->setStyleSheet(css::group_box_font0);
  ui.groupBox_11->setStyleSheet(css::group_box_font0);

}
void pwb::setting_channel(bool state, int cidx){
  switch(cidx){
    case(0): ui.pushButton->setEnabled(!state); break;
    case(1): ui.pushButton_2->setEnabled(!state); break;
    case(2): ui.pushButton_3->setEnabled(!state); break;
    case(3): ui.pushButton_4->setEnabled(!state); break;
  }
}

void pwb::reload_serialports(){
  ui.comboBox->clear();
  for(auto&& x : util::get_ports_name())
      ui.comboBox->addItem(x.c_str());
}
void pwb::init_future_tasks(){
  for(int i=0; i<4; ++i) m_set_v_tasks[i], m_set_v_flags[i].store(true);
}

void pwb::init_switch(){
  auto* sbt0 = new SwitchButton(ui.widget);
  connect(sbt0,SIGNAL(statusChanged(bool))
          ,this,SLOT(switch_channel0(bool)));
  auto* sbt1 = new SwitchButton(ui.widget_3);
  connect(sbt1,SIGNAL(statusChanged(bool))
          ,this,SLOT(switch_channel1(bool)));
  auto* sbt2 = new SwitchButton(ui.widget_4);
  connect(sbt2,SIGNAL(statusChanged(bool))
          ,this,SLOT(switch_channel2(bool)));
  auto* sbt3 = new SwitchButton(ui.widget_2);
  connect(sbt3,SIGNAL(statusChanged(bool))
          ,this,SLOT(switch_channel3(bool)));
  auto* sbt4 = new SwitchButton(ui.widget_9);
  connect(sbt4,SIGNAL(statusChanged(bool))
          ,this,SLOT(switch_channel0(bool)));
  auto* sbt5 = new SwitchButton(ui.widget_10);
  connect(sbt5,SIGNAL(statusChanged(bool))
          ,this,SLOT(switch_channel1(bool)));
  auto* sbt6 = new SwitchButton(ui.widget_11);
  connect(sbt6,SIGNAL(statusChanged(bool))
          ,this,SLOT(switch_channel2(bool)));
  auto* sbt7 = new SwitchButton(ui.widget_12);
  connect(sbt7,SIGNAL(statusChanged(bool))
          ,this,SLOT(switch_channel3(bool)));
  sbt0->set_friend(sbt4);
  sbt1->set_friend(sbt5);
  sbt2->set_friend(sbt6);
  sbt3->set_friend(sbt7);
  m_switch_buttons.insert(std::make_pair(0,sbt0));
  m_switch_buttons.insert(std::make_pair(1,sbt1));
  m_switch_buttons.insert(std::make_pair(2,sbt2));
  m_switch_buttons.insert(std::make_pair(3,sbt3));
  m_switch_buttons.insert(std::make_pair(4,sbt4));
  m_switch_buttons.insert(std::make_pair(5,sbt5));
  m_switch_buttons.insert(std::make_pair(6,sbt6));
  m_switch_buttons.insert(std::make_pair(7,sbt7));
  for (auto&& [_,x] : m_switch_buttons ) x->setGeometry(0,0,50,22);
  connect(ui.pushButton,&QAbstractButton::clicked,this,&pwb::set_channel0);
  connect(ui.pushButton_2,&QAbstractButton::clicked,this,&pwb::set_channel1);
  connect(ui.pushButton_3,&QAbstractButton::clicked,this,&pwb::set_channel2);
  connect(ui.pushButton_4,&QAbstractButton::clicked,this,&pwb::set_channel3);
  connect(ui.pushButton_15,&QAbstractButton::clicked,this,&pwb::set_channel0_hex);
  connect(ui.pushButton_17,&QAbstractButton::clicked,this,&pwb::set_channel1_hex);
  connect(ui.pushButton_18,&QAbstractButton::clicked,this,&pwb::set_channel2_hex);
  connect(ui.pushButton_16,&QAbstractButton::clicked,this,&pwb::set_channel3_hex);
  connect(ui.pushButton_5,&QAbstractButton::clicked,this,&pwb::start);
  connect(ui.pushButton_6,&QAbstractButton::clicked,this,&pwb::stop);
  connect(ui.pushButton_9,&QAbstractButton::clicked,this,&pwb::ConnectModbus);
  connect(ui.pushButton_7,&QAbstractButton::clicked,this,&pwb::save_as);

  connect(ui.tabWidget,SIGNAL(currentChanged(int)),this,SLOT(syn_tab(int)));

  ui.label_2->setStyleSheet(css::Voltage_font0);
  ui.label_27->setStyleSheet(css::Voltage_font0);
  ui.label_29->setStyleSheet(css::Voltage_font0);
  ui.label_31->setStyleSheet(css::Voltage_font0);
  ui.label_4->setStyleSheet(css::Voltage_font0);
  ui.label_6->setStyleSheet(css::Voltage_font0);
  ui.label_35->setStyleSheet(css::Voltage_font0);
  ui.label_8->setStyleSheet(css::Voltage_font0);

  ui.label_3->setStyleSheet(css::Current_font0);
  ui.label_28->setStyleSheet(css::Current_font0);
  ui.label_30->setStyleSheet(css::Current_font0);
  ui.label_32->setStyleSheet(css::Current_font0);
  ui.label_5->setStyleSheet(css::Current_font0);
  ui.label_7->setStyleSheet(css::Current_font0);
  ui.label_9->setStyleSheet(css::Current_font0);
  ui.label_36->setStyleSheet(css::Current_font0);

  ui.lineEdit_21->setEnabled(false); ui.lineEdit_22->setEnabled(false);
  ui.lineEdit_27->setEnabled(false); ui.lineEdit_28->setEnabled(false);
  ui.lineEdit_33->setEnabled(false); ui.lineEdit_34->setEnabled(false);
  ui.lineEdit_29->setEnabled(false); ui.lineEdit_30->setEnabled(false);
  connect(ui.pushButton_8,&QAbstractButton::clicked
    ,this,&pwb::update);

  //qDebug()<<ui.horizontalSlider->singleStep()<<" "<<ui.horizontalSlider->pageStep();
  //qDebug()<<ui.horizontalSlider_9->singleStep()<<" "<<ui.horizontalSlider_9->pageStep();
  //qDebug()<<ui.horizontalSlider->singleStep()<<" "<<ui.horizontalSlider->pageStep();
  //qDebug()<<ui.horizontalSlider_9->singleStep()<<" "<<ui.horizontalSlider_9->pageStep();
  syn_slider();
  slider_friend(0,ui.horizontalSlider,ui.horizontalSlider_9);
  slider_friend(0,ui.horizontalSlider_2,ui.horizontalSlider_10);
  slider_friend(0,ui.horizontalSlider_3,ui.horizontalSlider_11);
  slider_friend(0,ui.horizontalSlider_4,ui.horizontalSlider_12);
}
void pwb::log(size_t mode,std::string const& value){
    auto* active = ui.textBrowser_3;
    mode==0 ?  util::info_to<QTextBrowser,util::log_mode::k_info>(active,value) :
        mode==1 ?  util::info_to<QTextBrowser,util::log_mode::k_warning>(active,value) :
        mode==2 ?  util::info_to<QTextBrowser,util::log_mode::k_error>(active,value) :
        util::info_to<QTextBrowser,util::log_mode::k_error>(
          active,"unknow content mode, display as ERROR!");
}

#define syn_s_l(slider,lineedit)                                                      \
  slider->setMinimum(0); slider->setMaximum(2000);                                    \
  slider->setPageStep(50);                                                            \
  connect(slider,&QSlider::valueChanged                                               \
      ,[&,this](){lineedit->setText(std::to_string(slider->value()).c_str());});      \
  connect(lineedit,&QLineEdit::editingFinished                                        \
      ,[&,this](){ float value = std::stof(lineedit->text().toStdString().c_str());   \
        if (value != slider->value()) slider->setValue(value); });                    \
/**/
#define syn_s_l1(slider,lineedit)                                                     \
  slider->setMinimum(0); slider->setMaximum(0xFFFF);                                  \
  slider->setPageStep(0xFFFF/64);                                                     \
  connect(slider,&QSlider::valueChanged                                               \
      ,[&,this](){lineedit->setText(util::u162hexstr(slider->value()).c_str());});    \
  connect(lineedit,&QLineEdit::editingFinished                                        \
      ,[&,this](){ uint16_t value =                                                   \
        util::hexstr2u16(lineedit->text().toStdString().c_str());                     \
        if (value != slider->value()) slider->setValue(value); });                    \
/**/


void pwb::syn_slider(){
  syn_s_l(ui.horizontalSlider,ui.lineEdit_12);
  syn_s_l(ui.horizontalSlider_2,ui.lineEdit_13);
  syn_s_l(ui.horizontalSlider_3,ui.lineEdit_14);
  syn_s_l(ui.horizontalSlider_4,ui.lineEdit_11);
  syn_s_l1(ui.horizontalSlider_9,ui.lineEdit_25);
  syn_s_l1(ui.horizontalSlider_10,ui.lineEdit_23);
  syn_s_l1(ui.horizontalSlider_11,ui.lineEdit_26);
  syn_s_l1(ui.horizontalSlider_12,ui.lineEdit_24);
}
#undef syn_s_l
#undef syn_s_l1

void pwb::update(){
  if (!_is_connect){
    log(2,"link first please"); return; }
  Monitor();
  m_v_his[0]=2000.*m_data_frame.data[PWB_REG_SVV_CH0]/0xFFFF;
  m_v_his[1]=2000.*m_data_frame.data[PWB_REG_SVV_CH1]/0xFFFF;
  m_v_his[2]=2000.*m_data_frame.data[PWB_REG_SVV_CH2]/0xFFFF;
  m_v_his[3]=2000.*m_data_frame.data[PWB_REG_SVV_CH3]/0xFFFF;
  dispatch();
  ui.horizontalSlider->setValue(2000.*m_data_frame.data[PWB_REG_SVV_CH0]/0xFFFF);
  ui.horizontalSlider_2->setValue(2000.*m_data_frame.data[PWB_REG_SVV_CH1]/0xFFFF);
  ui.horizontalSlider_3->setValue(2000.*m_data_frame.data[PWB_REG_SVV_CH2]/0xFFFF);
  ui.horizontalSlider_4->setValue(2000.*m_data_frame.data[PWB_REG_SVV_CH3]/0xFFFF);
  ui.horizontalSlider_9->setValue(m_data_frame.data[PWB_REG_SVV_CH0]);
  ui.horizontalSlider_10->setValue(m_data_frame.data[PWB_REG_SVV_CH1]);
  ui.horizontalSlider_11->setValue(m_data_frame.data[PWB_REG_SVV_CH2]);
  ui.horizontalSlider_12->setValue(m_data_frame.data[PWB_REG_SVV_CH3]);

}

int pwb::set_v(float from, float to, QSpinBox* step_v,int index,size_t del){
    if (from==to) return 0;
    std::string text = m_button_map[index]->text().toStdString();
    float tot=std::abs(from-to);
    float init = from;
    bool up = (to-from)>=0;
    float process=0.;
    if (up){
      while(true){
        if(!m_set_v_flags[index].load()) { m_v_his[index]=from; return -4;}
        int step = step_v->value();
        uint16_t target = std::round(((from+step)-m_dac_kb[index].second)/m_dac_kb[index].first);
        if((from+step)<to){
          //if (from+step<=0){
          //  std::unique_lock<std::mutex> lock(m_mutex);
          //  int ec = modbus_write_register(m_modbus_context,index,0);
          //  lock.unlock();
          //  if (ec!=1) { log(2,"in set_v write error"); m_v_his[index]=0; return -1;}
          //  return -2;
          //}
          std::unique_lock<std::mutex> lock(m_mutex);
          int ec = modbus_write_register(m_modbus_context,index,target);
          lock.unlock();
          if (ec!=1) { log(2,"in set_v write error"); m_v_his[index]=from; return -1;}
          from+=step;
          process = std::abs(100.*(from-init)/tot);
          std::stringstream sstr; sstr<<std::floor(process)<<"% "<<text;
          m_button_map[index]->setText(sstr.str().c_str());
          QThread::msleep(del*0.98);
          continue;
        }else if((from+step)>=to){
          uint16_t to_dac = std::round((to-m_dac_kb[index].second)/m_dac_kb[index].first);
          std::unique_lock<std::mutex> lock(m_mutex);
          int ec = modbus_write_register(m_modbus_context,index,to_dac);
          lock.unlock();
          if (ec!=1) { log(2,"in set_v write error"); m_v_his[index] = from; return -1;}
          std::stringstream sstr; sstr<<100<<"% "<<m_button_map[index]->text().toStdString();
          m_button_map[index]->setText(sstr.str().c_str());
          m_v_his[index]=to;
          return ec==1 ? 0 : -1;
        }
      }
    }
    else{
      while(true){
        if(!m_set_v_flags[index].load()) { m_v_his[index]=from; return -4;}
        int step = step_v->value();
        uint16_t target = std::round(((from-step)-m_dac_kb[index].second)/m_dac_kb[index].first);
        if((from-step)>to){
          //if (from+step>=2000.){
          //  std::unique_lock<std::mutex> lock(m_mutex);
          //  int ec = modbus_write_register(m_modbus_context,index,0xFFFF);
          //  lock.unlock();
          //  if (ec!=1) { log(2,"in set_v write error"); m_v_his[index]=0xFFFF; return -1;}
          //  return -2;
          //}
          std::unique_lock<std::mutex> lock(m_mutex);
          int ec = modbus_write_register(m_modbus_context,index,target);
          lock.unlock();
          if (ec!=1) { log(2,"in set_v write error"); m_v_his[index]=from; return -1;}
          from-=step;
          process = std::abs(100.*(from-init)/tot);
          std::stringstream sstr; sstr<<std::floor(process)<<"% "<<text;
          m_button_map[index]->setText(sstr.str().c_str());
          QThread::msleep(del*0.98);
          continue;
        }else if((from-step)<=to){
          uint16_t to_dac = std::round((to-m_dac_kb[index].second)/m_dac_kb[index].first);
          std::unique_lock<std::mutex> lock(m_mutex);
          int ec = modbus_write_register(m_modbus_context,index,to_dac);
          lock.unlock();
          if (ec!=1) { log(2,"in set_v write error"); m_v_his[index] = from; return -1;}
          std::stringstream sstr; sstr<<100<<"% "<<m_button_map[index]->text().toStdString();
          m_button_map[index]->setText(sstr.str().c_str());
          m_v_his[index]=to;
          return ec==1 ? 0 : -1;
        }
      }
    }
}

#define function_register_switch(name,index)            \
void pwb::name(bool v){                                 \
  if(!_is_connect || !m_modbus_context){                \
    log(2,"link first please"); return;}                \
  std::unique_lock<std::mutex> lock(m_mutex);           \
  int ec = modbus_write_bit(m_modbus_context,index,v);  \
  lock.unlock();                                        \
  if (ec != 1){                                         \
    std::stringstream sstr;                             \
    sstr<<"channel "<<(index-4)<<" switch to "<<v<<     \
      " Failed";                                        \
    log(1,sstr.str());                					\
    m_switch_buttons[index-4]->switch_to(               \
        !m_switch_buttons[index-4]->state());           \
    m_switch_buttons[index]->switch_to(                 \
        !m_switch_buttons[index]->state());             \
    m_switch_buttons[index-4]->update();                \
    m_switch_buttons[index]->update();                  \
    return; }                                           \
  std::stringstream sstr;                               \
  sstr<<"channel "<<(index-4)<<" switch to "<<v<<" Ok"; \
  log(0,sstr.str()); }                					\
/**/
function_register_switch(switch_channel0,4)
function_register_switch(switch_channel1,5)
function_register_switch(switch_channel2,6)
function_register_switch(switch_channel3,7)
#undef function_register_switch
#define function_register_set(name,index,ui_to,ui_step)                                  \
void pwb::name(){                                                                        \
  float from = m_v_his[index];                                                           \
  std::stringstream sstr(ui_to->text().toStdString().c_str());                           \
  float value; sstr>>std::dec>>value;                                                    \
  if (sstr.fail() || (!sstr.fail() && (value<0 || value>2000))){                         \
    log(1,"please input grammar as 'float'(0.-2000.)"); return;}                         \
  float to = value;                                                                      \
  int idx = index;                                                                       \
  m_button_map[index]->setText("Setting");                                                \
  auto fun_binder =                                                                      \
      std::bind(&pwb::set_v,this                                                         \
        ,std::placeholders::_1                                                           \
        ,std::placeholders::_2                                                           \
        ,std::placeholders::_3                                                           \
        ,std::placeholders::_4                                                           \
        ,std::placeholders::_5);                                                         \
  m_set_v_flags[idx].store(true);                                                        \
   m_set_v_tasks[idx].second.setFuture(                                                  \
     m_set_v_tasks[idx].first=QtConcurrent::run(fun_binder,from,to,ui_step,index,1000)); \
}                                                                                        \
/**/
function_register_set(set_channel0,PWB_REG_SVV_CH0,ui.lineEdit_12,ui.spinBox_3)
function_register_set(set_channel1,PWB_REG_SVV_CH1,ui.lineEdit_13,ui.spinBox_4)
function_register_set(set_channel2,PWB_REG_SVV_CH2,ui.lineEdit_14,ui.spinBox_5)
function_register_set(set_channel3,PWB_REG_SVV_CH3,ui.lineEdit_11,ui.spinBox_6)
#undef function_register_set

#define function_register_set_hex(name,index,ui_to,ui_step)                              \
void pwb::name(){                                                                        \
  float from = m_v_his[index];                                                           \
  std::stringstream sstr(ui_to->text().toStdString().c_str());                           \
  uint16_t value; sstr>>std::hex>>value;                                                 \
  if (sstr.fail() || (!sstr.fail() && (value<0x0 || value>0xFFFF))){                     \
    log(1,"please input grammar as 'uint16_t'(0x0-0xFFFF)");return;}                     \
  float to = value*m_dac_kb[index].first+m_dac_kb[index].second;                         \
  int idx = index;                                                                       \
  auto fun_binder =                                                                      \
      std::bind(&pwb::set_v,this                                                         \
        ,std::placeholders::_1                                                           \
        ,std::placeholders::_2                                                           \
        ,std::placeholders::_3                                                           \
        ,std::placeholders::_4                                                           \
        ,std::placeholders::_5);                                                         \
   m_set_v_tasks[idx].second.setFuture(                                                  \
     m_set_v_tasks[idx].first=QtConcurrent::run(fun_binder,from,to,ui_step,index,1000)); \
}                                                                                        \
/**/
function_register_set_hex(set_channel0_hex,PWB_REG_SVV_CH0,ui.lineEdit_25,ui.spinBox_3)
function_register_set_hex(set_channel1_hex,PWB_REG_SVV_CH1,ui.lineEdit_23,ui.spinBox_4)
function_register_set_hex(set_channel2_hex,PWB_REG_SVV_CH2,ui.lineEdit_26,ui.spinBox_5)
function_register_set_hex(set_channel3_hex,PWB_REG_SVV_CH3,ui.lineEdit_24,ui.spinBox_6)
#undef function_register_set_hex

void pwb::Monitor(){
  std::unique_lock<std::mutex> lock(m_mutex);
  int target_read = 29;
  int ec = modbus_read_registers(m_modbus_context,0,target_read,m_data_frame.data);
  if (ec != target_read){ log(1,"PWB read registers failed"); return; }
  ec = modbus_read_bits(m_modbus_context,0,PWB_COIL_END,m_data_frame.device_coil);
  lock.unlock();
  if (ec != PWB_COIL_END){ log(1,"PWB read coils failed"); return; } }

void pwb::save_as(){
  m_csv_name = util::time_to_str()+".csv";
  QString path = QDir::currentPath() + "/" + QString(m_csv_name.c_str());
  QString fileName = QFileDialog::getSaveFileName(this, tr("Save Data"),
      path,tr("Csv Files (*.csv)"));
  m_csv_name=fileName.toStdString();
}

void pwb::start(){
  if (!m_fout.is_open()){
    m_csv_name = m_csv_name=="" ? util::time_to_str() : m_csv_name;
    m_csv_name+=".csv";
    m_fout.open(m_csv_name.c_str());
    for (auto&& x : util::item_names) m_fout<<x<<",";
    m_fout.flush()<<std::endl;
  }
  m_timers["monitor"]->start();
  m_timers["dispatch"]->start();

}
void pwb::syn_pv(){
  std::lock_guard<std::mutex> lock(m_mutex);
  m_switch_buttons[0]->switch_to(m_data_frame.device_coil[PWB_COIL_ONOFF_CH0]);
  m_switch_buttons[1]->switch_to(m_data_frame.device_coil[PWB_COIL_ONOFF_CH1]);
  m_switch_buttons[2]->switch_to(m_data_frame.device_coil[PWB_COIL_ONOFF_CH2]);
  m_switch_buttons[3]->switch_to(m_data_frame.device_coil[PWB_COIL_ONOFF_CH3]);
  m_switch_buttons[4]->switch_to(m_data_frame.device_coil[PWB_COIL_ONOFF_CH0]);
  m_switch_buttons[5]->switch_to(m_data_frame.device_coil[PWB_COIL_ONOFF_CH1]);
  m_switch_buttons[6]->switch_to(m_data_frame.device_coil[PWB_COIL_ONOFF_CH2]);
  m_switch_buttons[7]->switch_to(m_data_frame.device_coil[PWB_COIL_ONOFF_CH3]);
}

void pwb::stop(){
  if (m_fout.is_open()) m_fout.flush();
  m_timers["monitor"]->stop();
  m_timers["dispatch"]->stop();
}

void pwb::syn_tab(int index){
  slider_friend(index,ui.horizontalSlider,ui.horizontalSlider_9);
  slider_friend(index,ui.horizontalSlider_2,ui.horizontalSlider_10);
  slider_friend(index,ui.horizontalSlider_3,ui.horizontalSlider_11);
  slider_friend(index,ui.horizontalSlider_4,ui.horizontalSlider_12);
  ui.tabWidget_2->setCurrentIndex(index);
  ui.tabWidget_3->setCurrentIndex(index);
  ui.tabWidget_4->setCurrentIndex(index);
  ui.tabWidget_5->setCurrentIndex(index);
}
void pwb::dispatch(){
  uint64_t uuid = 0;
  meta::encode(uuid,util::b2i_indx
      ,m_data_frame.data[PWB_REG_BOARD_UUID]
      ,m_data_frame.data[PWB_REG_BOARD_UUID+1]
      ,m_data_frame.data[PWB_REG_BOARD_UUID+2]
      ,m_data_frame.data[PWB_REG_BOARD_UUID+3]
      );
  ui.lineEdit_35->setText(std::to_string(uuid).c_str());
  uint32_t seconds = 0;
  meta::encode(seconds,util::u162u32_indx
      ,m_data_frame.data[PWB_REG_HEARTBEAT_HI],m_data_frame.data[PWB_REG_HEARTBEAT_LW]);
  QString ts{util::uint2timestr(seconds).c_str()};
  ui.lineEdit_37->setText(ts);
  syn_pv();
  ui.lineEdit_21->setText(util::u162hexstr(m_data_frame.data[PWB_REG_PVV_CH0]).c_str());
  ui.lineEdit_22->setText(util::u162hexstr(m_data_frame.data[PWB_REG_PVI_CH0]).c_str());
  ui.lineEdit_27->setText(util::u162hexstr(m_data_frame.data[PWB_REG_PVV_CH1]).c_str());
  ui.lineEdit_28->setText(util::u162hexstr(m_data_frame.data[PWB_REG_PVI_CH1]).c_str());
  ui.lineEdit_33->setText(util::u162hexstr(m_data_frame.data[PWB_REG_PVV_CH2]).c_str());
  ui.lineEdit_34->setText(util::u162hexstr(m_data_frame.data[PWB_REG_PVI_CH2]).c_str());
  ui.lineEdit_29->setText(util::u162hexstr(m_data_frame.data[PWB_REG_PVV_CH3]).c_str());
  ui.lineEdit_30->setText(util::u162hexstr(m_data_frame.data[PWB_REG_PVI_CH3]).c_str());
  ui.lineEdit_9->setText(util::str_temperature(
    util::get_bmp280_1(m_data_frame.data+PWB_REG_TEMPERATURE_INT),"°C").c_str());
  ui.lineEdit_10->setText(util::str_humlity(
    util::get_bmp280_1(m_data_frame.data+PWB_REG_HUMIDITY_INT),"%").c_str());
  m_monitor_value[0] = m_adc_kb[0].first*m_data_frame.data[PWB_REG_PVV_CH0]+m_adc_kb[0].second;
  m_monitor_value[1] = m_adc_kb[1].first*m_data_frame.data[PWB_REG_PVV_CH1]+m_adc_kb[1].second;
  m_monitor_value[2] = m_adc_kb[2].first*m_data_frame.data[PWB_REG_PVV_CH2]+m_adc_kb[2].second;
  m_monitor_value[3] = m_adc_kb[3].first*m_data_frame.data[PWB_REG_PVV_CH3]+m_adc_kb[3].second;

  m_monitor_value[4] = std::abs(m_vi_rel[0][2])<=2 ?
    0. : (m_data_frame.data[PWB_REG_PVI_CH0]-(m_vi_rel[0][0]*m_monitor_value[0]+m_vi_rel[0][1]))/m_vi_rel[0][2];
  m_monitor_value[5] = std::abs(m_vi_rel[1][2])<=2 ?
    0. : (m_data_frame.data[PWB_REG_PVI_CH1]-(m_vi_rel[1][0]*m_monitor_value[1]+m_vi_rel[1][1]))/m_vi_rel[1][2];
  m_monitor_value[6] = std::abs(m_vi_rel[2][2])<=2 ?
    0. : (m_data_frame.data[PWB_REG_PVI_CH2]-(m_vi_rel[2][0]*m_monitor_value[2]+m_vi_rel[2][1]))/m_vi_rel[2][2];
  m_monitor_value[7] = std::abs(m_vi_rel[3][2])<=2 ?
    0. : (m_data_frame.data[PWB_REG_PVI_CH3]-(m_vi_rel[3][0]*m_monitor_value[3]+m_vi_rel[3][1]))/m_vi_rel[3][2];
  ui.lineEdit_2->setText(util::str_voltage(m_monitor_value[0],"V").c_str());
  ui.lineEdit_3->setText(util::str_voltage(m_monitor_value[1],"V").c_str());
  ui.lineEdit_6->setText(util::str_voltage(m_monitor_value[2],"V").c_str());
  ui.lineEdit_8->setText(util::str_voltage(m_monitor_value[3],"V").c_str());
  ui.lineEdit->setText(util::str_current(m_monitor_value[4],"nA").c_str());
  ui.lineEdit_4->setText(util::str_current(m_monitor_value[5],"nA").c_str());
  ui.lineEdit_5->setText(util::str_current(m_monitor_value[6],"nA").c_str());
  ui.lineEdit_7->setText(util::str_current(m_monitor_value[7],"nA").c_str());
  if (m_fout.is_open()){
    m_fout<<util::time_to_str()<<',';
    m_fout<<ts.toStdString()<<",";
    m_fout<<util::get_bmp280_1(m_data_frame.data+PWB_REG_TEMPERATURE_INT)<<",";
    m_fout<<util::get_bmp280_1(m_data_frame.data+PWB_REG_HUMIDITY_INT)<<",";
    m_fout<<(m_data_frame.data[PWB_REG_SVV_CH0])*m_dac_kb[0].first+m_dac_kb[0].second<<",";
    m_fout<<(m_data_frame.data[PWB_REG_SVV_CH1])*m_dac_kb[1].first+m_dac_kb[1].second<<",";
    m_fout<<(m_data_frame.data[PWB_REG_SVV_CH2])*m_dac_kb[2].first+m_dac_kb[2].second<<",";
    m_fout<<(m_data_frame.data[PWB_REG_SVV_CH3])*m_dac_kb[3].first+m_dac_kb[3].second<<",";
    for (auto&& x : m_monitor_value) m_fout<<x<<",";
    m_fout.flush()<<std::endl;
  }
  if (_is_connect_to_mqtt.load()){
      QString ss{m_subscribe.c_str()};
      to_bytes();
      for (auto&& [x,y] : m_mqtt_pub){
        if(m_mqtt_client->publish(QString{x.c_str()},y)==-1){
          std::stringstream sstr; sstr<<"MQTT publish  "<<x<<" faield; Please Check.";
          log(2,sstr.str());
        }
      }
      //if (m_mqtt_client->publish(ss,to_bytes())==-1){
      //    std::stringstream sstr; sstr<<"MQTT publish faield; Disconnect now. Please Check.";
      //    log(2,sstr.str());
      //    _is_connect_to_mqtt.store(false);
      //    disconnect_mqtt();
      //}
  }
  if (m_timers["monitor"]->isActive()) emit draw_signal(m_index++);
}

void pwb::abort(){
  std::unique_lock lock(m_mutex);
  _cv_is_lost_link.wait(lock,std::bind(&pwb::is_lost_connect,this));
  lock.unlock();
}

void pwb::init_canvas(){
  auto* c0_x = new QValueAxis; c0_x->setTickCount(1);
  auto* c1_x = new QValueAxis; c1_x->setTickCount(1);
  auto* c2_x = new QValueAxis; c2_x->setTickCount(1);
  auto* c3_x = new QValueAxis; c3_x->setTickCount(1);
  m_axis.emplace("c0_x",c0_x); m_axis.emplace("c1_x",c1_x);
  m_axis.emplace("c2_x",c2_x); m_axis.emplace("c3_x",c3_x);
  auto* c0_yv = new QValueAxis; c0_yv->setLinePenColor("#FC0316");  c0_yv->setLabelsColor("#FC0316");
  auto* c1_yv = new QValueAxis; c1_yv->setLinePenColor("#FC0316");  c1_yv->setLabelsColor("#FC0316");
  auto* c2_yv = new QValueAxis; c2_yv->setLinePenColor("#FC0316");  c2_yv->setLabelsColor("#FC0316");
  auto* c3_yv = new QValueAxis; c3_yv->setLinePenColor("#FC0316");  c3_yv->setLabelsColor("#FC0316");
  auto* c0_yi = new QValueAxis; c0_yi->setLinePenColor("#2F03FC");  c0_yi->setLabelsColor("#2F03FC");
  auto* c1_yi = new QValueAxis; c1_yi->setLinePenColor("#2F03FC");  c1_yi->setLabelsColor("#2F03FC");
  auto* c2_yi = new QValueAxis; c2_yi->setLinePenColor("#2F03FC");  c2_yi->setLabelsColor("#2F03FC");
  auto* c3_yi = new QValueAxis; c3_yi->setLinePenColor("#2F03FC");  c3_yi->setLabelsColor("#2F03FC");
  m_axis.emplace("c0_yv",c0_yv); m_axis.emplace("c1_yv",c1_yv);
  m_axis.emplace("c2_yv",c2_yv); m_axis.emplace("c3_yv",c3_yv);
  m_axis.emplace("c0_yi",c0_yi); m_axis.emplace("c1_yi",c1_yi);
  m_axis.emplace("c2_yi",c2_yi); m_axis.emplace("c3_yi",c3_yi);

  std::vector<std::pair<std::string,QChart*>> charts;

  auto* chart_c0 = new QChart();
  charts.emplace_back(std::make_pair("V&I",chart_c0));
  auto* c0_lv = new QLineSeries(chart_c0);
  auto* c0_li = new QLineSeries(chart_c0);
  chart_c0->addAxis(c0_x,Qt::AlignBottom);
  chart_c0->addAxis(c0_yv,Qt::AlignLeft);
  chart_c0->addAxis(c0_yi,Qt::AlignRight);
  chart_c0->addSeries(c0_lv); chart_c0->addSeries(c0_li);
  c0_lv->attachAxis(c0_x); c0_lv->attachAxis(c0_yv);
  c0_li->attachAxis(c0_x); c0_li->attachAxis(c0_yi);


  auto* chart_c1 = new QChart();
  charts.emplace_back(std::make_pair("V&I",chart_c1));
  auto* c1_lv = new QLineSeries(chart_c1);
  auto* c1_li = new QLineSeries(chart_c1);
  chart_c1->addAxis(c1_x,Qt::AlignBottom);
  chart_c1->addAxis(c1_yv,Qt::AlignLeft);
  chart_c1->addAxis(c1_yi,Qt::AlignRight);
  chart_c1->addSeries(c1_lv); chart_c1->addSeries(c1_li);
  c1_lv->attachAxis(c1_x); c1_lv->attachAxis(c1_yv);
  c1_li->attachAxis(c1_x); c1_li->attachAxis(c1_yi);

  auto* chart_c2 = new QChart();
  charts.emplace_back(std::make_pair("V&I",chart_c2));
  auto* c2_lv = new QLineSeries(chart_c2);
  auto* c2_li = new QLineSeries(chart_c2);
  chart_c2->addAxis(c2_x,Qt::AlignBottom);
  chart_c2->addAxis(c2_yv,Qt::AlignLeft);
  chart_c2->addAxis(c2_yi,Qt::AlignRight);
  chart_c2->addSeries(c2_lv); chart_c2->addSeries(c2_li);
  c2_lv->attachAxis(c2_x); c2_lv->attachAxis(c2_yv);
  c2_li->attachAxis(c2_x); c2_li->attachAxis(c2_yi);

  auto* chart_c3 = new QChart();
  charts.emplace_back(std::make_pair("V&I",chart_c3));
  auto* c3_lv = new QLineSeries(chart_c3);
  auto* c3_li = new QLineSeries(chart_c3);
  chart_c3->addAxis(c3_x,Qt::AlignBottom);
  chart_c3->addAxis(c3_yv,Qt::AlignLeft);
  chart_c3->addAxis(c3_yi,Qt::AlignRight);
  chart_c3->addSeries(c3_lv); chart_c3->addSeries(c3_li);
  c3_lv->attachAxis(c3_x); c3_lv->attachAxis(c3_yv);
  c3_li->attachAxis(c3_x); c3_li->attachAxis(c3_yi);

  m_series.emplace("c0_lv",c0_lv); m_series.emplace("c0_li",c0_li);
  m_series.emplace("c1_lv",c1_lv); m_series.emplace("c1_li",c1_li);
  m_series.emplace("c2_lv",c2_lv); m_series.emplace("c2_li",c2_li);
  m_series.emplace("c3_lv",c3_lv); m_series.emplace("c3_li",c3_li);

  for(auto&& [_,x] : charts)
    x->setTitle(_.c_str()), x->legend()->hide(),
    x->setAnimationOptions(QChart::AllAnimations),
    x->setMargins(QMargins(0,0,0,0)),
    x->layout()->setContentsMargins(0,0,0,0);
  ui.graphicsView->setChart(chart_c0); ui.graphicsView->setRenderHint(QPainter::Antialiasing);
  ui.graphicsView_2->setChart(chart_c1); ui.graphicsView_2->setRenderHint(QPainter::Antialiasing);
  ui.graphicsView_3->setChart(chart_c2); ui.graphicsView_3->setRenderHint(QPainter::Antialiasing);
  ui.graphicsView_4->setChart(chart_c3); ui.graphicsView_4->setRenderHint(QPainter::Antialiasing);

  m_series["c0_lv"]->setPen(QColor(m_axis["c0_yv"]->linePenColor()));
  m_series["c1_lv"]->setPen(QColor(m_axis["c1_yv"]->linePenColor()));
  m_series["c2_lv"]->setPen(QColor(m_axis["c2_yv"]->linePenColor()));
  m_series["c3_lv"]->setPen(QColor(m_axis["c3_yv"]->linePenColor()));
  m_series["c0_li"]->setPen(QColor(m_axis["c0_yi"]->linePenColor()));
  m_series["c1_li"]->setPen(QColor(m_axis["c1_yi"]->linePenColor()));
  m_series["c2_li"]->setPen(QColor(m_axis["c2_yi"]->linePenColor()));
  m_series["c3_li"]->setPen(QColor(m_axis["c3_yi"]->linePenColor()));
  connect(this,SIGNAL(draw_signal(float)),
      this,SLOT(draw(float)));
}
void pwb::draw(float x){
  m_series["c0_lv"]->append(x,m_monitor_value[0]);
  m_series["c1_lv"]->append(x,m_monitor_value[1]);
  m_series["c2_lv"]->append(x,m_monitor_value[2]);
  m_series["c3_lv"]->append(x,m_monitor_value[3]);
  m_series["c0_li"]->append(x,m_monitor_value[4]);
  m_series["c1_li"]->append(x,m_monitor_value[5]);
  m_series["c2_li"]->append(x,m_monitor_value[6]);
  m_series["c3_li"]->append(x,m_monitor_value[7]);

  auto const& adjust_axis_range = [&x](QValueAxis* axis){
    auto size = axis->max()-axis->min();
    if (x>(size*0.8+axis->min()))
      axis->setRange(axis->min()+0.2*size,axis->max()+0.2*size); };
  for (auto&& [x,y] : m_axis)
    if (x.find('x')!=std::string::npos) adjust_axis_range(y); }

void pwb::closeEvent(QCloseEvent* event){
  Q_UNUSED(event);
  for (auto&& [_,y] : m_timers)
    if (y->isActive()) y->stop();
  if (m_fout.is_open()){
    m_fout.flush();
    m_fout.close();
    QMessageBox msgBox;
    msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Save);
    msgBox.setText("Save File");
    int ret = msgBox.exec();
    switch(ret){
        case QMessageBox::Save:
            break;
        case QMessageBox::Cancel:
            std::filesystem::remove(m_csv_name.c_str());
            break;
    }
  }
  this->close();
}

void pwb::ConnectModbus(){
  if (!_is_connect){
      std::string device = ui.comboBox->currentText().toStdString();
      int slave_addr = ui.spinBox->value();
      auto cs = util::connect_modbus(device.c_str(),19200,'N',8,2,slave_addr);
      if (cs.second && cs.first){
          _is_connect = true;
          ui.pushButton_9->setText("close");
          ui.comboBox->setEnabled(false);
          ui.pushButton_10->setEnabled(false);
          ui.spinBox->setEnabled(false);
          m_modbus_context = cs.first;
          util::info_to<QTextBrowser,util::log_mode::k_info>(
              ui.textBrowser_3,
              "Link modbus device Ok.", " device: ", device
              ,"baud: ",19200
              ,"parity: ","None"
              ,"data_bit: ",8
              ,"stop_bit: ",2
              ,"slave_addr: ",slave_addr
              );
          Monitor();
          dispatch();
          m_v_his[0]=2000.*m_data_frame.data[PWB_REG_SVV_CH0]/0xFFFF;
          m_v_his[1]=2000.*m_data_frame.data[PWB_REG_SVV_CH1]/0xFFFF;
          m_v_his[2]=2000.*m_data_frame.data[PWB_REG_SVV_CH2]/0xFFFF;
          m_v_his[3]=2000.*m_data_frame.data[PWB_REG_SVV_CH3]/0xFFFF;
      }else{
          _is_connect = false;
          util::info_to<QTextBrowser,util::log_mode::k_error>(
              ui.textBrowser_3,
              "Link modbus device failed!");
      }
  }else{
      modbus_close(m_modbus_context); modbus_free(m_modbus_context);
      util::info_to<QTextBrowser,util::log_mode::k_info>(
          ui.textBrowser_3,"Disconnect modbus serialport, Bye");
      ui.spinBox->setEnabled(true);
      ui.comboBox->setEnabled(true);
      ui.pushButton_9->setText("open");
      ui.pushButton_10->setEnabled(true);
      _is_connect = false;
  }
}

void pwb::read_config(){
  std::ifstream fin("config.txt");
  if(!fin.is_open()) return;

  std::vector<float> y;
  std::vector<std::vector<float>> x;

  std::string str;
  size_t index=0;
  while(!fin.eof()){
    std::getline(fin,str);
    if (str.size()==0) continue;
    std::stringstream sstr(str.c_str());
    std::vector<float> buf(15);
    for (int i=0; i<15; ++i) sstr>>buf.at(i);
    if (index==0) y = buf;
    else x.emplace_back(buf);
    index++;
  }
  for (size_t i=0; i<4; ++i){
    if (i==0){
      m_kb[i] = util::least_squart_line_fit(std::begin(x[i])
          //,std::end(x[i]),std::begin(y),std::end(y));
          ,std::begin(x[i])+7,std::begin(y),std::begin(y)+7);
      std::cout<<std::dec;
    }else{
      m_kb[i] = util::least_squart_line_fit(std::begin(x[i])
          ,std::end(x[i]),std::begin(y),std::end(y));
    }
  }
  fin.close();
}

void pwb::read_config1(){
  std::ifstream fin("config_hv.txt");
    if(!fin.is_open()) { log(2,"config file can't access");return;}
  std::string sbuf;
  auto const& read_xy = [&]<class _tp>(std::vector<_tp>& px, std::vector<_tp>& py)->std::pair<double,double>{
    auto back = fin.tellg();
    std::getline(fin,sbuf);
    util::trim_space(sbuf);
    if(sbuf[0]=='#') {fin.seekg(back,std::ios::beg);return {};}
    auto x = util::str_to_values<float>(sbuf); px = x;
    std::getline(fin,sbuf);
    auto y = util::str_to_values<float>(sbuf); py = y;
    return util::least_squart_line_fit(std::begin(x),std::end(x),std::begin(y),std::end(y));
  };
  while(!fin.eof()){
    std::getline(fin,sbuf);
    util::trim_space(sbuf);
    if(sbuf=="#DC0"){
      std::string buf(""); std::getline(fin,buf); std::stringstream sstr(buf.c_str());
      sstr>>m_dac_kb[0].first>>m_dac_kb[0].second;
      if (sstr.fail()) log(1,"read channel 0 DAC config failed");
    }
    else if (sbuf=="#DC1"){
      std::string buf(""); std::getline(fin,buf); std::stringstream sstr(buf.c_str());
      sstr>>m_dac_kb[1].first>>m_dac_kb[1].second;
      if (sstr.fail()) log(1,"read channel 1 DAC config failed");
    }
    else if (sbuf=="#DC2"){
      std::string buf(""); std::getline(fin,buf); std::stringstream sstr(buf.c_str());
      sstr>>m_dac_kb[2].first>>m_dac_kb[2].second;
      if (sstr.fail()) log(1,"read channel 2 DAC config failed");
    }
    else if (sbuf=="#DC3"){
      std::string buf(""); std::getline(fin,buf); std::stringstream sstr(buf.c_str());
      sstr>>m_dac_kb[3].first>>m_dac_kb[3].second;
      if (sstr.fail()) log(1,"read channel 3 DAC config failed");
    }
    else if(sbuf=="#DC1"){ m_dac_kb[1]=read_xy(m_config_table["DC1X"],m_config_table["DC1Y"]); }
    else if(sbuf=="#DC2"){ m_dac_kb[2]=read_xy(m_config_table["DC2X"],m_config_table["DC2Y"]); }
    else if(sbuf=="#DC3"){ m_dac_kb[3]=read_xy(m_config_table["DC3X"],m_config_table["DC3Y"]); }
    else if(sbuf=="#AD0"){ m_adc_kb[0]=read_xy(m_config_table["AD0X"],m_config_table["AD0Y"]);}
    else if(sbuf=="#AD1"){ m_adc_kb[1]=read_xy(m_config_table["AD1X"],m_config_table["AD1Y"]);}
    else if(sbuf=="#AD2"){ m_adc_kb[2]=read_xy(m_config_table["AD2X"],m_config_table["AD2Y"]);}
    else if(sbuf=="#AD3"){ m_adc_kb[3]=read_xy(m_config_table["AD3X"],m_config_table["AD3Y"]);}
    else if(sbuf=="#VI0"){ m_v2i_kb[0]=read_xy(m_config_table["VI0X"],m_config_table["VI0Y"]);}
    else if(sbuf=="#VI1"){ m_v2i_kb[1]=read_xy(m_config_table["VI1X"],m_config_table["VI1Y"]);}
    else if(sbuf=="#VI2"){ m_v2i_kb[2]=read_xy(m_config_table["VI2X"],m_config_table["VI2Y"]);}
    else if(sbuf=="#VI3"){ m_v2i_kb[3]=read_xy(m_config_table["VI3X"],m_config_table["VI3Y"]);}
    else if(sbuf=="#VI0_50G"){ m_v2i_50g_kb[0] = read_xy(m_config_table["VI0X_50G"],m_config_table["VI0Y_50G"]);}
    else if(sbuf=="#VI1_50G"){ m_v2i_50g_kb[1] = read_xy(m_config_table["VI1X_50G"],m_config_table["VI1Y_50G"]);}
    else if(sbuf=="#VI2_50G"){ m_v2i_50g_kb[2] = read_xy(m_config_table["VI2X_50G"],m_config_table["VI2Y_50G"]);}
    else if(sbuf=="#VI3_50G"){ m_v2i_50g_kb[3] = read_xy(m_config_table["VI3X_50G"],m_config_table["VI3Y_50G"]);}
    else if(sbuf=="#VERSION"){
        std::getline(fin,m_version); util::trim_space(m_version);
        if (m_version.empty()) m_version="1.0.0";

    }
  }

  for (int i=0; i<4; ++i){
    float k,b;
    float ii = calibration_current(i,k,b);
    m_vi_rel[i] = {k,b,ii};
  }

  //log(0,std::to_string(m_dac_kb[0].first));
  //log(0,std::to_string(m_dac_kb[0].second));
  //log(0,std::to_string(m_dac_kb[1].first));
  //log(0,std::to_string(m_dac_kb[1].second));
  //log(0,std::to_string(m_dac_kb[2].first));
  //log(0,std::to_string(m_dac_kb[2].second));
  //log(0,std::to_string(m_dac_kb[3].first));
  //log(0,std::to_string(m_dac_kb[3].second));
}

float pwb::calibration_current(int cidx,float& k, float& b){
  std::vector<float> data0, data1;
  for(auto&& x : m_config_table.at(::vix_map.at(cidx)))
    data0.emplace_back(x*m_adc_kb.at(cidx).first+m_adc_kb.at(cidx).second);

  for(auto&& x : m_config_table.at(::vi50gx_map.at(cidx)))
    data1.emplace_back(x*m_adc_kb.at(cidx).first+m_adc_kb.at(cidx).second);
  auto kb_null = util::least_squart_line_fit(
      std::begin(data0),std::end(data0)
      ,std::begin(m_config_table.at(::viy_map.at(cidx))),std::end(m_config_table.at(::viy_map.at(cidx))));
  k = kb_null.first;
  b = kb_null.second;

  auto kb_50g = util::least_squart_line_fit(
      std::begin(data1),std::end(data1)
      ,std::begin(m_config_table.at(::vi50gy_map.at(cidx))),std::end(m_config_table.at(::vi50gy_map.at(cidx))));
  float p00 =
      -(kb_null.first*2000.+kb_null.second)+(kb_50g.first*2000.+kb_50g.second);
  float p01 =
      -kb_null.second+kb_50g.second;
  return (p00-p01)/40.;
}

double pwb::get_v(uint16_t value, size_t index) const{
  double value_f = (double)value/0xFFFF;
  if (m_kb.find(index) != m_kb.end())
    return 2000.*(m_kb.at(index).first*value_f+m_kb.at(index).second)/(0xFFFF);
  return 0.;
}

void pwb::connect_mqtt(){
  if(m_mqtt_client->state()==QMqttClient::Disconnected){
    ui.lineEdit_19->setEnabled(false);
    ui.spinBox_2->setEnabled(false);
    ui.pushButton_11->setText("disconnect");
    m_mqtt_client->connectToHost();
    _is_connect_to_mqtt.store(true);
    std::stringstream sstr;
    sstr<<"Connected to MQTT server "<<m_mqtt_client->hostname().toStdString()<<":"<<m_mqtt_client->port();
    log(0,sstr.str());
  }else{
    ui.lineEdit_19->setEnabled(true);
    ui.spinBox_2->setEnabled(true);
    ui.pushButton_11->setText("connect");
    m_mqtt_client->disconnectFromHost();
    _is_connect_to_mqtt.store(false);
    std::stringstream sstr;
    sstr<<"Disconnected from MQTT server "<<m_mqtt_client->hostname().toStdString()<<":"<<m_mqtt_client->port();
    log(0,sstr.str());
  }
}

void pwb::disconnect_mqtt(){
  ui.lineEdit_19->setEnabled(true);
  ui.spinBox_2->setEnabled(true);
  ui.pushButton_11->setText("connect");
}

void pwb::publish(){
}

void pwb::subscribe(){
}

void pwb::set_v_handle_started(int cidx){
  auto* button = m_button_map[cidx];
  button->setStyleSheet(css::Setting_font0);
  button->setText("Cancel");
  QObject::disconnect(button,nullptr,this,nullptr);
  QObject::connect(button,&QAbstractButton::clicked,
    [=,this](){this->set_v_handle_cancel(cidx);});
  std::stringstream sstr; sstr<<std::fixed<<std::setprecision(2);
  sstr<<"Start set channel "<<cidx<<". form '"<<m_v_his[cidx]<<"'";
  log(0,sstr.str());
}

void pwb::set_v_handle_finished(int cidx){
  auto* button = m_button_map[cidx];
  button->setStyleSheet("");
  button->setText("Set");
  button->disconnect();
  switch(cidx){
    case 0: QObject::connect(button,&QAbstractButton::clicked,this,&pwb::set_channel0); break;
    case 1: QObject::connect(button,&QAbstractButton::clicked,this,&pwb::set_channel1); break;
    case 2: QObject::connect(button,&QAbstractButton::clicked,this,&pwb::set_channel2); break;
    case 3: QObject::connect(button,&QAbstractButton::clicked,this,&pwb::set_channel3); break;
  }
  int result=m_set_v_tasks[cidx].first.result();
  if (result!=-4){
    std::stringstream sstr;
    sstr<<"Finish set channel "<<cidx<<" to "<<m_v_his[cidx]<<"V "
      <<(result==0 ? "Ok " : "Failed ")<<"("<<result<<")";
    log(result==0 ? 0 : 2,sstr.str());
  }
}
void pwb::set_v_handle_cancel(int cidx){
  m_set_v_flags[cidx].store(false);
  auto* button = m_button_map[cidx];
  button->setText("Set");
  button->disconnect();
  switch(cidx){
    case 0: QObject::connect(button,&QAbstractButton::clicked,this,&pwb::set_channel0); break;
    case 1: QObject::connect(button,&QAbstractButton::clicked,this,&pwb::set_channel1); break;
    case 2: QObject::connect(button,&QAbstractButton::clicked,this,&pwb::set_channel2); break;
    case 3: QObject::connect(button,&QAbstractButton::clicked,this,&pwb::set_channel3); break;
  }
  std::stringstream sstr;
  sstr<<"Terminate channel "<<cidx<<" V set";
  log(1,sstr.str());
}
void pwb::to_bytes(){
    if (!_is_connect_to_mqtt.load()) return;
    m_mqtt_pub.at("PWB/temprature")=QString{
      util::str_temperature(util::get_bmp280_1(m_data_frame.data+PWB_REG_TEMPERATURE_INT),"").c_str()}.toUtf8();
    m_mqtt_pub.at("PWB/humidity")=QString{
      util::str_temperature(util::get_bmp280_1(m_data_frame.data+PWB_REG_HUMIDITY_INT),"").c_str()}.toUtf8();
    m_mqtt_pub.at("PWB/voltage/0")=QString{util::str_voltage(m_monitor_value[0],"").c_str()}.toUtf8();
    m_mqtt_pub.at("PWB/voltage/1")=QString{util::str_voltage(m_monitor_value[1],"").c_str()}.toUtf8();
    m_mqtt_pub.at("PWB/voltage/2")=QString{util::str_voltage(m_monitor_value[2],"").c_str()}.toUtf8();
    m_mqtt_pub.at("PWB/voltage/3")=QString{util::str_voltage(m_monitor_value[3],"").c_str()}.toUtf8();
    m_mqtt_pub.at("PWB/current/0")=QString{util::str_voltage(m_monitor_value[4],"").c_str()}.toUtf8();
    m_mqtt_pub.at("PWB/current/1")=QString{util::str_voltage(m_monitor_value[5],"").c_str()}.toUtf8();
    m_mqtt_pub.at("PWB/current/2")=QString{util::str_voltage(m_monitor_value[6],"").c_str()}.toUtf8();
    m_mqtt_pub.at("PWB/current/3")=QString{util::str_voltage(m_monitor_value[7],"").c_str()}.toUtf8();
}
void pwb::keyPressEvent(QKeyEvent* key){
  if (key->key()==Qt::Key_C
         && key->modifiers()==Qt::ControlModifier){
        //qDebug()<<"CTRL+C";
         closeEvent(nullptr);
  }
}
void pwb::slider_friend(int index, QSlider* v0, QSlider* v1){
    auto min0 = v0->minimum();
    auto min1 = v1->minimum();
    auto max0 = v0->maximum();
    auto max1 = v1->maximum();
    double v02v1 = (max1-min1)/(double)(max0-min0);
    double v12v0 = (max0-min0)/(double)(max1-min1);
    //qDebug()<<min0<<" "<<min1<<" "<<max0<<" "<<max1<<" "<<v02v1<<" "<<v12v0;
    if (index==0){
      QObject::disconnect(v1,nullptr,v0,nullptr);
      QObject::connect(v0,qOverload<int>(&QSlider::valueChanged)
        ,v1,[=](int const value){v1->setValue(v02v1*(value-min0)+min1);});
    }else if(index==1){
      QObject::disconnect(v0,nullptr,v1,nullptr);
      QObject::connect(v1,qOverload<int>(&QSlider::valueChanged)
        ,v0,[=](int const value){v0->setValue(v12v0*(value-min1)+min0);});
    }
}
