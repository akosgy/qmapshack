/**********************************************************************************************
    Copyright (C) 2014 Oliver Eichler oliver.eichler@gmx.de

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

#include "gis/trk/CGisItemTrk.h"
#include "gis/trk/CScrOptTrk.h"
#include "gis/trk/CDetailsTrk.h"
#include "gis/CGisProject.h"
#include "gis/CGisDraw.h"
#include "plot/IPlot.h"
#include "CMainWindow.h"
#include "GeoMath.h"

#include <QtXml>
#include <QtWidgets>
#include <proj_api.h>

#define ASCEND_THRESHOLD    5
#define DEFAULT_COLOR       4
#define MIN_DIST_CLOSE_TO   10
#define MIN_DIST_FOCUS      40

const QColor CGisItemTrk::lineColors[] =
{
     Qt::black                    // 0
    ,Qt::darkRed                 // 1
    ,Qt::darkGreen               // 2
    ,Qt::darkYellow              // 3
    ,Qt::darkBlue                // 4
    ,Qt::darkMagenta             // 5
    ,Qt::darkCyan                // 6
    ,Qt::gray                    // 7
    ,Qt::darkGray                // 8
    ,Qt::red                     // 9
    ,Qt::green                   // 10
    ,Qt::yellow                  // 11
    ,Qt::blue                    // 12
    ,Qt::magenta                 // 13
    ,Qt::cyan                    // 14
    ,Qt::white                   // 15
    ,Qt::transparent             // 16
};

const QString CGisItemTrk::bulletColors[] =
{

                                 // 0
    QString("icons/8x8/bullet_black.png")
                                 // 1
    ,QString("icons/8x8/bullet_dark_red.png")
                                 // 2
    ,QString("icons/8x8/bullet_dark_green.png")
                                 // 3
    ,QString("icons/8x8/bullet_dark_yellow.png")
                                 // 4
    ,QString("icons/8x8/bullet_dark_blue.png")
                                 // 5
    ,QString("icons/8x8/bullet_dark_magenta.png")
                                 // 6
    ,QString("icons/8x8/bullet_dark_cyan.png")
                                 // 7
    ,QString("icons/8x8/bullet_gray.png")
                                 // 8
    ,QString("icons/8x8/bullet_dark_gray.png")
                                 // 9
    ,QString("icons/8x8/bullet_red.png")
                                 // 10
    ,QString("icons/8x8/bullet_green.png")
                                 // 11
    ,QString("icons/8x8/bullet_yellow.png")
                                 // 12
    ,QString("icons/8x8/bullet_blue.png")
                                 // 13
    ,QString("icons/8x8/bullet_magenta.png")
                                 // 14
    ,QString("icons/8x8/bullet_cyan.png")
                                 // 15
    ,QString("icons/8x8/bullet_white.png")
    ,QString("")                 // 16
};

const QPen CGisItemTrk::penBackground(Qt::white, 5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);

QString CGisItemTrk::keyUserFocus;

CGisItemTrk::CGisItemTrk(const QDomNode& xml, CGisProject * parent)
    : IGisItem(parent)
    , penForeground(Qt::blue, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin)
    , pointOfFocus(0)
{        
    // --- start read and process data ----
    setColor(penForeground.color());
    readTrk(xml, trk);
    // --- stop read and process data ----
    setText(0, trk.name);
    setToolTip(0, getInfo());
    genKey();
}

CGisItemTrk::~CGisItemTrk()
{
    // reset user focus if focused on this track
    if(key == keyUserFocus)
    {
        keyUserFocus.clear();
    }

    /**
        Delete all registerd plot as they can't exist without the item.
        As the plot objects will unregister via unregisterPlot() in their
        destructor things will get a bit complicated here. Better create
        a copy of the list befor we start to delete.
    */
    qDeleteAll(registeredPlots.toList());

    delete dlgDetails;

    delete srcOpt;
}

void CGisItemTrk::registerPlot(IPlot * plot)
{
    registeredPlots << plot;
}

void CGisItemTrk::unregisterPlot(IPlot * plot)
{
    registeredPlots.remove(plot);
}


void CGisItemTrk::genKey()
{
    if(key.isEmpty())
    {
        QCryptographicHash md5(QCryptographicHash::Md5);
        md5.addData((const char*)&trk, sizeof(trk));
        key = md5.result().toHex();
    }
}


QString CGisItemTrk::getInfo()
{    
    QString val1, unit1, val2, unit2;
    QString str = getName();
    if(cntVisiblePoints == 0)
    {
        return str;
    }

    IUnit::self().meter2distance(totalDistance, val1, unit1);
    str += "\n";
    str += QObject::tr("Length: %1 %2").arg(val1).arg(unit1);

    if(totalAscend != NOFLOAT && totalDescend != NOFLOAT)
    {
        IUnit::self().meter2elevation(totalAscend, val1, unit1);
        IUnit::self().meter2elevation(totalDescend, val2, unit2);

        str += QObject::tr(", %1%2 %3, %4%5 %6").arg(QChar(0x2197)).arg(val1).arg(unit1).arg(QChar(0x2198)).arg(val2).arg(unit2);
    }


    if(totalElapsedSeconds != NOTIME)
    {
        IUnit::self().seconds2time(totalElapsedSeconds, val1, unit1);
        str += "\n";
        str += QObject::tr("Time: %1").arg(val1);

        IUnit::self().meter2speed(totalDistance / totalElapsedSeconds, val1, unit1);
        str += QObject::tr(", Speed: %1 %2").arg(val1).arg(unit1);
    }

    if(totalElapsedSecondsMoving != NOTIME)
    {
        IUnit::self().seconds2time(totalElapsedSecondsMoving, val1, unit1);
        str += "\n";
        str += QObject::tr("Moving: %1").arg(val1);

        IUnit::self().meter2speed(totalDistance / totalElapsedSecondsMoving, val1, unit1);
        str += QObject::tr(", Speed: %1 %2").arg(val1).arg(unit1);
    }

    if(timeStart.isValid())
    {
        str += "\n";
        str += QObject::tr("Start: %1").arg(IUnit::datetime2string(timeStart, boundingRect.center()));
    }
    if(timeEnd.isValid())
    {
        str += "\n";
        str += QObject::tr("End: %1").arg(IUnit::datetime2string(timeEnd, boundingRect.center()));
    }

    str += "\n";
    str += QObject::tr("Points: %1 (%2)").arg(cntVisiblePoints).arg(cntTotalPoints);


    return str;
}

QString CGisItemTrk::getInfoTrkPt(const trkpt_t& pt)
{

    QString str, val1, unit1;
    str += IUnit::datetime2string(pt.time, QPointF(pt.lon, pt.lat) * DEG_TO_RAD) + "\n";
    IUnit::self().meter2elevation(pt.ele, val1, unit1);
    str += QObject::tr("Ele.: %1 %2").arg(val1).arg(unit1);
    if(pt.slope1 != NOFLOAT)
    {
        str += QObject::tr(" slope: %1°(%2%)").arg(pt.slope1,2,'f',0).arg(pt.slope2, 2,'f',0);
    }
    if(pt.speed != NOFLOAT)
    {
        IUnit::self().meter2speed(pt.speed, val1, unit1);
        str += QObject::tr(" speed: %1%2").arg(val1).arg(unit1);
    }

    return str;
}

IScrOpt * CGisItemTrk::getScreenOptions(const QPoint& origin, IMouse * mouse)
{
    if(srcOpt.isNull())
    {
        srcOpt = new CScrOptTrk(this, origin, mouse);
    }
    return srcOpt;
}

QPointF CGisItemTrk::getPointCloseBy(const QPoint& screenPos)
{
    qint32 d    = NOINT;
    QPointF pt  = NOPOINTF;
    foreach(const QPointF& point, line)
    {
        int tmp = (screenPos - point).manhattanLength();
        if(tmp < d)
        {
            pt  = point;
            d   = tmp;
        }
    }

    return pt;
}

void CGisItemTrk::readTrk(const QDomNode& xml, trk_t& trk)
{
    qreal north = -90;
    qreal east  = -180;
    qreal south =  90;
    qreal west  =  180;

    readXml(xml, "name", trk.name);
    readXml(xml, "cmt", trk.cmt);
    readXml(xml, "desc", trk.desc);
    readXml(xml, "src", trk.src);
    readXml(xml, "link", trk.links);
    readXml(xml, "number", trk.number);
    readXml(xml, "type", trk.type);

    const QDomNodeList& trksegs = xml.toElement().elementsByTagName("trkseg");
    int N = trksegs.count();
    trk.segs.resize(N);
    for(int n = 0; n < N; ++n)
    {
        const QDomNode& trkseg = trksegs.item(n);
        trkseg_t& seg = trk.segs[n];

        const QDomNodeList& xmlTrkpts = trkseg.toElement().elementsByTagName("trkpt");
        int M = xmlTrkpts.count();
        seg.pts.resize(M);
        for(int m = 0; m < M; ++m)
        {
            trkpt_t& trkpt = seg.pts[m];
            const QDomNode& xmlTrkpt = xmlTrkpts.item(m);
            readWpt(xmlTrkpt, trkpt);

            const QDomNode& ext = xmlTrkpt.namedItem("extensions");
            if(ext.isElement())
            {
                readXml(ext, "ql:flags", trkpt.flags);
            }

            if(trkpt.lon < west)  west    = trkpt.lon;
            if(trkpt.lon > east)  east    = trkpt.lon;
            if(trkpt.lat < south) south   = trkpt.lat;
            if(trkpt.lat > north) north   = trkpt.lat;

        }
    }

    boundingRect = QRectF(QPointF(west * DEG_TO_RAD, north * DEG_TO_RAD), QPointF(east * DEG_TO_RAD,south * DEG_TO_RAD));

    // decode some well known extensions
    const QDomNode& ext = xml.namedItem("extensions");
    if(ext.isElement())
    {
        QString str;
        readXml(ext, "ql:key", key);

        const QDomNode& gpxx = ext.namedItem("gpxx:TrackExtension");
        readXml(gpxx, "gpxx:DisplayColor", str);
        setColor(str2color(str));
    }

    deriveSecondaryData();
}

void CGisItemTrk::save(QDomNode& gpx)
{

    QDomDocument doc = gpx.ownerDocument();

    QDomElement xmlTrk = doc.createElement("trk");
    gpx.appendChild(xmlTrk);

    writeXml(xmlTrk, "name", trk.name);
    writeXml(xmlTrk, "cmt", trk.cmt);
    writeXml(xmlTrk, "desc", trk.desc);
    writeXml(xmlTrk, "src", trk.src);
    writeXml(xmlTrk, "link", trk.links);
    writeXml(xmlTrk, "number", trk.number);
    writeXml(xmlTrk, "type", trk.type);

    // write the key as extension tag
    QDomElement xmlExt  = doc.createElement("extensions");
    xmlTrk.appendChild(xmlExt);
    writeXml(xmlExt, "ql:key", key);

    // write other well known extensions
    QDomElement gpxx  = doc.createElement("gpxx:TrackExtension");
    xmlExt.appendChild(gpxx);
    writeXml(gpxx, "gpxx:DisplayColor", color2str(color));

    foreach(const trkseg_t& seg, trk.segs)
    {
        QDomElement xmlTrkseg = doc.createElement("trkseg");
        xmlTrk.appendChild(xmlTrkseg);

        foreach(const trkpt_t& pt, seg.pts)
        {
            QDomElement xmlTrkpt = doc.createElement("trkpt");
            xmlTrkseg.appendChild(xmlTrkpt);
            writeWpt(xmlTrkpt, pt);

            QDomElement xmlExt  = doc.createElement("extensions");
            xmlTrkpt.appendChild(xmlExt);
            writeXml(xmlExt, "ql:flags", pt.flags);
        }
    }
}


void CGisItemTrk::deriveSecondaryData()
{
    // reset all secondary data
    cntTotalPoints          = 0;
    cntVisiblePoints        = 0;
    timeStart               = QDateTime();
    timeEnd                 = QDateTime();
    totalDistance           = NOFLOAT;
    totalAscend             = NOFLOAT;
    totalDescend            = NOFLOAT;
    totalElapsedSeconds     = NOTIME;
    totalElapsedSecondsMoving = NOTIME;


    // remove empty segments
    QVector<trkseg_t>::iterator i = trk.segs.begin();
    while(i != trk.segs.end())
    {
        if((*i).pts.isEmpty())
        {
            i = trk.segs.erase(i);
            continue;
        }
        i++;
    }

    // no segments -> no data -> nothing to do
    if(trk.segs.isEmpty())
    {
        return;
    }

    trkpt_t * lastTrkpt     = 0;
    qreal timestampStart    = NOFLOAT;
    qreal lastEle           = NOFLOAT;

    for(int s = 0; s < trk.segs.size(); s++)
    {
        trkseg_t& seg = trk.segs[s];

        for(int p = 0; p < seg.pts.size(); p++)
        {
            trkpt_t& trkpt = seg.pts[p];

            trkpt.idx = cntTotalPoints++;
            if(trkpt.flags & trkpt_t::eDeleted)
            {
                trkpt.reset();
                continue;
            }
            cntVisiblePoints++;

            if(lastTrkpt != 0)
            {
                trkpt.deltaDistance     = GPS_Math_Distance(lastTrkpt->lon * DEG_TO_RAD, lastTrkpt->lat * DEG_TO_RAD, trkpt.lon * DEG_TO_RAD, trkpt.lat * DEG_TO_RAD);
                trkpt.distance          = lastTrkpt->distance + trkpt.deltaDistance;
                trkpt.elapsedSeconds    = trkpt.time.toMSecsSinceEpoch()/1000.0 - timestampStart;

                // ascend descend
                if(lastEle != NOFLOAT)
                {

                    qreal delta     = trkpt.ele - lastEle;
                    qreal absDelta  = qAbs(delta);

                    if(absDelta > ASCEND_THRESHOLD)
                    {
                        if(delta > 0)
                        {
                            trkpt.ascend  = lastTrkpt->ascend + delta;
                            trkpt.descend = lastTrkpt->descend;
                        }
                        else
                        {
                            trkpt.ascend  = lastTrkpt->ascend;
                            trkpt.descend = lastTrkpt->descend - delta;
                        }
                        lastEle = trkpt.ele;
                    }
                    else
                    {
                        trkpt.ascend     = lastTrkpt->ascend;
                        trkpt.descend    = lastTrkpt->descend;
                    }
                }

                // time moving
                qreal dt = (trkpt.time.toMSecsSinceEpoch() - lastTrkpt->time.toMSecsSinceEpoch()) / 1000.0;
                if(dt > 0 && ((trkpt.deltaDistance / dt) > 0.2))
                {
                    trkpt.elapsedSecondsMoving = lastTrkpt->elapsedSecondsMoving + dt;
                }
                else
                {
                    trkpt.elapsedSecondsMoving = lastTrkpt->elapsedSecondsMoving;
                }

            }
            else
            {
                timeStart       = trkpt.time;
                timestampStart  = timeStart.toMSecsSinceEpoch()/1000.0;
                lastEle         = trkpt.ele;

                trkpt.deltaDistance         = 0;
                trkpt.distance              = 0;
                trkpt.ascend                = 0;
                trkpt.descend               = 0;
                trkpt.elapsedSeconds        = 0;
                trkpt.elapsedSecondsMoving  = 0;
            }

            lastTrkpt = &trkpt;
        }
    }


    // speed and slope (short average +-25m)
    for(int s = 0; s < trk.segs.size(); s++)
    {
        trkseg_t& seg = trk.segs[s];

        for(int p = 0; p < seg.pts.size(); p++)
        {
            trkpt_t& trkpt = seg.pts[p];
            if(trkpt.flags & trkpt_t::eDeleted)
            {
                continue;
            }

            qreal d1 = trkpt.distance;
            qreal e1 = trkpt.ele;
            qreal t1 = trkpt.time.toMSecsSinceEpoch() / 1000.0;
            int n = p;

            while(n>0)
            {
                trkpt_t & trkpt2 = seg.pts[n];
                if((trkpt2.flags & trkpt_t::eDeleted) || (trkpt2.ele == NOINT))
                {
                    n--;
                    continue;
                }

                if(trkpt.distance - trkpt2.distance >= 25)
                {
                    d1 = trkpt2.distance;
                    e1 = trkpt2.ele;
                    t1 = trkpt2.time.toMSecsSinceEpoch()/1000.0;
                    break;
                }
                n--;
            }

            qreal d2 = trkpt.distance;
            qreal e2 = trkpt.ele;
            qreal t2 = trkpt.time.toMSecsSinceEpoch() / 1000.0;
            n = p;
            while(n < seg.pts.size())
            {
                trkpt_t & trkpt2 = seg.pts[n];;
                if((trkpt2.flags & trkpt_t::eDeleted) || (trkpt2.ele == NOINT))
                {
                    n++;
                    continue;
                }

                if(trkpt2.distance - trkpt.distance >= 25)
                {
                    d2 = trkpt2.distance;
                    e2 = trkpt2.ele;
                    t2 = trkpt2.time.toMSecsSinceEpoch() / 1000.0;
                    break;
                }
                n++;
            }

            qreal a         = atan((e2 - e1)/(d2 - d1));
            trkpt.slope1    = qAbs(a * 360.0/(2 * M_PI));
            trkpt.slope2    = qTan(trkpt.slope1 * DEG_TO_RAD) * 100;

            if((t2 - t1) > 0)
            {
                trkpt.speed    = (d2 - d1) / (t2 - t1);
            }
            else
            {
                trkpt.speed = NOFLOAT;
            }
        }
    }



    if(lastTrkpt != 0)
    {
        timeEnd                 = lastTrkpt->time;
        totalDistance           = lastTrkpt->distance;
        totalAscend             = lastTrkpt->ascend;
        totalDescend            = lastTrkpt->descend;
        totalElapsedSeconds     = lastTrkpt->elapsedSeconds;
        totalElapsedSecondsMoving = lastTrkpt->elapsedSecondsMoving;
    }

//    qDebug() << "--------------" << getName() << "------------------";
//    qDebug() << "totalDistance" << totalDistance;
//    qDebug() << "totalAscend" << totalAscend;
//    qDebug() << "totalDescend" << totalDescend;
//    qDebug() << "totalElapsedSeconds" << totalElapsedSeconds;
//    qDebug() << "totalElapsedSecondsMoving" << totalElapsedSecondsMoving;


}


bool CGisItemTrk::isCloseTo(const QPointF& pos)
{
    foreach(const QPointF& pt, line)
    {
        if((pt - pos).manhattanLength() < MIN_DIST_CLOSE_TO)
        {
            return true;
        }
    }
    return false;
}

void CGisItemTrk::gainUserFocus(bool yes)
{    
    keyUserFocus = yes ? key : "";
}

void CGisItemTrk::edit()
{
    if(dlgDetails.isNull())
    {
       dlgDetails = new CDetailsTrk(*this, 0);
       dlgDetails->setObjectName(getName());
    }

    CMainWindow::self().addWidgetToTab(dlgDetails);
}


void CGisItemTrk::drawItem(QPainter& p, const QRectF& viewport, QList<QRectF> &blockedAreas, CGisDraw *gis)
{
    line.clear();
    if(!viewport.intersects(boundingRect))
    {
        return;
    }

    QPointF     pt1;

    QPointF p1 = viewport.topLeft();
    QPointF p2 = viewport.bottomRight();
    gis->convertRad2Px(p1);
    gis->convertRad2Px(p2);
    QRectF extViewport(p1,p2);

    foreach (const trkseg_t& seg, trk.segs)
    {
        foreach(const trkpt_t& pt, seg.pts)
        {
            if(pt.flags & trkpt_t::eDeleted)
            {
                continue;
            }

            pt1.setX(pt.lon);
            pt1.setY(pt.lat);
            pt1 *= DEG_TO_RAD;                       
            line << pt1;
        }
    }

    gis->convertRad2Px(line);

    QList<QPolygonF> lines;
    splitLineToViewport(line, extViewport, lines);

    if(key == keyUserFocus)
    {
        p.setPen(QPen(Qt::red,11,Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        foreach(const QPolygonF& l, lines)
        {
            p.drawPolyline(l);
        }
    }

    p.setBrush(color);
    p.setPen(penBackground);
    foreach(const QPolygonF& l, lines)
    {
        p.drawPolyline(l);
        drawArrows(l, extViewport, p);
    }
    penForeground.setColor(color);
    p.setPen(penForeground);
    foreach(const QPolygonF& l, lines)
    {
        p.drawPolyline(l);
    }    
}

void CGisItemTrk::drawItem(QPainter& p, const QRectF& viewport, CGisDraw * gis)
{
    if(hasUserFocus() && pointOfFocus)
    {
        // derive anchor
        QPointF anchor(pointOfFocus->lon, pointOfFocus->lat);
        anchor *= DEG_TO_RAD;
        gis->convertRad2Px(anchor);

        // create trackpoint info text
        QString str, val1, unit1, val2, unit2;
        str = getInfoTrkPt(*pointOfFocus);

        // calculate bounding box of text
        QFont f = CMainWindow::self().getMapFont();
        QFontMetrics fm(f);
        QRect rectText = fm.boundingRect(QRect(0,0,500,0), Qt::AlignLeft|Qt::AlignTop|Qt::TextWordWrap, str);

        // create info box
        int w = rectText.width()  + 5 + 5;
        int h = rectText.height() + 5 + 5 + 2*(fm.height() + 8);


        QRect box(0, 0, w, h);
        box.moveBottomLeft(anchor.toPoint() + QPoint(-50,-50));

        // create bubble path
        QPainterPath path1;
        path1.addRoundedRect(box,5,5);

        QPolygonF poly2;
        poly2 << anchor << (box.bottomLeft() + QPointF(10,-5)) << (box.bottomLeft() + QPointF(30,-5)) << anchor;
        QPainterPath path2;
        path2.addPolygon(poly2);

        path1 = path1.united(path2);

        p.setFont(f);
        // draw bubble
        p.setPen(CCanvas::penBorderGray);
        p.setBrush(CCanvas::brushBackWhite);
        p.drawPolygon(path1.toFillPolygon());

        p.save();
        p.translate(box.topLeft());
        // draw progress bar distance
        p.translate(5,5);
        QRect rectBar1(0,0,rectText.width(), 5);
        p.setPen(Qt::black);
        p.setBrush(Qt::NoBrush);
        p.drawRect(rectBar1);
        qreal d = pointOfFocus->distance * rectBar1.width() / totalDistance;
        p.drawRect(d-1,-1, 3, 7);

        IUnit::self().meter2distance(pointOfFocus->distance, val1, unit1);
        IUnit::self().meter2distance(totalDistance - pointOfFocus->distance, val2, unit2);
        p.setPen(Qt::darkBlue);
        p.drawText(QRect(0,7,rectBar1.width(),fm.height()), Qt::AlignVCenter|Qt::AlignLeft, QString("%1%2").arg(val1).arg(unit1));
        p.drawText(QRect(0,7,rectBar1.width(),fm.height()), Qt::AlignCenter, QString("%1%").arg(pointOfFocus->distance * 100 / totalDistance, 0, 'f', 0));
        p.drawText(QRect(0,7,rectBar1.width(),fm.height()), Qt::AlignVCenter|Qt::AlignRight, QString("%1%2").arg(val2).arg(unit2));

        // draw progress bar time
        p.translate(0,fm.height() + 8);
        QRect rectBar2(0,0,rectText.width(), 5);
        p.setPen(Qt::black);
        p.setBrush(Qt::NoBrush);
        p.drawRect(rectBar2);
        qreal t = pointOfFocus->elapsedSecondsMoving * rectBar2.width() / totalElapsedSecondsMoving;
        p.drawRect(t-1,-1, 3, 7);

        IUnit::self().seconds2time(pointOfFocus->elapsedSecondsMoving, val1, unit1);
        IUnit::self().seconds2time(totalElapsedSecondsMoving - pointOfFocus->elapsedSecondsMoving, val2, unit2);
        p.setPen(Qt::darkBlue);
        p.drawText(QRect(0,7,rectBar1.width(),fm.height()), Qt::AlignVCenter|Qt::AlignLeft, QString("%1%2").arg(val1).arg(unit1));
        p.drawText(QRect(0,7,rectBar1.width(),fm.height()), Qt::AlignCenter, QString("%1%").arg(pointOfFocus->elapsedSecondsMoving * 100 / totalElapsedSecondsMoving, 0, 'f', 0));
        p.drawText(QRect(0,7,rectBar1.width(),fm.height()), Qt::AlignVCenter|Qt::AlignRight, QString("%1%2").arg(val2).arg(unit2));

        // draw text
        p.translate(0,fm.height() + 8);
        p.setPen(Qt::darkBlue);
        p.drawText(rectText, Qt::AlignLeft|Qt::AlignTop|Qt::TextWordWrap,str);

        p.restore();
    }

}

void CGisItemTrk::drawLabel(QPainter& p, const QRectF& viewport, QList<QRectF> &blockedAreas, const QFontMetricsF &fm, CGisDraw *gis)
{

}


void CGisItemTrk::drawHighlight(QPainter& p)
{
    if(line.isEmpty() || key == keyUserFocus)
    {
        return;
    }
    p.setPen(QPen(QColor(255,0,0,100),11,Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawPolyline(line);
}

void CGisItemTrk::setName(const QString& str)
{
    setText(0, str);
    trk.name = str;
    changed(QObject::tr("Changed name"));
}

void CGisItemTrk::setColor(int idx)
{
    int N = sizeof(lineColors)/sizeof(QColor);
    if(idx >= N)
    {
        return;
    }
    setColor(lineColors[idx]);
    changed(QObject::tr("Changed color"));
}

void CGisItemTrk::setColor(const QColor& c)
{
    int n;
    int N = sizeof(lineColors)/sizeof(QColor);

    for(n = 0; n < N; n++)
    {
        if(lineColors[n] == c)
        {
            colorIdx    = n;
            color       = lineColors[n];
            bullet      = QPixmap(bulletColors[n]);
            break;
        }
    }

    if(n == N)
    {
        colorIdx    = DEFAULT_COLOR;
        color       = lineColors[DEFAULT_COLOR];
        bullet      = QPixmap(bulletColors[DEFAULT_COLOR]);
    }

    setIcon(color.name());
}



void CGisItemTrk::setIcon(const QString& c)
{
    icon = QPixmap("://icons/48x48/Track.png");

    QPixmap mask( icon.size() );
    mask.fill( color );
    mask.setMask( icon.createMaskFromColor( Qt::transparent ) );
    icon = mask.scaled(22,22, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    QTreeWidgetItem::setIcon(0,icon);
}

void CGisItemTrk::setPointOfFocusByDistance(qreal dist, IPlot *initiator)
{
    const trkpt_t * newPointOfFocus = 0;

    if(dist != NOFLOAT)
    {
        qreal delta = totalDistance;

        /// @todo: optimze search by single out segment and then do a binary search

        foreach (const trkseg_t& seg, trk.segs)
        {
            foreach(const trkpt_t& pt, seg.pts)
            {
                if(pt.flags & trkpt_t::eDeleted)
                {
                    continue;
                }

                qreal d = qAbs(pt.distance - dist);
                if(d <= delta)
                {
                    newPointOfFocus = &pt;
                    delta = d;
                }
                else
                {
                    break;
                }
            }            
        }
    }

    publishPointOfFocus(newPointOfFocus, initiator);

}

void CGisItemTrk::setPointOfFocusByTime(quint32 time, IPlot * initiator)
{
    const trkpt_t * newPointOfFocus = 0;

    if(time != NOTIME)
    {
        /// @todo: optimze search by single out segment and then do a binary search

        qreal delta = totalElapsedSeconds;

        foreach (const trkseg_t& seg, trk.segs)
        {
            foreach(const trkpt_t& pt, seg.pts)
            {
                if(pt.flags & trkpt_t::eDeleted)
                {
                    continue;
                }

                qreal d = qAbs(qreal(pt.time.toTime_t()) - qreal(time));
                if(d <= delta)
                {
                    newPointOfFocus = &pt;
                    delta = d;
                }
                else
                {
                    break;
                }
            }
        }
    }

    publishPointOfFocus(newPointOfFocus, initiator);

}

void CGisItemTrk::setPointOfFocusByIndex(quint32 idx)
{
    const trkpt_t * newPointOfFocus = 0;

    foreach (const trkseg_t& seg, trk.segs)
    {
        foreach(const trkpt_t& pt, seg.pts)
        {
            if(pt.idx == idx)
            {
                newPointOfFocus = &pt;
                publishPointOfFocus(newPointOfFocus, 0);
                return;
            }
        }
    }
}

const CGisItemTrk::trkpt_t * CGisItemTrk::getVisibleTrkPtByIndex(quint32 idx)
{
    quint32 i = 0;
    foreach (const trkseg_t& seg, trk.segs)
    {
        foreach(const trkpt_t& pt, seg.pts)
        {
            if(pt.flags & trkpt_t::eDeleted)
            {
                continue;
            }
            if(i == idx)
            {
                return &pt;
            }
            i++;
        }
    }
    return 0;
}

void CGisItemTrk::setPointOfFocusByPoint(const QPoint& pt)
{
    const trkpt_t * newPointOfFocus = 0;

    if(hasUserFocus() || (pt != NOPOINTF))
    {
        /*
            Iterate over the polyline used to draw the track as it contains screen
            coordinates. The polyline has all visible track points. But there are
            invisible points, those marked as deleted, too. That is why the index
            into the polyline cant't be used directly. In a second step we have
            to iterate over all segments and visible points of the trk_t object
            until the visible index is reached. This is done by getVisibleTrkPtByIndex().
        */

        quint32 i   = 0;
        quint32 idx = 0;
        qint32 d    = NOINT;
        foreach(const QPointF& point, line)
        {
            int tmp = (pt - point).manhattanLength();
            if(tmp < d)
            {
                idx = i;
                d   = tmp;
            }
            i++;
        }

        if(d < MIN_DIST_FOCUS)
        {
            newPointOfFocus = getVisibleTrkPtByIndex(idx);
        }
    }
    publishPointOfFocus(newPointOfFocus, 0);
}


void CGisItemTrk::publishPointOfFocus(const trkpt_t * pt,  IPlot * initiator)
{
    if(pt != pointOfFocus)
    {
        pointOfFocus = pt;
        foreach(IPlot * plot, registeredPlots)
        {
            if(plot != initiator)
            {
                plot->setPointOfFocus(pointOfFocus);
            }
        }

        if(!dlgDetails.isNull())
        {
            dlgDetails->setPointOfFocus(pointOfFocus);
        }
    }
}
