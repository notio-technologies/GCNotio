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

#include "ProgressMessageDialog.h"

#include "GoldenCheetah.h"
#include "Settings.h"
#include "Colors.h"

#include <QLabel>
#include <QProgressBar>
#include <QLayout>

///////////////////////////////////////////////////////////////////////////////
/// \brief ProgressMessageDialog::ProgressMessageDialog
///        Constructor.
///
/// \param[in] iTitle           Dialog title.
/// \param[in] iLabelText       Text
/// \param[in] iInformativeText Informative text.
/// \param[in] iParent          Parent widget.
///////////////////////////////////////////////////////////////////////////////
ProgressMessageDialog::ProgressMessageDialog(QString iTitle, QString iLabelText, QString iInformativeText, QWidget *iParent) :
    QWidget(iParent)
{
    setWindowTitle(iTitle);
    if (iTitle.isEmpty())
    {
        // Frameless window without title bar.
        setWindowFlags(windowFlags() | Qt::FramelessWindowHint | Qt::CoverWindow);
#ifdef Q_OS_LINUX
    setWindowFlags(windowFlags() | Qt::X11BypassWindowManagerHint | Qt::CoverWindow);
#endif
    }
    else
        setWindowFlag(Qt::Dialog);

    // Set has modal window.
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowModality(Qt::ApplicationModal);
    setMinimumSize(static_cast<int>(300 * dpiXFactor), static_cast<int>(80 * dpiYFactor));

    // Create layout.
    QVBoxLayout *wLayout = new QVBoxLayout(this);
    wLayout->setSpacing(0);

    // Main text.
    m_labelText = new QLabel(m_labelText);
    m_labelText->setAlignment(Qt::AlignCenter);
    m_labelText->setText(iLabelText);
    wLayout->addWidget(m_labelText);

    // Informative text.
    m_informativeText = new QLabel(this);
    m_informativeText->setAlignment(Qt::AlignCenter);
    m_informativeText->setText(iInformativeText);
    wLayout->addWidget(m_informativeText);

    // Progress bar.
    m_progress = new QProgressBar(this);
    m_progress->setAlignment(Qt::AlignCenter);
    m_progress->setValue(0);
    wLayout->addSpacing(5);
    wLayout->addWidget(m_progress);

    // Hide progress bar for frameless window.
    if (iTitle.isEmpty())
        m_progress->hide();

    // Middle of the parent.
    QPoint wCenter = QApplication::screenAt(pos())->geometry().center();
    if (iParent)
        wCenter = iParent->geometry().center();

    move(wCenter - geometry().center());
    connect(this, SIGNAL(windowTitleChanged(QString)), SLOT(setWindowDisplayType()));
}

///////////////////////////////////////////////////////////////////////////////
/// \brief ProgressMessageDialog::setLabelText
///        This method sets the text to be displayed.
///
/// \param[in] iLabelText   Text to display.
///////////////////////////////////////////////////////////////////////////////
void ProgressMessageDialog::setLabelText(QString iLabelText)
{
    m_labelText->setText(iLabelText);
}

///////////////////////////////////////////////////////////////////////////////
/// \brief ProgressMessageDialog::setProgressValue
///        This method sets the progress bar value.
///
/// \param[in] iValue   Value in percentage.
///////////////////////////////////////////////////////////////////////////////
void ProgressMessageDialog::setProgressValue(int iValue)
{
    m_progress->setValue(iValue);
    m_progress->setHidden(windowTitle().isEmpty());
}

///////////////////////////////////////////////////////////////////////////////
/// \brief ProgressMessageDialog::setInformativeText
///        This method sets the informative text to be displayed.
///
/// \param[in] iInformativeText Informative text to display.
///////////////////////////////////////////////////////////////////////////////
void ProgressMessageDialog::setInformativeText(QString iInformativeText)
{
    m_informativeText->setText(iInformativeText);
}

///////////////////////////////////////////////////////////////////////////////
/// \brief ProgressMessageDialog::setWindowDisplayType
///        This method sets the window type depending on the title availability.
///////////////////////////////////////////////////////////////////////////////
void ProgressMessageDialog::setWindowDisplayType()
{
    // Frameless window.
    if (windowTitle().isEmpty())
    {
        setWindowFlags(windowFlags() | Qt::FramelessWindowHint | Qt::CoverWindow);
        m_progress->hide();
#ifdef Q_OS_LINUX
    setWindowFlags(windowFlags() | Qt::X11BypassWindowManagerHint | Qt::CoverWindow);
#endif
    }
    // Standard looking window.
    else
    {
        setWindowFlags((windowFlags() | Qt::Dialog | Qt::WindowTitleHint) & ~Qt::FramelessWindowHint);
        m_progress->show();
    }
}
