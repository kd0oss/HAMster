#include "mainwindow.h"
#if defined(Q_OS_WIN)
#include "ui_mainwindow_win.h"
#else
#include "ui_mainwindow.h"
#endif
#include "httpmanager.h"
#include <QtCore>
#include <QSerialPortInfo>

#define ENDLINE "\n"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_audio = new AudioEngine(m_audioin, m_audioout);
    m_audio->init();
#if !defined(Q_OS_WIN)
    m_audio->start_playback();
#endif
    m_audio->set_input_volume(0.70);
    isTx = false;
    txtimer = new QTimer(this);
    connect(txtimer, SIGNAL(timeout()), this, SLOT(send_mic_audio()));

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
    m_mdirect = false;
    m_settings = new QSettings(QSettings::IniFormat, QSettings::UserScope, "kd0oss", "HAMster", this);
    config_path = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_WIN)
    config_path += "/HAMster";
    updateLog(QString("Config path: %1").arg(config_path));
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
    connect(digihamlib, SIGNAL(audio_ready()), this, SLOT(process_audio()));
    connect(digihamlib, SIGNAL(update_data()), this, SLOT(update_status()));
    connect(ui->micGainSlider, SIGNAL(valueChanged(int)), this, SLOT(setMicGain(int)));
    connect(ui->modeCombo, SIGNAL(currentTextChanged(QString)), this, SLOT(process_mode_change(QString)));
    connect(ui->debugCB, SIGNAL(clicked(bool)), digihamlib, SLOT(set_debug(bool)));
    connect(ui->moduleCombo, SIGNAL(currentTextChanged(QString)), digihamlib, SLOT(set_module(QString)));
    connect(ui->m17CanCombo, SIGNAL(currentTextChanged(QString)), digihamlib, SLOT(set_modemM17CAN(QString)));
    connect(ui->reflectorCombo, SIGNAL(currentTextChanged(QString)), digihamlib, SLOT(set_reflector(QString)));
    connect(ui->tgidEdit, SIGNAL(textChanged(QString)), digihamlib, SLOT(set_dmrtgid(QString)));
    connect(ui->colorCodeCombo, SIGNAL(currentIndexChanged(int)), digihamlib, SLOT(set_cc(int)));
    connect(ui->slotCombo, SIGNAL(currentIndexChanged(int)), digihamlib, SLOT(set_slot(int)));
//    connect(ui->audioInCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(micDeviceChanged(int)));
//    connect(ui->audioOutCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(spkrDeviceChanged(int)));
    connect(ui->vocoderCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(vocoderChanged(int)));
    connect(ui->modemCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(modemChanged(int)));
    connect(ui->m17VoiceFullRadio, SIGNAL(toggled(bool)), this, SLOT(m17_voice_rate_changed(bool)));

    discover_devices();
    ui->micGainSlider->setValue(90);
}

MainWindow::~MainWindow()
{
    m_audio->stop_playback();
    delete m_audio;
    delete digihamlib;
    delete ui;
}

void MainWindow::save_settings()
{
    //m_settings->setValue("PLAYBACK", ui->comboPlayback->currentText());
    //m_settings->setValue("CAPTURE", ui->comboCapture->currentText());
    m_settings->setValue("IPV6", digihamlib->get_ipv6());
    m_settings->setValue("MODE", ui->modeCombo->currentText());
    m_settings->setValue("HOST", ui->reflectorCombo->currentText());
    m_settings->setValue("MODULE", ui->moduleCombo->currentText());
    m_settings->setValue("CALLSIGN", ui->callSignEdit->text());
    m_settings->setValue("DMRID", ui->dmrIdEdit->text());
    m_settings->setValue("ESSID", QString("%1").arg(ui->essidSpinBox->value()));
    m_settings->setValue("BMPASSWORD", ui->bmPasswordEdit->text());
    m_settings->setValue("TGIFPASSWORD", ui->tgifPasswordEdit->text());
    m_settings->setValue("DMRTGID", ui->tgidEdit->text());
    m_settings->setValue("DMRLAT", ui->latitudeEdit->text());
    m_settings->setValue("DMRLONG", ui->longitudeEdit->text());
    m_settings->setValue("DMRLOC", ui->locationEdit->text());
    m_settings->setValue("DMRDESC", ui->descriptionEdit->text());
    m_settings->setValue("DMRFREQ", digihamlib->get_freq());
    m_settings->setValue("DMRURL", ui->urlEdit->text());
    m_settings->setValue("DMRSWID", ui->softwareIDLabel->text());
    m_settings->setValue("DMRPKGID", ui->packageIDLabel->text());
    m_settings->setValue("DMROPTS", ui->dmrPlusOptsEdit->text());
    m_settings->setValue("MYCALL", ui->myCallEdit->text());
    m_settings->setValue("URCALL", ui->urCallEdit->text());
    m_settings->setValue("RPTR1", ui->rptr1Edit->text());
    m_settings->setValue("RPTR2", ui->rptr2Edit->text());
    m_settings->setValue("TXTIMEOUT", QString("%1").arg(ui->txTimeoutSpin->value()));
 //   m_settings->setValue("TXTOGGLE", m_toggletx ? "true" : "false");
    m_settings->setValue("XRF2REF", digihamlib->get_xrf2ref());
    m_settings->setValue("USRTXT", ui->slowSpeedDataEdit->text());
//    m_settings->setValue("IAXUSER", m_iaxuser);
//    m_settings->setValue("IAXPASS", m_iaxpassword);
//    m_settings->setValue("IAXNODE", m_iaxnode);
//    m_settings->setValue("IAXHOST", m_iaxhost);
//    m_settings->setValue("IAXPORT", m_iaxport);

    m_settings->setValue("ModemRxFreq", ui->rxFreqEdit->text());
    m_settings->setValue("ModemTxFreq", ui->txFreqEdit->text());
    m_settings->setValue("ModemRxOffset", ui->rxOffsetEdit->text());
    m_settings->setValue("ModemTxOffset", ui->txOffsetEdit->text());
    m_settings->setValue("ModemRxDCOffset", ui->rxDcOffsetEdit->text());
    m_settings->setValue("ModemTxDCOffset", ui->txDcOffsetEdit->text());
    m_settings->setValue("ModemRxLevel", ui->rxLevelEdit->text());
    m_settings->setValue("ModemTxLevel", ui->txLevelEdit->text());
    m_settings->setValue("ModemRFLevel", ui->rfLevelEdit->text());
    m_settings->setValue("ModemTxDelay", ui->txDelayEdit->text());
    m_settings->setValue("ModemCWIdTxLevel", ui->cwIdTxLevelEdit->text());
    m_settings->setValue("ModemDstarTxLevel", ui->dstarTxLevelEdit->text());
    m_settings->setValue("ModemDMRTxLevel", ui->dmrTxLevelEdit->text());
    m_settings->setValue("ModemYSFTxLevel", ui->ysfTxLevelEdit->text());
    m_settings->setValue("ModemP25TxLevel", ui->p25TxLevelEdit->text());
    m_settings->setValue("ModemNXDNTxLevel", ui->nxdnTxLevelEdit->text());
    m_settings->setValue("ModemBaud", ui->modemBaudCombo->currentText());
    m_settings->setValue("ModemM17CAN", ui->m17CanCombo->currentText());
    m_settings->setValue("ModemTxInvert", QString("%1").arg(ui->txInvertCB->isChecked()));
    m_settings->setValue("ModemRxInvert", QString("%1").arg(ui->rxInvertCB->isChecked()));
    m_settings->setValue("ModemPTTInvert", QString("%1").arg(ui->pttInvertCB->isChecked()));
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

    process_mode_change(m_settings->value("MODE", "M17").toString().simplified());
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
    updateLog("Downloaded " + url);
}

void MainWindow::file_downloaded(QString filename)
{
    updateLog("Updated " + filename);
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
    digihamlib->m_hostmap.clear();
    digihamlib->m_hostsmodel.clear();
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
                    digihamlib->m_hostmap[ll.at(0).simplified()] = ll.at(1).simplified() + "," + port;
                }
            }

            digihamlib->m_customhosts = m_localhosts.split('\n');
            for (const auto& i : std::as_const(digihamlib->m_customhosts)){
                QStringList line = i.simplified().split(' ');

                if (line.at(0) == m){
                    digihamlib->m_hostmap[line.at(1).simplified()] = line.at(2).simplified() + "," + line.at(3).simplified();
                }
            }

            QMap<QString, QString>::const_iterator i = digihamlib->m_hostmap.constBegin();
            while (i != digihamlib->m_hostmap.constEnd()) {
                digihamlib->m_hostsmodel.append(i.key());
                ++i;
            }
        }
        f.close();
    }
}

void MainWindow::process_ysf_hosts()
{
    digihamlib->m_hostmap.clear();
    digihamlib->m_hostsmodel.clear();
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
                    digihamlib->m_hostmap[ll.at(1).simplified()] = ll.at(3) + "," + ll.at(4);
                }
            }

            digihamlib->m_customhosts = digihamlib->m_localhosts.split('\n');
            for (const auto& i : std::as_const(digihamlib->m_customhosts)){
                QStringList line = i.simplified().split(' ');

                if (line.at(0) == "YSF"){
                    digihamlib->m_hostmap[line.at(1).simplified()] = line.at(2).simplified() + "," + line.at(3).simplified();
                }
            }

            QMap<QString, QString>::const_iterator i = digihamlib->m_hostmap.constBegin();
            while (i != digihamlib->m_hostmap.constEnd()) {
                digihamlib->m_hostsmodel.append(i.key());
                ++i;
            }
        }
        f.close();
    }
}

void MainWindow::process_fcs_rooms()
{
    digihamlib->m_hostmap.clear();
    digihamlib->m_hostsmodel.clear();
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
                        digihamlib->m_hostmap[ll.at(0).simplified() + " - " + ll.at(1).simplified()] = ll.at(2).left(6).toLower() + ".xreflector.net,62500";
                    }
                }
            }

            digihamlib->m_customhosts = digihamlib->m_localhosts.split('\n');
            for (const auto& i : std::as_const(digihamlib->m_customhosts)){
                QStringList line = i.simplified().split(' ');

                if (line.at(0) == "FCS"){
                    digihamlib->m_hostmap[line.at(1).simplified()] = line.at(2).simplified() + "," + line.at(3).simplified();
                }
            }

            QMap<QString, QString>::const_iterator i = digihamlib->m_hostmap.constBegin();
            while (i != digihamlib->m_hostmap.constEnd()) {
                digihamlib->m_hostsmodel.append(i.key());
                ++i;
            }
        }
        f.close();
    }
}

void MainWindow::process_dmr_hosts()
{
    digihamlib->m_hostmap.clear();
    digihamlib->m_hostsmodel.clear();
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
                        digihamlib->m_hostmap[ll.at(0).simplified()] = ll.at(2) + "," + ll.at(4) + "," + ll.at(3);
                    }
                }
            }

            digihamlib->m_customhosts = digihamlib->m_localhosts.split('\n');
            for (const auto& i : std::as_const(digihamlib->m_customhosts)){
                QStringList line = i.simplified().split(' ');

                if (line.at(0) == "DMR"){
                    digihamlib->m_hostmap[line.at(1).simplified()] = line.at(2).simplified() + "," + line.at(3).simplified() + "," + line.at(4).simplified();
                }
            }

            QMap<QString, QString>::const_iterator i = digihamlib->m_hostmap.constBegin();
            while (i != digihamlib->m_hostmap.constEnd()) {
                digihamlib->m_hostsmodel.append(i.key());
                ++i;
            }
        }
        f.close();
    }
}

void MainWindow::process_p25_hosts()
{
    digihamlib->m_hostmap.clear();
    digihamlib->m_hostsmodel.clear();
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
                    digihamlib->m_hostmap[ll.at(0).simplified()] = ll.at(1) + "," + ll.at(2);
                }
            }

            digihamlib->m_customhosts = digihamlib->m_localhosts.split('\n');
            for (const auto& i : std::as_const(digihamlib->m_customhosts)){
                QStringList line = i.simplified().split(' ');

                if (line.at(0) == "P25"){
                    digihamlib->m_hostmap[line.at(1).simplified()] = line.at(2).simplified() + "," + line.at(3).simplified();
                }
            }

            QMap<QString, QString>::const_iterator i = digihamlib->m_hostmap.constBegin();
            while (i != digihamlib->m_hostmap.constEnd()) {
                digihamlib->m_hostsmodel.append(i.key());
                ++i;
            }
            QMap<int, QString> m;
            for (auto s : digihamlib->m_hostsmodel) m[s.toInt()] = s;
            digihamlib->m_hostsmodel = QStringList(m.values());
        }
        f.close();
    }
}

void MainWindow::process_nxdn_hosts()
{
    digihamlib->m_hostmap.clear();
    digihamlib->m_hostsmodel.clear();

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
                    digihamlib->m_hostmap[ll.at(0).simplified()] = ll.at(1) + "," + ll.at(2);
                }
            }

            digihamlib->m_customhosts = digihamlib->m_localhosts.split('\n');
            for (const auto& i : std::as_const(digihamlib->m_customhosts)){
                QStringList line = i.simplified().split(' ');

                if (line.at(0) == "NXDN"){
                    digihamlib->m_hostmap[line.at(1).simplified()] = line.at(2).simplified() + "," + line.at(3).simplified();
                }
            }

            QMap<QString, QString>::const_iterator i = digihamlib->m_hostmap.constBegin();
            while (i != digihamlib->m_hostmap.constEnd()) {
                digihamlib->m_hostsmodel.append(i.key());
                ++i;
            }
            QMap<int, QString> m;
            for (auto s : digihamlib->m_hostsmodel) m[s.toInt()] = s;
            digihamlib->m_hostsmodel = QStringList(m.values());
        }
        f.close();
    }
}

void MainWindow::process_m17_hosts()
{
    digihamlib->m_hostmap.clear();
    digihamlib->m_hostsmodel.clear();

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

            digihamlib->m_customhosts = digihamlib->m_localhosts.split('\n');
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
                    digihamlib->m_dmrids[llids.at(0).toUInt()] = llids.at(1);
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
                    digihamlib->m_nxdnids[llids.at(0).toUInt()] = llids.at(1);
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
    digihamlib->set_mode(m);
    if (m.contains(" "))
        m_protocol = m.split(" ").at(1);
    else
        m_protocol = m;
    digihamlib->set_protocol(m_protocol);
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

    digihamlib->set_protocol(m_protocol);

    if ((m_protocol == "REF") || (m_protocol == "DCS") || (m_protocol == "XRF"))
    {
        process_dstar_hosts(m_protocol);
        ui->moduleLabel->setVisible(true);
        ui->moduleCombo->setVisible(true);
        ui->moduleCombo->setCurrentText(digihamlib->get_module());
    }
    if (m_protocol == "YSF")
    {
        process_ysf_hosts();
    }
    if (m_protocol == "FCS")
    {
        process_fcs_rooms();
    }
    if (m_protocol == "DMR")
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
    if (m_protocol == "P25")
    {
        process_p25_hosts();
        ui->tgidEdit->setVisible(true);
        ui->tgidLabel->setVisible(true);
        ui->tgidEdit->setText(digihamlib->get_dmrtgid());
    }
    if (m_protocol == "NXDN")
    {
        process_nxdn_hosts();
    }
    if (m_protocol == "M17")
    {
        process_m17_hosts();
        ui->m17CanLabel->setVisible(true);
        ui->m17CanCombo->setVisible(true);
        ui->moduleLabel->setVisible(true);
        ui->moduleCombo->setVisible(true);
        ui->moduleCombo->setCurrentText(digihamlib->get_module());
    }
    if (m_protocol == "IAX")
    {
    }
    ui->reflectorCombo->clear();
    for (int i=0; i<digihamlib->m_hostsmodel.count(); i++)
        ui->reflectorCombo->addItem(digihamlib->m_hostsmodel.at(i));
    //  emit mode_changed();
}

void MainWindow::discover_devices()
{
    QStringList m_playbacks;
    QStringList m_captures;

    m_playbacks.append("OS Default");
    m_captures.append("OS Default");
    m_playbacks.append(AudioEngine::discover_audio_devices(AUDIO_OUT));
    m_captures.append(AudioEngine::discover_audio_devices(AUDIO_IN));

    for (int i=0; i<m_playbacks.count(); i++)
        ui->audioOutCombo->addItem(m_playbacks.at(i));
    for (int i=0; i<m_captures.count(); i++)
        ui->audioInCombo->addItem(m_captures.at(i));

    ui->vocoderCombo->clear();
    ui->modemCombo->clear();
    digihamlib->m_vocoders.clear();
    digihamlib->m_modems.clear();
    digihamlib->m_vocoders.append("Software vocoder");
    ui->vocoderCombo->addItem("Software vocoder");
    digihamlib->m_modems.append("None");
    ui->modemCombo->addItem("None");
#if !defined(Q_OS_IOS)
    QMap<QString, QString> l = discover_serial_devices();
    QMap<QString, QString>::const_iterator i = l.constBegin();

    while (i != l.constEnd())
    {
        digihamlib->m_vocoders.append(i.value());
        ui->vocoderCombo->addItem(i.value());
        digihamlib->m_modems.append(i.value());
        ui->modemCombo->addItem(i.value());
        ++i;
    }
#endif
}

QMap<QString, QString> MainWindow::discover_serial_devices()
{
    QMap<QString, QString> devlist;
    const QString blankString = "N/A";
    QString out;
#ifndef Q_OS_ANDROID
    const auto serialPortInfos = QSerialPortInfo::availablePorts();

    if (serialPortInfos.count())
    {
        for (const QSerialPortInfo &serialPortInfo : serialPortInfos)
        {
            out = "Port: " + serialPortInfo.portName() + ENDLINE
                  + "Location: " + serialPortInfo.systemLocation() + ENDLINE
                  + "Description: " + (!serialPortInfo.description().isEmpty() ? serialPortInfo.description() : blankString) + ENDLINE
                  + "Manufacturer: " + (!serialPortInfo.manufacturer().isEmpty() ? serialPortInfo.manufacturer() : blankString) + ENDLINE
                  + "Serial number: " + (!serialPortInfo.serialNumber().isEmpty() ? serialPortInfo.serialNumber() : blankString) + ENDLINE
                  + "Vendor Identifier: " + (serialPortInfo.hasVendorIdentifier() ? QByteArray::number(serialPortInfo.vendorIdentifier(), 16) : blankString) + ENDLINE
                  + "Product Identifier: " + (serialPortInfo.hasProductIdentifier() ? QByteArray::number(serialPortInfo.productIdentifier(), 16) : blankString) + ENDLINE;
            //fprintf(stderr, "%s", out.toStdString().c_str());fflush(stderr);
            devlist[serialPortInfo.systemLocation()] = serialPortInfo.description() + ":" + serialPortInfo.systemLocation();
        }
    }
#else
    QStringList list = AndroidSerialPort::GetInstance().discover_devices();
    for ( const auto& i : list  )
    {
        devlist[i] = i;
    }
#endif
    return devlist;
}

void MainWindow::do_connect(void)
{
    save_settings();
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
#if defined(Q_OS_WIN)
        m_audio->stop_playback();
#endif
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
//    if (!digihamlib->m_gps.isEmpty())
  //      fprintf(stderr, "GPSL: %s\n", digihamlib->m_gps.toLatin1().data());
}

void MainWindow::process_audio()
{
    int i = 0;

    while (!digihamlib->m_rxaudioq.isEmpty())
    {
        i++;
        m_audio->m_rxaudioq.enqueue(digihamlib->m_rxaudioq.dequeue());
//        QApplication::processEvents();
        if (i == 160) break;
    }
    if (!isTx)
        ui->volumeLevelBar->setValue((m_audio->level() / 32767.0f)  * 100);
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

void MainWindow::setMicGain(int gain)
{
    m_audio->set_input_volume(gain / 100.0f);
}

void MainWindow::send_mic_audio()
{
    int16_t pcm[160];
    memset(pcm, 0, 160 * sizeof(int16_t));
    m_audio->read(pcm, 160);
    for (int i=0; i<160; i++)
    {
//        QApplication::processEvents();
        digihamlib->m_mode->m_micData.enqueue(pcm[i]);
    }
  //  fprintf(stderr, "Level: %f\n", m_audio->level() / 32767.0f);
    ui->volumeLevelBar->setValue((m_audio->level() / 32767.0f)  * 100);
}

void MainWindow::start_tx(void)
{
    m_audio->set_input_buffer_size(640);
    m_audio->start_capture();
    digihamlib->m_mode->m_micData.clear();
    txtimer->start(19);
    isTx = true;
}

void MainWindow::stop_tx(void)
{
    txtimer->stop();
    m_audio->stop_capture();
    isTx = false;
    ui->volumeLevelBar->setValue(0);
}

void MainWindow::micDeviceChanged(int selection)
{
    qDebug() << "Mic selected is device number: " << selection;
//    m_audio->stop_capture();
    m_audio->stop_playback();
    m_audio->deleteLater();
    m_audio = new AudioEngine(ui->audioInCombo->itemText(selection), ui->audioOutCombo->currentText());
    m_audio->init();
}

void MainWindow::spkrDeviceChanged(int selection)
{
    qDebug() << "Speaker selected is device number: " << selection;
//    m_audio->stop_capture();
    m_audio->stop_playback();
    m_audio->deleteLater();
    m_audio = new AudioEngine(ui->audioInCombo->currentText(), ui->audioOutCombo->itemText(selection));
    m_audio->init();
    m_audio->start_playback();
}

void MainWindow::vocoderChanged(int index)
{
    digihamlib->m_vocoder = digihamlib->m_vocoders.at(index);
}

void MainWindow::modemChanged(int index)
{
    digihamlib->m_modem = digihamlib->m_modems.at(index);
}

void MainWindow::m17_voice_rate_changed(bool state)
{
    emit digihamlib->m17_rate_changed(state);
}
