/*
 * Copyright (C) 2004, 2005 Nikolay Pultsin <geometer@mawhrin.net>
 * Copyright (C) 2005 Mikhail Sobolev <mss@mawhrin.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <cctype>

#include <qcheckbox.h>
#include <qcombobox.h>
#include <qlabel.h>
#include <qbuttongroup.h>
#include <qradiobutton.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qlineedit.h>
#include <qslider.h>
#include <qlayout.h>

#include <abstract/ZLStringUtil.h>

#include "QOptionView.h"
#include "QOptionsDialog.h"

void BooleanOptionView::_createItem() {
	myCheckBox = new QCheckBox(myOption->name().c_str(), myTab);
	myCheckBox->setChecked(((ZLBooleanOptionEntry*)myOption)->initialState());
	myTab->addItem(myCheckBox, myRow, myFromColumn, myToColumn);
	connect(myCheckBox, SIGNAL(toggled(bool)), this, SLOT(onValueChange(bool)));
}

void BooleanOptionView::_show() {
	myCheckBox->show();
}

void BooleanOptionView::_hide() {
	myCheckBox->hide();
}

void BooleanOptionView::_onAccept() const {
	((ZLBooleanOptionEntry*)myOption)->onAccept(myCheckBox->isChecked());
}

void BooleanOptionView::onValueChange(bool state) const {
	((ZLBooleanOptionEntry*)myOption)->onValueChange(state);
}

void ChoiceOptionView::_createItem() {
	myGroup = new QButtonGroup(myOption->name().c_str(), myTab);
	QVBoxLayout *layout = new QVBoxLayout(myGroup, 12);
	layout->addSpacing(myGroup->fontMetrics().height());
	myButtons = new QRadioButton*[((ZLChoiceOptionEntry*)myOption)->choiceNumber()];
	for (int i = 0; i < ((ZLChoiceOptionEntry*)myOption)->choiceNumber(); i++) {
		myButtons[i] = new QRadioButton((QButtonGroup*)layout->parent());
		myButtons[i]->setText(((ZLChoiceOptionEntry*)myOption)->text(i).c_str());
		layout->addWidget(myButtons[i]);
	}
	myButtons[((ZLChoiceOptionEntry*)myOption)->initialCheckedIndex()]->setChecked(true);
	myTab->addItem(myGroup, myRow, myFromColumn, myToColumn);
}

void ChoiceOptionView::_show() {
	myGroup->show();
}

void ChoiceOptionView::_hide() {
	myGroup->hide();
}

void ChoiceOptionView::_onAccept() const {
	for (int i = 0; i < ((ZLChoiceOptionEntry*)myOption)->choiceNumber(); i++) {
		if (myButtons[i]->isChecked()) {
			((ZLChoiceOptionEntry*)myOption)->onAccept(i);
			return;
		}
	}
}

void ComboOptionView::_createItem() {
	myLabel = new QLabel(myOption->name().c_str(), myTab);
	myComboBox = new QComboBox(myTab);
	const std::vector<std::string> &values = ((ZLComboOptionEntry*)myOption)->values();
	const std::string &initial = ((ZLComboOptionEntry*)myOption)->initialValue();
	int selectedIndex = -1;
	int index = 0;
	for (std::vector<std::string>::const_iterator it = values.begin(); it != values.end(); it++, index++) {
		myComboBox->insertItem(it->c_str());
		if (*it == initial) {
			selectedIndex = index;
		}
	}
	connect(myComboBox, SIGNAL(activated(int)), this, SLOT(onValueChange(int)));
	if (selectedIndex >= 0) {
		myComboBox->setCurrentItem(selectedIndex);
	}
	int width = myToColumn - myFromColumn + 1;
	myTab->addItem(myLabel, myRow, myFromColumn, myFromColumn + width / 2 - 1);
	myTab->addItem(myComboBox, myRow, myToColumn - width / 2 + 1, myToColumn);
}

void ComboOptionView::_show() {
	myLabel->show();
	myComboBox->show();
}

void ComboOptionView::_hide() {
	myLabel->hide();
	myComboBox->hide();
}

void ComboOptionView::_setActive(bool active) {
	myComboBox->setEnabled(active);
}

void ComboOptionView::_onAccept() const {
	((ZLComboOptionEntry*)myOption)->onAccept(myComboBox->currentText().ascii());
}

void ComboOptionView::onValueChange(int index) {
	ZLComboOptionEntry *o = (ZLComboOptionEntry*)myOption;
	if ((index >= 0) && (index < (int)o->values().size())) {
		o->onValueChange(o->values()[index]);
	}
}

void SpinOptionView::_createItem() {
	myLabel = new QLabel(myOption->name().c_str(), myTab);
	mySpinBox = new QSpinBox(
		((ZLSpinOptionEntry*)myOption)->minValue(),
		((ZLSpinOptionEntry*)myOption)->maxValue(),
		((ZLSpinOptionEntry*)myOption)->step(), myTab
	);
	mySpinBox->setValue(((ZLSpinOptionEntry*)myOption)->initialValue());
	int width = myToColumn - myFromColumn + 1;
	myTab->addItem(myLabel, myRow, myFromColumn, myFromColumn + width * 2 / 3 - 1);
	myTab->addItem(mySpinBox, myRow, myFromColumn + width * 2 / 3, myToColumn);
}

void SpinOptionView::_show() {
	myLabel->show();
	mySpinBox->show();
}

void SpinOptionView::_hide() {
	myLabel->hide();
	mySpinBox->hide();
}

void SpinOptionView::_onAccept() const {
	((ZLSpinOptionEntry*)myOption)->onAccept(mySpinBox->value());
}

void StringOptionView::_createItem() {
	myLabel = new QLabel(myOption->name().c_str(), myTab);
	myLineEdit = new QLineEdit(myTab);
	myLineEdit->setText(QString::fromUtf8(((ZLStringOptionEntry*)myOption)->initialValue().c_str()));
	myLineEdit->cursorLeft(false, myLineEdit->text().length());
	int width = myToColumn - myFromColumn + 1;
	myTab->addItem(myLabel, myRow, myFromColumn, myFromColumn + width / 4 - 1);
	myTab->addItem(myLineEdit, myRow, myFromColumn + width / 4, myToColumn);
}

void StringOptionView::_show() {
	myLabel->show();
	myLineEdit->show();
}

void StringOptionView::_hide() {
	myLabel->hide();
	myLineEdit->hide();
}

void StringOptionView::_setActive(bool active) {
	myLineEdit->setReadOnly(!active);
}

void StringOptionView::_onAccept() const {
	((ZLStringOptionEntry*)myOption)->onAccept((const char*)myLineEdit->text().utf8());
}

class KeyButton : public QPushButton {

public:
	KeyButton(KeyOptionView &keyView);

protected:
	void focusInEvent(QFocusEvent*);
	void focusOutEvent(QFocusEvent*);
	void mousePressEvent(QMouseEvent *);
	void keyPressEvent(QKeyEvent *keyEvent);

private:
	KeyOptionView &myKeyView;
};

KeyButton::KeyButton(KeyOptionView &keyView) : QPushButton(keyView.myWidget), myKeyView(keyView) {
	focusOutEvent(0);
}

void KeyButton::focusInEvent(QFocusEvent*) {
	setText("Press key to set action");
	grabKeyboard();
}

void KeyButton::focusOutEvent(QFocusEvent*) {
	releaseKeyboard();
	setText("Press this button to select key");
}

void KeyButton::mousePressEvent(QMouseEvent*) {
	setFocus();
}

static std::string keyName(int key) {
	if ((key < 128) && isprint(key) && (!isspace(key)) && (key != '&')) {
		std::string name;
		name += (char)toupper(key);
		return name;
	}
	switch (key) {
		case QObject::Key_Escape:
		case 27:
			return "Esc";
		case QObject::Key_Tab:
			return "Tab";
		case QObject::Key_BackTab:
			return "BackTab";
		case QObject::Key_BackSpace:
			return "BackSpace";
		case QObject::Key_Return:
		case 13:
			return "Return";
		case QObject::Key_Enter:
			return "Enter";
		case QObject::Key_Insert:
			return "Insert";
		case QObject::Key_Delete:
			return "Delete";
		case QObject::Key_Pause:
			return "Pause";
		case QObject::Key_Print:
			return "Print";
		case QObject::Key_SysReq:
			return "SysReq";
		case QObject::Key_Clear:
			return "Clear";
		case QObject::Key_Home:
			return "Home";
		case QObject::Key_End:
			return "End";
		case QObject::Key_Left:
			return "LeftArrow";
		case QObject::Key_Up:
			return "UpArrow";
		case QObject::Key_Right:
			return "RightArrow";
		case QObject::Key_Down:
			return "DownArrow";
		case QObject::Key_PageUp:
			return "PgUp";
		case QObject::Key_PageDown:
			return "PgDown";
		case QObject::Key_F1:
			return "F1";
		case QObject::Key_F2:
			return "F2";
		case QObject::Key_F3:
			return "F3";
		case QObject::Key_F4:
			return "F4";
		case QObject::Key_F5:
			return "F5";
		case QObject::Key_F6:
			return "F6";
		case QObject::Key_F7:
			return "F7";
		case QObject::Key_F8:
			return "F8";
		case QObject::Key_F9:
			return "F9";
		case QObject::Key_F10:
			return "F10";
		case QObject::Key_F11:
			return "F11";
		case QObject::Key_F12:
			return "F12";
		case ' ':
			return "Space";
		case '&':
			return "&&";
		case QObject::Key_CapsLock:
			return "CapsLock";
		case QObject::Key_NumLock:
			return "NumLock";
		case QObject::Key_ScrollLock:
			return "ScrollLock";
	}
	return "";
}

static std::string text(QKeyEvent *keyEvent) {
	QString txt = keyEvent->text();
	int key = keyEvent->key();
	int state = keyEvent->state();

	std::string name = (const char*)txt.upper().utf8();
	if (name.empty() || ((name.length() == 1) && !isprint(name[0]) || isspace(name[0]))) {
		name = keyName(key);
	}
	if (name.empty()) {
		return "";
	}

	name = '<' + name + '>';
	if (state & 0x400) {
		name = "<Alt>+" + name;
	}
	if (state & 0x200) {
		name = "<Ctrl>+" + name;
	}
	return name;
}

void KeyButton::keyPressEvent(QKeyEvent *keyEvent) {
	std::string keyText = ::text(keyEvent);
	if (!keyText.empty()) {
		myKeyView.myLabel->setText("Action For " + QString::fromUtf8(keyText.c_str()));
		myKeyView.myLabel->show();
		myKeyView.myComboBox->show();
	}
}

void KeyOptionView::_createItem() {
	myWidget = new QWidget(myTab);
	QGridLayout *layout = new QGridLayout(myWidget, 2, 2, 0, 10);
	myKeyButton = new KeyButton(*this);
	layout->addMultiCellWidget(myKeyButton, 0, 0, 0, 1);
	myLabel = new QLabel(myWidget);
	myLabel->setTextFormat(QObject::PlainText);
	layout->addWidget(myLabel, 1, 0);
	myComboBox = new QComboBox(myWidget);
	const std::vector<std::string> &actions = ((ZLKeyOptionEntry*)myOption)->actionNames();
	for (std::vector<std::string>::const_iterator it = actions.begin(); it != actions.end(); it++) {
		myComboBox->insertItem(it->c_str());
	}
	layout->addWidget(myComboBox, 1, 1);
	myTab->addItem(myWidget, myRow, myFromColumn, myToColumn);
}

void KeyOptionView::_show() {
	myWidget->show();
	myKeyButton->show();
	if (!myCurrentKey.empty()) {
		myLabel->show();
		myComboBox->show();
	} else {
		myLabel->hide();
		myComboBox->hide();
	}
}

void KeyOptionView::_hide() {
	myKeyButton->hide();
	myWidget->hide();
	myLabel->hide();
	myComboBox->hide();
}

void KeyOptionView::_onAccept() const {
}

void ColorOptionView::_createItem() {
	myWidget = new QWidget(myTab);
	QGridLayout *layout = new QGridLayout(myWidget, 3, 3, 0, 10);
	layout->addWidget(new QLabel("Red", myWidget), 0, 0);
	layout->addWidget(new QLabel("Green", myWidget), 1, 0);
	layout->addWidget(new QLabel("Blue", myWidget), 2, 0);
	const ZLColor &color = ((ZLColorOptionEntry*)myOption)->color();
	myRSlider = new QSlider(0, 255, 1, color.Red, QSlider::Horizontal, myWidget);
	myGSlider = new QSlider(0, 255, 1, color.Green, QSlider::Horizontal, myWidget);
	myBSlider = new QSlider(0, 255, 1, color.Blue, QSlider::Horizontal, myWidget);
	connect(myRSlider, SIGNAL(sliderMoved(int)), this, SLOT(onSliderMove(int)));
	connect(myGSlider, SIGNAL(sliderMoved(int)), this, SLOT(onSliderMove(int)));
	connect(myBSlider, SIGNAL(sliderMoved(int)), this, SLOT(onSliderMove(int)));
	layout->addWidget(myRSlider, 0, 1);
	layout->addWidget(myGSlider, 1, 1);
	layout->addWidget(myBSlider, 2, 1);
	myColorBar = new QLabel("                  ", myWidget);
	myColorBar->setBackgroundColor(QColor(color.Red, color.Green, color.Blue));
	myColorBar->setFrameStyle(QFrame::Panel | QFrame::Plain);
	layout->addMultiCellWidget(myColorBar, 0, 2, 2, 2);
	myTab->addItem(myWidget, myRow, myFromColumn, myToColumn);
}

void ColorOptionView::_show() {
	myWidget->show();
}

void ColorOptionView::_hide() {
	myWidget->hide();
}

void ColorOptionView::onSliderMove(int) {
	myColorBar->setBackgroundColor(QColor(myRSlider->value(), myGSlider->value(), myBSlider->value()));
}

void ColorOptionView::_onAccept() const {
	((ZLColorOptionEntry*)myOption)->onAccept(ZLColor(myRSlider->value(), myGSlider->value(), myBSlider->value()));
}
