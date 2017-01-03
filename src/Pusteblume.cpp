/**
 * Copyright (C) 2017 Braden Walters
 *
 * This file is licensed under the MIT Expat License. See LICENSE.txt.
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QVBoxLayout>
#include <QWebView>
#include "Pusteblume.hpp"

Pusteblume::Pusteblume(QWidget *parent) : QMainWindow(parent)
{
  // Set title.
  setWindowTitle("Pusteblume");

  // Create layout.
  auto mainLayout = new QVBoxLayout();

  // Create test WebKit.
  auto webkit = new QWebView();
  webkit->setHtml(QString("<html><body><b>Hello world!</b></body></html>"));
  mainLayout->addWidget(webkit);

  // Set central widget with layout.
  auto central = new QWidget();
  central->setLayout(mainLayout);
  setCentralWidget(central);
}

Pusteblume::~Pusteblume()
{
}