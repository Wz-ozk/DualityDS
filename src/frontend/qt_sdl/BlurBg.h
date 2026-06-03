#pragma once
#include <QWidget>
#include <QImage>

class EmuInstance;

class BlurBgWidget : public QWidget
{
    Q_OBJECT
public:
    explicit BlurBgWidget(EmuInstance* inst, QWidget* parent = nullptr);
    void refresh();
    void setGameAR(float ar);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    static QImage boxBlur(const QImage& src, int radius);
    static void applyColorAdjust(QImage& img, float brightness, float saturation);

    EmuInstance* emuInstance;
    QImage bgImage;
    QImage prevTiny;
    float gameAR;
    bool showBlur;
};
