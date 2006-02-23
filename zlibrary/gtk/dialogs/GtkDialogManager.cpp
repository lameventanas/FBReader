/*
 * Copyright (C) 2004-2006 Nikolay Pultsin <geometer@mawhrin.net>
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

#include <gtk/gtk.h>

#include <gdk/gdkkeysyms.h>

#include <abstract/ZLOpenFileDialog.h>

#include "GtkDialogManager.h"
#include "GtkOptionsDialog.h"
#include "GtkOpenFileDialog.h"
#include "GtkWaitMessage.h"

ZLOptionsDialog *GtkDialogManager::createOptionsDialog(const std::string &id, const std::string &title) const {
	return new GtkOptionsDialog(id, title);
}

int GtkDialogManager::informationBox(const char *title, const char *message, const char *button0, const char *button1, const char *button2) const {
	GtkDialog *dialog = createDialog(title);

	if (button0 != 0) {
		gtk_dialog_add_button(dialog, button0, 0);
	}
	if (button1 != 0) {
		gtk_dialog_add_button(dialog, button1, 1);
	}
	if (button2 != 0) {
		gtk_dialog_add_button(dialog, button2, 2);
	}

	GtkWidget *contents = gtk_hbox_new(FALSE, 10);
	gtk_container_set_border_width(GTK_CONTAINER(contents), 10);
	GtkWidget *image = gtk_image_new_from_stock(GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_DIALOG);
	gtk_misc_set_alignment(GTK_MISC(image), 0.5, 0.0);

	GtkWidget *label = gtk_label_new(message);
	gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
	gtk_box_pack_start(GTK_BOX(contents), image, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(contents), label, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(dialog->vbox), contents, TRUE, TRUE, 0);
	gtk_widget_show_all(GTK_WIDGET(dialog));
	gint response = gtk_dialog_run(dialog);
	gtk_widget_destroy(GTK_WIDGET(dialog));

	return response == GTK_RESPONSE_REJECT ? -1 : response;
}

void GtkDialogManager::openFileDialog(const std::string &title, const ZLTreeHandler &handler) const {
	GtkOpenFileDialog(title.c_str(), handler).runWithSize();
}

void QDialogManager::wait(ZLRunnable &runnable, const std::string &message) const {
	GtkWaitMessage waitMessage(message);
	runnable.run();
}

static bool dialogDefaultKeys(GtkWidget *dialog, GdkEventKey *key, gpointer) {
	if (!((GtkDialogManager&)GtkDialogManager::instance()).isKeyboardGrabbed() && (key->state == 0)) {
		if (key->keyval == GDK_Return) {
			gtk_dialog_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
			return true;
		}

		if (key->keyval == GDK_Escape) {
			gtk_dialog_response(GTK_DIALOG(dialog), GTK_RESPONSE_REJECT);
			return true;
		}
	}

	return false;
}

GtkDialog *GtkDialogManager::createDialog(const std::string& title) const {
	GtkWindow *dialog = GTK_WINDOW(gtk_dialog_new());

	gtk_window_set_title(dialog, title.c_str());

	if (myWindow != 0) {
		gtk_window_set_transient_for(dialog, myWindow);
	}

	gtk_window_set_modal(dialog, TRUE);

	gtk_signal_connect(GTK_OBJECT(dialog), "key-press-event", G_CALLBACK(dialogDefaultKeys), NULL);

	return GTK_DIALOG(dialog);
}
