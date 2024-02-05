#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QObject>
#include <QDebug>
#include <QTimer>
#include "Audio.h"
#include "Audioinput.h"
#include "../digihamlib/digihamlib.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void download_file(QString, bool u = false);
    void url_downloaded(QString);
    void file_downloaded(QString);

private slots:
    void do_connect(void);
    void updateLog(QString);
    void process_audio(int16_t*, int);
    void update_status(void);
    void micSendAudio(QQueue<qint16>* queue);
    void updateMicLevel(qreal);
    void start_tx(void);
    void stop_tx(void);
    void discover_devices(void);
    void setMicGain(int);
    void process_dstar_hosts(QString);
    void process_ysf_hosts();
    void process_fcs_rooms();
    void process_dmr_hosts();
    void process_p25_hosts();
    void process_nxdn_hosts();
    void process_m17_hosts();
    void process_dmr_ids();
    void process_nxdn_ids();
    void process_mode_change(QString);
    void audioDeviceChanged(int);
    void micDeviceChanged(int);
    void audioDeviceChanged(QAudioDevice info,int rate,int channels);
    void micDeviceChanged(QAudioDevice info,int rate,int channels);
    void vocoderChanged(int);
    void modemChanged(int);

private:
    void save_settings();
    void process_settings();
    void check_host_files();
    void update_host_files();
    void update_custom_hosts(QString);
    void update_dmr_ids();
    void update_nxdn_ids();

    Ui::MainWindow *ui;
    Digihamlib *digihamlib;
    Audio* audio;
    AudioInput* audioinput;
    QAudioDevice audio_device;
    QString m_audioin;
    QString m_audioout;
    QTimer *mtimer;
    bool m_update_host_files;
    QSettings *m_settings;
    QString config_path;
    QString hosts_filename;
    QString m_callsign;
    QString m_host;
    QString m_refname;
    QString m_protocol;
    QString m_bm_password;
    QString m_tgif_password;
    QString m_latitude;
    QString m_longitude;
    QString m_location;
    QString m_description;
    QString m_freq;
    QString m_url;
    QString m_swid;
    QString m_pkgid;
    QString m_dmropts;
    QString m_mycall;
    QString m_urcall;
    QString m_rptr1;
    QString m_rptr2;
    QString m_dstarusertxt;
    QStringList m_hostsmodel;
    QMap<QString, QString> m_hostmap;
    QStringList m_customhosts;
    QString m_saved_refhost;
    QString m_saved_dcshost;
    QString m_saved_xrfhost;
    QString m_saved_ysfhost;
    QString m_saved_fcshost;
    QString m_saved_dmrhost;
    QString m_saved_p25host;
    QString m_saved_nxdnhost;
    QString m_saved_m17host;
    QString m_saved_iaxhost;
    bool m_mdirect;
    char m_module;
    bool m_xrf2ref;
    bool m_ipv6;
    uint32_t m_dmrid;
    uint8_t m_essid;
    uint32_t m_dmr_srcid;
    uint32_t m_dmr_destid;
    QMap<uint32_t, QString> m_dmrids;
    QMap<uint16_t, QString> m_nxdnids;
    int m_txtimeout;
    bool m_settings_processed;
    bool m_modelchange;
    int m_tts;
    QString m_ttstxt;
    QString m_localhosts;

    QString m_modemRxFreq;
    QString m_modemTxFreq;
    QString m_modemRxOffset;
    QString m_modemTxOffset;
    QString m_modemRxDCOffset;
    QString m_modemTxDCOffset;
    QString m_modemRxLevel;
    QString m_modemTxLevel;
    QString m_modemRFLevel;
    QString m_modemTxDelay;
    QString m_modemCWIdTxLevel;
    QString m_modemDstarTxLevel;
    QString m_modemDMRTxLevel;
    QString m_modemYSFTxLevel;
    QString m_modemP25TxLevel;
    QString m_modemNXDNTxLevel;
    QString m_modemBaud;
    QString m_modemM17CAN;
    bool m_modemTxInvert;
    bool m_modemRxInvert;
    bool m_modemPTTInvert;

    QString myCall;
    QString urCall;
    QString rptr1;
    QString rptr2;
    QString slowSpeed;
    QString host;
    QString module;
    QStringList m_vocoders;
    QStringList m_modems;

    bool isTx;
};
#endif // MAINWINDOW_H
