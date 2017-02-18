/**********************************************************************************************
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

#ifndef CROUTERBROUTERSETUP_H
#define CROUTERBROUTERSETUP_H

#include <QtCore>
#include "setup/IAppSetup.h"

class CRouterBRouterSetup
{
public:
    CRouterBRouterSetup();
    ~CRouterBRouterSetup();
    enum mode_e { ModeLocal, ModeOnline };
    bool expertMode;
    mode_e installMode;
    QString onlineWebUrl;
    QString onlineServiceUrl;
    QString onlineProfilesUrl;
    QStringList onlineProfiles;
    QStringList onlineProfilesAvailable;
    QString localDir;
    QString localProfileDir;
    QString localSegmentsDir;
    QStringList localProfiles;
    QString localHost;
    QString localPort;
    QString binariesUrl;

    void load();
    void save();

    const QString getProfileContent(const int index);
    const QString getProfileContent(const QString profile);
    const QStringList getProfiles();
    const QStringList getProfilesAvailable();
    const QDir getProfileDir();

    void readProfiles();
    void loadOnlineConfig();
    const QString getOnlineProfileContent(const int index);
    void installOnlineProfile(const int index);

private:
    const bool defaultExpertMode = false;
    const mode_e defaultInstallMode = ModeOnline;
    const QString defaultOnlineWebUrl = "http://brouter.de/brouter-web/";
    const QString defaultOnlineServiceUrl = "http://h2096617.stratoserver.net:443";
    const QString defaultOnlineProfilesUrl = "http://brouter.de/brouter/profiles2/";
    const QString defaultLocalDir = ".";
    const QString defaultLocalProfileDir = "profiles2";
    const QString defaultLocalSegmentsDir = "segments4";
    const QString defaultLocalHost = "127.0.0.1";
    const QString defaultLocalPort = "17777";
    const QString defaultBinariesUrl = "http://brouter.de/brouter_bin/";

    void readProfiles(mode_e mode);
    const QString getProfileContent(mode_e mode, QString profile);
    const QDir getProfileDir(mode_e mode);
    const QByteArray loadOnlineProfile(int index);

    const mode_e modeFromString(QString mode);
    const QString stringFromMode(mode_e mode);
};

class CRouterBRouterSetupException : public QException
{
public:
    void raise() const override { throw *this; }
    CRouterBRouterSetupException *clone() const override { return new CRouterBRouterSetupException(*this); }
};
#endif
