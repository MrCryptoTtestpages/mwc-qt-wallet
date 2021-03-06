// Copyright 2019 The MWC Developers
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "sendcoinsparamsdialog.h"
#include "ui_sendcoinsparamsdialog.h"
#include "../control_desktop/messagebox.h"

namespace wnd {

SendCoinsParamsDialog::SendCoinsParamsDialog(QWidget *parent, int _inputConfirmationNumber,
                        int _changeOutputs) :
    control::MwcDialog(parent),
    ui(new Ui::SendCoinsParamsDialog),
    inputConfirmationNumber(_inputConfirmationNumber),
    changeOutputs(_changeOutputs)
{
    ui->setupUi(this);

    ui->inputsConfEdit->setText( QString::number(inputConfirmationNumber ) );
    ui->changeOutputsEdit->setText( QString::number(changeOutputs ) );
}

SendCoinsParamsDialog::~SendCoinsParamsDialog()
{
    delete ui;
}

void SendCoinsParamsDialog::on_okButton_clicked()
{
    bool ok = false;
    inputConfirmationNumber = ui->inputsConfEdit->text().trimmed().toInt(&ok);
    if (!ok || inputConfirmationNumber<=0) {
        control::MessageBox::messageText(this, "Need info", "Please input correct value for minimum number of confirmations");
        return;
    }

    ok = false;
    changeOutputs = ui->changeOutputsEdit->text().trimmed().toInt(&ok);
    if (!ok || changeOutputs<=0) {
        control::MessageBox::messageText(this, "Need info", "Please input correct value for change outputs");
        return;
    }

    accept();
}

void SendCoinsParamsDialog::on_cancelButton_clicked()
{
    reject();
}

}

