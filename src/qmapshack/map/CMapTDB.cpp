/**********************************************************************************************
    Copyright (C) 2014 Oliver Eichler <oliver.eichler@gmx.de>

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

/*
 * Garmin TDB map aggregate format handler
 *
 * Based on the description at: https://wiki.openstreetmap.org/wiki/OSM_Map_On_Garmin/TDB_File_Format
 *
 * Copyright (C) 2020 Akos Gyarmathy
 */

#include "helpers/CDraw.h"
#include "inttypes.h"
#include "map/CMapDraw.h"
#include "map/CMapTDB.h"
#include "map/CMapIMG.h"
#include "units/IUnit.h"

#include <QtGui>

static void readCString(QDataStream& stream, QString& str)
{
	quint8 byte;
	QByteArray ba;

	ba.clear();

	stream >> byte;
	while(byte != 0)
	{
		ba += byte;
		stream >> byte;
	}
	str = QString::fromUtf8(ba);
}

CMapTDB::CMapTDB(const QString &filename, CMapDraw *parent)
    : IMap(filename,eFeatVisibility | eFeatVectorItems | eFeatTypFile, parent)
{
    qDebug() << "------------------------------";
	qDebug() << "TDB: try to open" << filename;

    readFile(filename, parent);
}

CMapTDB::~CMapTDB()
{
    // Iterate through the IMG tiles release the resources
    for(QVector<QPointer<CMapIMG> >::iterator it = imgTiles.begin(); it != imgTiles.end(); ++it)
    {
        if(!it->isNull())
        {
            delete (*it);
            (*it) = NULL;
        }
    }
}

void CMapTDB::readFile(const QString& filename, CMapDraw *parent)
{
	int i;
	blkhdr_t hdr;
	copyright_t cp;

    qDebug() << filename;

    QFileInfo fi(filename);
    QFile file(filename);
    file.open(QIODevice::ReadOnly);

    QDataStream stream(&file);
	stream.setByteOrder(QDataStream::LittleEndian);

	// Reading TDB blocks
	do {
		// Read block header
		hdr.blkid = 0; hdr.blklen = 0;
		stream >> hdr.blkid;
		stream >> hdr.blklen;
		if(0 == hdr.blklen)
			break;

		// Read block data
		QByteArray blkData;
		blkData.resize(hdr.blklen);
		int bytesRead = stream.readRawData(blkData.data(), hdr.blklen);
		if(bytesRead < hdr.blklen)
			break;

		qDebug() << hex << "Block ID" << hdr.blkid << dec << "Length" << hdr.blklen;

		// Process block data
		QDataStream blks(blkData);
        blks.setByteOrder(QDataStream::LittleEndian);
        switch(hdr.blkid) {
		case TDB_BLOCK_HEADER:
			blkHdr.hdr = hdr;
			blks >> blkHdr.productId;
			blks >> blkHdr.familyId;
			blks >> blkHdr.tdbVersion;
			readCString(blks,blkHdr.mapName);
			blks >> blkHdr.prodVersion;
			readCString(blks,blkHdr.famName);
			break;
		case TDB_BLOCK_COPYRIGHT:
			blkCopyrights.hdr = hdr;
			blks >> cp.magic;
			while(!blks.status()) {
				readCString(blks,cp.str);
				qDebug() << cp.str;
				blkCopyrights.copyrights.push_back(cp);
				cp.str.clear();
				blks >> cp.magic;
			}
			break;
		case TDB_BLOCK_OVERVIEW_MAP:
			blkOverview.hdr = hdr;
			blks >> blkOverview.mapNum;
			blks >> blkOverview.parentMapNum;
			blks >> blkOverview.north;
			blks >> blkOverview.east;
			blks >> blkOverview.south;
			blks >> blkOverview.west;
			readCString(blks,blkOverview.desc);
			break;
		case TDB_BLOCK_DETAIL_MAP:
		{
			detail_map_blk_t detail;
			detail.hdr = hdr;
			blks >> detail.mapNum;
			blks >> detail.parentMapNum;
			blks >> detail.north;
			blks >> detail.east;
			blks >> detail.south;
			blks >> detail.west;
			readCString(blks,detail.desc);
			blks >> detail.nMagicNum;
			blks >> detail.nSubFiles;
			if(detail.nSubFiles < 20) {
				for(i=0;i<detail.nSubFiles;++i) {
					quint32 val;
					blks >> val;
					detail.subFileSizes.push_back(val);
				}
				blks.skipRawData(7);
				for(i=0;i<detail.nSubFiles;++i) {
					QString s;
					readCString(blks,s);
					detail.subFiles.push_back(s);
				}
			}
			blkDetails.push_back(detail);
			break;
		}
		case TDB_BLOCK_CHECKSUM:
			blkChecksum.hdr = hdr;
			break;
		default:
			break;
		}
	}while(!stream.status());

	qDebug() << "Version   :" << (blkHdr.tdbVersion / 100) << "Prod" <<  (blkHdr.prodVersion / 100);
	qDebug() << "Map name  :" << blkHdr.mapName << blkHdr.productId;
	qDebug() << "Map family:" << blkHdr.famName << blkHdr.familyId;

	qDebug() << "Copyrights:";
	for(QVector<copyright_t>::const_iterator it = blkCopyrights.copyrights.begin();it != blkCopyrights.copyrights.end();++it)
	{
        // Set first item as MapItem copyright
        if(it == blkCopyrights.copyrights.begin())
        {
            copyright = it->str;
        }
		qDebug() << it->str;
	}

    // Basemap IMG filename is the same as the TDB file with .img extension
    blkOverview.imgName = fi.completeBaseName() + ".img";
    // Open the basemap tile
    imgTiles.push_back(new CMapIMG(fi.path() + "/" + blkOverview.imgName,parent));

    qDebug() << "Overview  :" << blkOverview.imgName <<
				"(" << blkOverview.north << "," << blkOverview.east <<
				"," << blkOverview.south << "," << blkOverview.west << ")" << blkOverview.desc;

	qDebug() << "Map files:";
    for(QVector<detail_map_blk_t>::iterator sit = blkDetails.begin();sit != blkDetails.end();++sit)
	{
        detail_map_blk_t &d = *sit;
        // IMG file name is the map number padded with zeros up to 8 digits + .img extension
        d.imgName = QString("%1.img").arg(d.mapNum, 8, 10, QChar('0'));
        imgTiles.push_back(new CMapIMG(fi.path() + "/" + d.imgName,parent));
        qDebug() << "Detail   :" << d.imgName <<
					"(" << d.north << "," << d.east <<
					"," << d.south << "," << d.west << ")" << d.desc;
	}

    isActivated = true;
}

void CMapTDB::slotSetShowPolygons(bool yes)
{
    IMap::slotSetShowPolygons(yes);

    // Iterate through the IMG tiles and set the typ file for each subtile
    for(QVector<QPointer<CMapIMG> >::iterator it = imgTiles.begin(); it != imgTiles.end(); ++it)
    {
        if(!it->isNull())
        {
            (*it)->slotSetShowPolygons(yes);
        }
    }
}

void CMapTDB::slotSetShowPolylines(bool yes)
{
    IMap::slotSetShowPolylines(yes);

    // Iterate through the IMG tiles and set the typ file for each subtile
    for(QVector<QPointer<CMapIMG> >::iterator it = imgTiles.begin(); it != imgTiles.end(); ++it)
    {
        if(!it->isNull())
        {
            (*it)->slotSetShowPolylines(yes);
        }
    }
}

void CMapTDB::slotSetShowPOIs(bool yes)
{
    IMap::slotSetShowPOIs(yes);

    // Iterate through the IMG tiles and set the typ file for each subtile
    for(QVector<QPointer<CMapIMG> >::iterator it = imgTiles.begin(); it != imgTiles.end(); ++it)
    {
        if(!it->isNull())
        {
            (*it)->slotSetShowPOIs(yes);
        }
    }
}

void CMapTDB::slotSetAdjustDetailLevel(qint32 level)
{
    IMap::slotSetAdjustDetailLevel(level);

    // Iterate through the IMG tiles and set the typ file for each subtile
    for(QVector<QPointer<CMapIMG> >::iterator it = imgTiles.begin(); it != imgTiles.end(); ++it)
    {
        if(!it->isNull())
        {
            (*it)->slotSetAdjustDetailLevel(level);
        }
    }
}

void CMapTDB::slotSetTypeFile(const QString& filename)
{
    IMap::slotSetTypeFile(filename);

    // Iterate through the IMG tiles and set the typ file for each subtile
    for(QVector<QPointer<CMapIMG> >::iterator it = imgTiles.begin(); it != imgTiles.end(); ++it)
    {
        if(!it->isNull())
        {
            (*it)->slotSetTypeFile(filename);
        }
    }
}

void CMapTDB::draw(IDrawContext::buffer_t& buf) /* override */
{
    if(map->needsRedraw())
    {
        return;
    }

    // Iterate through the IMG tiles and call the draw function
    for(QVector<QPointer<CMapIMG> >::iterator it = imgTiles.begin(); it != imgTiles.end(); ++it)
    {
        if(!it->isNull())
        {
            (*it)->draw(buf);
        }
    }
}
