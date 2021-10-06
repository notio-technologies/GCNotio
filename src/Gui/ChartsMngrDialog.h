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

#ifndef CHARTSMNGRDIALOG_H
#define CHARTSMNGRDIALOG_H
#include "GoldenCheetah.h"

#include "Pages.h"
#include <QDialog>
#include <QObject>

class Context;

///////////////////////////////////////////////////////////////////////////////////
/// \brief The ChartsMngrDialog class
///        This class creates a dialog window to manage the charts layout for an
///        athlete different views.
///
///        It currently only manage the order of the charts displayed.
///////////////////////////////////////////////////////////////////////////////////
class ChartsMngrDialog : public QDialog
{
    Q_OBJECT

public:
    ChartsMngrDialog(Context *iContext);

public slots:
    void saveClicked();

private:
    Context *m_context;

    ChartMngrPage *m_chartsMngrPage;
    QPushButton *m_saveButton;
    QPushButton *m_cancelButton;

    QTabWidget *m_tabWidget;
    QVBoxLayout *m_mainLayout;
};

#endif // CHARTSMNGRDIALOG_H
