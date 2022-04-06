#ifndef MESSAGEBOX_H
#define MESSAGEBOX_H

#include <QDialog>
#include <QMessageBox>

namespace Ui {
class MessageBox;
}

class MessageBox : public QDialog
{
	Q_OBJECT

public:
	explicit MessageBox(QMessageBox::Icon icon, const QString &title, const QString& message, const QString &details, QWidget *parent = nullptr);
	~MessageBox();

private:
	Ui::MessageBox *ui;
	bool showDetails{false};
};

#endif // MESSAGEBOX_H
