/**********************************************************************************************
    Copyright (C) 2014-2015 Oliver Eichler oliver.eichler@gmx.de
    Copyright (C) 2017 Norbert Truchsess norbert.truchsess@t-online.de

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

**********************************************************************************************/

#include "CMainWindow.h"
#include "GeoMath.h"
#include "canvas/CCanvas.h"
#include "gis/CGisDraw.h"
#include "gis/rte/router/CRouterSetup.h"
#include "mouse/line/ILineOp.h"
#include "mouse/line/IMouseEditLine.h"

#include <QtWidgets>

ILineOp::ILineOp(SGisLine& points, CGisDraw *gis, CCanvas *canvas, IMouseEditLine *parent)
    : QObject(parent)
    , parentHandler(parent)
    , points(points)
    , canvas(canvas)
    , gis(gis)
{
    timerRouting = new QTimer(this);
    timerRouting->setSingleShot(true);
    timerRouting->setInterval(400);
    connect(timerRouting, &QTimer::timeout, this, &ILineOp::slotTimeoutRouting);
}

ILineOp::~ILineOp()
{
    canvas->reportStatus("Routino", QString());
}

void ILineOp::cancelDelayedRouting()
{
    timerRouting->stop();
}

void ILineOp::startDelayedRouting()
{
    if(parentHandler->useAutoRouting())
    {
        timerRouting->start();
    }
    else if(parentHandler->useVectorRouting())
    {
        slotTimeoutRouting();
    }
}

void ILineOp::slotTimeoutRouting()
{
    timerRouting->stop();
    finalizeOperation(idxFocus);
    canvas->slotTriggerCompleteUpdate(CCanvas::eRedrawMouse);
}


void ILineOp::drawBg(QPainter& p)
{
    drawLeadLine(leadLinePixel1,p);
    drawLeadLine(leadLinePixel2,p);
}

void ILineOp::drawSinglePointSmall(const QPointF& pt, QPainter& p)
{
    QRect r(0,0,3,3);
    r.moveCenter(pt.toPoint());

    p.setPen(QPen(Qt::white, 2));
    p.setBrush(Qt::white);
    p.drawRect(r);

    p.setPen(Qt::black);
    p.setBrush(Qt::black);
    p.drawRect(r);
}

void ILineOp::drawSinglePointLarge(const QPointF &pt, QPainter& p)
{
    rectPoint.moveCenter(pt.toPoint());

    p.setPen(penBgPoint);
    p.setBrush(brushBgPoint);
    p.drawRect(rectPoint);

    p.setPen(penFgPoint);
    p.setBrush(brushFgPoint);
    p.drawRect(rectPoint);
}

void ILineOp::drawLeadLine(const QPolygonF& line, QPainter& p) const
{
    p.setPen(QPen(Qt::yellow, 7, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawPolyline(line);
}

void ILineOp::mouseMove(const QPoint& pos)
{
    updateLeadLines(idxFocus);
}

void ILineOp::leftButtonDown(const QPoint& pos)
{
    timerRouting->stop();
    showRoutingErrorMessage(QString());
}

void ILineOp::rightButtonDown(const QPoint& pos)
{
    //TODO check whether this is appropriate:
    leftButtonDown(pos);
}

void ILineOp::scaleChanged()
{
    timerRouting->stop();
}


//void ILineOp::mousePressEvent(QMouseEvent * e)
//{
//    if(idxFocus != NOIDX)
//    {
//        mousePressEventEx(e);
//    }


//    if(e->button() == Qt::LeftButton)
//    {
//        lastPos    = e->pos();
//        mapMove    = true;
//    }

//    // make sure that on-the-fly-routing will
//    // not trigger before the mouse has been moved a bit
//    startMouseMove(e->pos());
//    // make sure a click is actually shorter than longButtonPressTimeout
//    buttonPressTime.start();
//    ignoreClick = false;

//    showRoutingErrorMessage(QString());
//}

//void ILineOp::mouseMoveEvent(QMouseEvent * e)
//{
//    const QPoint& pos = e->pos();

//    // do not take the mouse as moving unless it has been moved
//    // by significant distance away from starting point.
//    // this helps doing clicks with the finger on a touchscreen
//    // and suppresses routing triggered by very small movements.
//    if (!mouseDidMove && (pos - firstPos).manhattanLength() >= IMouse::minimalMouseMovingDistance)
//    {
//        mouseDidMove = true;
//    }

//    if (mouseDidMove)
//    {
//        if(mapMove)
//        {
//            QPoint delta = pos - lastPos;
//            canvas->moveMap(delta);
//        }
//        else
//        {
//            updateLeadLines(idxFocus);
//            mouseMoveEventEx(e);
//        }
//    }

//    lastPos = pos;
//}

//void ILineOp::mouseReleaseEvent(QMouseEvent *e)
//{
//    // suppress map-movement, long-clicks and button-release after zooming or display of CProgressDialog
//    if(!(mouseDidMove && mapMove) && !ignoreClick && (buttonPressTime.elapsed() < IMouse::longButtonPressTimeout))
//    {
//        mouseReleaseEventEx(e);
//    }

//    mapMove     = false;
//    mouseDidMove  = false;
//}

//void ILineOp::wheelEvent(QWheelEvent *e)
//{
//    // suppress little mouse-movements that are likely to happen when scrolling the mousewheel.
//    startMouseMove(e->pos());
//    if (e->buttons() != Qt::NoButton)
//    {
//        // no shortclick by releasing button right after scrolling the wheel
//        ignoreClick = true;
//    }
//}

//void ILineOp::pinchGestureEvent(QPinchGesture *e)
//{
//    // consider finger being down (equivalent to button pressed) during pinch
//    mapMove = true;
//    // no shortclick by lifting the finger right after a pinch
//    ignoreClick = true;
//    // no on-the-fly-routing during pinch
//    timerRouting->stop();
//}

//TODO check startMouseMove calling timerRouting->stop:

//void ILineOp::afterMouseLostEvent(QMouseEvent *e)
//{
//    // pinch or modal dialog interrupt tracking of mouse. As result the mouse
//    // is at an arbitrary position.
//    if (e->type() == QEvent::MouseMove)
//    {
//        // suppress jump of map when touching screen right afterwards
//        lastPos    = e->pos();
//        // consider the move starting at this position
//        startMouseMove(e->pos());
//    }
//    mapMove = e->buttons() & Qt::LeftButton;
//}

void ILineOp::startMouseMove(const QPointF& point)
{
//    // the mouse is not considered as moving
//    // as long it has not been moved away from firstPos
//    // by at least a few pixels.
//    firstPos = pos.toPoint();
//    mouseDidMove = false;
//    // as long the mouse is not taken as moving
//    // to not trigger on-the-fly-routing
    parentHandler->startMouseMove(point.toPoint());
    timerRouting->stop();
}

void ILineOp::updateLeadLines(qint32 idx)
{
    leadLinePixel1.clear();
    leadLinePixel2.clear();
    subLinePixel1.clear();
    subLinePixel2.clear();

    if(parentHandler->useVectorRouting() && (idx != NOIDX))
    {
        leadLineCoord1.clear();
        leadLineCoord2.clear();
        subLineCoord1.clear();
        subLineCoord2.clear();

        if(idx > 0)
        {
            const IGisLine::point_t& pt1 = points[idx - 1];
            const IGisLine::point_t& pt2 = points[idx];
            if(canvas->findPolylineCloseBy(pt2.pixel, pt2.pixel, 10, leadLineCoord1))
            {
                leadLinePixel1 = leadLineCoord1;
                gis->convertRad2Px(leadLinePixel1);

                segment_t result;
                GPS_Math_SubPolyline(pt1.pixel, pt2.pixel, 10, leadLinePixel1, result);
                result.apply(leadLineCoord1, leadLinePixel1, subLineCoord1, subLinePixel1, gis);
            }
        }

        if(idx < points.size() - 1)
        {
            const IGisLine::point_t& pt1 = points[idx];
            const IGisLine::point_t& pt2 = points[idx + 1];
            if(canvas->findPolylineCloseBy(pt1.pixel, pt1.pixel, 10, leadLineCoord2))
            {
                leadLinePixel2 = leadLineCoord2;
                gis->convertRad2Px(leadLinePixel2);

                segment_t result;
                GPS_Math_SubPolyline(pt1.pixel, pt2.pixel, 10, leadLinePixel2, result);
                result.apply(leadLineCoord2, leadLinePixel2, subLineCoord2, subLinePixel2, gis);
            }
        }
    }
}

void ILineOp::showRoutingErrorMessage(const QString &msg) const
{
    if(msg.isEmpty())
    {
        canvas->reportStatus("Routino", QString());
    }
    else
    {
        canvas->reportStatus("Routino", QString("<span style='color: red;'><b>%1</b><br />%2</span>").arg(tr("Routing")).arg(msg));
    }
}

void ILineOp::tryRouting(IGisLine::point_t& pt1, IGisLine::point_t& pt2) const
{
    QPolygonF subs;

    try
    {
        if(CRouterSetup::self().calcRoute(pt1.coord, pt2.coord, subs) >= 0)
        {
            pt1.subpts.clear();
            for(const QPointF &sub : subs)
            {
                pt1.subpts << IGisLine::subpt_t(sub);
            }
        }
        showRoutingErrorMessage(QString());
    }
    catch(const QString &msg)
    {
        showRoutingErrorMessage(msg);
    }
    // that is a workaround for canvas loosing mousetracking caused by CProgressDialog being modal:
    canvas->setMouseTracking(true);
}

void ILineOp::finalizeOperation(qint32 idx)
{
    if(idx == NOIDX)
    {
        return;
    }

    if(parentHandler->useAutoRouting())
    {
        CCanvas::setOverrideCursor(Qt::WaitCursor,"ILineOp::finalizeOperation");
        if(idx > 0)
        {
            tryRouting(points[idx - 1], points[idx]);
        }
        if(idx < (points.size() - 1))
        {
            tryRouting(points[idx], points[idx + 1]);
        }
        CCanvas::restoreOverrideCursor("ILineOp::finalizeOperation");
    }
    else if(parentHandler->useVectorRouting())
    {
        if(idx > 0)
        {
            IGisLine::point_t& pt1 = points[idx - 1];
            pt1.subpts.clear();
            for(const QPointF &pt : subLineCoord1)
            {
                pt1.subpts << IGisLine::subpt_t(pt);
            }
        }

        if(idx < (points.size() - 1))
        {
            IGisLine::point_t& pt1 = points[idx];
            pt1.subpts.clear();
            for(const QPointF &pt : subLineCoord2)
            {
                pt1.subpts << IGisLine::subpt_t(pt);
            }
        }
    }

    // need to move the mouse away by some pixels to trigger next routing event
    startMouseMove(points[idx].pixel);

    parentHandler->updateStatus();
}


qint32 ILineOp::isCloseTo(const QPoint& pos) const
{
    qint32 min = 30;
    qint32 idx = NOIDX;
    const int N = points.size();
    for(int i = 0; i < N; i++)
    {
        const IGisLine::point_t& pt = points[i];

        qint32 d = (pos - pt.pixel).manhattanLength();
        if(d < min)
        {
            min = d;
            idx = i;
        }
    }

    return idx;
}

qint32 ILineOp::isCloseToLine(const QPoint& pos) const
{
    qint32 idx = NOIDX;
    qreal dist = 60;

    for(int i = 0; i < points.size() - 1; i++)
    {
        QPolygonF line;
        const IGisLine::point_t& pt1 = points[i];
        const IGisLine::point_t& pt2 = points[i + 1];

        if(pt1.subpts.isEmpty())
        {
            line << pt1.pixel << pt2.pixel;
        }
        else
        {
            line << pt1.pixel;
            for(const IGisLine::subpt_t& pt : pt1.subpts)
            {
                line << pt.pixel;
            }
            line << pt2.pixel;
        }

        qreal d = GPS_Math_DistPointPolyline(line, pos);
        if(d < dist)
        {
            dist = d;
            idx  = i;
        }
    }

    return idx;
}
