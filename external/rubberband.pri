INCLUDEPATH *= $$PWD/rubberband/rubberband

win32:CONFIG(release, debug|release): LIBS += -L"$$OUT_PWD/../external/release/" -lrubberband
else:win32:CONFIG(debug, debug|release): LIBS += -L"$$OUT_PWD/../external/debug/" -lrubberband
else:unix: LIBS += -L"$$OUT_PWD/../external/" -lrubberband

win32:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../external/release/rubberband.lib
else:win32:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../external/debug/rubberband.lib
else:unix: PRE_TARGETDEPS += $$OUT_PWD/../external/librubberband.a
