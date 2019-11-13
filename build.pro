TEMPLATE = subdirs

SUBDIRS = \
    hibikase \
    rubberband

rubberband.file = external/rubberband.pro

hibikase.depends = rubberband
