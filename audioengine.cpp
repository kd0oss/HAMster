/*
	Copyright (C) 2019-2021 Doug McLain

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "audioengine.h"
#include <QDebug>
#include <cmath>

#if defined (Q_OS_MACOS) || defined(Q_OS_IOS)
#define MACHAK 1
#else
#define MACHAK 0
#endif

Audio_playback::Audio_playback() : QIODevice()
{
    queue.clear();
    pdecoded_buffer = &queue;
}


Audio_playback::~Audio_playback()
{
}


void Audio_playback::start()
{
    open(QIODevice::ReadOnly);
} // end start


void Audio_playback::stop()
{
    close();
} // end stop


void Audio_playback::set_decoded_buffer(QQueue<qint16> *pBuffer)
{
    pdecoded_buffer = pBuffer;
} // end set_decoded_buffer


qint64 Audio_playback::readData(char *data, qint64 maxlen)
{
    qint64 bytes_read;
    qint16 v;
    int audio_byte_order = 0;
    qint64 bytes_to_read = maxlen > 320 ? 320: maxlen;
   // int    j=0;
    //qint64 bytes_to_read = maxlen;
    bytes_read = 0;
 //   m_maxlevel = 0;

    if (pdecoded_buffer->isEmpty())
    {
        // probably not connected or late arrival of packets.  Send silence.
        memset(data, 0, bytes_to_read);
        bytes_read = bytes_to_read;
    } else {
        while ((!pdecoded_buffer->isEmpty()) && (bytes_read < bytes_to_read))
        {
         //   v = pdecoded_buffer->tryDequeue(&j);
            v = pdecoded_buffer->dequeue();
        //    if (j == 1)
            {
//                if (v > m_maxlevel)
  //                  m_maxlevel = v;
                switch (audio_byte_order)
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
            }
        }
        while (bytes_read < bytes_to_read) data[bytes_read++] = 0;
    }
    return bytes_read;
} // end readData


qint64 Audio_playback::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data)
    Q_UNUSED(len)
    return 0;
} // end writeData


qint64 Audio_playback::bytesAvailable() const
{
 //   qDebug("Size: %ld", (long int)(QIODevice::bytesAvailable()));
    return 320; //(pdecoded_buffer->size() * sizeof(qint16)) + QIODevice::bytesAvailable();
} // end bytesAvailable


AudioEngine::AudioEngine(QString in, QString out) :
	m_outputdevice(out),
	m_inputdevice(in),
	m_out(nullptr),
	m_in(nullptr),
	m_srm(1)
{
	m_audio_out_temp_buf_p = m_audio_out_temp_buf;
	memset(m_aout_max_buf, 0, sizeof(float) * 200);
	m_aout_max_buf_p = m_aout_max_buf;
	m_aout_max_buf_idx = 0;
	m_aout_gain = 100;
	m_volume = 1.0f;
    started = false;
    atimer = new QTimer(this);
    atimer->setInterval(5);
    atimer->setSingleShot(true);
    connect(atimer, SIGNAL(timeout()), this, SLOT(send_audio()));
    atimer->start();
}

AudioEngine::~AudioEngine()
{
    delete atimer;
}

QStringList AudioEngine::discover_audio_devices(uint8_t d)
{
	QStringList list;
	QList<QAudioDevice> devices;

    if (d)
    {
		devices = QMediaDevices::audioOutputs();
	}
    else
    {
		devices = QMediaDevices::audioInputs();
	}

    for (QList<QAudioDevice>::ConstIterator it = devices.constBegin(); it != devices.constEnd(); ++it )
    {
		//fprintf(stderr, "Playback device name = %s\n", (*it).deviceName().toStdString().c_str());fflush(stderr);
		list.append((*it).description());
	}

	return list;
}

void AudioEngine::init()
{
    if (m_outputdevice == "OS Default")
    {
        m_outputdevice.clear();
    }
    if (m_inputdevice == "OS Default")
    {
        m_inputdevice.clear();
    }

    m_agc = true;

    m_playback = new Audio_playback();

	QList<QAudioDevice> devices = QMediaDevices::audioOutputs();
    if (devices.size() == 0)
    {
        qDebug() << "No audio playback hardware found";
	}
    else
    {
		QAudioDevice device(QMediaDevices::defaultAudioOutput());
        for (QList<QAudioDevice>::ConstIterator it = devices.constBegin(); it != devices.constEnd(); ++it )
        {

            qDebug() << "Playback device name = " << (*it).description();
            qDebug() << (*it).supportedSampleFormats();
            qDebug() << (*it).preferredFormat();

            if ((*it).description() == m_outputdevice)
            {
				device = *it;
			}
		}
        QAudioFormat format = device.preferredFormat();
        format.setSampleRate(8000);
        format.setChannelCount(1);
        format.setSampleFormat(QAudioFormat::Int16);

        if (!device.isFormatSupported(format))
        {
            qWarning() << "Raw audio format not supported by playback device";
        }
        m_outputdevice = device.description();
        qDebug() << "Playback device: " << device.description() << "SR: " << format.sampleRate();
        spkr = device;
        m_out = new QAudioSink(device, format, this);
        m_out->setBufferSize(1024 * 20);
		connect(m_out, SIGNAL(stateChanged(QAudio::State)), this, SLOT(handleStateChanged(QAudio::State)));
	}

	devices = QMediaDevices::audioInputs();

    if (devices.size() == 0)
    {
        qDebug() <<  "No audio capture hardware found";
	}
    else
    {
		QAudioDevice device(QMediaDevices::defaultAudioInput());
        for (QList<QAudioDevice>::ConstIterator it = devices.constBegin(); it != devices.constEnd(); ++it )
        {
            if (MACHAK)
            {
                qDebug() << "Capture device name = " << (*it).description();
				qDebug() << (*it).supportedSampleFormats();
				qDebug() << (*it).preferredFormat();
			}
            if ((*it).description() == m_inputdevice)
            {
				device = *it;
			}
		}
        QAudioFormat format = device.preferredFormat();
        format.setSampleRate(8000);
        format.setChannelCount(1);
        format.setSampleFormat(QAudioFormat::Int16);

        if (!device.isFormatSupported(format))
        {
            qWarning() << "Raw audio format not supported by capture device";
        }
        m_inputdevice = device.description();
		int sr = 8000;
        if (MACHAK)
        {
			sr = device.preferredFormat().sampleRate();
			m_srm = (float)sr / 8000.0;
		}
		format.setSampleRate(sr);
        m_in = new QAudioSource(device, format, this);
        qDebug() << "Capture device: " <<  device.description() << " SR: " << sr << " resample factor: " << m_srm;
	}
}

void AudioEngine::start_capture()
{
	m_audioinq.clear();
    if (m_in != nullptr)
    {
		m_indev = m_in->start();
        if (MACHAK) m_srm = (float)(m_in->format().sampleRate()) / 8000.0;
		connect(m_indev, SIGNAL(readyRead()), SLOT(input_data_received()));
	}
}

void AudioEngine::stop_capture()
{
    if (m_in != nullptr)
    {
		m_indev->disconnect();
		m_in->stop();
	}
}

void AudioEngine::start_playback()
{
    if (m_out != nullptr)
    {
      //  atimer->start();
        m_playback->start();
   //   qDebug("Bytes: %ld\n", (long int)m_playback->bytesAvailable());
        m_out->start(m_playback);
    }
}

void AudioEngine::stop_playback()
{
    if (m_out != nullptr)
    {
        //m_outdev->reset();
   //     atimer->stop();
        m_playback->stop();
        m_out->reset();
        m_out->stop();
    }
}

void AudioEngine::input_data_received()
{
	QByteArray data = m_indev->readAll();

    if (data.size() > 0)
    {
        if (MACHAK)
        {
			std::vector<int16_t> samples;
            for (int i = 0; i < data.size(); i += 2)
            {
				samples.push_back(((data.data()[i+1] << 8) & 0xff00) | (data.data()[i] & 0xff));
			}
            for (int i = 0; i < data.size()/2; i += m_srm)
            {
				m_audioinq.enqueue(samples[i]);
			}
		}
        else
        {
            for (int i = 0; i < data.size(); i += (2 * m_srm))
            {
				m_audioinq.enqueue(((data.data()[i+1] << 8) & 0xff00) | (data.data()[i] & 0xff));
			}
		}
	}
}

void AudioEngine::write(int16_t *pcm, size_t s)
{
    if (m_out == nullptr) return;

    if (m_agc)
    {
        process_audio(pcm, s);
	}

	size_t l = m_outdev->write((const char *) pcm, sizeof(int16_t) * s);

    if (l*2 < s)
    {
//		qDebug() << "AudioEngine::write() " << s << ":" << l << ":" << (int)m_out->bytesFree() << ":" << m_out->bufferSize() << ":" << m_out->error();
	}
}

uint16_t AudioEngine::read(int16_t *pcm, int s)
{
	m_maxlevel = 0;

    if (m_audioinq.size() >= s)
    {
        for (int i = 0; i < s; ++i)
        {
            pcm[i] = m_audioinq.dequeue();
            if (pcm[i] > m_maxlevel)
            {
				m_maxlevel = pcm[i];
			}
		}
		return 1;
	}
    else if (m_in == nullptr)
    {
		memset(pcm, 0, sizeof(int16_t) * s);
		return 1;
	}
    else
    {
		return 0;
	}
}

uint16_t AudioEngine::read(int16_t *pcm)
{
	int s;
	m_maxlevel = 0;

    if (m_audioinq.size() >= 160)
    {
		s = 160;
	}
    else
    {
		s = m_audioinq.size();
	}

    for (int i = 0; i < s; ++i)
    {
		pcm[i] = m_audioinq.dequeue();
        if (pcm[i] > m_maxlevel)
        {
			m_maxlevel = pcm[i];
		}
	}

	return s;
}

void AudioEngine::send_audio()
{
    int16_t pcm[160];
    int i = 0;
 //   static bool started;
    m_maxlevel = 0;
    memset(pcm, 0, 160 * sizeof(int16_t));
//    memset(pcm+(79 * sizeof(int16_t)), 100, 80 * sizeof(int16_t));

    while (m_rxaudioq.size() >= 160)
    {
        pcm[i++] = m_rxaudioq.dequeue();
        if (i == 160) break;
    }
    if (i >= 160)
    {
        process_audio(pcm, 160);
        for (i=0; i<160; i++)
        {
            if (pcm[i] > m_maxlevel)
                m_maxlevel = pcm[i];
            m_playback->pdecoded_buffer->enqueue(pcm[i]);
        }
        if (!started)
        {
            started = true;
            start_playback();
        }
    }
    atimer->start();
}

// process_audio() based on code from DSD https://github.com/szechyjs/dsd
void AudioEngine::process_audio(int16_t *pcm, size_t s)
{
	float aout_abs, max, gainfactor, gaindelta, maxbuf;

    for (size_t i = 0; i < s; ++i)
    {
		m_audio_out_temp_buf[i] = static_cast<float>(pcm[i]);
	}

	// detect max level
	max = 0;
	m_audio_out_temp_buf_p = m_audio_out_temp_buf;

    for (size_t i = 0; i < s; i++)
    {
		aout_abs = fabsf(*m_audio_out_temp_buf_p);

        if (aout_abs > max)
        {
			max = aout_abs;
		}

		m_audio_out_temp_buf_p++;
	}

	*m_aout_max_buf_p = max;
	m_aout_max_buf_p++;
	m_aout_max_buf_idx++;

    if (m_aout_max_buf_idx > 24)
    {
		m_aout_max_buf_idx = 0;
		m_aout_max_buf_p = m_aout_max_buf;
	}

	// lookup max history
    for (size_t i = 0; i < 25; i++)
    {
		maxbuf = m_aout_max_buf[i];

        if (maxbuf > max)
        {
			max = maxbuf;
		}
	}

	// determine optimal gain level
    if (max > static_cast<float>(0))
    {
		gainfactor = (static_cast<float>(30000) / max);
	}
    else
    {
		gainfactor = static_cast<float>(50);
	}

    if (gainfactor < m_aout_gain)
    {
		m_aout_gain = gainfactor;
		gaindelta = static_cast<float>(0);
	}
    else
    {
        if (gainfactor > static_cast<float>(50))
        {
			gainfactor = static_cast<float>(50);
        }

		gaindelta = gainfactor - m_aout_gain;

        if (gaindelta > (static_cast<float>(0.05) * m_aout_gain))
        {
			gaindelta = (static_cast<float>(0.05) * m_aout_gain);
		}
	}

	gaindelta /= static_cast<float>(s); //160

	// adjust output gain
	m_audio_out_temp_buf_p = m_audio_out_temp_buf;

    for (size_t i = 0; i < s; i++)
    {
		*m_audio_out_temp_buf_p = (m_aout_gain + (static_cast<float>(i) * gaindelta)) * (*m_audio_out_temp_buf_p);
		m_audio_out_temp_buf_p++;
	}

	m_aout_gain += (static_cast<float>(s) * gaindelta);
	m_audio_out_temp_buf_p = m_audio_out_temp_buf;

    for (size_t i = 0; i < s; i++)
    {
		*m_audio_out_temp_buf_p *= m_volume;
        if (*m_audio_out_temp_buf_p > static_cast<float>(32760))
        {
			*m_audio_out_temp_buf_p = static_cast<float>(32760);
		}
        else if (*m_audio_out_temp_buf_p < static_cast<float>(-32760))
        {
			*m_audio_out_temp_buf_p = static_cast<float>(-32760);
		}
		pcm[i] = static_cast<int16_t>(*m_audio_out_temp_buf_p);
		m_audio_out_temp_buf_p++;
	}
}

void AudioEngine::handleStateChanged(QAudio::State newState)
{
    switch (newState)
    {
	case QAudio::ActiveState:
        qDebug() << "AudioOut state active";
		break;
	case QAudio::SuspendedState:
        qDebug() << "AudioOut state suspended";
		break;
	case QAudio::IdleState:
        qDebug() << "AudioOut state idle";
        stop_playback();
		break;
	case QAudio::StoppedState:
        qDebug() << "AudioOut state stopped";
        started = false;
        break;
	default:
		break;
	}
}
