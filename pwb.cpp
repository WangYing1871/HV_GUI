#include <iostream>
#include <sstream>
#include <cfloat>
#include <cmath>
#include <stdexcept>
#include <iomanip>
#include <fstream>
#include "pwb.h"
#include "util.h"

#include <QTime>
#include <QFileDialog>
//#include <QValueAxis>
//#include <QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QValueAxis>
#include <QGraphicsLayout>

int const pwb::s_tolerate_failed = 10;

pwb::pwb(QWidget* p):
  QWidget(p){
  ui.setupUi(this);

  read_config();
  read_config1();
  for(auto&& x : m_monitor_value) x=0.;
  this->setWindowTitle("PWB");

  m_timers.insert(std::make_pair("monitor",new QTimer));
  m_timers.insert(std::make_pair("dispatch",new QTimer));
  m_timers["monitor"]->setInterval(1000);
  m_timers["dispatch"]->setInterval(1000);

  connect(m_timers["monitor"],&QTimer::timeout,this,&pwb::Monitor);
  connect(m_timers["dispatch"],&QTimer::timeout,this,&pwb::dispatch);

  ui.lcdNumber->setDigitCount(8);
  ui.lcdNumber_2->setDigitCount(16);

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
  ui.lineEdit_19->setEnabled(false);
  ui.lineEdit_20->setEnabled(false);
  ui.lineEdit_21->setEnabled(false);
  ui.lineEdit_22->setEnabled(false);
  init_switch();
  init_canvas();

  reload_serialports();
  connect(ui.pushButton_10,&QAbstractButton::clicked,this,&pwb::reload_serialports);
  for (auto&& x : m_axis){
    if (x.first.find('x')!=std::string::npos) x.second->setRange(0,100);
    if (x.first.find('y')!=std::string::npos) x.second->setRange(0,2000);
  }
}

void pwb::reload_serialports(){
  ui.comboBox->clear();
  for(auto&& x : util::get_ports_name())
      ui.comboBox->addItem(x.c_str());


}

void pwb::init_switch(){
  auto* sbt0 = new SwitchButton(ui.widget);
  connect(sbt0,SIGNAL(statusChanged(bool)),this,SLOT(switch_channel0(bool)));
  auto* sbt1 = new SwitchButton(ui.widget_3);
  connect(sbt1,SIGNAL(statusChanged(bool)),this,SLOT(switch_channel1(bool)));
  auto* sbt2 = new SwitchButton(ui.widget_4);
  connect(sbt2,SIGNAL(statusChanged(bool)),this,SLOT(switch_channel2(bool)));
  auto* sbt3 = new SwitchButton(ui.widget_2);
  connect(sbt3,SIGNAL(statusChanged(bool)),this,SLOT(switch_channel3(bool)));
  m_switch_buttons.insert(std::make_pair(0,sbt0));
  m_switch_buttons.insert(std::make_pair(1,sbt1));
  m_switch_buttons.insert(std::make_pair(2,sbt2));
  m_switch_buttons.insert(std::make_pair(3,sbt3));
  for (auto&& [_,x] : m_switch_buttons ) x->setGeometry(0,0,69,25);

  connect(ui.pushButton,&QAbstractButton::clicked,this,&pwb::set_channel0);
  connect(ui.pushButton_2,&QAbstractButton::clicked,this,&pwb::set_channel1);
  connect(ui.pushButton_3,&QAbstractButton::clicked,this,&pwb::set_channel2);
  connect(ui.pushButton_4,&QAbstractButton::clicked,this,&pwb::set_channel3);

  connect(ui.pushButton_5,&QAbstractButton::clicked,this,&pwb::start);
  connect(ui.pushButton_6,&QAbstractButton::clicked,this,&pwb::stop);
  connect(ui.pushButton_9,&QAbstractButton::clicked,this,&pwb::ConnectModbus);
  connect(ui.pushButton_7,&QAbstractButton::clicked,this,&pwb::save_as);

  ui.checkBox->setEnabled(false); ui.checkBox_2->setEnabled(false);
  ui.checkBox_3->setEnabled(false); ui.checkBox_4->setEnabled(false);
  ui.lineEdit_15->setEnabled(false); ui.lineEdit_16->setEnabled(false);
  ui.lineEdit_17->setEnabled(false); ui.lineEdit_18->setEnabled(false);

  connect(ui.pushButton_8,&QAbstractButton::clicked,this,&pwb::update);
  syn_slider();


}
void pwb::log(size_t mode,std::string const& value){
    auto* active = ui.textBrowser_3;
    mode==0 ?  util::info_to<QTextBrowser,util::log_mode::k_info>(active,value) :
        mode==1 ?  util::info_to<QTextBrowser,util::log_mode::k_warning>(active,value) :
        mode==2 ?  util::info_to<QTextBrowser,util::log_mode::k_error>(active,value) :
        util::info_to<QTextBrowser,util::log_mode::k_error>(active,"unknow content mode, display as ERROR!");
}

#define syn_s_l(slider,lineedit)                                                      \
  slider->setMinimum(0), slider->setMaximum(2000);                                    \
  slider->setPageStep(50);                                                            \
  connect(slider,&QSlider::valueChanged                                               \
      ,[&,this](){lineedit->setText(std::to_string(slider->value()).c_str());});      \
  connect(lineedit,&QLineEdit::editingFinished                                        \
      ,[&,this](){ float value = std::stof(lineedit->text().toStdString().c_str());   \
        if (value != slider->value()) slider->setValue(value); });                    \
/**/


void pwb::syn_slider(){
  syn_s_l(ui.horizontalSlider,ui.lineEdit_12);
  syn_s_l(ui.horizontalSlider_2,ui.lineEdit_13);
  syn_s_l(ui.horizontalSlider_3,ui.lineEdit_14);
  syn_s_l(ui.horizontalSlider_4,ui.lineEdit_11); }
#undef syn_s_l

void pwb::update(){
  if (!_is_connect){
    log(2,"link first please"); return; }
  Monitor();
  dispatch();
  ui.horizontalSlider->setValue(2000.*m_data_frame.data[PWB_REG_SVV_CH0]/0xFFFF);
  ui.horizontalSlider_2->setValue(2000.*m_data_frame.data[PWB_REG_SVV_CH1]/0xFFFF);
  ui.horizontalSlider_3->setValue(2000.*m_data_frame.data[PWB_REG_SVV_CH2]/0xFFFF);
  ui.horizontalSlider_4->setValue(2000.*m_data_frame.data[PWB_REG_SVV_CH3]/0xFFFF);
}


#define function_register_switch(name,index)            \
void pwb::name(bool v){                                 \
  auto* md_ctx = m_modbus_context;  					          \
  int ec = modbus_write_bit(md_ctx,index,v);            \
  if (ec != 1){                                         \
    std::stringstream sstr;                             \
    sstr<<"channel "<<(index-4)<<" switch to "<<v<<     \
      " Failed";                                        \
    log(1,sstr.str());                					        \
    m_switch_buttons[index-4]->switch_to(               \
        !m_switch_buttons[index-4]->state());           \
    return; }                                           \
  std::stringstream sstr;                               \
  sstr<<"channel "<<(index-4)<<" switch to "<<v<<" Ok"; \
  log(0,sstr.str()); }                					        \
/**/
function_register_switch(switch_channel0,4)
function_register_switch(switch_channel1,5)
function_register_switch(switch_channel2,6)
function_register_switch(switch_channel3,7)
#undef function_register_switch


//  if (m_caches[index]-value != 0) ui.to->setPageStep(value-m_caches[index]);
//  m_caches[index] = value;

#define function_register_set(index,name,from,to,REG)                 \
void pwb::name(){                                                     \
  std::stringstream sstr(ui.from->text().toStdString().c_str());      \
  float value; sstr>>std::hex>>value>>std::dec;                       \
  if (!sstr.fail()){                                                  \
    if (value<0 || value>2000){                                       \
      log(1,"please input grammar as 'float'(0.-2000.)");             \
      return;                                                         \
    }                                                                 \
    auto* md_ctx = m_modbus_context;                          				\
    uint16_t input_value = (value/2000.f)*0xFFFF;                     \
    int ec = modbus_write_register(md_ctx,REG,input_value);           \
    if (ec != 1){ log(1,"set channel value Failed"); return; }        \
    std::stringstream sstr; sstr<<#name<<" to value "<<value<<" Ok";  \
    log(0,sstr.str());                                                \
  }else{ std::stringstream sstr;                                      \
    log(1,"please input grammar as 'float'(0.-2000.)"); } }           \
/**/
function_register_set(0,set_channel0,lineEdit_12,horizontalSlider,PWB_REG_SVV_CH0)
function_register_set(1,set_channel1,lineEdit_13,horizontalSlider_2,PWB_REG_SVV_CH1)
function_register_set(2,set_channel2,lineEdit_14,horizontalSlider_3,PWB_REG_SVV_CH2)
function_register_set(3,set_channel3,lineEdit_11,horizontalSlider_4,PWB_REG_SVV_CH3)
#undef function_register_set


void pwb::Monitor(){
  auto* md_ctx = m_modbus_context;

  std::unique_lock<std::mutex> lock(m_mutex);
  int ec = modbus_read_registers(md_ctx,0,PWB_REG_END,m_data_frame.data);
  if (ec != PWB_REG_END){ log(1,"PWB read registers failed"); return; }
  ec = modbus_read_bits(md_ctx,0,PWB_COIL_END,m_data_frame.device_coil);
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
}

void pwb::stop(){
  if (m_fout.is_open()) m_fout.flush();
  m_timers["monitor"]->stop();
  m_timers["dispatch"]->stop();
}
void pwb::dispatch(){

  if (m_fout.is_open()){
    m_fout<<util::time_to_str()<<',';
    for (int i=0; i<PWB_REG_END-1; ++i) m_fout<<m_data_frame.data[i]<<',';
    m_fout<<m_data_frame.data[PWB_REG_END-1]<<std::endl; }


  uint64_t uuid = 0;
  meta::encode(uuid,util::b2i_indx
      ,m_data_frame.data[PWB_REG_BOARD_UUID]
      ,m_data_frame.data[PWB_REG_BOARD_UUID+1]
      ,m_data_frame.data[PWB_REG_BOARD_UUID+2]
      ,m_data_frame.data[PWB_REG_BOARD_UUID+3]
      );
  ui.lcdNumber_2->display(QString{std::to_string(uuid).c_str()});

  uint32_t seconds = 0;
  meta::encode(seconds,util::u162u32_indx
      ,m_data_frame.data[PWB_REG_HEARTBEAT_HI],m_data_frame.data[PWB_REG_HEARTBEAT_LW]);
  QString ts{util::uint2timestr(seconds).c_str()};
  ui.lcdNumber->display(ts);

  syn_pv();
  ui.checkBox->setCheckState(
      m_data_frame.device_coil[PWB_COIL_ONOFF_CH0]==1 ? Qt::Checked : Qt::Unchecked);
  ui.checkBox_2->setCheckState(
      m_data_frame.device_coil[PWB_COIL_ONOFF_CH1]==1 ? Qt::Checked : Qt::Unchecked);
  ui.checkBox_4->setCheckState(
      m_data_frame.device_coil[PWB_COIL_ONOFF_CH2]==1 ? Qt::Checked : Qt::Unchecked);
  ui.checkBox_3->setCheckState(
      m_data_frame.device_coil[PWB_COIL_ONOFF_CH3]==1 ? Qt::Checked : Qt::Unchecked);

  ui.lineEdit_18->setText(util::u162hexstr(m_data_frame.data[PWB_REG_SVV_CH0]).c_str());
  ui.lineEdit_17->setText(util::u162hexstr(m_data_frame.data[PWB_REG_SVV_CH1]).c_str());
  ui.lineEdit_16->setText(util::u162hexstr(m_data_frame.data[PWB_REG_SVV_CH2]).c_str());
  ui.lineEdit_15->setText(util::u162hexstr(m_data_frame.data[PWB_REG_SVV_CH3]).c_str());

  ui.lineEdit_9->setText(QString::number(
        util::get_bmp280_1(m_data_frame.data+PWB_REG_TEMPERATURE_INT)));
  ui.lineEdit_10->setText(QString::number(
        util::get_bmp280_1(m_data_frame.data+PWB_REG_HUMIDITY_INT)));

  //ui.lineEdit  ->setText(util::u162hexstr(m_data_frame.data[PWB_REG_PVI_CH0]).c_str());
  
  //((float)m_data_frame.data[PWB_REG_PVI_CH0]/0xFFFF)*get_k(0)/0xFFFF;

  //ui.lineEdit_4->setText(util::u162hexstr(m_data_frame.data[PWB_REG_PVI_CH1]).c_str());
  //ui.lineEdit_5->setText(util::u162hexstr(m_data_frame.data[PWB_REG_PVI_CH2]).c_str());
  //ui.lineEdit_7->setText(util::u162hexstr(m_data_frame.data[PWB_REG_PVI_CH3]).c_str());
  m_monitor_value[0] = 5.*get_v(m_data_frame.data[PWB_REG_PVV_CH0],0);
  m_monitor_value[1] = 5.*get_v(m_data_frame.data[PWB_REG_PVV_CH1],1);
  m_monitor_value[2] = 5.*get_v(m_data_frame.data[PWB_REG_PVV_CH2],2);
  m_monitor_value[3] = 5.*get_v(m_data_frame.data[PWB_REG_PVV_CH3],3);
  
  ui.lineEdit_2->setText(std::to_string(m_monitor_value[0]).c_str());
  ui.lineEdit_3->setText(std::to_string(m_monitor_value[1]).c_str());
  ui.lineEdit_6->setText(std::to_string(m_monitor_value[2]).c_str());
  ui.lineEdit_8->setText(std::to_string(m_monitor_value[3]).c_str());
  emit draw_signal(m_index++);

  float hv1 = (m_dac_kb[1].first*m_data_frame.data[PWB_REG_SVV_CH1]+m_dac_kb[1].second)/**400*/;
  float hv3 = (m_dac_kb[3].first*m_data_frame.data[PWB_REG_SVV_CH3]+m_dac_kb[3].second)/**400*/;

  //float hv2 = (m_dac_kb[2].first*m_data_frame.data[PWB_REG_SVV_CH0]+m_dac_kb[0].second)*400;

  //ui.lineEdit_19->setText(std::to_string(hv0).c_str());
  ui.lineEdit_20->setText(std::to_string(hv1).c_str());
  ui.lineEdit_22->setText(std::to_string(hv3).c_str());
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
  auto* c0_yv = new QValueAxis; c0_yv->setLinePenColor("#FC0316");
  auto* c1_yv = new QValueAxis; c1_yv->setLinePenColor("#FC0316");
  auto* c2_yv = new QValueAxis; c2_yv->setLinePenColor("#FC0316");
  auto* c3_yv = new QValueAxis; c3_yv->setLinePenColor("#FC0316");
  auto* c0_yi = new QValueAxis; c0_yi->setLinePenColor("#2F03FC");
  auto* c1_yi = new QValueAxis; c1_yi->setLinePenColor("#2F03FC");
  auto* c2_yi = new QValueAxis; c2_yi->setLinePenColor("#2F03FC");
  auto* c3_yi = new QValueAxis; c3_yi->setLinePenColor("#2F03FC");
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
  m_fout.flush();
  m_fout.close();
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
          //auto const& update_mcu = [&,this](){
          //    int ec0 = modbus_read_registers(m_modbus_context,0,REG_END,m_data_frame.data);
          //    int ec1 = modbus_read_bits(m_modbus_context,0,COIL_END,m_data_frame.device_coil);
          //    if (ec0!=REG_END || ec1!=COIL_END){
          //        log(2,"Update firmware status failed!"); return; }
          //    update_mode();
          //    update_forward();
          //};
          //update_mcu();
      }else{
          _is_connect = false;
          util::info_to<QTextBrowser,util::log_mode::k_error>(
              ui.textBrowser_3,
              "Link modbus device failed!");
      }
  }else{
      modbus_close(m_modbus_context); modbus_free(m_modbus_context);
      util::info_to<QTextBrowser,util::log_mode::k_info>(ui.textBrowser_3,"disconnect modbus serial, Bye");
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
  if(!fin.is_open()) return;
  std::string sbuf;
  auto const& read_xy = [&]()->std::pair<double,double>{
    std::getline(fin,sbuf);
    auto x = util::str_to_values<uint32_t>(sbuf);
    std::getline(fin,sbuf);
    auto y = util::str_to_values<float>(sbuf);
    return util::least_squart_line_fit(
          std::begin(x),std::end(x),std::begin(y),std::end(y));
  };
  while(!fin.eof()){
    std::getline(fin,sbuf);
    util::trim_space(sbuf);
    if (sbuf=="#DC0"){
    }else if(sbuf=="#DC1"){
      m_dac_kb[1]=read_xy();
    }else if(sbuf=="#DC2"){
    }else if(sbuf=="#DC3"){
      m_dac_kb[3]=read_xy();
    }

  }




}

double pwb::get_v(uint16_t value, size_t index) const{
  double value_f = (double)value/0xFFFF;
  if (m_kb.find(index) != m_kb.end())
    return 2000.*(m_kb.at(index).first*value_f+m_kb.at(index).second)/(0xFFFF);
  return 0.;
}

//double pwb::get_i()
