#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "httpmanager.h"
#include <QtCore>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    audio = new Audio();
    audio->updateAudioDevices(ui->audioOutCombo);
    audioinput = new AudioInput();
    audioinput->get_audioinput_devices(ui->audioInCombo);
    isTx = false;

    ui->connectButton->setEnabled(true);
    ui->tgidEdit->setVisible(false);
    ui->tgidLabel->setVisible(false);
    ui->slotLabel->setVisible(false);
    ui->slotCombo->setVisible(false);
    ui->m17CanLabel->setVisible(false);
    ui->m17CanCombo->setVisible(false);
    ui->moduleLabel->setVisible(false);
    ui->moduleCombo->setVisible(false);
    ui->colorCodeLabel->setVisible(false);
    ui->colorCodeCombo->setVisible(false);

    digihamlib = new Digihamlib();

    m_settings_processed = false;
    m_modelchange = false;
    m_settings = new QSettings(QSettings::IniFormat, QSettings::UserScope, "kd0oss", "Digihamlib", this);
    config_path = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_WIN)
    config_path += "/kd0oss";
    qDebug() << "Config path:" << config_path;
#endif
#if defined(Q_OS_ANDROID)
    keepScreenOn();
    m_USBmonitor = &AndroidSerialPort::GetInstance();
    connect(m_USBmonitor, SIGNAL(devices_changed()), this, SLOT(discover_devices()));
#endif
    check_host_files();
    process_settings();

    ui->volumeLevelBar->setValue(0);

    connect(ui->connectButton, SIGNAL(released()), this, SLOT(do_connect()));
    connect(digihamlib, SIGNAL(update_log(QString)), this, SLOT(updateLog(QString)));
    connect(digihamlib, SIGNAL(send_audio(int16_t*,int)), this, SLOT(process_audio(int16_t*,int)));
    connect(digihamlib, SIGNAL(update_data()), this, SLOT(update_status()));
    connect(audioinput, SIGNAL(mic_send_audio(QQueue<qint16>*)), this, SLOT(micSendAudio(QQueue<qint16>*)));
    connect(ui->micGainSlider, SIGNAL(valueChanged(int)), this, SLOT(setMicGain(int)));
    connect(ui->modeCombo, SIGNAL(currentTextChanged(QString)), this, SLOT(process_mode_change(QString)));
    connect(ui->debugCB, SIGNAL(clicked(bool)), digihamlib, SLOT(set_debug(bool)));
    connect(ui->moduleCombo, SIGNAL(currentTextChanged(QString)), digihamlib, SLOT(set_module(QString)));
    connect(ui->m17CanCombo, SIGNAL(currentTextChanged(QString)), digihamlib, SLOT(set_modemM17CAN(QString)));
    connect(ui->reflectorCombo, SIGNAL(currentTextChanged(QString)), digihamlib, SLOT(set_reflector(QString)));
    connect(ui->tgidEdit, SIGNAL(currentTextChanged(QString)), digihamlib, SLOT(set_dmrtgid(QString)));
    connect(ui->colorCodeCombo, SIGNAL(currentIndexChanged(int)), digihamlib, SLOT(set_cc(int)));
    connect(ui->slotCombo, SIGNAL(currentIndexChanged(int)), digihamlib, SLOT(set_slot(int)));
    connect(ui->audioOutCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(audioDeviceChanged(int)));
    connect(ui->audioInCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(micDeviceChanged(int)));
    connect(ui->vocoderCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(vocoderChanged(int)));
    connect(ui->modemCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(modemChanged(int)));

    discover_devices();
    audioinput->setMicGain(90);
    ui->micGainSlider->setValue(90);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::save_settings()
{
    //m_settings->setValue("PLAYBACK", ui->comboPlayback->currentText());
    //m_settings->setValue("CAPTURE", ui->comboCapture->currentText());
    m_settings->setValue("IPV6", m_ipv6 ? "true" : "false");
    m_settings->setValue("MODE", digihamlib->get_mode());
    m_settings->setValue("HOST", digihamlib->get_reflector());
    m_settings->setValue("MODULE", digihamlib->get_module());
    m_settings->setValue("CALLSIGN", digihamlib->get_callsign());
    m_settings->setValue("DMRID", digihamlib->get_dmrid());
    m_settings->setValue("ESSID", digihamlib->get_essid());
    m_settings->setValue("BMPASSWORD", digihamlib->get_bm_password());
    m_settings->setValue("TGIFPASSWORD", digihamlib->get_tgif_password());
    m_settings->setValue("DMRTGID", digihamlib->get_dmrtgid());
    m_settings->setValue("DMRLAT", digihamlib->get_latitude());
    m_settings->setValue("DMRLONG", digihamlib->get_longitude());
    m_settings->setValue("DMRLOC", digihamlib->get_location());
    m_settings->setValue("DMRDESC", digihamlib->get_description());
    m_settings->setValue("DMRFREQ", digihamlib->get_freq());
    m_settings->setValue("DMRURL", digihamlib->get_url());
    m_settings->setValue("DMRSWID", digihamlib->get_swid());
    m_settings->setValue("DMRPKGID", digihamlib->get_pkgid());
    m_settings->setValue("DMROPTS", digihamlib->get_dmr_options());
    m_settings->setValue("MYCALL", digihamlib->get_mycall());
    m_settings->setValue("URCALL", digihamlib->get_urcall());
    m_settings->setValue("RPTR1", digihamlib->get_rptr1());
    m_settings->setValue("RPTR2", digihamlib->get_rptr2());
    m_settings->setValue("TXTIMEOUT", digihamlib->get_txtimeout());
 //   m_settings->setValue("TXTOGGLE", m_toggletx ? "true" : "false");
    m_settings->setValue("XRF2REF", digihamlib->get_xrf2ref());
    m_settings->setValue("USRTXT", digihamlib->get_dstarusertxt());
//    m_settings->setValue("IAXUSER", m_iaxuser);
//    m_settings->setValue("IAXPASS", m_iaxpassword);
//    m_settings->setValue("IAXNODE", m_iaxnode);
//    m_settings->setValue("IAXHOST", m_iaxhost);
//    m_settings->setValue("IAXPORT", m_iaxport);

    m_settings->setValue("ModemRxFreq", digihamlib->get_modemRxFreq());
    m_settings->setValue("ModemTxFreq", digihamlib->get_modemTxFreq());
    m_settings->setValue("ModemRxOffset", digihamlib->get_modemRxOffset());
    m_settings->setValue("ModemTxOffset", digihamlib->get_modemTxOffset());
    m_settings->setValue("ModemRxDCOffset", digihamlib->get_modemRxDCOffset());
    m_settings->setValue("ModemTxDCOffset", digihamlib->get_modemTxDCOffset());
    m_settings->setValue("ModemRxLevel", digihamlib->get_modemRxLevel());
    m_settings->setValue("ModemTxLevel", digihamlib->get_modemTxLevel());
    m_settings->setValue("ModemRFLevel", digihamlib->get_modemRFLevel());
    m_settings->setValue("ModemTxDelay", digihamlib->get_modemTxDelay());
    m_settings->setValue("ModemCWIdTxLevel", digihamlib->get_modemCWIdTxLevel());
    m_settings->setValue("ModemDstarTxLevel", digihamlib->get_modemDstarTxLevel());
    m_settings->setValue("ModemDMRTxLevel", digihamlib->get_modemDMRTxLevel());
    m_settings->setValue("ModemYSFTxLevel", digihamlib->get_modemYSFTxLevel());
    m_settings->setValue("ModemP25TxLevel", digihamlib->get_modemP25TxLevel());
    m_settings->setValue("ModemNXDNTxLevel", digihamlib->get_modemNXDNTxLevel());
    m_settings->setValue("ModemBaud", digihamlib->get_modemBaud());
    m_settings->setValue("ModemM17CAN", digihamlib->get_modemM17CAN());
    m_settings->setValue("ModemTxInvert", digihamlib->get_modemTxInvert());
    m_settings->setValue("ModemRxInvert", digihamlib->get_modemRxInvert());
    m_settings->setValue("ModemPTTInvert", digihamlib->get_modemPTTInvert());
}

void MainWindow::process_settings()
{
    digihamlib->set_ipv6((m_settings->value("IPV6").toString().simplified() == "true") ? true : false);
    digihamlib->set_callsign(m_settings->value("CALLSIGN").toString().simplified());
    ui->callSignEdit->setText(digihamlib->get_callsign());
    digihamlib->set_dmrid(m_settings->value("DMRID").toString().simplified());
    ui->dmrIdEdit->setText(digihamlib->get_dmrid());
    digihamlib->set_essid(m_settings->value("ESSID").toString().simplified());
    ui->essidSpinBox->setValue(digihamlib->get_essid().toInt());
    digihamlib->set_bm_password(m_settings->value("BMPASSWORD").toString().simplified());
    ui->bmPasswordEdit->setText(digihamlib->get_bm_password());
    digihamlib->set_tgif_password(m_settings->value("TGIFPASSWORD").toString().simplified());
    ui->tgifPasswordEdit->setText(digihamlib->get_tgif_password());
    digihamlib->set_latitude(m_settings->value("DMRLAT", "0").toString().simplified());
    ui->latitudeEdit->setText(digihamlib->get_latitude());
    digihamlib->set_longitude(m_settings->value("DMRLONG", "0").toString().simplified());
    ui->longitudeEdit->setText(digihamlib->get_longitude());
    digihamlib->set_location(m_settings->value("DMRLOC").toString().simplified());
    ui->locationEdit->setText(digihamlib->get_location());
    digihamlib->set_description(m_settings->value("DMRDESC", "").toString().simplified());
    ui->descriptionEdit->setText(digihamlib->get_description());
    digihamlib->set_freq(m_settings->value("DMRFREQ", "438800000").toString().simplified());
    digihamlib->set_url(m_settings->value("DMRURL", "www.qrz.com").toString().simplified());
    ui->urlEdit->setText(digihamlib->get_url());
    digihamlib->set_swid(m_settings->value("DMRSWID", "20200922").toString().simplified());
    ui->softwareIDLabel->setText(digihamlib->get_swid());
    digihamlib->set_pkgid(m_settings->value("DMRPKGID", "MMDVM_MMDVM_HS_Hat").toString().simplified());
    ui->packageIDLabel->setText(digihamlib->get_pkgid());
    digihamlib->set_dmr_options(m_settings->value("DMROPTS").toString().simplified());
    ui->dmrPlusOptsEdit->setText(digihamlib->get_dmr_options());
    digihamlib->set_dmrtgid(m_settings->value("DMRTGID", "4000").toString().simplified());
    ui->tgidEdit->setText(digihamlib->get_dmrtgid());
    digihamlib->set_mycall(m_settings->value("MYCALL").toString().simplified());
    digihamlib->set_urcall(m_settings->value("URCALL", "CQCQCQ").toString().simplified());
    digihamlib->set_rptr1(m_settings->value("RPTR1").toString().simplified());
    digihamlib->set_rptr2(m_settings->value("RPTR2").toString().simplified());
    digihamlib->set_txtimeout(m_settings->value("TXTIMEOUT", "300").toString().simplified());
    ui->txTimeoutSpin->setValue(digihamlib->get_txtimeout().toInt());
//    m_toggletx = (m_settings->value("TXTOGGLE", "true").toString().simplified() == "true") ? true : false;
    digihamlib->set_usrtxt(m_settings->value("USRTXT").toString().simplified());
    digihamlib->set_xrf2ref((m_settings->value("XRF2REF").toString().simplified() == "true") ? true : false);
//    m_iaxuser = m_settings->value("IAXUSER").toString().simplified();
//    m_iaxpassword = m_settings->value("IAXPASS").toString().simplified();
//    m_iaxnode = m_settings->value("IAXNODE").toString().simplified();
//    m_saved_iaxhost = m_iaxhost = m_settings->value("IAXHOST").toString().simplified();
//    m_iaxport = m_settings->value("IAXPORT", "4569").toString().simplified().toUInt();
    digihamlib->set_localhosts(m_settings->value("LOCALHOSTS").toString());

    digihamlib->set_modemRxFreq(m_settings->value("ModemRxFreq", "438800000").toString().simplified());
    ui->rxFreqEdit->setText(digihamlib->get_modemRxFreq());
    digihamlib->set_modemTxFreq(m_settings->value("ModemTxFreq", "438800000").toString().simplified());
    ui->txFreqEdit->setText(digihamlib->get_modemTxFreq());
    digihamlib->set_modemRxOffset(m_settings->value("ModemRxOffset", "0").toString().simplified());
    ui->rxOffsetEdit->setText(digihamlib->get_modemRxOffset());
    digihamlib->set_modemTxOffset(m_settings->value("ModemTxOffset", "0").toString().simplified());
    ui->txOffsetEdit->setText(digihamlib->get_modemTxOffset());
    digihamlib->set_modemRxDCOffset(m_settings->value("ModemRxDCOffset", "0").toString().simplified());
    ui->rxDcOffsetEdit->setText(digihamlib->get_modemRxDCOffset());
    digihamlib->set_modemTxDCOffset(m_settings->value("ModemTxDCOffset", "0").toString().simplified());
    ui->txDcOffsetEdit->setText(digihamlib->get_modemTxDCOffset());
    digihamlib->set_modemRxLevel(m_settings->value("ModemRxLevel", "50").toString().simplified());
    ui->rxLevelEdit->setText(digihamlib->get_modemRxLevel());
    digihamlib->set_modemTxLevel(m_settings->value("ModemTxLevel", "50").toString().simplified());
    ui->txLevelEdit->setText(digihamlib->get_modemTxLevel());
    digihamlib->set_modemRFLevel(m_settings->value("ModemRFLevel", "100").toString().simplified());
    ui->rfLevelEdit->setText(digihamlib->get_modemRFLevel());
    digihamlib->set_modemTxDelay(m_settings->value("ModemTxDelay", "100").toString().simplified());
    ui->txDelayEdit->setText(digihamlib->get_modemTxDelay());
    digihamlib->set_modemCWIdTxLevel(m_settings->value("ModemCWIdTxLevel", "50").toString().simplified());
    ui->cwIdTxLevelEdit->setText(digihamlib->get_modemCWIdTxLevel());
    digihamlib->set_modemDstarTxLevel(m_settings->value("ModemDstarTxLevel", "50").toString().simplified());
    ui->dstarTxLevelEdit->setText(digihamlib->get_modemDstarTxLevel());
    digihamlib->set_modemDMRTxLevel(m_settings->value("ModemDMRTxLevel", "50").toString().simplified());
    ui->dmrTxLevelEdit->setText(digihamlib->get_modemDMRTxLevel());
    digihamlib->set_modemYSFTxLevel(m_settings->value("ModemYSFTxLevel", "50").toString().simplified());
    ui->ysfTxLevelEdit->setText(digihamlib->get_modemYSFTxLevel());
    digihamlib->set_modemP25TxLevel(m_settings->value("ModemP25TxLevel", "50").toString().simplified());
    ui->p25TxLevelEdit->setText(digihamlib->get_modemP25TxLevel());
    digihamlib->set_modemNXDNTxLevel(m_settings->value("ModemNXDNTxLevel", "50").toString().simplified());
    ui->nxdnTxLevelEdit->setText(digihamlib->get_modemNXDNTxLevel());
    digihamlib->set_modemBaud(m_settings->value("ModemBaud", "115200").toString().simplified());
    ui->modemBaudCombo->setCurrentText(digihamlib->get_modemBaud());
    digihamlib->set_modemM17CAN(m_settings->value("ModemM17CAN", "0").toString().simplified());
    ui->m17CanCombo->setCurrentText(digihamlib->get_modemM17CAN());
    digihamlib->set_modemTxInvert((m_settings->value("ModemTxInvert", "true").toString().simplified() == "true") ? true : false);
    ui->txInvertCB->setChecked(digihamlib->get_modemTxInvert());
    digihamlib->set_modemRxInvert((m_settings->value("ModemRxInvert", "true").toString().simplified() == "true") ? true : false);
    ui->rxInvertCB->setChecked(digihamlib->get_modemRxInvert());
    digihamlib->set_modemPTTInvert((m_settings->value("ModemPTTInvert", "true").toString().simplified() == "true") ? true : false);
    ui->pttInvertCB->setChecked(digihamlib->get_modemPTTInvert());

    process_mode_change(m_settings->value("MODE").toString().simplified());
    digihamlib->set_reflector(m_settings->value("HOST").toString().simplified());
    digihamlib->set_module(m_settings->value("MODULE").toString().simplified());
    ui->moduleCombo->setCurrentText(digihamlib->get_module());
    ui->myCallEdit->setText(digihamlib->get_mycall());
    ui->urCallEdit->setText(digihamlib->get_urcall());
    ui->rptr1Edit->setText(digihamlib->get_rptr1());
    ui->rptr2Edit->setText(digihamlib->get_rptr2());
    ui->slowSpeedDataEdit->setText(digihamlib->get_dstarusertxt());
    ui->modeCombo->setCurrentText(digihamlib->get_mode());
    ui->reflectorCombo->setCurrentText(digihamlib->get_reflector());
    //    emit update_settings();
}

void MainWindow::download_file(QString f, bool u)
{
    HttpManager *http = new HttpManager(f, u);
    QThread *httpThread = new QThread;
    http->moveToThread(httpThread);
    connect(httpThread, SIGNAL(started()), http, SLOT(process()));
    if (u){
        connect(http, SIGNAL(file_downloaded(QString)), this, SLOT(url_downloaded(QString)));
    }
    else{
        connect(http, SIGNAL(file_downloaded(QString)), this, SLOT(file_downloaded(QString)));
    }
    connect(httpThread, SIGNAL(finished()), http, SLOT(deleteLater()));
    httpThread->start();
}

void MainWindow::url_downloaded(QString url)
{
    emit updateLog("Downloaded " + url);
}

void MainWindow::file_downloaded(QString filename)
{
    emit updateLog("Updated " + filename);
    {
        if (filename == "dplus.txt" && m_protocol == "REF"){
            process_dstar_hosts(m_protocol);
        }
        else if (filename == "dextra.txt" && m_protocol == "XRF"){
            process_dstar_hosts(m_protocol);
        }
        else if (filename == "dcs.txt" && m_protocol == "DCS"){
            process_dstar_hosts(m_protocol);
        }
        else if (filename == "YSFHosts.txt" && m_protocol == "YSF"){
            process_ysf_hosts();
        }
        else if (filename == "FCSHosts.txt" && m_protocol == "FCS"){
            process_fcs_rooms();
        }
        else if (filename == "P25Hosts.txt" && m_protocol == "P25"){
            process_p25_hosts();
        }
        else if (filename == "DMRHosts.txt" && m_protocol == "DMR"){
            process_dmr_hosts();
        }
        else if (filename == "NXDNHosts.txt" && m_protocol == "NXDN"){
            process_nxdn_hosts();
        }
        else if (filename == "M17Hosts-full.csv" && m_protocol == "M17"){
            process_m17_hosts();
        }
        else if (filename == "DMRIDs.dat"){
            process_dmr_ids();
        }
        else if (filename == "NXDN.csv"){
            process_nxdn_ids();
        }
    }
}

void MainWindow::update_custom_hosts(QString h)
{
    m_settings->setValue("LOCALHOSTS", h);
    m_localhosts = m_settings->value("LOCALHOSTS").toString();
}

void MainWindow::process_dstar_hosts(QString m)
{
    m_hostmap.clear();
    m_hostsmodel.clear();
    QString filename, port;
    if (m == "REF"){
        filename = "dplus.txt";
        port = "20001";
    }
    else if (m == "DCS"){
        filename = "dcs.txt";
        port = "30051";
    }
    else if (m == "XRF"){
        filename = "dextra.txt";
        port = "30001";
    }

    QFileInfo check_file(config_path + "/" + filename);

    if (check_file.exists() && check_file.isFile()){
        QFile f(config_path + "/" + filename);
        if (f.open(QIODevice::ReadOnly)){
            while(!f.atEnd()){
                QString l = f.readLine();
                if (l.at(0) == '#'){
                    continue;
                }
                QStringList ll = l.split('\t');
                if (ll.size() > 1){
                    m_hostmap[ll.at(0).simplified()] = ll.at(1).simplified() + "," + port;
                }
            }

            m_customhosts = m_localhosts.split('\n');
            for (const auto& i : std::as_const(m_customhosts)){
                QStringList line = i.simplified().split(' ');

                if (line.at(0) == m){
                    m_hostmap[line.at(1).simplified()] = line.at(2).simplified() + "," + line.at(3).simplified();
                }
            }

            QMap<QString, QString>::const_iterator i = m_hostmap.constBegin();
            while (i != m_hostmap.constEnd()) {
                m_hostsmodel.append(i.key());
                ++i;
            }
        }
        f.close();
    }
}

void MainWindow::process_ysf_hosts()
{
    m_hostmap.clear();
    m_hostsmodel.clear();
    QFileInfo check_file(config_path + "/YSFHosts.txt");
    if (check_file.exists() && check_file.isFile()){
        QFile f(config_path + "/YSFHosts.txt");
        if (f.open(QIODevice::ReadOnly)){
            while(!f.atEnd()){
                QString l = f.readLine();
                if (l.at(0) == '#'){
                    continue;
                }
                QStringList ll = l.split(';');
                if (ll.size() > 4){
                    m_hostmap[ll.at(1).simplified()] = ll.at(3) + "," + ll.at(4);
                }
            }

            m_customhosts = m_localhosts.split('\n');
            for (const auto& i : std::as_const(m_customhosts)){
                QStringList line = i.simplified().split(' ');

                if (line.at(0) == "YSF"){
                    m_hostmap[line.at(1).simplified()] = line.at(2).simplified() + "," + line.at(3).simplified();
                }
            }

            QMap<QString, QString>::const_iterator i = m_hostmap.constBegin();
            while (i != m_hostmap.constEnd()) {
                m_hostsmodel.append(i.key());
                ++i;
            }
        }
        f.close();
    }
}

void MainWindow::process_fcs_rooms()
{
    m_hostmap.clear();
    m_hostsmodel.clear();
    QFileInfo check_file(config_path + "/FCSHosts.txt");
    if (check_file.exists() && check_file.isFile()){
        QFile f(config_path + "/FCSHosts.txt");
        if (f.open(QIODevice::ReadOnly)){
            while(!f.atEnd()){
                QString l = f.readLine();
                if (l.at(0) == '#'){
                    continue;
                }
                QStringList ll = l.split(';');
                if (ll.size() > 4){
                    if (ll.at(1).simplified() != "nn"){
                        m_hostmap[ll.at(0).simplified() + " - " + ll.at(1).simplified()] = ll.at(2).left(6).toLower() + ".xreflector.net,62500";
                    }
                }
            }

            m_customhosts = m_localhosts.split('\n');
            for (const auto& i : std::as_const(m_customhosts)){
                QStringList line = i.simplified().split(' ');

                if (line.at(0) == "FCS"){
                    m_hostmap[line.at(1).simplified()] = line.at(2).simplified() + "," + line.at(3).simplified();
                }
            }

            QMap<QString, QString>::const_iterator i = m_hostmap.constBegin();
            while (i != m_hostmap.constEnd()) {
                m_hostsmodel.append(i.key());
                ++i;
            }
        }
        f.close();
    }
}

void MainWindow::process_dmr_hosts()
{
    m_hostmap.clear();
    m_hostsmodel.clear();
    QFileInfo check_file(config_path + "/DMRHosts.txt");
    if (check_file.exists() && check_file.isFile()){
        QFile f(config_path + "/DMRHosts.txt");
        if (f.open(QIODevice::ReadOnly)){
            while(!f.atEnd()){
                QString l = f.readLine();
                if (l.at(0) == '#'){
                    continue;
                }
                QStringList ll = l.simplified().split(' ');
                if (ll.size() > 4){
                    if ( (ll.at(0).simplified() != "DMRGateway")
                        && (ll.at(0).simplified() != "DMR2YSF")
                        && (ll.at(0).simplified() != "DMR2NXDN"))
                    {
                        m_hostmap[ll.at(0).simplified()] = ll.at(2) + "," + ll.at(4) + "," + ll.at(3);
                    }
                }
            }

            m_customhosts = m_localhosts.split('\n');
            for (const auto& i : std::as_const(m_customhosts)){
                QStringList line = i.simplified().split(' ');

                if (line.at(0) == "DMR"){
                    m_hostmap[line.at(1).simplified()] = line.at(2).simplified() + "," + line.at(3).simplified() + "," + line.at(4).simplified();
                }
            }

            QMap<QString, QString>::const_iterator i = m_hostmap.constBegin();
            while (i != m_hostmap.constEnd()) {
                m_hostsmodel.append(i.key());
                ++i;
            }
        }
        f.close();
    }
}

void MainWindow::process_p25_hosts()
{
    m_hostmap.clear();
    m_hostsmodel.clear();
    QFileInfo check_file(config_path + "/P25Hosts.txt");
    if (check_file.exists() && check_file.isFile()){
        QFile f(config_path + "/P25Hosts.txt");
        if (f.open(QIODevice::ReadOnly)){
            while(!f.atEnd()){
                QString l = f.readLine();
                if (l.at(0) == '#'){
                    continue;
                }
                QStringList ll = l.simplified().split(' ');
                if (ll.size() > 2){
                    m_hostmap[ll.at(0).simplified()] = ll.at(1) + "," + ll.at(2);
                }
            }

            m_customhosts = m_localhosts.split('\n');
            for (const auto& i : std::as_const(m_customhosts)){
                QStringList line = i.simplified().split(' ');

                if (line.at(0) == "P25"){
                    m_hostmap[line.at(1).simplified()] = line.at(2).simplified() + "," + line.at(3).simplified();
                }
            }

            QMap<QString, QString>::const_iterator i = m_hostmap.constBegin();
            while (i != m_hostmap.constEnd()) {
                m_hostsmodel.append(i.key());
                ++i;
            }
            QMap<int, QString> m;
            for (auto s : m_hostsmodel) m[s.toInt()] = s;
            m_hostsmodel = QStringList(m.values());
        }
        f.close();
    }
}

void MainWindow::process_nxdn_hosts()
{
    m_hostmap.clear();
    m_hostsmodel.clear();

    QFileInfo check_file(config_path + "/NXDNHosts.txt");
    if (check_file.exists() && check_file.isFile()){
        QFile f(config_path + "/NXDNHosts.txt");
        if (f.open(QIODevice::ReadOnly)){
            while(!f.atEnd()){
                QString l = f.readLine();
                if (l.at(0) == '#'){
                    continue;
                }
                QStringList ll = l.simplified().split(' ');
                if (ll.size() > 2){
                    m_hostmap[ll.at(0).simplified()] = ll.at(1) + "," + ll.at(2);
                }
            }

            m_customhosts = m_localhosts.split('\n');
            for (const auto& i : std::as_const(m_customhosts)){
                QStringList line = i.simplified().split(' ');

                if (line.at(0) == "NXDN"){
                    m_hostmap[line.at(1).simplified()] = line.at(2).simplified() + "," + line.at(3).simplified();
                }
            }

            QMap<QString, QString>::const_iterator i = m_hostmap.constBegin();
            while (i != m_hostmap.constEnd()) {
                m_hostsmodel.append(i.key());
                ++i;
            }
            QMap<int, QString> m;
            for (auto s : m_hostsmodel) m[s.toInt()] = s;
            m_hostsmodel = QStringList(m.values());
        }
        f.close();
    }
}

void MainWindow::process_m17_hosts()
{
    m_hostmap.clear();
    m_hostsmodel.clear();

    QFileInfo check_file(config_path + "/M17Hosts-full.csv");
    if (check_file.exists() && check_file.isFile())
    {
        QFile f(config_path + "/M17Hosts-full.csv");
        if (f.open(QIODevice::ReadOnly))
        {
            while (!f.atEnd())
            {
                QString l = f.readLine();
                if (l.at(0) == '#')
                {
                    continue;
                }
                QStringList ll = l.simplified().split(',');
                if (ll.size() > 3)
                {
                    digihamlib->m_hostmap[ll.at(0).simplified()] = ll.at(2) + "," + ll.at(4) + "," + ll.at(3);
                }
            }

            digihamlib->m_customhosts = m_localhosts.split('\n');
            for (const auto& i : std::as_const(digihamlib->m_customhosts))
            {
                QStringList line = i.simplified().split(' ');

                if (line.at(0) == "M17"){
                    digihamlib->m_hostmap[line.at(1).simplified()] = line.at(2).simplified() + "," + line.at(3).simplified();
                }
            }

            if (m_mdirect)
            {
                digihamlib->m_hostmap["ALL"] = "ALL";
                digihamlib->m_hostmap["UNLINK"] = "UNLINK";
                digihamlib->m_hostmap["ECHO"] = "ECHO";
                digihamlib->m_hostmap["INFO"] = "INFO";
            }

            QMap<QString, QString>::const_iterator i = digihamlib->m_hostmap.constBegin();
            while (i != digihamlib->m_hostmap.constEnd())
            {
                digihamlib->m_hostsmodel.append(i.key());
                ++i;
            }
        }
        f.close();
    }
}

void MainWindow::process_dmr_ids()
{
    QFileInfo check_file(config_path + "/DMRIDs.dat");
    if (check_file.exists() && check_file.isFile()){
        QFile f(config_path + "/DMRIDs.dat");
        if (f.open(QIODevice::ReadOnly)){
            while(!f.atEnd()){
                QString lids = f.readLine();
                if (lids.at(0) == '#'){
                    continue;
                }
                QStringList llids = lids.simplified().split(' ');

                if (llids.size() >= 2){
                    m_dmrids[llids.at(0).toUInt()] = llids.at(1);
                }
            }
        }
        f.close();
    }
}

void MainWindow::update_dmr_ids()
{
    QFileInfo check_file(config_path + "/DMRIDs.dat");
    if (check_file.exists() && check_file.isFile()){
        QFile f(config_path + "/DMRIDs.dat");
        f.remove();
    }
    process_dmr_ids();
    update_nxdn_ids();
}

void MainWindow::process_nxdn_ids()
{
    QFileInfo check_file(config_path + "/NXDN.csv");
    if (check_file.exists() && check_file.isFile()){
        QFile f(config_path + "/NXDN.csv");
        if (f.open(QIODevice::ReadOnly)){
            while(!f.atEnd()){
                QString lids = f.readLine();
                if (lids.at(0) == '#'){
                    continue;
                }
                QStringList llids = lids.simplified().split(',');

                if (llids.size() > 1){
                    m_nxdnids[llids.at(0).toUInt()] = llids.at(1);
                }
            }
        }
        f.close();
    }
}

void MainWindow::update_nxdn_ids()
{
    QFileInfo check_file(config_path + "/NXDN.csv");
    if (check_file.exists() && check_file.isFile()){
        QFile f(config_path + "/NXDN.csv");
        f.remove();
    }
    process_nxdn_ids();
}

void MainWindow::update_host_files()
{
    m_update_host_files = true;
    check_host_files();
}

void MainWindow::check_host_files()
{
    if (!QDir(config_path).exists())
    {
        QDir().mkdir(config_path);
    }

    QFileInfo check_file(config_path + "/dplus.txt");
    if ( (!check_file.exists() && !(check_file.isFile())) || m_update_host_files ){
        download_file("/dplus.txt");
    }

    check_file.setFile(config_path + "/dextra.txt");
    if ( (!check_file.exists() && !check_file.isFile() ) || m_update_host_files  ){
        download_file("/dextra.txt");
    }

    check_file.setFile(config_path + "/dcs.txt");
    if ( (!check_file.exists() && !check_file.isFile()) || m_update_host_files ){
        download_file( "/dcs.txt");
    }

    check_file.setFile(config_path + "/YSFHosts.txt");
    if ( (!check_file.exists() && !check_file.isFile()) || m_update_host_files ){
        download_file("/YSFHosts.txt");
    }

    check_file.setFile(config_path + "/FCSHosts.txt");
    if ( (!check_file.exists() && !check_file.isFile()) || m_update_host_files ){
        download_file("/FCSHosts.txt");
    }

    check_file.setFile(config_path + "/DMRHosts.txt");
    if ( (!check_file.exists() && !check_file.isFile()) || m_update_host_files ){
        download_file("/DMRHosts.txt");
    }

    check_file.setFile(config_path + "/P25Hosts.txt");
    if ( (!check_file.exists() && !check_file.isFile()) || m_update_host_files ){
        download_file("/P25Hosts.txt");
    }

    check_file.setFile(config_path + "/NXDNHosts.txt");
    if ((!check_file.exists() && !check_file.isFile()) || m_update_host_files ){
        download_file("/NXDNHosts.txt");
    }

    check_file.setFile(config_path + "/M17Hosts-full.csv");
    if ( (!check_file.exists() && !check_file.isFile()) || m_update_host_files ){
        download_file("/M17Hosts-full.csv");
    }

    check_file.setFile(config_path + "/DMRIDs.dat");
    if (!check_file.exists() && !check_file.isFile()){
        download_file("/DMRIDs.dat");
    }
    else {
        process_dmr_ids();
    }

    check_file.setFile(config_path + "/NXDN.csv");
    if (!check_file.exists() && !check_file.isFile()){
        download_file("/NXDN.csv");
    }
    else{
        process_nxdn_ids();
    }
    m_update_host_files = false;
    //process_mode_change(ui->modeCombo->currentText().simplified());
}

void MainWindow::process_mode_change(QString m)
{
    ui->tgidEdit->setVisible(false);
    ui->tgidLabel->setVisible(false);
    ui->m17CanLabel->setVisible(false);
    ui->m17CanCombo->setVisible(false);
    ui->slotLabel->setVisible(false);
    ui->slotCombo->setVisible(false);
    ui->moduleLabel->setVisible(false);
    ui->moduleCombo->setVisible(false);
    ui->colorCodeLabel->setVisible(false);
    ui->colorCodeCombo->setVisible(false);

    digihamlib->set_protocol(m);

    if ((m == "REF") || (m == "DCS") || (m == "XRF"))
    {
        process_dstar_hosts(m);
        ui->moduleLabel->setVisible(true);
        ui->moduleCombo->setVisible(true);
        ui->moduleCombo->setCurrentText(digihamlib->get_module());
    }
    if (m == "YSF")
    {
        process_ysf_hosts();
    }
    if (m == "FCS")
    {
        process_fcs_rooms();
    }
    if (m == "DMR")
    {
        process_dmr_hosts();
        ui->tgidEdit->setVisible(true);
        ui->tgidEdit->setText(digihamlib->get_dmrtgid());
        ui->tgidLabel->setVisible(true);
        ui->slotLabel->setVisible(true);
        ui->slotCombo->setVisible(true);
        ui->slotCombo->setCurrentText("2");
        ui->colorCodeLabel->setVisible(true);
        ui->colorCodeCombo->setVisible(true);
        //process_dmr_ids();
    }
    if (m == "P25")
    {
        process_p25_hosts();
        ui->tgidEdit->setVisible(true);
        ui->tgidLabel->setVisible(true);
        ui->tgidEdit->setText(digihamlib->get_dmrtgid());
    }
    if (m == "NXDN")
    {
        process_nxdn_hosts();
    }
    if (m == "M17")
    {
        process_m17_hosts();
        ui->m17CanLabel->setVisible(true);
        ui->m17CanCombo->setVisible(true);
        ui->moduleLabel->setVisible(true);
        ui->moduleCombo->setVisible(true);
        ui->moduleCombo->setCurrentText(digihamlib->get_module());
    }
    if (m == "IAX")
    {
    }
    ui->reflectorCombo->clear();
    for (int i=0; i<digihamlib->m_hostsmodel.count(); i++)
        ui->reflectorCombo->addItem(digihamlib->m_hostsmodel.at(i));
    //  emit mode_changed();
}

void MainWindow::discover_devices()
{
    ui->vocoderCombo->clear();
    ui->modemCombo->clear();
    digihamlib->m_vocoders.clear();
    digihamlib->m_modems.clear();
    digihamlib->m_vocoders.append("Software vocoder");
    ui->vocoderCombo->addItem("Software vocoder");
    digihamlib->m_modems.append("None");
    ui->modemCombo->addItem("None");
#if !defined(Q_OS_IOS)
    QMap<QString, QString> l = SerialAMBE::discover_devices();
    QMap<QString, QString>::const_iterator i = l.constBegin();

    while (i != l.constEnd())
    {
        digihamlib->m_vocoders.append(i.value());
        ui->vocoderCombo->addItem(i.value());
        digihamlib->m_modems.append(i.value());
        ui->modemCombo->addItem(i.value());
        ++i;
    }
 //   emit update_devices();
#endif
}

void MainWindow::do_connect(void)
{
    digihamlib->set_debug(ui->debugCB->isChecked());

    if (ui->connectButton->text() != "Disconnect")
    {
        ui->txButton->setEnabled(true);
        connect(ui->txButton, SIGNAL(pressed()), digihamlib, SLOT(press_tx()));
        connect(ui->txButton, SIGNAL(pressed()), this, SLOT(start_tx()));
        connect(ui->txButton, SIGNAL(released()), digihamlib, SLOT(release_tx()));
        connect(ui->txButton, SIGNAL(released()), this, SLOT(stop_tx()));
        ui->volumeLevelBar->setValue(0);
        ui->connectButton->setText("Disconnect");
    }
    else
    {
        ui->txButton->setEnabled(false);
        disconnect(ui->txButton, SIGNAL(pressed()), digihamlib, SLOT(press_tx()));
        disconnect(ui->txButton, SIGNAL(pressed()), this, SLOT(start_tx()));
        disconnect(ui->txButton, SIGNAL(released()), digihamlib, SLOT(release_tx()));
        disconnect(ui->txButton, SIGNAL(released()), this, SLOT(stop_tx()));
    }
    digihamlib->process_connect();
} // end connect

void MainWindow::updateLog(QString status)
{
    if (status.contains( "Disconnected"))
    {
        ui->ambeVocoderLabel->setText("AMBE:");
        ui->mmdvmModemLabel->setText("MMDVM:");
        ui->pingCntLabel->setText("Ping Cnt");
        ui->connectButton->setText("Connect");
    }
    ui->processLogList->insertItem(0, status);
    ui->volumeLevelBar->setValue(0);
}

void MainWindow::process_audio(int16_t *data, int len)
{
    audio->process_audio(data, len);
    int level = ((float)audio->maxlevel / 32767) * 100;
    if (!isTx)
        ui->volumeLevelBar->setValue(level);
}

void MainWindow::update_status()
{
    ui->myCallLabel->setText(digihamlib->m_mode->m_modeinfo.src);
    ui->urCallLabel->setText(digihamlib->m_mode->m_modeinfo.dst);
    ui->rptr1Label->setText(digihamlib->m_mode->m_modeinfo.gw);
    ui->rptr2Label->setText(digihamlib->m_mode->m_modeinfo.gw2);
    ui->slowSpeeddataLabel->setText(digihamlib->m_mode->m_modeinfo.usertxt);
    ui->pingCntLabel->setText(digihamlib->get_netstatustxt());
    ui->ambeVocoderLabel->setText(digihamlib->get_ambestatustxt());
    ui->mmdvmModemLabel->setText(digihamlib->get_mmdvmstatustxt());
}

void MainWindow::updateMicLevel(qreal level)
{
    if (isTx)
        ui->volumeLevelBar->setValue(level * 100);
}

void MainWindow::micSendAudio(QQueue<qint16>* queue)
{
    while (!queue->isEmpty())
    {
        qint16 i = queue->dequeue();
        if (isTx)
            digihamlib->m_mode->m_micData.enqueue(i);
    }
}

void MainWindow::setMicGain(int gain)
{
    audioinput->setMicGain(gain);
}

void MainWindow::start_tx(void)
{
    digihamlib->m_mode->m_micData.clear();
    isTx = true;
    connect(audioinput, SIGNAL(mic_update_level(qreal)), this, SLOT(updateMicLevel(qreal)));
}

void MainWindow::stop_tx(void)
{
    disconnect(audioinput, SIGNAL(mic_update_level(qreal)), this, SLOT(updateMicLevel(qreal)));
    isTx = false;
    ui->volumeLevelBar->setValue(0);
}

void MainWindow::audioDeviceChanged(QAudioDevice info,int rate,int channels)
{
    audio->select_audio(info, rate, channels);
} // end audioDeviceChanged


void MainWindow::micDeviceChanged(QAudioDevice info,int rate,int channels)
{
    audioinput->select_audio(info, rate, channels);
} // end micDeviceChanged


void MainWindow::audioDeviceChanged(int selection)
{
    audioDeviceChanged(ui->audioOutCombo->itemData(selection).value<QAudioDevice>(), 8000, 1);
}


void MainWindow::micDeviceChanged(int selection)
{
    qDebug() << "Mic selected is device number: " << selection;
    micDeviceChanged(ui->audioInCombo->itemData(selection).value<QAudioDevice >(), 8000, 1);
}

void MainWindow::vocoderChanged(int index)
{
    digihamlib->m_vocoder = digihamlib->m_vocoders.at(index);
}

void MainWindow::modemChanged(int index)
{
    digihamlib->m_modem = digihamlib->m_modems.at(index);
}
