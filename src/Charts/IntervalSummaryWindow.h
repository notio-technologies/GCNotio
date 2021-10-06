/*
 * Copyright (c) 2011 Eric Brandt (eric.l.brandt@gmail.com)
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

#ifndef INTERVALSUMMARYWINDOW_H_
#define INTERVALSUMMARYWINDOW_H_

#include <QtGui>
#include <QTextEdit>

#include "RideFile.h"

class Context;
class IntervalItem;
class RideFile;

class IntervalSummaryWindow : public QTextEdit {
	Q_OBJECT;

public:
    IntervalSummaryWindow(QWidget *iParent, Context *context);
	virtual ~IntervalSummaryWindow();

public slots:

    void intervalSelected();
    void intervalHover(IntervalItem*);

protected:
    QString summary(QList<IntervalItem*>, QString&);
    QString summary(IntervalItem *);
    void resetFakeRides();

    Context *context;
    QWidget *m_parent;

    // Fake rides to compute intervals summary.
    static RideFile *m_fake;
    static RideFile *m_notFake;
};

#endif /* INTERVALSUMMARYWINDOW_H_ */
