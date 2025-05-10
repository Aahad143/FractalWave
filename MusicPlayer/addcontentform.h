#ifndef ADDCONTENTFORM_H
#define ADDCONTENTFORM_H

#include <QFrame>
#include <qlistwidget.h>

namespace Ui {
class AddContentForm;
}

class AddContentForm : public QFrame
{
    Q_OBJECT

public:
    explicit AddContentForm(QWidget *parent = nullptr);
    ~AddContentForm();

    void resetForm();

signals:
    /// Emitted whenever the form is hidden (via hide())
    void closed();
    void createPlaylist(const QString& playlistName);
    void addTrack(const QString& ytURL);

private slots:
    void onListItemClicked(QListWidgetItem* item);

protected:
    void hideEvent(QHideEvent *event) override;

private:
    Ui::AddContentForm *ui;
};

#endif // ADDCONTENTFORM_H
