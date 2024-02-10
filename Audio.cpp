/* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*
*/

#include "Audio.h"

Audio_playback::Audio_playback():QIODevice()
{
    recv_ts = 0;
    pdecoded_buffer = &queue;
}


Audio_playback::~Audio_playback()
{
}


void Audio_playback::start()
{
    open(QIODevice::ReadOnly);
}

void Audio_playback::stop()
{
    close();
}


void Audio_playback::set_decoded_buffer(QHQueue<qint16> *pBuffer)
{
    pdecoded_buffer = pBuffer;
}


qint64 Audio_playback::readData(char *data, qint64 maxlen)
{
    qint16 bytes_read;
    qint16 v;
    qint16 bytes_to_read = maxlen > 2000 ? 2000: maxlen;
    int audio_byte_order = 0;
    bytes_read = 0;
    maxlevel = 0;

    if (pdecoded_buffer->isEmpty())
    {
        // probably not connected or late arrival of packets.  Send silence.
        memset(data, 0, bytes_to_read);
        bytes_read = bytes_to_read;
    } else {
        while ((!pdecoded_buffer->isEmpty()) && (bytes_read < bytes_to_read))
        {
            v = pdecoded_buffer->dequeue();
            switch(audio_byte_order)
            {
            case 0:
                data[bytes_read++]=(char)(v&0xFF);
                data[bytes_read++]=(char)((v>>8)&0xFF);
                break;
            case 1:
                data[bytes_read++]=(char)((v>>8)&0xFF);
                data[bytes_read++]=(char)(v&0xFF);
                break;
            }
            for (qint16 i = 0; i < maxlen; i=i+2)
            {
                if (v > maxlevel)
                {
                    maxlevel = (int16_t)v;
                }
            }
        }
        while (bytes_read < bytes_to_read) data[bytes_read++] = 0;
    }
    return bytes_read;
}

qint64 Audio_playback::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data)
    Q_UNUSED(len)
    return 0;
}


Audio::Audio() : m_devices(new QMediaDevices(this))
{
    audio_output = NULL;
    connected = false;
    sampleRate = 8000;
    audio_channels = 1;

    initializeAudio(m_devices->defaultAudioOutput());
    m_audioOutput->start(m_audioPlayback.data());
}


Audio::~Audio()
{
}


void Audio::initializeAudio(const QAudioDevice &deviceInfo)
{
    QAudioFormat format = deviceInfo.preferredFormat();

    format.setSampleRate(sampleRate);
    format.setSampleFormat(QAudioFormat::Int16);
    format.setChannelCount(audio_channels);
    m_audioPlayback.reset(new Audio_playback());
    m_audioPlayback->set_decoded_buffer(&decoded_buffer);
    m_audioOutput.reset(new QAudioSink(deviceInfo, format));
    m_audioOutput->setBufferSize(1280);
    m_audioPlayback->start();
}


void Audio::set_volume(qreal vol)
{
    m_audioOutput->setVolume(vol);
}

void Audio::clear_decoded_buffer(void)
{
    decoded_buffer.clear();
}


void Audio::get_audio_device(QAudioDevice * device)
{
    *device = audio_device;
}


void Audio::deviceChanged(int index)
{
    m_audioPlayback->stop();
    m_audioOutput->stop();
    m_audioOutput->disconnect(this);
    initializeAudio(devList->itemData(index).value<QAudioDevice>());
    m_audioOutput->start(m_audioPlayback.data());
}

void Audio::updateAudioDevices(QComboBox *comboBox)
{
    devList = comboBox;
    const QAudioDevice &defaultDeviceInfo = m_devices->defaultAudioOutput();
    if (comboBox != NULL)
    {
        comboBox->disconnect();
        comboBox->clear();
        comboBox->addItem(defaultDeviceInfo.description(), QVariant::fromValue(defaultDeviceInfo));
    }
    m_devices->disconnect();
    const QList<QAudioDevice> devices = m_devices->audioOutputs();
    if (comboBox != NULL)
    {
        for (auto &deviceInfo : m_devices->audioOutputs())
        {
            if (deviceInfo != defaultDeviceInfo)
                comboBox->addItem(deviceInfo.description(), QVariant::fromValue(deviceInfo));
        }
        connect(comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(deviceChanged(int)));
   //     connect(m_devices, SIGNAL(audioOutputsChanged()), this, SLOT(updateAudioDevices(comboBox)));
    }
}

void Audio::process_audio(int16_t* buffer, int length)
{
    for (int i=0; i < length; i++)
    {
        decoded_buffer.enqueue(buffer[i]);
    }
    maxlevel = m_audioPlayback->maxlevel;
}
