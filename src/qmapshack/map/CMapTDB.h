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

#ifndef CMAPTDB_H
#define CMAPTDB_H

#include "map/IMap.h"

// TDB block IDs
#define TDB_BLOCK_HEADER 0x50
#define TDB_BLOCK_COPYRIGHT 0x44
#define TDB_BLOCK_OVERVIEW_MAP 0x42
#define TDB_BLOCK_DETAIL_MAP 0x4C
#define TDB_BLOCK_CHECKSUM 0x54

class CMapIMG;
class CMapDraw;

class CMapTDB : public IMap
{
public:
	CMapTDB(const QString& filename, CMapDraw *parent);
    virtual ~CMapTDB();

    QString getMapName() const override;
    void draw(IDrawContext::buffer_t& buf) override;
    void getToolTip(const QPoint& px, QString& infotext) const override;

public slots:
    void slotSetShowPolygons(bool yes) override;
    void slotSetShowPolylines(bool yes) override;
    void slotSetShowPOIs(bool yes) override;
    void slotSetAdjustDetailLevel(qint32 level) override;
    void slotSetTypeFile(const QString& filename) override;

private:
#pragma pack(1)
	struct blkhdr_t
	{
		quint8 blkid;        // Block ID (0x50)
		quint16 blklen;      // Block length [byte]
	};
#ifdef WIN32
#pragma pack()
#else
#pragma pack(0)
#endif

	struct hdr_blk_t
	{
		struct blkhdr_t hdr; // Block header
		quint16 productId;   // Garmin Product ID
		quint16 familyId;    // Family ID
		quint16 tdbVersion;  // TDB file version * 100
		QString mapName;     // Map series name
		quint16 prodVersion; // Map version * 100
		QString famName;     // Map family name
	};

	struct copyright_t       // One copyright segment inside the copyright block
	{
		quint32 magic;       // Magic ID (0x00000306)
		QString str;         // Copyright string
	};

	struct copyright_blk_t
	{
		struct blkhdr_t hdr; // Block header
		QVector<copyright_t> copyrights; // Copyright segments
	};

	struct overview_map_blk_t
	{
		struct blkhdr_t hdr; // Block header
		quint32 mapNum;
		quint32 parentMapNum;
		quint32 north;
		quint32 east;
		quint32 south;
		quint32 west;
		QString desc;        // Description
        QString imgName;     // IMG file name based on 'mapNum'
	};

	struct detail_map_blk_t
    {
		struct blkhdr_t hdr; // Block header
		quint32 mapNum;
		quint32 parentMapNum;
		quint32 north;
		quint32 east;
		quint32 south;
		quint32 west;
		QString desc;        // Description
        QString imgName;     // IMG file name based on 'mapNum'

		quint16 nMagicNum;   // Unknown (always 0x0004 or 0x0005?)
		quint16 nSubFiles;   // Number of subfiles in IMG-file = N
		QVector<quint32> subFileSizes;
		QVector<QString> subFiles; // Subfile names
	};

	struct checksum_blk_t
	{
		struct blkhdr_t hdr; // Block header
		quint32 chksum;      // 32-bit CRC
		QByteArray reserved; // Rest of the block
	};

    void readFile(const QString& filename, CMapDraw *parent);

	// TDB file contents
	hdr_blk_t blkHdr;
	copyright_blk_t blkCopyrights;
	overview_map_blk_t blkOverview;
	QVector<detail_map_blk_t> blkDetails;
	checksum_blk_t blkChecksum;

    // IMG tiles
    QVector<QPointer<CMapIMG> > imgTiles;
};

#endif // CMAPTDB_H
