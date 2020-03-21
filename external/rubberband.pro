TARGET = rubberband
TEMPLATE = lib
CONFIG += static

# Rubberband is external code we don't maintain, so trivial compiler warnings
# in it are of no interest to us.
CONFIG += warn_off

win32-msvc* {
    DEFINES += __MSVC__
    DEFINES += NOMINMAX
}

!win32-msvc* {
    DEFINES += USE_PTHREADS
}

DEFINES += NDEBUG
DEFINES += NO_THREAD_CHECKS
DEFINES += NO_TIMING

DEFINES += USE_KISSFFT
DEFINES += USE_SPEEX

INCLUDEPATH += rubberband
INCLUDEPATH += rubberband/src

contains(DEFINES, USE_KISSFFT) {
    SOURCES += \
        rubberband/src/kissfft/kiss_fft.c \
        rubberband/src/kissfft/kiss_fftr.c
}

contains(DEFINES, USE_SPEEX) {
    HEADERS += rubberband/src/speex/speex_resampler.h
    SOURCES += rubberband/src/speex/resample.c
}

HEADERS += \
    rubberband/rubberband/rubberband-c.h \
    rubberband/rubberband/RubberBandStretcher.h

HEADERS += \
    rubberband/src/StretcherChannelData.h \
    rubberband/src/float_cast/float_cast.h \
    rubberband/src/StretcherImpl.h \
    rubberband/src/StretchCalculator.h \
    rubberband/src/base/Profiler.h \
    rubberband/src/base/RingBuffer.h \
    rubberband/src/base/Scavenger.h \
    rubberband/src/dsp/AudioCurveCalculator.h \
    rubberband/src/audiocurves/CompoundAudioCurve.h \
    rubberband/src/audiocurves/ConstantAudioCurve.h \
    rubberband/src/audiocurves/HighFrequencyAudioCurve.h \
    rubberband/src/audiocurves/PercussiveAudioCurve.h \
    rubberband/src/audiocurves/SilentAudioCurve.h \
    rubberband/src/audiocurves/SpectralDifferenceAudioCurve.h \
    rubberband/src/dsp/Resampler.h \
    rubberband/src/dsp/FFT.h \
    rubberband/src/dsp/MovingMedian.h \
    rubberband/src/dsp/SincWindow.h \
    rubberband/src/dsp/Window.h \
    rubberband/src/system/Allocators.h \
    rubberband/src/system/Thread.h \
    rubberband/src/system/VectorOps.h \
    rubberband/src/system/VectorOpsComplex.h \
    rubberband/src/system/sysutils.h

SOURCES += \
    rubberband/src/rubberband-c.cpp \
    rubberband/src/RubberBandStretcher.cpp \
    rubberband/src/StretcherProcess.cpp \
    rubberband/src/StretchCalculator.cpp \
    rubberband/src/base/Profiler.cpp \
    rubberband/src/dsp/AudioCurveCalculator.cpp \
    rubberband/src/audiocurves/CompoundAudioCurve.cpp \
    rubberband/src/audiocurves/SpectralDifferenceAudioCurve.cpp \
    rubberband/src/audiocurves/HighFrequencyAudioCurve.cpp \
    rubberband/src/audiocurves/SilentAudioCurve.cpp \
    rubberband/src/audiocurves/ConstantAudioCurve.cpp \
    rubberband/src/audiocurves/PercussiveAudioCurve.cpp \
    rubberband/src/dsp/Resampler.cpp \
    rubberband/src/dsp/FFT.cpp \
#    rubberband/src/system/Allocators.cpp \ (unused in our configuration)
    rubberband/src/system/sysutils.cpp \
    rubberband/src/system/Thread.cpp \
#    rubberband/src/system/VectorOpsComplex.cpp \ (unused in our configuration)
    rubberband/src/StretcherChannelData.cpp \
    rubberband/src/StretcherImpl.cpp
