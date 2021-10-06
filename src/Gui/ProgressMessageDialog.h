/*
 * Copyright (c) 2020 Michael Beaulieu (michael.beaulieu@notio.ai)
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

#ifndef ProgressMessageDialog_h
#define ProgressMessageDialog_h

#include <QWidget>
#include <QObject>

class QLabel;
class QProgressBar;

///////////////////////////////////////////////////////////////////////////////
/// \brief The ProgressMessageDialog class
///        This class defines a progress message dialog.
///////////////////////////////////////////////////////////////////////////////
class ProgressMessageDialog : public QWidget
{
    Q_OBJECT
public:
    ProgressMessageDialog(QString iTitle, QString iLabelText, QString iInformativeText, QWidget *iParent = nullptr);
    ~ProgressMessageDialog() {}

public slots:
    void setLabelText(QString iLabelText);
    void setProgressValue(int iValue);
    void setInformativeText(QString iInformativeText);
    void setWindowDisplayType();

private:
    QLabel *m_labelText = nullptr;
    QLabel *m_informativeText = nullptr;
    QProgressBar *m_progress = nullptr;
};

#endif // CUSTOMPROGRESSDIALOG_H
