#ifndef SWITCHBUTTON_H
#define SWITCHBUTTON_H

#include <QObject>
#include <QWidget>
#include <QTimer>
#include <QColor>
#include <QPainter>
#include <QString>

class SwitchButton : public QWidget
{
    Q_OBJECT
public:
    explicit SwitchButton(QWidget *parent = nullptr);

    void switch_to(bool);
  inline bool state() const {return m_checked;}

signals:
    void statusChanged(bool checked);

public slots:

private slots:
    void UpdateValue();

private:
    void drawBackGround(QPainter *painter);
    void drawSlider(QPainter *painter);

protected:
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);


private:
    int m_space;
    int m_radius;
    bool m_checked;
    bool m_showText;
    bool m_showCircle;
    bool m_animation;

    QColor m_bgColorOn;
    QColor m_bgColorOff;
    QColor m_sliderColorOn;
    QColor m_sliderColorOff;
    QColor m_textColor;

    QString m_textOn;
    QString m_textOff;

    QTimer *m_timer;
    int m_step;
    int m_startX;
    int m_endX;

public:
    int space()                 const;
    int radius()                const;
    bool checked()              const;
    bool showText()             const;
    bool showCircle()           const;
    bool animation()            const;

    QColor bgColorOn()          const;
    QColor bgColorOff()         const;
    QColor sliderColorOn()      const;
    QColor sliderColorOff()     const;
    QColor textColor()          const;

    QString textOn()            const;
    QString textOff()           const;

    int step()                  const;
    int startX()                const;
    int endX()                  const;

public Q_SLOTS:
    void setSpace(int space);
    void setRadius(int radius);
    void setChecked(bool checked);
    void setShowText(bool show);
    void setShowCircle(bool show);
    void setAnimation(bool ok);

    void setBgColorOn(const QColor &color);
    void setBgColorOff(const QColor &color);
    void setSliderColorOn(const QColor &color);
    void setSliderColorOff(const QColor &color);
    void setTextColor(const QColor &color);

    void setTextOn(const QString &text);
    void setTextOff(const QString &text);
};

#endif
