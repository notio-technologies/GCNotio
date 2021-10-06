/*
 * Copyright (c) 2018 Michael Beaulieu (michael.beaulieu@notiotechnologies.com)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "ChartsMngrDialog.h"
#include "TabView.h"
#include "Tab.h"
#include "MainWindow.h"

///////////////////////////////////////////////////////////////////////////////////
/// \brief ChartsMngrDialog::ChartsMngrDialog
///        Construtor.
///
/// \param[in] iContext Context.
///////////////////////////////////////////////////////////////////////////////////
ChartsMngrDialog::ChartsMngrDialog(Context *iContext) : m_context(iContext)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setModal(true);

    setMinimumSize(static_cast<int>(450 * dpiXFactor), static_cast<int>(650 * dpiYFactor));

    QString wTitle = QString(tr("Reorder Charts"));
    setWindowTitle(wTitle);

    // Set window layout
    m_mainLayout = new QVBoxLayout;

    // Charts manager page.
    m_chartsMngrPage = new ChartMngrPage(m_context);
    m_mainLayout->addWidget(m_chartsMngrPage);

    // Button layout
    m_cancelButton = new QPushButton(tr("Cancel"));
    m_saveButton = new QPushButton(tr("Save"));

    QHBoxLayout *wButtonsLayout = new QHBoxLayout;
    wButtonsLayout->addStretch();
    wButtonsLayout->setSpacing(static_cast<int>(5 * dpiXFactor));
    wButtonsLayout->addWidget(m_cancelButton);
    wButtonsLayout->addWidget(m_saveButton);

    m_mainLayout->addLayout(wButtonsLayout);

    setLayout(m_mainLayout);

    // Connect button signals with slots.
    connect(m_cancelButton, SIGNAL(clicked()), this, SLOT(close()));
    connect(m_saveButton, SIGNAL(clicked()), this, SLOT(saveClicked()));
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief ChartsMngrDialog::saveClicked
///        This method is called when the save button is clicked.
///////////////////////////////////////////////////////////////////////////////////
void ChartsMngrDialog::saveClicked()
{
    // What changed ?
    qint32 changed = 0;

    changed |= m_chartsMngrPage->saveClicked();

    hide();

    // we're done.
    m_context->notifyConfigChanged(changed);
    accept();
}
