// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>
#include <QtMultimedia/qmediaplayer.h>
#include <QtMultimedia/private/qmediaplayer_p.h>
#include <QtMultimedia/private/qgstreamer_platformspecificinterface_p.h>
#include <QtQGstreamerMediaPlugin/private/qgstpipeline_p.h>

#include <qscopedenvironmentvariable.h>

#include <memory>

QT_USE_NAMESPACE

using namespace Qt::Literals;

class tst_QMediaPlayerGStreamer : public QObject
{
    Q_OBJECT

public:
    tst_QMediaPlayerGStreamer();

public slots:
    void initTestCase();
    void init();
    void cleanup();

private slots:
    void constructor_preparesGstPipeline();
    void videoSink_constructor_overridesConversionElement();
    void videoSink_constructor_overridesConversionElement_withMultipleElements();

private:
    std::unique_ptr<QMediaPlayer> player;

    GstPipeline *getGstPipeline()
    {
        auto *iface = QGStreamerPlatformSpecificInterface::instance();
        return iface ? iface->gstPipeline(player.get()) : nullptr;
    }

    QGstPipeline getPipeline()
    {
        return QGstPipeline{
            getGstPipeline(),
            QGstPipeline::NeedsRef,
        };
    }

    void dumpGraph(const char *fileNamePrefix)
    {
        GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(getGstPipeline()),
                                  GstDebugGraphDetails(GST_DEBUG_GRAPH_SHOW_VERBOSE),
                                  fileNamePrefix);
    }

    bool mediaSupported = false;
};

tst_QMediaPlayerGStreamer::tst_QMediaPlayerGStreamer()
{
    qputenv("QT_MEDIA_BACKEND", "gstreamer");
}

void tst_QMediaPlayerGStreamer::initTestCase()
{
    using namespace std::chrono_literals;

    QMediaPlayer player;

    QVideoSink sink;
    player.setVideoSink(&sink);
    player.setSource(QUrl("qrc:/testdata/color_matrix.mp4"));

    for (;;) {
        QMediaPlayer::MediaStatus status = player.mediaStatus();
        switch (status) {
        case QMediaPlayer::MediaStatus::InvalidMedia: {
            mediaSupported = false;
            return;
        }
        case QMediaPlayer::MediaStatus::NoMedia:
        case QMediaPlayer::MediaStatus::StalledMedia:
        case QMediaPlayer::MediaStatus::LoadingMedia:
            QTest::qWait(20ms);
            continue;

        default: {
            mediaSupported = true;
            return;
        }
        }
    }
}

void tst_QMediaPlayerGStreamer::init()
{
    player = std::make_unique<QMediaPlayer>();
}

void tst_QMediaPlayerGStreamer::cleanup()
{
    player.reset();
}

void tst_QMediaPlayerGStreamer::constructor_preparesGstPipeline()
{
    auto *rawPipeline = getGstPipeline();
    QVERIFY(rawPipeline);

    QGstPipeline pipeline{
        rawPipeline,
        QGstPipeline::NeedsRef,
    };
    QVERIFY(pipeline);

    QVERIFY(pipeline.findByName("videoInputSelector"));
    QVERIFY(pipeline.findByName("audioInputSelector"));
    QVERIFY(pipeline.findByName("subTitleInputSelector"));

    dumpGraph("constructor_preparesGstPipeline");
}

void tst_QMediaPlayerGStreamer::videoSink_constructor_overridesConversionElement()
{
    if (!mediaSupported)
        QSKIP("Media playback not supported");

    QScopedEnvironmentVariable convOverride{
        "QT_GSTREAMER_OVERRIDE_VIDEO_CONVERSION_ELEMENT",
        "identity name=myConverter",
    };

    QVideoSink sink;
    player->setVideoSink(&sink);
    player->setSource(QUrl("qrc:/testdata/color_matrix.mp4"));

    QGstPipeline pipeline = getPipeline();
    QTEST_ASSERT(pipeline);

    QTRY_VERIFY(pipeline.findByName("myConverter"));

    dumpGraph("videoSink_constructor_overridesConversionElement");
}

void tst_QMediaPlayerGStreamer::
        videoSink_constructor_overridesConversionElement_withMultipleElements()
{
    if (!mediaSupported)
        QSKIP("Media playback not supported");

    QScopedEnvironmentVariable convOverride{
        "QT_GSTREAMER_OVERRIDE_VIDEO_CONVERSION_ELEMENT",
        "identity name=myConverter ! identity name=myConverter2",
    };

    QVideoSink sink;
    player->setVideoSink(&sink);
    player->setSource(QUrl("qrc:/testdata/color_matrix.mp4"));

    QGstPipeline pipeline = getPipeline();
    QTEST_ASSERT(pipeline);

    QTRY_VERIFY(pipeline.findByName("myConverter"));
    QTRY_VERIFY(pipeline.findByName("myConverter2"));

    dumpGraph("videoSink_constructer_overridesConversionElement_withMultipleElements");
}

QTEST_GUILESS_MAIN(tst_QMediaPlayerGStreamer)

#include "tst_qmediaplayer_gstreamer.moc"
