// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QComboBox>
#include <QDialog>
#include <QSize>
#include <QTextBrowser>

namespace Ui {
class AboutDialog;
}

class AboutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AboutDialog(QWidget* parent = nullptr);
    ~AboutDialog() override;

    QSize sizeHint() const override;

private slots:
    void OnCurrentIndexChanged(int index);

private:
    void PopulateComboBox();

    QComboBox* m_combo_box;
    QTextBrowser* m_text_browser;
};
