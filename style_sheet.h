#ifndef STYLE_SHEET_H
#define STYLE_SHEET_H
#include <string>
#include <QString>

namespace css{
static QString const group_box_font0 = "\
  QGroupBox{ \
     font-size: 14px; \
     font-weight: bold; \
    }\
   QGroupBox::title{ \
     subcontrol-origin: margin; \
     subcontrol-position: top left; \
     color: blue; \
     padding: 0 3px; \
     font-size: 6px; \
     font-family: Arial; \
   }\
   ";
static QString const Voltage_font0="color:#FC0316;";
static QString const Current_font0="color:#2F03FC;";
static QString const Setting_font0="color:green;";
}
#endif // STYLE_SHEET_H
