#ifndef pwb_v1_H
#define pwb_v1_H 1 

#include <vector>
#include <map>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <functional>

#include "QObject"
#include "QWidget"
#include "QDialog"
#include "QTimer"
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#ifdef QT_CHARTS_USE_NAMESPACE
QT_CHARTS_USE_NAMESPACE
#endif

#include "switch.h"
#include "ui_pwb.h"
#include "modbus_pwb_ifc.h"
#include "modbus.h"

class pwb : public QWidget{
  Q_OBJECT

  struct data_frame_t{
    bool _is_valid = false;
    int64_t tag = 0; uint16_t data[PWB_REG_END] = {0};
    uint8_t device_coil[PWB_COIL_END] = {false};
    data_frame_t(){ }
    ~data_frame_t() noexcept = default;
  };
  data_frame_t m_data_frame;

protected:
  void closeEvent(QCloseEvent* event) override; 
  
public:

  pwb(QWidget* parent=0);
  ~pwb() noexcept {}

  Ui::pwb ui;
  std::mutex m_mutex;

  void init_switch();
  void modbus_context(modbus_t* value) {m_modbus_ctx = value;}

private:
  std::map<int,SwitchButton*> m_switch_buttons;
  std::map<std::string,QTimer*> m_timers;
  std::unordered_map<std::string,QLineSeries*> m_series;
  std::unordered_map<std::string,QValueAxis*> m_axis;
  modbus_t* m_modbus_ctx;

  QTimer m_timer;

  void dispatach();

  void Monitor();
  void syn_pv();
  void syn_slider();

  void init_canvas();
  uint16_t m_caches[4];
  float m_index=0.;
  std::atomic<int> m_read_failed;
  std::condition_variable _cv_is_lost_link;
  void abort();
  void log(size_t mode,std::string const& value);
  bool _is_connect = false;
  modbus_t* m_modbus_context;


private:
  bool is_lost_connect() const {return m_read_failed.load()==s_tolerate_failed;}



public slots:
  void switch_channel0(bool);
  void switch_channel1(bool);
  void switch_channel2(bool);
  void switch_channel3(bool);
  void set_channel0();
  void set_channel1();
  void set_channel2();
  void set_channel3();
  void start();
  void stop();
  void update();
  void draw(float);
  void ConnectModbus();
  void reload_serialports();

signals:
  void draw_signal(float);

private:
  static int const s_tolerate_failed;
 
};

#endif
