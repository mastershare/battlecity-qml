/****************************************************************************
**
** Copyright (C) 2011 Kirill (spirit) Klochkov.
** Contact: klochkov.kirill@gmail.com
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
****************************************************************************/

#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QPixmapCache>
#include <QKeyEvent>

#include "bcboard.h"
#include "bcglobal.h"
#include "bctank.h"

BCBoard::BCBoard(QDeclarativeItem *parent) :
    QDeclarativeItem(parent),
    m_boardSize(13),
    m_cellSize(35.0),
    m_gridVisible(false),
    m_falcon(0),
    m_playerTank(0)
{
    setFlag(QGraphicsItem::ItemHasNoContents, false);
    setFlag(QGraphicsItem::ItemIsFocusable, true);
    setClip(true);
    setFocus(true);
    setCursor(BattleCity::Ground);

    init();
}

void BCBoard::setBoardSize(int boardSize)
{
    if (m_boardSize == boardSize)
        return;
    m_boardSize = boardSize;
    emit boardSizeChanged();
    update();
}

void BCBoard::setCellSize(qreal size)
{
    if (m_cellSize == size)
        return;
    m_cellSize = size;
    emit cellSizeChanged(m_cellSize);
    update();
}

qreal BCBoard::obsticaleSize() const
{
    return m_cellSize / 2;
}

void BCBoard::init()
{
    foreach (const QList<BCItem *> &cells, m_cells)
        qDeleteAll(cells);
    m_cells.clear();

    qDeleteAll(m_enemyTanks);
    m_enemyTanks.clear();

    delete m_falcon;
    delete m_playerTank;

    const int size = m_boardSize * m_cellSize + 1;
    setImplicitWidth(size);
    setImplicitHeight(size);
    qreal x = 0.0;
    qreal y = 0.0;
    for (int row = 0; row < m_boardSize * 2; ++row) {
        QList<BCItem *> items;
        for (int column = 0; column < m_boardSize * 2; ++column) {
            BCGroud *cell = new BCGroud(this);
            connect(this, SIGNAL(cellSizeChanged(qreal)), cell, SLOT(setSize(qreal)));
            cell->setPosition(row, column);
            cell->setSize(obsticaleSize());
            items << cell;
            x += obsticaleSize();
        }
        m_cells << items;
        x = 0.0;
        y += obsticaleSize();
    }

    m_playerTank = new BCPlayerTank(this);
    m_playerTank->setSize(m_cellSize);
    m_playerTank->setPosition(4, 12);

    m_falcon = new BCFalcon(this);
    m_falcon->setSize(m_cellSize);
    m_falcon->setPosition(6, 12);

    static const quint8 enemyTanksCount = 20;
//    for (quint8 i = 0; i < enemyTanksCount; ++i) {

//    }
    BCArmorTank *tank = new BCArmorTank(this);
    tank->setSize(m_cellSize);
    tank->setPosition(0, 0);
    tank->setBonus(true);
    tank->hit();
    tank->hit();
//    tank->hit();
    m_enemyTanks << tank;
}

BCItem *BCBoard::obstacle(int row, int column) const
{
    if (row >= m_cells.count() || row < 0 || column < 0)
        return 0;
    const QList<BCItem *> &items = m_cells[row];
    if (column >= items.count())
        return 0;
    return items[column];
}

static BCItem *createObstacle(BattleCity::ObstacleType type, BCBoard *parent)
{
    BCItem *obstacle = 0;
    switch (type) {
    case BattleCity::BricksWall:
        obstacle = new BCBricks(parent);
        break;
    case BattleCity::ConcreteWall:
        obstacle = new BCConcrete(parent);
        break;
    case BattleCity::Ice:
        obstacle = new BCIce(parent);
        break;
    case BattleCity::Camouflage:
        obstacle = new BCCamouflage(parent);
        break;
    case BattleCity::Water:
        obstacle = new BCWater(parent);
        break;
    default:
        obstacle = new BCGroud(parent);
        break;
    }
    return obstacle;
}

void BCBoard::setObstacleType(int row, int column, int type)
{
    BCItem *obstacle = this->obstacle(row, column);
    if (!obstacle || obstacle->type() == type)
        return;
    BCItem *newObstacle = ::createObstacle(BattleCity::ObstacleType(type), this);
    newObstacle->setPosition(obstacle->row(), obstacle->column());
    newObstacle->setSize(obstacle->size());
    obstacle->deleteLater();
    m_cells[row][column] = newObstacle;
}

//int BCBoard::xToRow(qreal x) const
//{
//    return (qRound(x) % qRound(implicitWidth())) / obsticaleSize();
//}

//int BCBoard::yToColumn(qreal y) const
//{
//    return (qRound(y) % qRound(implicitHeight())) / obsticaleSize();
//}

void BCBoard::setGridVisible(bool visible)
{
    if (m_gridVisible == visible)
        return;
    m_gridVisible = visible;
    emit gridVisibleChanged();
    update();
}

void BCBoard::setCursor(int type)
{
    QPixmap pixmap = BattleCity::cursorPixmap(BattleCity::ObstacleType(type));
    pixmap = pixmap.scaled(QSize(pixmap.width() * 2, pixmap.height() * 2));
    QDeclarativeItem::setCursor(QCursor(pixmap));
}

QDataStream &operator << (QDataStream &out, const BCBoard &board)
{
    out << board.m_boardSize;
    foreach (const QList<BCItem *> &rows, board.m_cells) {
        foreach (BCItem *obstacle, rows)
            out << obstacle->type();
    }
    return out;
}

QDataStream &operator >> (QDataStream &in, BCBoard &board)
{
    in >> board.m_boardSize;
    board.init();
    for (int row = 0; row < board.m_boardSize * 2; ++row) {
        for (int colum = 0; colum < board.m_boardSize * 2; ++colum) {
            int type = -1;
            in >> type;
            board.setObstacleType(row, colum, type);
        }
    }

    return in;
}

void BCBoard::keyPressEvent(QKeyEvent *event)   //TODO: must be in Controller
{
    BCAbstractTank *tank = /*m_enemyTanks.first();*/m_playerTank;
    if (event->key() == Qt::Key_Up)
        tank->move(BattleCity::Forward);
    if (event->key() == Qt::Key_Down)
        tank->move(BattleCity::Backward);
    if (event->key() == Qt::Key_Left)
        tank->move(BattleCity::Left);
    if (event->key() == Qt::Key_Right)
        tank->move(BattleCity::Right);
}
#ifdef BC_DEBUG_RECT
void BCBoard::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    painter->fillRect(m_debugRect, Qt::red);
}
#endif
