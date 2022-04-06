#include "messagebox.h"
#include "ui_messagebox.h"
#include "guiassert.h"

#include <QStyle>

MessageBox::MessageBox(QMessageBox::Icon icon, const QString& title, const QString &message, const QString& details, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::MessageBox)
{
	ui->setupUi(this);
	setWindowTitle(title);

	QStyle *s = style();

	// Inspired by QMessageBoxPrivate.h
	QIcon tmpIcon;
	switch (icon) {
	case QMessageBox::Information:
		tmpIcon = s->standardIcon(QStyle::SP_MessageBoxInformation, 0, this);
		break;
	case QMessageBox::Warning:
		tmpIcon = s->standardIcon(QStyle::SP_MessageBoxWarning, 0, this);
		break;
	case QMessageBox::Critical:
		tmpIcon = s->standardIcon(QStyle::SP_MessageBoxCritical, 0, this);
		break;
	case QMessageBox::Question:
		tmpIcon = s->standardIcon(QStyle::SP_MessageBoxQuestion, 0, this);
		break;
	default:
		// No Icon
		break;
	}
	if (!tmpIcon.isNull())
		ui->l_icon->setPixmap(tmpIcon.pixmap(30, 30));
	ui->l_text->setText(message);
	ui->te_details->setText(details);
	ui->te_details->setVisible(false);

	connect(ui->pb_show_details, &QPushButton::pressed, [=]() {
		showDetails = !showDetails;
		ui->te_details->setVisible(showDetails);
	});

	connect(ui->pb_ok, &QPushButton::pressed, [=] () {
		accept();
	});
}

MessageBox::~MessageBox()
{
	delete ui;
}
