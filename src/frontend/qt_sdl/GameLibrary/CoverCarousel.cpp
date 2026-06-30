/*
    Copyright 2016-2025 melonDS team, DualityDS contributors

    This file is part of DualityDS, based on melonDS.

    DualityDS is free software: you can redistribute it and/or modify it under
    the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    melonDS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with melonDS. If not, see http://www.gnu.org/licenses/.
*/

#include "CoverCarousel.h"
#include "GameLibraryModel.h"

#include <QGraphicsScene>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsSceneMouseEvent>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QPainter>
#include <QPainterPath>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QtMath>

namespace
{
// Card geometry (the slot a cover is drawn into), and the fan parameters.
constexpr qreal CARD_W       = 190.0;
constexpr qreal CARD_H       = 250.0;
constexpr qreal CORNER       = 10.0;  // placeholder rounding only
constexpr qreal ART_CORNER   = 7.0;   // soft rounding on the cover artwork edges

constexpr int   VISIBLE      = 4;     // covers shown each side of center
constexpr qreal CENTER_SCALE = 2.0;   // center cover ~2x the base card
constexpr qreal SCALE_DECAY  = 0.75;  // each step out shrinks to 0.75x the last
constexpr qreal SCALE_MIN    = 0.20;
constexpr qreal SPACING      = CARD_W * 1.25; // horizontal step between covers
constexpr qreal ARC_DROP     = 34.0;  // px each cover sinks per step outward
constexpr qreal TILT_DEG     = 15.0;  // rotation per step, capped
constexpr qreal TILT_CAP     = 60.0;
constexpr qreal OPACITY_STEP = 0.18;
constexpr qreal OPACITY_MIN  = 0.40;
constexpr qreal CENTER_Y_FRAC= 0.46;  // vertical center of the fan (push up a bit)
constexpr int   ANIM_MS      = 200;
}

// ---------------------------------------------------------------- CoverItem

CoverItem::CoverItem(int row, QGraphicsItem* parent)
    : QGraphicsObject(parent), m_row(row)
{
    // Rotate/scale about the card center, not the top-left corner.
    setTransformOriginPoint(CARD_W / 2.0, CARD_H / 2.0);

    // Soft drop shadow lifts the cover off the backdrop — bumped a touch so the
    // art reads as "popped up" / more 3D.
    auto* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(32);
    shadow->setOffset(0, 11);
    shadow->setColor(QColor(0, 0, 0, 165));
    setGraphicsEffect(shadow);
}

void CoverItem::setCover(const QPixmap& pix)
{
    if (pix.isNull())
    {
        m_pix = QPixmap();
    }
    else
    {
        // Retain enough resolution for the 2x center cover; only downscale if the
        // source is larger (never upscale a small cover into blur).
        const int maxW = int(CARD_W * CENTER_SCALE);
        const int maxH = int(CARD_H * CENTER_SCALE);
        if (pix.width() > maxW || pix.height() > maxH)
            m_pix = pix.scaled(maxW, maxH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        else
            m_pix = pix;
    }
    update();
}

QRectF CoverItem::boundingRect() const
{
    return QRectF(0, 0, CARD_W, CARD_H);
}

void CoverItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter->setPen(Qt::NoPen);

    if (!m_pix.isNull())
    {
        // Artwork only — no card/border. Fit hi-res pixmap into the card rect; the
        // painter downsamples through the view's transform, keeping it crisp at 2x.
        QSize fit = m_pix.size();
        fit.scale(int(CARD_W), int(CARD_H), Qt::KeepAspectRatio);
        QRectF target((CARD_W - fit.width()) / 2.0, (CARD_H - fit.height()) / 2.0,
                      fit.width(), fit.height());

        // Slightly rounded corners instead of hard 90° edges.
        QPainterPath clip;
        clip.addRoundedRect(target, ART_CORNER, ART_CORNER);
        painter->save();
        painter->setClipPath(clip);
        painter->drawPixmap(target, m_pix, m_pix.rect());
        painter->restore();
    }
    else
    {
        // Subtle placeholder while the cover is still being fetched.
        painter->setBrush(QColor(0xDD, 0xDD, 0xDD));
        painter->drawRoundedRect(QRectF(0, 0, CARD_W, CARD_H), CORNER, CORNER);
    }
}

void CoverItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
        emit clicked(m_row);
    QGraphicsObject::mousePressEvent(event);
}

// ------------------------------------------------------------ CoverCarousel

CoverCarousel::CoverCarousel(GameLibraryModel* model, QWidget* parent)
    : QGraphicsView(parent), m_model(model), m_center(0)
{
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);

    setBackgroundBrush(QColor(0xE8, 0xE8, 0xE8));
    setFrameShape(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    setFocusPolicy(Qt::StrongFocus);

    m_anim = new QParallelAnimationGroup(this);

    connect(m_model, &QAbstractItemModel::rowsInserted, this, &CoverCarousel::onRowsInserted);
    connect(m_model, &QAbstractItemModel::modelReset, this, &CoverCarousel::onModelReset);
    connect(m_model, &QAbstractItemModel::dataChanged, this, &CoverCarousel::onDataChanged);

    rebuild();
}

int CoverCarousel::count() const
{
    return m_items.size();
}

void CoverCarousel::rebuild()
{
    m_anim->stop();
    m_anim->clear();
    qDeleteAll(m_items);
    m_items.clear();
    m_scene->clear();

    const int rows = m_model->rowCount();
    for (int r = 0; r < rows; r++)
    {
        auto* item = new CoverItem(r);
        item->setCover(m_model->index(r).data(GameLibraryModel::CoverPixmapRole).value<QPixmap>());
        connect(item, &CoverItem::clicked, this, &CoverCarousel::onItemClicked);
        m_scene->addItem(item);
        m_items.append(item);
    }

    if (m_center >= rows) m_center = rows > 0 ? rows - 1 : 0;
    relayout(false);
}

void CoverCarousel::updateItemPixmap(int row)
{
    if (row < 0 || row >= m_items.size()) return;
    m_items[row]->setCover(
        m_model->index(row).data(GameLibraryModel::CoverPixmapRole).value<QPixmap>());
}

void CoverCarousel::relayout(bool animated)
{
    m_anim->stop();
    m_anim->clear();

    const qreal cx = viewport()->width() / 2.0;
    const qreal cy = viewport()->height() * CENTER_Y_FRAC;

    for (int i = 0; i < m_items.size(); i++)
    {
        CoverItem* item = m_items[i];
        const int d = i - m_center;
        const int ad = qAbs(d);

        if (ad > VISIBLE)
        {
            item->setVisible(false);
            continue;
        }
        item->setVisible(true);

        // Target transform for this card's distance from center.
        const qreal scale = qMax(SCALE_MIN, CENTER_SCALE * qPow(SCALE_DECAY, ad));
        const qreal rot   = qBound(-TILT_CAP, d * TILT_DEG, TILT_CAP);
        const qreal opac  = qMax(OPACITY_MIN, 1.0 - OPACITY_STEP * ad);

        // Card center on the arc, then convert to top-left for pos().
        const qreal centerX = cx + d * SPACING;
        const qreal centerY = cy + ad * ARC_DROP;
        const QPointF topLeft(centerX - CARD_W / 2.0, centerY - CARD_H / 2.0);

        item->setZValue(100 - ad);

        if (animated)
        {
            auto add = [&](const char* prop, const QVariant& to) {
                auto* a = new QPropertyAnimation(item, prop);
                a->setDuration(ANIM_MS);
                a->setEasingCurve(QEasingCurve::InOutQuad);
                a->setEndValue(to);
                m_anim->addAnimation(a);
            };
            add("pos", topLeft);
            add("scale", scale);
            add("rotation", rot);
            add("opacity", opac);
        }
        else
        {
            item->setPos(topLeft);
            item->setScale(scale);
            item->setRotation(rot);
            item->setOpacity(opac);
        }
    }

    if (animated && m_anim->animationCount() > 0)
        m_anim->start();
}

void CoverCarousel::setCurrentIndex(int index)
{
    if (m_items.isEmpty()) return;
    index = qBound(0, index, m_items.size() - 1);
    if (index == m_center) return;
    m_center = index;
    relayout(true);
    emit currentIndexChanged(m_center);
}

void CoverCarousel::next()
{
    setCurrentIndex(m_center + 1);
}

void CoverCarousel::prev()
{
    setCurrentIndex(m_center - 1);
}

void CoverCarousel::onRowsInserted(const QModelIndex& parent, int first, int last)
{
    if (parent.isValid()) return;
    // Rows always append during a scan; rebuild keeps row↔item indices aligned.
    Q_UNUSED(first); Q_UNUSED(last);
    rebuild();
}

void CoverCarousel::onModelReset()
{
    m_center = 0;
    rebuild();
}

void CoverCarousel::onDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight,
                                  const QList<int>& roles)
{
    if (!roles.isEmpty() && !roles.contains(GameLibraryModel::CoverPixmapRole)
        && !roles.contains(Qt::DecorationRole))
        return;
    for (int r = topLeft.row(); r <= bottomRight.row(); r++)
        updateItemPixmap(r);
}

void CoverCarousel::onItemClicked(int row)
{
    if (row == m_center)
        emit coverActivated(row);
    else
        setCurrentIndex(row);
}

void CoverCarousel::keyPressEvent(QKeyEvent* event)
{
    switch (event->key())
    {
    case Qt::Key_Left:
        prev();
        return;
    case Qt::Key_Right:
        next();
        return;
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Space:
        if (!m_items.isEmpty())
            emit coverActivated(m_center);
        return;
    default:
        QGraphicsView::keyPressEvent(event);
    }
}

void CoverCarousel::wheelEvent(QWheelEvent* event)
{
    if (event->angleDelta().y() > 0)
        prev();
    else if (event->angleDelta().y() < 0)
        next();
    event->accept();
}

void CoverCarousel::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);
    m_scene->setSceneRect(0, 0, viewport()->width(), viewport()->height());
    relayout(false);
}
