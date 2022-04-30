// SPDX-License-Identifier: GPL-2.0-or-later

#include "SettingsDialog.h"

#include "Settings.h"

#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Hibikase Preferences"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    QGridLayout* grid = new QGridLayout(this);

    int row = 0;

    for (Setting<qreal>* const setting : Settings::REAL_SETTINGS)
    {
        QLabel* label = new QLabel(setting->friendly_name, this);

        QDoubleSpinBox* input = new QDoubleSpinBox(this);
        input->setValue(setting->Get());
        connect(input, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [setting](double new_value) {
            setting->Set(new_value);
        });

        QPushButton* reset_button = new QPushButton(QStringLiteral("Reset"), this);
        connect(reset_button, &QAbstractButton::clicked, [setting, input] {
            setting->Reset();
            input->setValue(setting->Get());
        });
        reset_button->setAutoDefault(false);

        grid->addWidget(label, row, 0);
        grid->addWidget(input, row, 1);
        grid->addWidget(reset_button, row, 2);
        row++;
    }

    for (Setting<int>* const setting : Settings::INT_SETTINGS)
    {
        QLabel* label = new QLabel(setting->friendly_name, this);

        QSpinBox* input = new QSpinBox(this);
        input->setMaximum(1000);
        input->setValue(setting->Get());
        connect(input, QOverload<int>::of(&QSpinBox::valueChanged), [setting](int new_value) {
            setting->Set(new_value);
        });

        QPushButton* reset_button = new QPushButton(QStringLiteral("Reset"), this);
        connect(reset_button, &QAbstractButton::clicked, [setting, input] {
            setting->Reset();
            input->setValue(setting->Get());
        });
        reset_button->setAutoDefault(false);

        grid->addWidget(label, row, 0);
        grid->addWidget(input, row, 1);
        grid->addWidget(reset_button, row, 2);
        row++;
    }
}

SettingsDialog::~SettingsDialog() = default;
