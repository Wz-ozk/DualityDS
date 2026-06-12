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

#pragma once

#include <QGraphicsView>
#include <QGraphicsObject>
#include <QPixmap>
#include <QList>

class QGraphicsScene;
class QParallelAnimationGroup;
class GameLibraryModel;

// A single cover card in the carousel. A QGraphicsObject so its pos/scale/
// rotation/opacity are animatable Q_PROPERTYs; the scene applies a real
// QGraphicsDropShadowEffect and composites the fan for us.
class CoverItem : public QGraphicsObject
{
    Q_OBJECT
public:
    explicit CoverItem(int row, QGraphicsItem* parent = nullptr);

    int row() const { return m_row; }
    void setCover(const QPixmap& pix); // scaled-to-fit inside the card

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* opt, QWidget* w) override;

signals:
    void clicked(int row);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;

private:
    int m_row;
    QPixmap m_pix; // pre-scaled to fit the card, may be null until a cover loads
};

// USB Loader GX-style fan/arc carousel of game covers. Reads cover pixmaps from
// a GameLibraryModel (CoverPixmapRole) and centers one cover at a time, with
// neighbors tilted and scaled down along an arc.
class CoverCarousel : public QGraphicsView
{
    Q_OBJECT
public:
    explicit CoverCarousel(GameLibraryModel* model, QWidget* parent = nullptr);

    int currentIndex() const { return m_center; }
    int count() const;

public slots:
    void setCurrentIndex(int index);
    void next();
    void prev();

signals:
    void currentIndexChanged(int index);
    void coverActivated(int row); // boot this row's game

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onRowsInserted(const QModelIndex& parent, int first, int last);
    void onModelReset();
    void onDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight,
                       const QList<int>& roles);
    void onItemClicked(int row);

private:
    void rebuild();
    void relayout(bool animated);
    void updateItemPixmap(int row);

    GameLibraryModel* m_model;
    QGraphicsScene* m_scene;
    QList<CoverItem*> m_items;
    QParallelAnimationGroup* m_anim;
    int m_center;
};
