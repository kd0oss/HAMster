#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QObject>
#include <QDebug>
#include <QTimer>
//#include "Audio.h"
//#include "Audioinput.h"
#include "audioengine.h"
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
    void process_audio(void);
    void update_status(void);
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
    void micDeviceChanged(int);
    void micDeviceChanged(QAudioDevice info,int rate,int channels);
    void vocoderChanged(int);
    void modemChanged(int);
    void send_mic_audio(void);

private:
    void save_settings();
    void process_settings();
    void check_host_files();
    void update_host_files();
    void update_custom_hosts(QString);
    void update_dmr_ids();
    void update_nxdn_ids();
    QMap<QString, QString> discover_serial_devices(void);

    Ui::MainWindow *ui;
    Digihamlib *digihamlib;
 //   Audio* audio;
 //   AudioInput* audioinput;
    AudioEngine *m_audio;
    QAudioDevice audio_device;
    QString m_audioin;
    QString m_audioout;
    QTimer *mtimer;
    bool m_update_host_files;
    QSettings *m_settings;
    QString config_path;
    int m_txtimeout;
    bool m_settings_processed;
    bool m_mdirect;
    int m_tts;
    QString m_ttstxt;
    QString m_protocol;

    QString host;
    QStringList m_vocoders;
    QStringList m_modems;
    QString m_localhosts;
    QTimer *txtimer;

    bool isTx;
};
#endif // MAINWINDOW_H
