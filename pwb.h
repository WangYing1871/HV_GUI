#ifndef pwb_v1_H
#define pwb_v1_H 1 
#include <map>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <fstream>
#include "QObject"
#include "QWidget"
#include "QDialog"
#include "QTimer"
#include "QByteArray"
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QPushButton>
#ifdef QT_CHARTS_USE_NAMESPACE
QT_CHARTS_USE_NAMESPACE
#endif
#include <QtMqtt/QMqttClient>

#include <QFutureWatcher>
#include <QtConcurrent>

#include "switch.h"
#include "ui_pwb.h"
#include "modbus_pwb_ifc.h"
#include "modbus.h"

class pwb : public QWidget{
  Q_OBJECT

  struct data_frame_t{
    bool _is_valid = false;
    int64_t tag = 0;
    uint16_t data[PWB_REG_END] = {0};
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
  void init_future_tasks();
  void init_mqtt();

  inline void modbus_context(modbus_t* value) {
      m_modbus_ctx = value;}

private:
  QMqttClient* m_mqtt_client;
  std::map<int,SwitchButton*> m_switch_buttons;
  std::map<std::string,QTimer*> m_timers;
  std::unordered_map<std::string,QLineSeries*> m_series;
  std::unordered_map<std::string,QValueAxis*> m_axis;
  modbus_t* m_modbus_ctx;

  std::map<int,std::pair<double,double>> m_kb;
  std::map<int,std::pair<double,double>> m_dac_kb;
  std::map<int,std::pair<double,double>> m_adc_kb;
  std::map<int,std::pair<double,double>> m_v2i_kb;
  std::map<int,std::pair<double,double>> m_v2i_50g_kb;
  std::map<int,float> m_i_map;
  std::unordered_map<std::string,std::vector<float>> m_config_table;
  std::unordered_map<int,std::array<float,3>> m_vi_rel;

  QTimer m_timer;

  void dispatch();

  void Monitor();
  void syn_pv();
  void syn_slider();
  void read_config();
  void read_config1();
  float calibration_current(int index,float&,float&);

  void init_canvas();
  uint16_t m_caches[4];
  float m_index=0.;
  std::atomic<int> m_read_failed;
  std::atomic<bool> _is_connect_to_mqtt;
  std::condition_variable _cv_is_lost_link;
  void abort();
  void log(size_t mode,std::string const& value);

  bool _is_connect = false;
  modbus_t* m_modbus_context;
  double m_monitor_value[8];
  uint16_t m_current_sub[4];
  float m_v_his[4] = {0.,0.,0.,0.};

  std::string m_csv_name;
  std::ofstream m_fout;

  QFuture<int> m_future;
  QFutureWatcher<int> m_future_watcher;

  double get_v(uint16_t,size_t) const;
  typedef int future_task_result_t;
  future_task_result_t set_v(float, float,QSpinBox*, int, size_t del=1000);
  template <class _tp>
  using future_task_t = std::pair<QFuture<_tp>,QFutureWatcher<_tp>>;
  /*
   * asynchronous tasks for set voltage
   */
  std::unordered_map<int,future_task_t<future_task_result_t>> m_set_v_tasks;
  std::unordered_map<int,std::atomic<bool>> m_set_v_flags;
  std::unordered_map<int,QPushButton*> m_button_map;
  std::unordered_map<std::string,QByteArray> m_mqtt_pub;

private:
  bool is_lost_connect() const {return m_read_failed.load()==s_tolerate_failed;}
  void slider_friend(int index, QSlider* v0, QSlider* v1);
  void set_style_sheet();

public slots:
  void switch_channel0(bool);
  void switch_channel1(bool);
  void switch_channel2(bool);
  void switch_channel3(bool);
  void set_channel0();
  void set_channel1();
  void set_channel2();
  void set_channel3();
  void set_channel0_hex();
  void set_channel1_hex();

  void set_channel2_hex();
  void set_channel3_hex();
  void start();
  void stop();
  void update();
  void draw(float);
  void ConnectModbus();
  void reload_serialports();
  void save_as();
  void syn_tab(int);
  void connect_mqtt();
  void disconnect_mqtt();
  void publish();
  void subscribe();
  void setting_channel(bool,int);
  void set_v_handle_started(int);
  void set_v_handle_finished(int);
  void set_v_handle_cancel(int);

protected:
  void keyPressEvent(QKeyEvent*) override;

private slots:

signals:
  void draw_signal(float);
  void setting_channel_s(bool,int);
private:
  static int const s_tolerate_failed;
private:
  std::string m_version="1.0.0";
  std::string m_host;
  std::string m_subscribe = "IV";
  void to_bytes();
};
#endif
