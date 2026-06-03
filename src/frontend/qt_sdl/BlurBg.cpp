#include "BlurBg.h"
#include "EmuInstance.h"
#include <QPainter>
#include <QPaintEvent>
#include <cmath>

BlurBgWidget::BlurBgWidget(EmuInstance* inst, QWidget* parent)
    : QWidget(parent), emuInstance(inst), gameAR(256.0f / 384.0f), showBlur(false)
{
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAutoFillBackground(false);
}

void BlurBgWidget::setGameAR(float ar)
{
    gameAR = ar;
}

void BlurBgWidget::refresh()
{
    auto nds = emuInstance ? emuInstance->getNDS() : nullptr;
    if (!nds) { showBlur = false; return; }

    void* topBuf = nullptr;
    void* botBuf = nullptr;
    if (!nds->GPU.GetFramebuffers(&topBuf, &botBuf)) { showBlur = false; return; }

    // Only draw blur when window AR doesn't match game AR (letterboxing present)
    float winAR = (float)width() / (float)height();
    if (std::abs(winAR / gameAR - 1.0f) <= 0.02f)
    {
        prevTiny = QImage(); // reset so next letterbox start has no stale frame
        showBlur = false;
        update();
        return;
    }
    showBlur = true;

    // Edge-strip sampling: left 15% and right 15% of combined 256x384 frame
    // Matches Electron: drawImage(src, 0,0, edge,sh, ...) + drawImage(src, sw-edge,0, edge,sh, ...)
    const int SW = 256, SH = 384;
    const int TW = 64, TH = 96;
    const int edgeW = (int)(SW * 0.20f); // ~51px

    QImage combined(SW, SH, QImage::Format_RGB32);
    memcpy(combined.scanLine(0),   topBuf, 256 * 192 * 4);
    memcpy(combined.scanLine(192), botBuf, 256 * 192 * 4);

    QImage tiny(TW, TH, QImage::Format_RGB32);
    for (int y = 0; y < TH; y++)
    {
        QRgb* out = reinterpret_cast<QRgb*>(tiny.scanLine(y));
        int sy = y * SH / TH;
        const QRgb* row = reinterpret_cast<const QRgb*>(combined.scanLine(sy));
        for (int x = 0; x < TW / 2; x++)
            out[x] = row[x * edgeW / (TW / 2)];
        for (int x = 0; x < TW / 2; x++)
            out[TW / 2 + x] = row[(SW - edgeW) + x * edgeW / (TW / 2)];
    }

    // Temporal smoothing: blend 15% new frame into 85% previous
    if (!prevTiny.isNull() && prevTiny.size() == tiny.size())
    {
        float keep = 0.85f;
        int tw = tiny.width(), th = tiny.height();
        for (int y = 0; y < th; y++)
        {
            const QRgb* pRow = reinterpret_cast<const QRgb*>(prevTiny.constScanLine(y));
            QRgb*       cRow = reinterpret_cast<QRgb*>(tiny.scanLine(y));
            for (int x = 0; x < tw; x++)
            {
                int r = (int)(qRed(pRow[x])   * keep + qRed(cRow[x])   * 0.15f);
                int g = (int)(qGreen(pRow[x]) * keep + qGreen(cRow[x]) * 0.15f);
                int b = (int)(qBlue(pRow[x])  * keep + qBlue(cRow[x])  * 0.15f);
                cRow[x] = qRgb(r, g, b);
            }
        }
    }
    prevTiny = tiny;

    QImage blurred = boxBlur(tiny, 5);
    applyColorAdjust(blurred, 0.73f, 1.2f);

    bgImage = blurred.scaled(512, 512, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    update();
}

void BlurBgWidget::paintEvent(QPaintEvent* event)
{
    QPainter p(this);
    if (showBlur && !bgImage.isNull())
    {
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);
        int bw = (int)(width()  * 1.12f);
        int bh = (int)(height() * 1.12f);
        int ox = -(int)(width()  * 0.06f);
        int oy = -(int)(height() * 0.06f);
        p.drawImage(QRect(ox, oy, bw, bh), bgImage);
    }
    else
        p.fillRect(rect(), Qt::black);
}

// 2-pass box blur (horizontal then vertical)
QImage BlurBgWidget::boxBlur(const QImage& src, int radius)
{
    QImage img = src.convertToFormat(QImage::Format_RGB32);
    int w = img.width(), h = img.height();
    QImage tmp(w, h, QImage::Format_RGB32);

    auto clamp = [](int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); };

    for (int y = 0; y < h; y++)
    {
        const QRgb* in  = reinterpret_cast<const QRgb*>(img.scanLine(y));
        QRgb*       out = reinterpret_cast<QRgb*>(tmp.scanLine(y));
        for (int x = 0; x < w; x++)
        {
            int r=0,g=0,b=0,n=0;
            for (int k = -radius; k <= radius; k++)
            {
                int xi = clamp(x+k, 0, w-1);
                QRgb c = in[xi];
                r += qRed(c); g += qGreen(c); b += qBlue(c); n++;
            }
            out[x] = qRgb(r/n, g/n, b/n);
        }
    }

    QImage result(w, h, QImage::Format_RGB32);
    for (int x = 0; x < w; x++)
    {
        for (int y = 0; y < h; y++)
        {
            int r=0,g=0,b=0,n=0;
            for (int k = -radius; k <= radius; k++)
            {
                int yi = clamp(y+k, 0, h-1);
                QRgb c = reinterpret_cast<const QRgb*>(tmp.scanLine(yi))[x];
                r += qRed(c); g += qGreen(c); b += qBlue(c); n++;
            }
            reinterpret_cast<QRgb*>(result.scanLine(y))[x] = qRgb(r/n, g/n, b/n);
        }
    }
    return result;
}

// brightness(0.73) saturate(1.2) — matches Electron CSS filter
void BlurBgWidget::applyColorAdjust(QImage& img, float brightness, float saturation)
{
    int w = img.width(), h = img.height();
    for (int y = 0; y < h; y++)
    {
        QRgb* row = reinterpret_cast<QRgb*>(img.scanLine(y));
        for (int x = 0; x < w; x++)
        {
            float r = (float)qRed(row[x]);
            float g = (float)qGreen(row[x]);
            float b = (float)qBlue(row[x]);
            float luma = 0.299f*r + 0.587f*g + 0.114f*b;
            r = luma + saturation * (r - luma);
            g = luma + saturation * (g - luma);
            b = luma + saturation * (b - luma);
            r *= brightness; g *= brightness; b *= brightness;
            auto cl = [](float v) -> int { return v < 0.0f ? 0 : (v > 255.0f ? 255 : (int)v); };
            row[x] = qRgb(cl(r), cl(g), cl(b));
        }
    }
}
